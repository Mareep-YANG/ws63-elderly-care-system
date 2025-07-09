from pathlib import Path
from functools import lru_cache
from dotenv import load_dotenv
from pydantic_settings import BaseSettings  # Pydantic v2 官方推荐         # 兼容旧版

# 项目根目录
BASE_DIR = Path(__file__).resolve().parent.parent
# .env 文件路径（可不存在）
ENV_FILE = BASE_DIR / ".env"

# 先加载 .env 中的环境变量，让 Settings 能读取到
load_dotenv(ENV_FILE, override = True)


class Settings(BaseSettings):
	"""项目统一配置类，通过环境变量或 .env 文件进行覆盖。"""

	# 日志等级，例如 DEBUG/INFO/WARNING/ERROR
	loglevel: str = "INFO"
	# 上传文件存放目录
	uploads_dir: str = str(BASE_DIR / "uploads")
	# WebSocket 传输时的块大小
	chunk_size: int = 64 * 1024
	# ASR模型名称（从ModelScope自动下载）
	stt_model_name: str = "paraformer-zh"
	# LangSmith项目名
	langsmith_project: str
	# LangSmith API Key
	langsmith_api_key: str
	# OpenAI 格式API KEY
	openai_api_key: str
	# OpenAI 格式API Base URL
	openai_api_base: str
	# Agent使用的模型名字
	agent_model: str = "openai/Pro/deepseek-ai/DeepSeek-V3"
	# TAVILY API Key
	tavily_api_key: str
	
	# MQTT 相关配置
	mqtt_broker_host: str = "localhost"
	mqtt_broker_port: int = 1883
	mqtt_username: str = ""
	mqtt_password: str = ""
	mqtt_client_id: str = "hispark_agent"
	mqtt_keepalive: int = 120
	mqtt_clean_session: bool = True
	# MQTT 主题配置
	mqtt_topic_prefix: str = "hispark/"
	
	# MQTT 报告配置
	mqtt_subscribe_topic: str = "report"
	mqtt_report_interval: int = 5  # 发送间隔，单位秒
	mqtt_weather_report: int = 4  # 天气报告：0多云，1下雪，2下雨，3晴天，4阴天，5打雷
	mqtt_caution_report: int = 0  # 警告报告，暂时固定为0

	class Config:
		env_file = ENV_FILE
		env_file_encoding = "utf-8"


@lru_cache()
def get_settings() -> "Settings":
	"""单例化 Settings，避免重复解析。"""
	return Settings()


# 对外暴露单例对象，方便直接导入使用
settings: Settings = get_settings()
