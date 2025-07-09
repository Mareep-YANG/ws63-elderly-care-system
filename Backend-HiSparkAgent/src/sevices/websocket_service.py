import os
import json
import asyncio
from typing import Optional
import uuid

from starlette.websockets import WebSocket, WebSocketDisconnect

from agent.graph import run_agent_graph
from sevices import convert_to_wav
from sevices.tts_service import text_to_speech
from src.agent import graph
from src.server import app, logger
from src.config import settings
from src.sevices.connection_manager import connection_manager
from src.sevices.audio_stream_manager import audio_stream_manager
from src.sevices.speaktotext_service import speak_to_text

UPLOAD_DIR = settings.uploads_dir
CHUNK_SIZE = settings.chunk_size  # bytes

os.makedirs(UPLOAD_DIR, exist_ok = True)


@app.websocket("/ws")
async def file_hub( websocket: WebSocket ):
	"""长连接文件中心：支持 UPLOAD 与 DOWNLOAD 指令。"""
	await websocket.accept()
	client = websocket.client
	# 为每个连接生成唯一的客户端ID
	client_id = f"{client.host}:{client.port}_{uuid.uuid4().hex[:8]}"
	logger.info("客户端 %s 已连接，分配ID: %s", client, client_id)
	
	# 注册连接到全局管理器
	connection_manager.add_connection(client_id, websocket)

	try:
		while True:
			# 只接收文本帧作为指令
			first_msg = await websocket.receive()

			if first_msg.get("type") == "websocket.disconnect":
				logger.info("客户端 %s 断开连接", client)
				break

			if (cmd_text := first_msg.get("text")) is None:
				logger.warning("收到非文本控制帧，忽略: %s", first_msg)
				continue

			parts = cmd_text.strip().split(maxsplit = 1)
			if not parts:
				continue

			command = parts[0].upper()
			arg: Optional[str] = parts[1] if len(parts) > 1 else None

			if command == "UPLOAD" and arg:
				filename = arg
				await handle_upload(websocket, filename)

			elif command == "STREAM_REQUEST" and arg:
				filename = arg
				await handle_stream_request(websocket, client_id, filename)

			elif command == "STREAM_CONTROL" and arg:
				# 格式: STREAM_CONTROL <stream_id> <action>
				control_parts = arg.split(maxsplit=1)
				if len(control_parts) == 2:
					stream_id, action = control_parts
					await handle_stream_control(websocket, stream_id, action)
				else:
					await websocket.send_text(f"ERROR 流控制指令格式错误: {cmd_text}")

			elif command == "STREAM_STATUS" and arg:
				# 格式: STREAM_STATUS <stream_id> <status>
				status_parts = arg.split(maxsplit=1)
				if len(status_parts) == 2:
					stream_id, status = status_parts
					await handle_stream_status(websocket, stream_id, status)
				else:
					await websocket.send_text(f"ERROR 流状态指令格式错误: {cmd_text}")

			elif command == "BYE":
				logger.info("收到 BYE，关闭会话")
				break
			else:
				await websocket.send_text(f"ERROR 未知指令: {cmd_text}")
	except WebSocketDisconnect:
		logger.warning("客户端 %s 非正常断开", client)
	except Exception as exc:
		logger.exception("处理过程中发生异常: %s", exc)
		try:
			await websocket.send_text(f"ERROR {exc}")
		except Exception:
			pass
	finally:
		# 清理客户端的所有音频流
		audio_stream_manager.cleanup_client_streams(client_id)
		# 从连接管理器中移除连接
		connection_manager.remove_connection(client_id)
		logger.info("与客户端 %s 的会话结束", client)





async def handle_upload( ws: WebSocket, filename: str ):
	"""处理上传文件流程。"""
	path = os.path.join(UPLOAD_DIR, os.path.basename(filename))
	logger.debug("准备接收上传文件 %s", filename)
	try:
		with open(path, "wb") as f:
			while True:
				message = await ws.receive()
				if message.get("type") == "websocket.disconnect":
					raise WebSocketDisconnect(message.get("code", 1006))
				data_bytes = message.get("bytes")
				data_text = message.get("text")
				if data_bytes is not None:
					f.write(data_bytes)
				elif data_text is not None:
					if data_text == "EOF":
						break
					else:
						logger.warning("上传期间收到未知文本: %s", data_text)
				else:
					logger.warning("上传期间收到未知帧: %s", message)
		await ws.send_text(f"OK UPLOAD {filename} {os.path.getsize(path)}")
		logger.info("文件 %s 上传完成，开始ASR流程", filename)
		asr_content = await speak_to_text(path)
		print(asr_content)
		response = await run_agent_graph(asr_content)
		print(response)
		path = await text_to_speech(response)
		print("音频已保存：", path)
		path_wav = await convert_to_wav(path)
		await handle_stream_request(ws,connection_manager.get_client_list()[-1] , path_wav)
	except WebSocketDisconnect:
		logger.warning("上传文件 %s 时客户端断开，删除临时文件", filename)
		if os.path.exists(path):
			os.remove(path)
		raise
	except Exception as exc:
		logger.exception("上传文件失败: %s", exc)
		if os.path.exists(path):
			os.remove(path)
		await ws.send_text(f"ERROR UPLOAD {filename}: {exc}")




