from __future__ import annotations

"""FFmpeg 服务：音频格式转换。

提供一个统一入口 `convert_to_wav`，将任意输入文件（音频或视频）
转换为符合下述约束的 WAV 文件，并返回目标文件路径：

- 采样率：48 000 Hz
- 位深：16-bit
- 声道数：2（立体声）

依赖前置：
1. 系统需已安装 `ffmpeg` ⬝ 6.0+（推荐）。
2. `ffmpeg` 可通过 PATH 正常调用。

如果系统无法调用 `ffmpeg`，将抛出 `RuntimeError`。
"""

import asyncio
import subprocess
import uuid
from pathlib import Path
from typing import Optional

from src.config import settings

# 默认转换参数
_SAMPLE_RATE = 48_000  # Hz
_CHANNELS = 2          # 立体声
_BIT_DEPTH = 16        # 16-bit PCM（s16le）

__all__ = [
    "convert_to_wav",
]


def _ensure_ffmpeg_available() -> None:
    """确认系统已安装并可调用 ffmpeg，否则抛出异常。"""
    try:
        subprocess.run(["ffmpeg", "-version"], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except (FileNotFoundError, subprocess.CalledProcessError):
        raise RuntimeError("未找到 ffmpeg，请确认已安装并添加到 PATH。")


# ---------------------------------------------------------------------------
# 同步转换核心，供线程池调用
# ---------------------------------------------------------------------------

def _sync_convert(
    input_path: str | Path,
    output_path: Optional[str | Path] = None,
    sample_rate: int = _SAMPLE_RATE,
    channels: int = _CHANNELS,
    bit_depth: int = _BIT_DEPTH,
) -> str:
    """同步执行 ffmpeg 转换，并返回输出文件绝对路径。"""

    # 输入文件检查
    src = Path(input_path).expanduser().resolve()
    if not src.exists():
        raise FileNotFoundError(f"输入文件不存在: {src}")

    # 准备输出路径
    if output_path is None:
        # 使用 uploads 目录，文件名保持唯一
        upload_dir = Path(settings.uploads_dir)
        upload_dir.mkdir(parents=True, exist_ok=True)
        out_name = f"{src.stem}_{uuid.uuid4().hex[:8]}.wav"
        dst = upload_dir / out_name
    else:
        dst = Path(output_path).expanduser().resolve()
        dst.parent.mkdir(parents=True, exist_ok=True)

    # ffmpeg 命令参数
    cmd = [
        "ffmpeg",
        "-y",               # 覆盖输出
        "-i", str(src),      # 输入文件
        "-ar", str(sample_rate),  # 采样率
        "-ac", str(channels),     # 声道数
        "-sample_fmt", "s16",    # 位深 16-bit（PCM）
        "-acodec", "pcm_s16le",  # 明确编码器，避免封装层自选
        str(dst),
    ]

    # 运行 ffmpeg
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            "ffmpeg 转换失败: "
            f"\nCMD: {' '.join(cmd)}"
            f"\nSTDERR: {result.stderr.strip()}"
        )

    return str(dst)


# ---------------------------------------------------------------------------
# 异步包装函数，供外部直接调用
# ---------------------------------------------------------------------------

async def convert_to_wav(
    input_path: str | Path,
    *,
    output_path: Optional[str | Path] = None,
    sample_rate: int = _SAMPLE_RATE,
    channels: int = _CHANNELS,
    bit_depth: int = _BIT_DEPTH,
) -> str:
    """异步将输入文件转换为标准 WAV，返回输出文件路径。

    参数说明:
        input_path:  待转换文件路径。
        output_path: 指定输出路径，默认为 `uploads` 目录自动生成。
        sample_rate: 目标采样率，默认 48000 Hz。
        channels:    目标声道数，默认 2。 
        bit_depth:   目标位深，仅支持 16。其它值将抛出 `ValueError`。
    """

    if bit_depth != 16:
        raise ValueError("目前仅支持 16-bit 输出。")

    # 确保 ffmpeg 可用
    _ensure_ffmpeg_available()

    loop = asyncio.get_running_loop()
    return await loop.run_in_executor(
        None,
        _sync_convert,
        input_path,
        output_path,
        sample_rate,
        channels,
        bit_depth,
    ) 