import asyncio
import json
import logging
import re
from typing import Optional, Dict, Any, Callable, List
from datetime import datetime

import paho.mqtt.client as mqtt

# 兼容不同版本的paho-mqtt
try:
	from paho.mqtt.enums import CallbackAPIVersion

	MQTT_V2_API = True
except ImportError:
	# 较老版本的paho-mqtt不支持CallbackAPIVersion
	MQTT_V2_API = False
	CallbackAPIVersion = None

from src.config import settings

# 获取logger实例，不重复配置日志
logger = logging.getLogger(__name__)
logger.setLevel(getattr(logging, settings.loglevel.upper(), logging.INFO))

logger.info("🚀 MQTT服务模块已加载，日志系统已初始化")


class MQTTService:
	"""MQTT客户端服务类"""

	def __init__( self ):
		self.client: Optional[mqtt.Client] = None
		self.is_connected = False
		self.message_handlers: Dict[str, Callable] = {}
		self.topic_patterns: Dict[str, str] = {}  # 存储主题模式，用于匹配
		self.connection_retry_count = 0
		self.max_retry_count = 5
		self.loop: Optional[asyncio.AbstractEventLoop] = None
		
		# 定时报告相关属性
		self.report_task: Optional[asyncio.Task] = None
		self.weather_report = settings.mqtt_weather_report
		self.caution_report = settings.mqtt_caution_report
		
		logger.info("🔧 MQTT服务实例已创建")

	def initialize( self ):
		"""初始化MQTT客户端"""
		try:
			logger.info("⚙️ 开始初始化MQTT客户端...")

			# 获取当前事件循环
			try:
				self.loop = asyncio.get_running_loop()
				logger.info("✅ 获取到事件循环")
			except RuntimeError:
				self.loop = None
				logger.warning("⚠️ 没有运行中的事件循环，异步功能可能受限")

			# 创建MQTT客户端，兼容不同版本
			if MQTT_V2_API:
				# 新版本API (paho-mqtt >= 2.0)
				self.client = mqtt.Client(
					callback_api_version = CallbackAPIVersion.VERSION2,
					client_id = settings.mqtt_client_id,
					clean_session = settings.mqtt_clean_session
				)
				logger.info("✅ 使用 MQTT V2 API 创建客户端")
			else:
				# 旧版本API (paho-mqtt < 2.0)
				self.client = mqtt.Client(
					client_id = settings.mqtt_client_id,
					clean_session = settings.mqtt_clean_session
				)
				logger.info("✅ 使用 MQTT V1 API 创建客户端")

			# 设置用户名和密码
			if settings.mqtt_username and settings.mqtt_password:
				self.client.username_pw_set(
					settings.mqtt_username,
					settings.mqtt_password
				)
				logger.info("🔐 设置了MQTT认证信息")

			# 设置回调函数 - 根据API版本使用不同的回调签名
			if MQTT_V2_API:
				self.client.on_connect = self._on_connect_v2
				self.client.on_disconnect = self._on_disconnect_v2
				self.client.on_message = self._on_message_v2
				self.client.on_subscribe = self._on_subscribe_v2
				self.client.on_publish = self._on_publish_v2
			else:
				self.client.on_connect = self._on_connect_v1
				self.client.on_disconnect = self._on_disconnect_v1
				self.client.on_message = self._on_message_v1
				self.client.on_subscribe = self._on_subscribe_v1
				self.client.on_publish = self._on_publish_v1

			logger.info(f"✅ MQTT客户端初始化完成 (API版本: {'V2' if MQTT_V2_API else 'V1'})")

		except Exception as e:
			logger.error(f"❌ MQTT客户端初始化失败: {e}")
			raise

	async def connect( self ):
		"""异步连接到MQTT代理"""
		if not self.client:
			self.initialize()

		# 确保有事件循环
		if not self.loop:
			self.loop = asyncio.get_running_loop()

		try:
			logger.info(f"🔌 尝试连接到MQTT代理: {settings.mqtt_broker_host}:{settings.mqtt_broker_port}")

			# 设置连接参数
			self.client.connect_async(
				host = settings.mqtt_broker_host,
				port = settings.mqtt_broker_port,
				keepalive = settings.mqtt_keepalive
			)

			# 启动网络循环
			self.client.loop_start()
			logger.info("🔄 MQTT网络循环已启动")

			# 等待连接建立
			retry_count = 0
			while not self.is_connected and retry_count < 10:
				await asyncio.sleep(0.5)
				retry_count += 1
				logger.debug(f"⏳ 等待连接建立... ({retry_count}/10)")

			if not self.is_connected:
				raise Exception("MQTT连接超时")

			logger.info(f"✅ 成功连接到MQTT代理: {settings.mqtt_broker_host}:{settings.mqtt_broker_port}")

		except Exception as e:
			logger.error(f"❌ 连接MQTT代理失败: {e}")
			raise

	def disconnect( self ):
		"""断开MQTT连接"""
		if self.client:
			# 停止定时报告任务
			self.stop_periodic_report()
			# 断开连接
			self.client.loop_stop()
			self.client.disconnect()
			self.is_connected = False
			logger.info("🔌 MQTT连接已断开")

	# V2 API 回调函数 (paho-mqtt >= 2.0)
	def _on_connect_v2( self, client, userdata, flags, reason_code, properties = None ):
		"""连接回调 - V2 API"""
		self._handle_connect(reason_code)

	def _on_disconnect_v2( self, client, userdata, flags, reason_code, properties = None ):
		"""断开连接回调 - V2 API"""
		self._handle_disconnect(reason_code)

	def _on_message_v2( self, client, userdata, message ):
		"""消息接收回调 - V2 API"""
		self._handle_message(message)

	def _on_subscribe_v2( self, client, userdata, mid, reason_codes, properties = None ):
		"""订阅回调 - V2 API"""
		self._handle_subscribe(mid)

	def _on_publish_v2( self, client, userdata, mid, reason_codes = None, properties = None ):
		"""发布回调 - V2 API"""
		self._handle_publish(mid)

	# 统一的回调处理函数
	def _handle_connect( self, reason_code ):
		"""统一的连接处理"""
		if reason_code == 0:
			self.is_connected = True
			self.connection_retry_count = 0
			logger.info("🟢 MQTT连接成功")

		else:
			self.is_connected = False
			logger.error(f"🔴 MQTT连接失败，错误代码: {reason_code}")

	def _handle_disconnect( self, reason_code ):
		"""统一的断开连接处理"""
		self.is_connected = False
		logger.warning(f"🟡 MQTT连接断开，错误代码: {reason_code}")

		# 如果是意外断开，尝试重连
		if reason_code != 0 and self.connection_retry_count < self.max_retry_count:
			self.connection_retry_count += 1
			logger.info(f"🔄 尝试重连MQTT ({self.connection_retry_count}/{self.max_retry_count})")

	def _handle_message( self, message ):
		"""统一的消息处理"""
		try:
			topic = message.topic
			payload = message.payload.decode('utf-8')


			# 查找所有匹配的消息处理器
			matching_handlers = self._find_matching_handlers(topic)

			if matching_handlers:
				for pattern, handler in matching_handlers:
					self._schedule_async_handler(handler, topic, payload)
			else:
				logger.info(f"ℹ️ 没有注册的处理器处理主题: {topic}")

		except Exception as e:
			logger.error(f"❌ 处理MQTT消息失败: {e}")

	def _handle_subscribe( self, mid ):
		"""统一的订阅处理"""
		logger.info(f"✅ MQTT订阅成功，消息ID: {mid}")

	def _handle_publish( self, mid ):
		"""统一的发布处理"""
		logger.debug(f"📤 MQTT消息发布成功，消息ID: {mid}")

	def _schedule_async_handler( self, handler: Callable, topic: str, payload: str ):
		"""在正确的事件循环中调度异步处理器"""
		try:
			if self.loop and not self.loop.is_closed():
				# 使用线程安全的方式调度协程
				logger.debug(f"🔄 调度异步处理器: {topic}")
				future = asyncio.run_coroutine_threadsafe(
					handler(topic, payload),
					self.loop
				)
			# 不等待结果，让它在后台运行
			else:
				# 如果没有事件循环，记录警告
				logger.warning("⚠️ 没有可用的事件循环，无法调度异步处理器")
		except Exception as e:
			logger.error(f"❌ 调度异步处理器失败: {e}")

	def subscribe( self, topic: str, qos: int = 0 ):
		"""订阅主题"""
		if self.client and self.is_connected:
			result = self.client.subscribe(topic, qos)
			logger.info(f"📥 订阅主题: {topic} (QoS={qos})")
			return result
		else:
			logger.warning("⚠️ MQTT客户端未连接，无法订阅主题")
			return None

	def unsubscribe( self, topic: str ):
		"""取消订阅主题"""
		if self.client and self.is_connected:
			result = self.client.unsubscribe(topic)
			logger.info(f"📤 取消订阅主题: {topic}")
			return result
		else:
			logger.warning("⚠️ MQTT客户端未连接，无法取消订阅主题")
			return None

	def publish( self, topic: str, payload: Any, qos: int = 0, retain: bool = False ):
		"""发布消息"""
		if self.client and self.is_connected:
			# 如果payload是字典或列表，转换为JSON字符串
			if isinstance(payload, (dict, list)):
				payload = json.dumps(payload, ensure_ascii = False)

			result = self.client.publish(topic, payload, qos, retain)
			logger.info(f"📤 发布消息到主题: {topic}, 内容: {payload}")
			return result
		else:
			logger.warning("⚠️ MQTT客户端未连接，无法发布消息")
			return None

	def _topic_matches_pattern( self, topic: str, pattern: str ) -> bool:
		"""
		检查主题是否匹配给定的模式
		支持MQTT通配符：
		- + : 单级通配符，匹配一个层级
		- # : 多级通配符，匹配多个层级（只能在最后）
		"""
		# 如果模式就是普通字符串，检查是否为父主题
		if '+' not in pattern and '#' not in pattern:
			# 如果topic以pattern开头并且后面跟着'/'，则匹配
			if topic.startswith(pattern):
				if topic == pattern or topic.startswith(pattern + '/'):
					return True
			return False

		# 处理MQTT标准通配符
		# 将模式转换为正则表达式
		regex_pattern = pattern.replace('+', '[^/]+').replace('#', '.*')
		regex_pattern = '^' + regex_pattern + '$'

		try:
			return bool(re.match(regex_pattern, topic))
		except re.error:
			logger.error(f"❌ 无效的正则表达式模式: {regex_pattern}")
			return False

	def _find_matching_handlers( self, topic: str ) -> List[tuple]:
		"""
		查找所有匹配给定主题的处理器
		返回: [(pattern, handler), ...]
		"""
		matching_handlers = []

		for pattern, handler in self.message_handlers.items():
			if self._topic_matches_pattern(topic, pattern):
				matching_handlers.append((pattern, handler))
				logger.debug(f"🎯 主题 '{topic}' 匹配模式 '{pattern}'")

		return matching_handlers

	def add_message_handler( self, topic_pattern: str, handler: Callable, auto_subscribe: bool = True ):
		"""
		添加消息处理器，支持主题模式匹配

		Args:
			topic_pattern: 主题模式，支持：
						  - 精确匹配: "devices/sensor1"
						  - 父主题匹配: "devices" (会匹配 devices/*)
						  - 单级通配符: "devices/+/status" (匹配 devices/sensor1/status)
						  - 多级通配符: "devices/#" (匹配 devices下的所有子主题)
			handler: 消息处理函数
			auto_subscribe: 是否自动订阅相应的MQTT主题
		"""
		# 存储原始模式
		self.message_handlers[topic_pattern] = handler
		logger.info(f"🔧 为主题模式 '{topic_pattern}' 添加消息处理器")

		# 如果需要自动订阅
		if auto_subscribe:
			mqtt_topic = self._convert_pattern_to_mqtt_topic(topic_pattern)
			if mqtt_topic != topic_pattern:
				logger.info(f"🔄 转换主题模式: '{topic_pattern}' -> '{mqtt_topic}'")
			self.subscribe(mqtt_topic)

	def _convert_pattern_to_mqtt_topic( self, pattern: str ) -> str:
		"""
		将自定义模式转换为标准MQTT订阅主题
		"""
		# 如果已经包含通配符，直接返回
		if '+' in pattern or '#' in pattern:
			return pattern

		# 如果是普通字符串，添加多级通配符以匹配所有子主题
		if not pattern.endswith('/'):
			return pattern + '/#'
		else:
			return pattern + '#'

	def remove_message_handler( self, topic_pattern: str, auto_unsubscribe: bool = True ):
		"""
		移除消息处理器

		Args:
			topic_pattern: 要移除的主题模式
			auto_unsubscribe: 是否自动取消订阅
		"""
		if topic_pattern in self.message_handlers:
			del self.message_handlers[topic_pattern]
			logger.info(f"🗑️ 移除主题模式 '{topic_pattern}' 的消息处理器")

			if auto_unsubscribe:
				mqtt_topic = self._convert_pattern_to_mqtt_topic(topic_pattern)
				self.unsubscribe(mqtt_topic)

	def list_handlers( self ) -> Dict[str, str]:
		"""
		列出所有注册的消息处理器

		Returns:
			dict: {pattern: handler_name}
		"""
		handlers_info = {}
		for pattern, handler in self.message_handlers.items():
			handler_name = handler.__name__ if hasattr(handler, '__name__') else str(handler)
			handlers_info[pattern] = handler_name

		logger.info(f"📋 当前注册的处理器: {handlers_info}")
		return handlers_info

	def get_status( self ) -> Dict[str, Any]:
		"""获取MQTT客户端状态"""
		status = {
			"connected": self.is_connected,
			"broker_host": settings.mqtt_broker_host,
			"broker_port": settings.mqtt_broker_port,
			"client_id": settings.mqtt_client_id,
			"subscribed_topics": list(self.message_handlers.keys()),
			"retry_count": self.connection_retry_count,
			"has_event_loop": self.loop is not None and not self.loop.is_closed(),
			"api_version": "V2" if MQTT_V2_API else "V1"
		}
		logger.debug(f"📊 MQTT状态: {status}")
		return status

	def _calculate_time_report( self ) -> float:
		"""
		计算时间报告，格式为HH.MM的浮点数
		例如：13:14 -> 13.14
		"""
		now = datetime.now()
		time_report = now.hour + now.minute / 100.0
		return time_report

	async def _send_periodic_report( self ):
		"""定时发送报告的异步任务"""
		while True:
			try:
				if self.is_connected:
					# 构建报告数据
					report_data = {
						"weather": self.weather_report,
						"caution": self.caution_report,
						"time": self._calculate_time_report()
					}
					
					# 发送报告
					self.publish(settings.mqtt_subscribe_topic, report_data,retain = True)
					logger.info(f"📤 定时报告已发送: {report_data}")
				else:
					logger.warning("⚠️ MQTT未连接，跳过定时报告发送")
					
				# 等待下一次发送
				await asyncio.sleep(settings.mqtt_report_interval)
				
			except asyncio.CancelledError:
				logger.info("🛑 定时报告任务已取消")
				break
			except Exception as e:
				logger.error(f"❌ 发送定时报告失败: {e}")
				await asyncio.sleep(5)  # 出错后等待5秒再重试

	def start_periodic_report( self ):
		"""启动定时报告任务"""
		try:
			if self.report_task is None or self.report_task.done():
				if self.loop:
					self.report_task = self.loop.create_task(self._send_periodic_report())
					logger.info(f"🕐 定时报告任务已启动，间隔: {settings.mqtt_report_interval}秒")
				else:
					logger.warning("⚠️ 没有可用的事件循环，无法启动定时报告任务")
			else:
				logger.info("ℹ️ 定时报告任务已经在运行中")
		except Exception as e:
			logger.error(f"❌ 启动定时报告任务失败: {e}")

	def stop_periodic_report( self ):
		"""停止定时报告任务"""
		try:
			if self.report_task and not self.report_task.done():
				self.report_task.cancel()
				logger.info("🛑 定时报告任务已停止")
			else:
				logger.info("ℹ️ 定时报告任务未运行")
		except Exception as e:
			logger.error(f"❌ 停止定时报告任务失败: {e}")

	def set_weather_report( self, weather_code: int ):
		"""
		设置天气报告
		
		Args:
			weather_code (int): 天气代码
				0: 多云
				1: 下雪  
				2: 下雨
				3: 晴天
				4: 阴天
				5: 打雷
		"""
		if 0 <= weather_code <= 5:
			self.weather_report = weather_code
			logger.info(f"🌤️ 天气报告已设置为: {weather_code}")
		else:
			logger.error(f"❌ 无效的天气代码: {weather_code}，有效范围: 0-5")

	def set_caution_report( self, caution_code: int ):
		"""
		设置警告报告
		
		Args:
			caution_code (int): 警告代码
		"""
		self.caution_report = caution_code
		logger.info(f"⚠️ 警告报告已设置为: {caution_code}")

	def get_weather_description( self, weather_code: int ) -> str:
		"""
		获取天气代码的中文描述
		
		Args:
			weather_code (int): 天气代码
			
		Returns:
			str: 天气描述
		"""
		weather_descriptions = {
			0: "多云",
			1: "下雪",
			2: "下雨", 
			3: "晴天",
			4: "阴天",
			5: "打雷"
		}
		return weather_descriptions.get(weather_code, "未知")

	def get_report_status( self ) -> Dict[str, Any]:
		"""获取定时报告状态"""
		status = {
			"report_running": self.report_task is not None and not self.report_task.done(),
			"report_interval": settings.mqtt_report_interval,
			"weather_report": self.weather_report,
			"weather_description": self.get_weather_description(self.weather_report),
			"caution_report": self.caution_report,
			"subscribe_topic": settings.mqtt_subscribe_topic,
			"current_time_report": self._calculate_time_report()
		}
		logger.debug(f"📊 定时报告状态: {status}")
		return status


