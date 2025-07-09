import logging
from typing import Optional
import asyncio
from contextlib import asynccontextmanager

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from funasr import AutoModel

from src.config import settings

logging.basicConfig(
	level = getattr(logging, settings.loglevel.upper(), logging.INFO),
	format = "[%(asctime)s] %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)



@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理"""
    # 启动时执行
    logger.info("应用启动中...")
    
    # 启动MQTT服务
    try:
        from src.sevices.mqtt_service import start_mqtt_service
        await start_mqtt_service()
        logger.info("MQTT服务已启动")
    except Exception as e:
        logger.error(f"启动MQTT服务失败: {e}")
    
    yield
    
    # 关闭时执行
    logger.info("应用关闭中...")
    
    # 停止MQTT服务
    try:
        from src.sevices.mqtt_service import stop_mqtt_service
        stop_mqtt_service()
        logger.info("MQTT服务已停止")
    except Exception as e:
        logger.error(f"停止MQTT服务失败: {e}")


app = FastAPI(title="Bidirectional File WebSocket", lifespan=lifespan)

# 添加CORS中间件
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 在生产环境中应该指定具体的域名
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
# 关键：导入后面的模块，让装饰器运行
from src.sevices import websocket_service  # noqa: E402, F401
