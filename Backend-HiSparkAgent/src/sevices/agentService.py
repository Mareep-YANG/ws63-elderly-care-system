import logging

from .mqtt_service import publish_message

logger = logging.getLogger(__name__)


def control_engine( engine_id: int, angle: int ):
	"""
	控制单个窗帘
	:param engine_id: 窗帘ID (1, 2, or 3)
	:param angle: 角度，即窗帘的打开程度 (0-180)
	"""
	topic = f"Control/EngineControl_{engine_id}"
	payload = {"topic": f"EngineControl_{engine_id}", "Angle": angle}
	logger.info(f"发送引擎控制消息: topic={topic}, payload={payload}")
	result = publish_message(topic, payload, retain = True)
	if result is None:
		logger.error(f"❌ 引擎控制消息发送失败: {topic}")
		raise ConnectionError("MQTT消息发送失败，请检查MQTT连接状态")
	return result


def control_steering( start: int ):
	"""
	控制风扇
	:param start: 1 for start, 0 for stop
	"""
	topic = "Control/SteeringControl"
	payload = {"topic": "SteeringControl", "Start": start}
	logger.info(f"发送风扇控制消息: topic={topic}, payload={payload}")
	result = publish_message(topic, payload, retain = True)
	if result is None:
		logger.error(f"❌ 风扇控制消息发送失败: {topic}")
		raise ConnectionError("MQTT消息发送失败，请检查MQTT连接状态")
	return result


def control_rgb( mode: int, R: int , G: int , B: int ):
	"""
	控制RGB灯
	:param mode: 灯光模式,0表示炫彩呼吸灯,1表示常亮不同的灯光颜色,当mode为1时，RGB三个属性使能
	:param R: 红色通道值 (0-255)
	:param G: 绿色通道值 (0-255)
	:param B: 蓝色通道值 (0-255)
	"""
	topic = "Control/RGBControl"
	payload = {"topic": "RGBControl", "Mode": mode, "Red": R, "Green": G, "Blue": B}
	logger.info(f"发送RGB控制消息: topic={topic}, payload={payload}")
	result = publish_message(topic, payload, retain = True)
	if result is None:
		logger.error(f"❌ RGB控制消息发送失败: {topic}")
		raise ConnectionError("MQTT消息发送失败，请检查MQTT连接状态")
	return result


def control_buzz( start: int ):
	"""
	控制蜂鸣器
	:param start: 1 for start, 0 for stop
	"""
	topic = "Control/BuzzControl"
	payload = {"topic": "BuzzControl", "Start": start}
	logger.info(f"发送蜂鸣器控制消息: topic={topic}, payload={payload}")
	result = publish_message(topic, payload, retain = True)
	if result is None:
		logger.error(f"❌ 蜂鸣器控制消息发送失败: {topic}")
		raise ConnectionError("MQTT消息发送失败，请检查MQTT连接状态")
	return result



