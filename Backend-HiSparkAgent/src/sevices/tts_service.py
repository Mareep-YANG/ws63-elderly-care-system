from __future__ import annotations

"""TTS 服务：调用硅基流动（SiliconFlow）文本转语音接口。

该接口遵循 OpenAI 兼容协议，可通过 `openai` Python SDK 直接访问。
"""

import asyncio
import uuid
from pathlib import Path
from typing import Optional

from openai import OpenAI

from src.config import settings

# ----------------------------------------------------------------------------
# 初始化 SiliconFlow Client
# ----------------------------------------------------------------------------
# Base URL 默认为 SiliconFlow OpenAI 兼容接口，如有需要可在 .env 中覆盖
# 参考文档：https://docs.siliconflow.cn/cn/userguide/capabilities/text-to-speech
# ---------------------------------------------------------------------------
_client = OpenAI(
    api_key="api_key",
    base_url="https://api.siliconflow.cn/v1",
)

# 默认模型及音色，可根据需要覆盖
_DEFAULT_MODEL = "FunAudioLLM/CosyVoice2-0.5B"
_DEFAULT_VOICE = f"{_DEFAULT_MODEL}:alex"


async def text_to_speech(
    text: str,
    *,
    model: str = _DEFAULT_MODEL,
    voice: str = _DEFAULT_VOICE,
    response_format: str = "wav",
    speed: float = 1.0,
    gain: float = 0.0,
    sample_rate: Optional[int] = None,
    output_dir: Optional[str | Path] = None,
) -> str:
    """将文本转换为语音文件并返回文件路径。

    参数说明:
        text:           待合成的文本。
        model:          TTS 模型名称，见文档《支持模型列表》。
        voice:          音色名称，可为系统预置或用户自定义音色 URI。
        response_format:输出音频格式，支持 mp3/wav/opus/pcm。
        speed:          语速控制，默认 1.0，取值范围 0.25~4.0。
        gain:           音频增益，单位 dB，默认 0.0，取值范围 -10~10。
        sample_rate:    采样率，可选。
        output_dir:     输出目录，默认为 uploads 目录。

    返回:
        生成文件的绝对路径 (str)。
    """

    loop = asyncio.get_running_loop()
    return await loop.run_in_executor(
        None,
        _sync_tts_request,
        text,
        model,
        voice,
        response_format,
        speed,
        gain,
        sample_rate,
        output_dir,
    )


# ---------------------------------------------------------------------------
# 同步执行，供线程池调用，避免阻塞事件循环
# ---------------------------------------------------------------------------

def _sync_tts_request(
    text: str,
    model: str,
    voice: str,
    response_format: str,
    speed: float,
    gain: float,
    sample_rate: Optional[int],
    output_dir: Optional[str | Path],
) -> str:
    """同步执行 TTS 请求并保存文件，返回文件路径。"""

    # 准备输出文件
    out_dir = Path(output_dir or settings.uploads_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    file_name = f"tts.{response_format}"
    out_path = out_dir / file_name

    # 组装额外参数，遵循 SiliconFlow 文档
    extra_body: dict[str, object] = {
        "speed": speed,
        "gain": gain,
    }
    if sample_rate is not None:
        extra_body["sample_rate"] = sample_rate

    # 向 SiliconFlow 发送 TTS 请求，并流式保存文件
    with _client.audio.speech.with_streaming_response.create(
        model=model,
        voice=voice,
        input=text,
        response_format=response_format,
        extra_body=extra_body,
    ) as response:
        response.stream_to_file(out_path)

    return str(out_path) 