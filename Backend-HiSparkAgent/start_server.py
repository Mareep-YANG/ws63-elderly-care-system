#!/usr/bin/env python3
"""
HiSparkAgent WebSocket服务器启动脚本
"""

import uvicorn
from src.config import settings

def main():
    """启动WebSocket服务器"""
    print("🚀 启动HiSparkAgent WebSocket服务器...")
    print(f"📁 上传目录: {settings.uploads_dir}")
    print(f"📊 日志级别: {settings.loglevel}")
    print(f"📦 传输块大小: {settings.chunk_size} bytes")
    
    uvicorn.run(
        "src.server:app",
        host="0.0.0.0",  # 允许所有IP访问
        port=8000,
        log_level=settings.loglevel.lower(),
        reload=True,  # 生产环境关闭热重载
        ws_ping_interval=20,  # WebSocket ping间隔
        ws_ping_timeout=20,   # WebSocket ping超时
        access_log=True,
    )

if __name__ == "__main__":
    main() 