# 创建全局MQTT服务实例
mqtt_service = MQTTService()
# 创建全局的传感器数据表
sensor_data: Dict[str, Any] = {}
# 全局报警状态变量
alert_active = False

async def devices_handler( topic: str, payload: str ):
	"""
	处理所有devices下的传感器消息
	支持：devices/TemperatureSenser, devices/HumiditySenser, devices/LightSenser, devices/AirSenser
	"""

	try:
		# 解析主题获取传感器类型
		topic_parts = topic.split('/')
		if len(topic_parts) >= 2:
			sensor_type = topic_parts[1]  # TemperatureSenser, HumiditySenser 等
			# 尝试解析JSON数据
			try:
				data = json.loads(payload)
				# 把除了topic字段外的其他字段存进字典
				if isinstance(data, dict):
					sensor_data[sensor_type] = {k: v for k, v in data.items() if k != 'topic'}
					logger.info(f"📊 更新传感器数据: {sensor_type} -> {sensor_data[sensor_type]}")
				else:
					logger.warning(f"⚠️ 无效的传感器数据格式: {payload}")
			except json.JSONDecodeError:
				# 如果不是JSON，当作普通数值处理
				logger.error(f"JSON格式错误: {payload}")
	except Exception as e:
		logger.error(f"❌ 处理设备传感器消息失败: {e}")

