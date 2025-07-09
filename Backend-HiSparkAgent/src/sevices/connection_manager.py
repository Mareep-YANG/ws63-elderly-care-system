import asyncio
import threading
from typing import Dict, Set
from starlette.websockets import WebSocket
import logging

logger = logging.getLogger(__name__)


class ConnectionManager:
    """全局WebSocket连接管理器"""
    
    def __init__(self):
        self._connections: Dict[str, WebSocket] = {}
        self._lock = threading.Lock()
    
    def add_connection(self, client_id: str, websocket: WebSocket):
        """添加连接"""
        with self._lock:
            self._connections[client_id] = websocket
            logger.info(f"添加连接: {client_id}, 当前连接数: {len(self._connections)}")
    
    def remove_connection(self, client_id: str):
        """移除连接"""
        with self._lock:
            if client_id in self._connections:
                del self._connections[client_id]
                logger.info(f"移除连接: {client_id}, 当前连接数: {len(self._connections)}")
    
    def get_connection(self, client_id: str) -> WebSocket:
        """获取指定连接"""
        with self._lock:
            return self._connections.get(client_id)
    
    def get_all_connections(self) -> Dict[str, WebSocket]:
        """获取所有连接"""
        with self._lock:
            return self._connections.copy()
    
    def get_client_list(self) -> list:
        """获取所有客户端ID列表"""
        with self._lock:
            return list(self._connections.keys())
    
    async def send_file_to_client(self, client_id: str, filename: str, chunk_size: int = 64 * 1024) -> bool:
        """向指定客户端发送文件"""
        websocket = self.get_connection(client_id)
        if not websocket:
            logger.error(f"客户端 {client_id} 未连接")
            return False
        
        import os
        from src.config import settings
        
        file_path = os.path.join(settings.uploads_dir, os.path.basename(filename))
        if not os.path.exists(file_path):
            logger.error(f"文件 {filename} 不存在")
            return False
        
        try:
            await websocket.send_text(f"FILE {filename}")
            logger.info(f"开始向客户端 {client_id} 发送文件 {filename}")
            
            with open(file_path, "rb") as f:
                while True:
                    chunk = f.read(chunk_size)
                    if not chunk:
                        break
                    await websocket.send_bytes(chunk)
            
            await websocket.send_text("EOF")
            logger.info(f"文件 {filename} 发送完成给客户端 {client_id}")
            return True
            
        except Exception as exc:
            logger.exception(f"向客户端 {client_id} 发送文件失败: {exc}")
            try:
                await websocket.send_text(f"ERROR DOWNLOAD {filename}: {exc}")
            except Exception:
                pass
            return False
    
    async def stream_audio_to_client(self, client_id: str, filename: str) -> bool:
        """向指定客户端流式传输音频文件"""
        websocket = self.get_connection(client_id)
        if not websocket:
            logger.error(f"客户端 {client_id} 未连接")
            return False
        
        import os
        from src.config import settings
        from src.sevices.audio_stream_manager import audio_stream_manager
        
        file_path = os.path.join(settings.uploads_dir, os.path.basename(filename))
        if not os.path.exists(file_path):
            logger.error(f"音频文件 {filename} 不存在")
            return False
        
        # 检查是否为WAV文件
        if not filename.lower().endswith('.wav'):
            logger.error(f"文件 {filename} 不是WAV格式")
            return False
        
        try:
            # 创建音频流
            stream_id = audio_stream_manager.create_stream(client_id, filename, file_path)
            if not stream_id:
                logger.error(f"创建音频流失败: {filename}")
                return False
            
            # 获取音频格式信息
            format_info = audio_stream_manager.get_stream_format_info(stream_id)
            if not format_info:
                logger.error(f"获取音频格式失败: {filename}")
                return False
            
            # 开始流传输
            if not audio_stream_manager.start_stream(stream_id):
                logger.error(f"启动流传输失败: {filename}")
                return False
            
            # 发送流开始消息
            import json
            format_json = json.dumps(format_info)
            await websocket.send_text(f"STREAM_START {stream_id} {filename} {format_json}")
            logger.info(f"开始向客户端 {client_id} 流式传输音频: {filename}, 流ID: {stream_id}")
            
            # 启动音频数据流传输任务
            import asyncio
            from src.sevices.websocket_service import stream_audio_data
            asyncio.create_task(stream_audio_data(websocket, stream_id))
            
            return True
            
        except Exception as exc:
            logger.exception(f"向客户端 {client_id} 流式传输音频失败: {exc}")
            return False


# 全局连接管理器实例
connection_manager = ConnectionManager()