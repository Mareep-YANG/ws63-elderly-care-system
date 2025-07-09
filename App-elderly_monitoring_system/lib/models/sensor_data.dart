import 'dart:convert';

// 基础传感器数据模型
abstract class SensorData {
  final String topic;
  final DateTime timestamp;
  
  SensorData({required this.topic, required this.timestamp});
  
  Map<String, dynamic> toJson();
  
  factory SensorData.fromJson(Map<String, dynamic> json) {
    switch (json['topic']) {
      case 'TemperatureSenser':
        return TemperatureSensorData.fromJson(json);
      case 'HumiditySenser':
        return HumiditySensorData.fromJson(json);
      case 'LightSenser':
        return LightSensorData.fromJson(json);
      case 'AirSenser':
        return AirSensorData.fromJson(json);
      default:
        throw ArgumentError('Unknown sensor topic: ${json['topic']}');
    }
  }
}

// 温度传感器数据
class TemperatureSensorData extends SensorData {
  final double temperature;
  
  TemperatureSensorData({
    required this.temperature,
    DateTime? timestamp,
  }) : super(topic: 'TemperatureSenser', timestamp: timestamp ?? DateTime.now());
  
  factory TemperatureSensorData.fromJson(Map<String, dynamic> json) {
    return TemperatureSensorData(
      temperature: (json['temperature'] as num).toDouble(),
      timestamp: DateTime.now(),
    );
  }
  
  @override
  Map<String, dynamic> toJson() => {
    'topic': topic,
    'temperature': temperature,
  };
}

// 湿度传感器数据
class HumiditySensorData extends SensorData {
  final double humidity;
  
  HumiditySensorData({
    required this.humidity,
    DateTime? timestamp,
  }) : super(topic: 'HumiditySenser', timestamp: timestamp ?? DateTime.now());
  
  factory HumiditySensorData.fromJson(Map<String, dynamic> json) {
    return HumiditySensorData(
      humidity: (json['humidity'] as num).toDouble(),
      timestamp: DateTime.now(),
    );
  }
  
  @override
  Map<String, dynamic> toJson() => {
    'topic': topic,
    'humidity': humidity,
  };
}

// 光照传感器数据
class LightSensorData extends SensorData {
  final int light;
  
  LightSensorData({
    required this.light,
    DateTime? timestamp,
  }) : super(topic: 'LightSenser', timestamp: timestamp ?? DateTime.now());
  
  factory LightSensorData.fromJson(Map<String, dynamic> json) {
    return LightSensorData(
      light: json['light'] as int,
      timestamp: DateTime.now(),
    );
  }
  
  @override
  Map<String, dynamic> toJson() => {
    'topic': topic,
    'light': light,
  };
}

// 空气质量传感器数据
class AirSensorData extends SensorData {
  final double air;
  
  AirSensorData({
    required this.air,
    DateTime? timestamp,
  }) : super(topic: 'AirSenser', timestamp: timestamp ?? DateTime.now());
  
  factory AirSensorData.fromJson(Map<String, dynamic> json) {
    return AirSensorData(
      air: (json['air'] as num).toDouble(),
      timestamp: DateTime.now(),
    );
  }
  
  @override
  Map<String, dynamic> toJson() => {
    'topic': topic,
    'air': air,
  };
  
  // 获取空气质量级别描述
  String get airQualityDescription {
    if (air <= 5.0) return '优';
    if (air <= 10.0) return '良';
    if (air <= 20.0) return '轻度污染';
    if (air <= 30.0) return '中度污染';
    return '重度污染';
  }
}

// 环境数据聚合模型
class EnvironmentData {
  final TemperatureSensorData? temperature;
  final HumiditySensorData? humidity;
  final LightSensorData? light;
  final AirSensorData? air;
  
  EnvironmentData({
    this.temperature,
    this.humidity,
    this.light,
    this.air,
  });
  
  // 计算环境综合评分
  double get safetyScore {
    double score = 100.0;
    
    // 温度评分 (18-26°C为最佳)
    if (temperature != null) {
      final temp = temperature!.temperature;
      if (temp < 10 || temp > 35) score -= 20;
      else if (temp < 18 || temp > 26) score -= 10;
    }
    
    // 湿度评分 (40-60%为最佳)
    if (humidity != null) {
      final hum = humidity!.humidity;
      if (hum < 30 || hum > 80) score -= 15;
      else if (hum < 40 || hum > 60) score -= 5;
    }
    
    // 空气质量评分
    if (air != null) {
      final airQuality = air!.air;
      if (airQuality > 30.0) score -= 30;
      else if (airQuality > 20.0) score -= 20;
      else if (airQuality > 10.0) score -= 10;
    }
    
    return score.clamp(0, 100);
  }
} 