async def caution_status_handler( topic: str, payload: str ):
	"""
	处理CautionStatus主题消息
	payload格式：{"topic":"CautionStatus","caution":1或0}
	当caution为1时，切换报警状态
	"""
	global alert_active
	
	try:
		# 解析JSON数据
		data = json.loads(payload)
		logger.info(f"📥 收到CautionStatus消息: {data}")
		
		if isinstance(data, dict) and 'caution' in data:
			caution_value = data['caution']
			logger.info(f"🚨 CautionStatus caution值: {caution_value}")
			
			if caution_value == 1:
				# 切换报警状态
				alert_active = not alert_active
				logger.info(f"🔄 报警状态切换为: {'激活' if alert_active else '关闭'}")
				
				# 导入agentService模块
				from .agentService import control_rgb, control_buzz
				
				if alert_active:
					# 激活报警：设置RGB为红灯，开启蜂鸣器
					logger.info("🔴 报警状态激活，设置RGB为红灯")
					control_rgb(mode=1, R=255, G=0, B=0)
					
					logger.info("🔊 报警状态激活，开启蜂鸣器")
					control_buzz(start=1)
				else:
					# 关闭报警：关闭RGB灯和蜂鸣器
					logger.info("✅ 报警状态关闭，关闭RGB灯和蜂鸣器")
					control_rgb(mode=0, R=0, G=0, B=0)  # 关闭RGB灯
					control_buzz(start=0)  # 关闭蜂鸣器
				
		else:
			logger.warning(f"⚠️ CautionStatus消息格式不正确: {payload}")
			
	except json.JSONDecodeError:
		logger.error(f"❌ CautionStatus消息JSON解析失败: {payload}")
	except Exception as e:
		logger.error(f"❌ 处理CautionStatus消息失败: {e}")

