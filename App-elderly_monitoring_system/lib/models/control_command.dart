import 'dart:convert';

// 基础控制指令模型
abstract class ControlCommand {
  final String topic;
  final DateTime timestamp;

  ControlCommand({required this.topic, required this.timestamp});

  Map<String, dynamic> toJson();
  String get mqttTopic;
}

// 窗帘控制指令
class EngineControlCommand extends ControlCommand {
  final int engineId;
  final int angle;

  EngineControlCommand({
    required this.engineId,
    required this.angle,
    DateTime? timestamp,
  }) : super(
         topic: 'EngineControl_$engineId',
         timestamp: timestamp ?? DateTime.now(),
       );

  @override
  String get mqttTopic => 'Control/EngineControl_$engineId';

  @override
  Map<String, dynamic> toJson() => {'topic': topic, 'Angle': angle};

  // 获取窗帘开启百分比
  double get openPercentage => (angle / 180.0 * 100).clamp(0, 100);
}

// 风扇控制指令
class SteeringControlCommand extends ControlCommand {
  final bool isStart;

  SteeringControlCommand({required this.isStart, DateTime? timestamp})
    : super(topic: 'SteeringControl', timestamp: timestamp ?? DateTime.now());

  @override
  String get mqttTopic => 'Control/SteeringControl';

  @override
  Map<String, dynamic> toJson() => {'topic': topic, 'Start': isStart ? 1 : 0};
}

// RGB灯控制指令
class RGBControlCommand extends ControlCommand {
  final int mode; // 0: 炫彩呼吸灯, 1: 常量模式
  final int red;
  final int green;
  final int blue;

  RGBControlCommand({
    required this.mode,
    this.red = 0,
    this.green = 0,
    this.blue = 0,
    DateTime? timestamp,
  }) : super(topic: 'RGBControl', timestamp: timestamp ?? DateTime.now());

  // 创建炫彩呼吸灯模式
  factory RGBControlCommand.breathingMode() {
    return RGBControlCommand(mode: 0);
  }

  // 创建常量颜色模式
  factory RGBControlCommand.constantColor({
    required int red,
    required int green,
    required int blue,
  }) {
    return RGBControlCommand(
      mode: 1,
      red: red.clamp(0, 255),
      green: green.clamp(0, 255),
      blue: blue.clamp(0, 255),
    );
  }

  @override
  String get mqttTopic => 'Control/RGBControl';

  @override
  Map<String, dynamic> toJson() => {
    'topic': topic,
    'Mode': mode,
    'Red': red,
    'Green': green,
    'Blue': blue,
  };

  // 获取颜色描述
  String get colorDescription {
    if (mode == 0) return '炫彩呼吸灯';
    return 'RGB($red, $green, $blue)';
  }
}

// 蜂鸣器控制指令
class BuzzControlCommand extends ControlCommand {
  final bool isStart;

  BuzzControlCommand({required this.isStart, DateTime? timestamp})
    : super(topic: 'BuzzControl', timestamp: timestamp ?? DateTime.now());

  @override
  String get mqttTopic => 'Control/BuzzControl';

  @override
  Map<String, dynamic> toJson() => {'topic': topic, 'Start': isStart ? 1 : 0};
}

// 设备状态模型
class DeviceStatus {
  final String deviceId;
  final String deviceName;
  final bool isOnline;
  final bool hasError;
  final String? errorMessage;
  final DateTime lastUpdate;

  DeviceStatus({
    required this.deviceId,
    required this.deviceName,
    required this.isOnline,
    this.hasError = false,
    this.errorMessage,
    DateTime? lastUpdate,
  }) : lastUpdate = lastUpdate ?? DateTime.now();

  // 获取设备状态描述
  String get statusDescription {
    if (!isOnline) return '离线';
    if (hasError) return '异常';
    return '正常';
  }
}

// 预定义的常用控制指令
class CommonControlCommands {
  // 窗帘控制
  static EngineControlCommand openCurtain(
    int engineId, {
    int percentage = 100,
  }) {
    return EngineControlCommand(
      engineId: engineId,
      angle: (percentage / 100.0 * 180).round(),
    );
  }

  static EngineControlCommand closeCurtain(int engineId) {
    return EngineControlCommand(engineId: engineId, angle: 0);
  }

  // 风扇控制
  static SteeringControlCommand turnOnFan() {
    return SteeringControlCommand(isStart: true);
  }

  static SteeringControlCommand turnOffFan() {
    return SteeringControlCommand(isStart: false);
  }

  // RGB灯控制
  static RGBControlCommand turnOnBreathingLight() {
    return RGBControlCommand.breathingMode();
  }

  static RGBControlCommand setWhiteLight() {
    return RGBControlCommand.constantColor(red: 255, green: 255, blue: 255);
  }

  static RGBControlCommand setWarmLight() {
    return RGBControlCommand.constantColor(red: 255, green: 180, blue: 120);
  }

  static RGBControlCommand turnOffLight() {
    return RGBControlCommand.constantColor(red: 0, green: 0, blue: 0);
  }

  // 蜂鸣器控制
  static BuzzControlCommand turnOnBuzz() {
    return BuzzControlCommand(isStart: true);
  }

  static BuzzControlCommand turnOffBuzz() {
    return BuzzControlCommand(isStart: false);
  }
}
