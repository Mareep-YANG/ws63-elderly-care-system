import asyncio
import json
import os
import struct
import wave
import threading
from typing import Dict, Optional, Any
from dataclasses import dataclass, asdict
from enum import Enum
import secrets
import logging

logger = logging.getLogger(__name__)


class StreamState(Enum):
    """流状态枚举"""
    READY = "READY"
    STREAMING = "STREAMING"
    PAUSED = "PAUSED"
    STOPPED = "STOPPED"
    ERROR = "ERROR"


@dataclass
class AudioFormat:
    """音频格式信息"""
    sample_rate: int
    channels: int
    bit_depth: int
    duration: float
    file_size: int
    bitrate: int


@dataclass
class StreamSession:
    """流会话信息"""
    stream_id: str
    client_id: str
    filename: str
    file_path: str
    audio_format: AudioFormat
    state: StreamState
    position: int = 0  # 当前读取位置
    chunk_size: int = 8192  # 数据块大小
    is_active: bool = True
    _file_handle: Optional[Any] = None
    _lock: threading.Lock = None
    
    def __post_init__(self):
        if self._lock is None:
            self._lock = threading.Lock()


class AudioStreamManager:
    """音频流管理器"""
    
    def __init__(self, chunk_size: int = 8192):
        self.chunk_size = chunk_size
        self.active_streams: Dict[str, StreamSession] = {}
        self._lock = threading.Lock()
    
    def _parse_wav_file(self, file_path: str) -> Optional[AudioFormat]:
        """解析WAV文件格式信息"""
        try:
            with wave.open(file_path, 'rb') as wav_file:
                frames = wav_file.getnframes()
                sample_rate = wav_file.getframerate()
                channels = wav_file.getnchannels()
                bit_depth = wav_file.getsampwidth() * 8
                duration = frames / sample_rate
                
                # 计算文件大小和比特率
                file_size = os.path.getsize(file_path)
                bitrate = (sample_rate * channels * bit_depth)
                
                return AudioFormat(
                    sample_rate=sample_rate,
                    channels=channels,
                    bit_depth=bit_depth,
                    duration=duration,
                    file_size=file_size,
                    bitrate=bitrate
                )
        except Exception as e:
            logger.error(f"解析WAV文件失败 {file_path}: {e}")
            return None
    
    def create_stream(self, client_id: str, filename: str, file_path: str) -> Optional[str]:
        """创建新的音频流会话"""
        if not os.path.exists(file_path):
            logger.error(f"音频文件不存在: {file_path}")
            return None
        
        # 解析音频格式
        audio_format = self._parse_wav_file(file_path)
        if not audio_format:
            return None
        
        # 生成唯一的流ID
        stream_id = secrets.token_hex(4)
        
        # 创建流会话
        session = StreamSession(
            stream_id=stream_id,
            client_id=client_id,
            filename=filename,
            file_path=file_path,
            audio_format=audio_format,
            state=StreamState.READY,
            chunk_size=self.chunk_size
        )
        
        with self._lock:
            self.active_streams[stream_id] = session
        
        logger.info(f"创建音频流 {stream_id} for 客户端 {client_id}: {filename}")
        return stream_id
    
    def get_stream(self, stream_id: str) -> Optional[StreamSession]:
        """获取流会话"""
        with self._lock:
            return self.active_streams.get(stream_id)
    
    def start_stream(self, stream_id: str) -> bool:
        """开始流传输"""
        session = self.get_stream(stream_id)
        if not session:
            return False
        
        with session._lock:
            if session.state != StreamState.READY:
                logger.warning(f"流 {stream_id} 状态错误: {session.state}")
                return False
            
            try:
                # 打开WAV文件，跳过文件头
                session._file_handle = wave.open(session.file_path, 'rb')
                session.state = StreamState.STREAMING
                session.position = 0
                logger.info(f"开始流传输 {stream_id}")
                return True
            except Exception as e:
                logger.error(f"开始流传输失败 {stream_id}: {e}")
                session.state = StreamState.ERROR
                return False
    
    def read_chunk(self, stream_id: str) -> Optional[bytes]:
        """读取音频数据块"""
        session = self.get_stream(stream_id)
        if not session or not session._file_handle:
            return None
        
        with session._lock:
            if session.state != StreamState.STREAMING:
                return None
            
            try:
                # 读取音频帧数据
                frames = session._file_handle.readframes(
                    session.chunk_size // (session.audio_format.channels * session.audio_format.bit_depth // 8)
                )
                
                if frames:
                    session.position += len(frames)
                    return frames
                else:
                    # 文件读取完毕
                    session.state = StreamState.STOPPED
                    return None
                    
            except Exception as e:
                logger.error(f"读取音频数据失败 {stream_id}: {e}")
                session.state = StreamState.ERROR
                return None
    
    def pause_stream(self, stream_id: str) -> bool:
        """暂停流传输"""
        session = self.get_stream(stream_id)
        if not session:
            return False
        
        with session._lock:
            if session.state == StreamState.STREAMING:
                session.state = StreamState.PAUSED
                logger.info(f"暂停流传输 {stream_id}")
                return True
        return False
    
    def resume_stream(self, stream_id: str) -> bool:
        """恢复流传输"""
        session = self.get_stream(stream_id)
        if not session:
            return False
        
        with session._lock:
            if session.state == StreamState.PAUSED:
                session.state = StreamState.STREAMING
                logger.info(f"恢复流传输 {stream_id}")
                return True
        return False
    
    def stop_stream(self, stream_id: str) -> bool:
        """停止流传输"""
        session = self.get_stream(stream_id)
        if not session:
            return False
        
        with session._lock:
            session.state = StreamState.STOPPED
            session.is_active = False
            
            # 关闭文件句柄
            if session._file_handle:
                try:
                    session._file_handle.close()
                except Exception:
                    pass
                session._file_handle = None
            
            logger.info(f"停止流传输 {stream_id}")
        
        # 从活跃流中移除
        with self._lock:
            self.active_streams.pop(stream_id, None)
        
        return True
    
    def cleanup_client_streams(self, client_id: str):
        """清理客户端的所有流"""
        streams_to_remove = []
        
        with self._lock:
            for stream_id, session in self.active_streams.items():
                if session.client_id == client_id:
                    streams_to_remove.append(stream_id)
        
        for stream_id in streams_to_remove:
            self.stop_stream(stream_id)
            logger.info(f"清理客户端 {client_id} 的流 {stream_id}")
    
    def get_stream_format_info(self, stream_id: str) -> Optional[Dict[str, Any]]:
        """获取流的格式信息"""
        session = self.get_stream(stream_id)
        if not session:
            return None
        
        return asdict(session.audio_format)
    
    def get_client_streams(self, client_id: str) -> list:
        """获取客户端的所有活跃流"""
        streams = []
        with self._lock:
            for stream_id, session in self.active_streams.items():
                if session.client_id == client_id and session.is_active:
                    streams.append({
                        'stream_id': stream_id,
                        'filename': session.filename,
                        'state': session.state.value,
                        'format': asdict(session.audio_format)
                    })
        return streams
    
    def get_all_streams(self) -> Dict[str, Dict[str, Any]]:
        """获取所有活跃流的信息"""
        streams = {}
        with self._lock:
            for stream_id, session in self.active_streams.items():
                if session.is_active:
                    streams[stream_id] = {
                        'client_id': session.client_id,
                        'filename': session.filename,
                        'state': session.state.value,
                        'position': session.position,
                        'format': asdict(session.audio_format)
                    }
        return streams


# 全局音频流管理器实例
audio_stream_manager = AudioStreamManager()