# 便捷函数
async def start_mqtt_service():
	"""启动MQTT服务"""
	try:
		logger.info("🚀 启动MQTT服务...")
		mqtt_service.initialize()
		await mqtt_service.connect()
		logger.info("✅ MQTT服务启动成功，开始添加订阅")
		mqtt_service.add_message_handler("devices/#", devices_handler, auto_subscribe=True)
		mqtt_service.add_message_handler("CautionStatus", caution_status_handler, auto_subscribe=True)
		# 启动定时报告任务
		mqtt_service.start_periodic_report()
		logger.info("✅ 定时报告任务已启动")
	except Exception as e:
		logger.error(f"❌ MQTT服务启动失败: {e}")
		raise


def stop_mqtt_service():
	"""停止MQTT服务"""
	try:
		# 停止定时报告任务
		mqtt_service.stop_periodic_report()
		# 断开MQTT连接
		mqtt_service.disconnect()
		logger.info("🛑 MQTT服务已停止")
	except Exception as e:
		logger.error(f"❌ 停止MQTT服务时出错: {e}")


def publish_message( topic: str, payload: Any, qos: int = 0, retain: bool = False ):
	"""发布消息的便捷函数"""
	return mqtt_service.publish(topic, payload, qos, retain)


def subscribe_topic( topic: str, handler: Optional[Callable] = None, qos: int = 0 ):
	"""订阅主题的便捷函数"""
	result = mqtt_service.subscribe(topic, qos)
	if handler:
		mqtt_service.add_message_handler(topic, handler)
	return result


def set_weather_report( weather_code: int ):
	"""设置天气报告的便捷函数"""
	return mqtt_service.set_weather_report(weather_code)


def set_caution_report( caution_code: int ):
	"""设置警告报告的便捷函数"""
	return mqtt_service.set_caution_report(caution_code)


def get_report_status():
	"""获取定时报告状态的便捷函数"""
	return mqtt_service.get_report_status()


def start_periodic_report():
	"""启动定时报告的便捷函数"""
	return mqtt_service.start_periodic_report()


def stop_periodic_report():
	"""停止定时报告的便捷函数"""
	return mqtt_service.stop_periodic_report()


def get_alert_status():
	"""获取当前报警状态"""
	return alert_active


def set_alert_status(status: bool):
	"""设置报警状态"""
	global alert_active
	alert_active = status
	logger.info(f"🔧 手动设置报警状态为: {'激活' if alert_active else '关闭'}")