async def handle_stream_request(ws: WebSocket, client_id: str, filename: str):
	"""处理流式播放请求"""
	try:
		# 构建文件路径
		file_path = os.path.join(UPLOAD_DIR, os.path.basename(filename))
		
		# 检查文件是否存在
		if not os.path.exists(file_path):
			await ws.send_text(f"ERROR STREAM 1001 文件不存在: {filename}")
			logger.warning(f"请求流式播放不存在的文件: {filename}")
			return
		
		# 检查文件是否为WAV格式
		if not filename.lower().endswith('.wav'):
			await ws.send_text(f"ERROR STREAM 1002 文件格式不支持: {filename}")
			logger.warning(f"请求流式播放非WAV文件: {filename}")
			return
		
		# 创建音频流
		stream_id = audio_stream_manager.create_stream(client_id, filename, file_path)
		if not stream_id:
			await ws.send_text(f"ERROR STREAM 1002 创建音频流失败: {filename}")
			logger.error(f"创建音频流失败: {filename}")
			return
		
		# 获取音频格式信息
		format_info = audio_stream_manager.get_stream_format_info(stream_id)
		if not format_info:
			await ws.send_text(f"ERROR STREAM 1002 获取音频格式失败: {filename}")
			logger.error(f"获取音频格式失败: {filename}")
			return
		
		# 开始流传输
		if not audio_stream_manager.start_stream(stream_id):
			await ws.send_text(f"ERROR STREAM 1004 启动流传输失败: {filename}")
			logger.error(f"启动流传输失败: {filename}")
			return
		
		# 发送流开始消息
		format_json = json.dumps(format_info)
		await ws.send_text(f"STREAM_START {stream_id} {filename} {format_json}")
		logger.info(f"开始流式播放: {filename}, 流ID: {stream_id}")
		
		# 启动音频数据流传输任务
		asyncio.create_task(stream_audio_data(ws, stream_id))
		
	except Exception as exc:
		logger.exception(f"处理流式播放请求失败: {exc}")
		await ws.send_text(f"ERROR STREAM 1005 处理请求失败: {exc}")


async def handle_stream_control(ws: WebSocket, stream_id: str, action: str):
	"""处理流控制指令"""
	try:
		action = action.upper()
		
		if action == "PAUSE":
			if audio_stream_manager.pause_stream(stream_id):
				await ws.send_text(f"STREAM_PAUSE {stream_id}")
				logger.info(f"暂停流: {stream_id}")
			else:
				await ws.send_text(f"ERROR STREAM {stream_id} 1003 流不存在或状态错误")
		
		elif action == "RESUME":
			if audio_stream_manager.resume_stream(stream_id):
				await ws.send_text(f"STREAM_RESUME {stream_id}")
				logger.info(f"恢复流: {stream_id}")
			else:
				await ws.send_text(f"ERROR STREAM {stream_id} 1003 流不存在或状态错误")
		
		elif action == "STOP":
			if audio_stream_manager.stop_stream(stream_id):
				await ws.send_text(f"STREAM_STOP {stream_id}")
				logger.info(f"停止流: {stream_id}")
			else:
				await ws.send_text(f"ERROR STREAM {stream_id} 1003 流不存在")
		
		else:
			await ws.send_text(f"ERROR STREAM {stream_id} 1004 未知控制指令: {action}")
			logger.warning(f"未知流控制指令: {action}")
	
	except Exception as exc:
		logger.exception(f"处理流控制指令失败: {exc}")
		await ws.send_text(f"ERROR STREAM {stream_id} 1005 处理失败: {exc}")


async def handle_stream_status(ws: WebSocket, stream_id: str, status: str):
	"""处理客户端流状态反馈"""
	try:
		logger.debug(f"收到流状态反馈: {stream_id} -> {status}")
		
		# 这里可以根据客户端状态调整服务器行为
		# 例如：如果客户端缓冲区满了，可以暂停发送
		if status == "BUFFERING":
			logger.info(f"客户端 {stream_id} 缓冲中")
		elif status == "PLAYING":
			logger.info(f"客户端 {stream_id} 正在播放")
		elif status == "ERROR":
			logger.warning(f"客户端 {stream_id} 播放错误")
			# 可以选择停止流
			audio_stream_manager.stop_stream(stream_id)
		
	except Exception as exc:
		logger.exception(f"处理流状态反馈失败: {exc}")


async def stream_audio_data(ws: WebSocket, stream_id: str):
	"""流式传输音频数据的后台任务"""
	try:
		logger.info(f"开始音频数据流传输任务: {stream_id}")
		
		while True:
			# 读取音频数据块
			chunk = audio_stream_manager.read_chunk(stream_id)
			
			if chunk is None:
				# 数据读取完毕或出错
				session = audio_stream_manager.get_stream(stream_id)
				if session and session.state.value == "STOPPED":
					# 正常结束
					await ws.send_text(f"STREAM_END {stream_id}")
					logger.info(f"音频流传输完成: {stream_id}")
				else:
					# 出错
					await ws.send_text(f"ERROR STREAM {stream_id} 1005 数据读取错误")
					logger.error(f"音频流数据读取错误: {stream_id}")
				break
			
			# 发送音频数据
			await ws.send_bytes(chunk)
			
			# 检查流状态
			session = audio_stream_manager.get_stream(stream_id)
			if not session or not session.is_active:
				break
			
			# 如果暂停，等待恢复
			if session.state.value == "PAUSED":
				while session.state.value == "PAUSED" and session.is_active:
					await asyncio.sleep(0.1)
				if not session.is_active:
					break
			
			# 控制发送速度，避免发送过快
			await asyncio.sleep(0.01)  # 10ms延迟
		
		# 清理流
		audio_stream_manager.stop_stream(stream_id)
		
	except Exception as exc:
		logger.exception(f"音频数据流传输任务失败: {exc}")
		try:
			await ws.send_text(f"ERROR STREAM {stream_id} 1005 传输任务异常: {exc}")
		except Exception:
			pass
		finally:
			audio_stream_manager.stop_stream(stream_id)
