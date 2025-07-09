import 'dart:convert';
import 'package:flutter/material.dart';

// 天气枚举
enum WeatherType {
  cloudy(0, '多云'),
  snowy(1, '下雪'),
  rainy(2, '下雨'),
  sunny(3, '晴天'),
  overcast(4, '阴天'),
  thunderstorm(5, '打雷');
  
  const WeatherType(this.value, this.description);
  
  final int value;
  final String description;
  
  static WeatherType fromValue(int value) {
    return WeatherType.values.firstWhere(
      (e) => e.value == value,
      orElse: () => WeatherType.cloudy,
    );
  }
}

// 警告级别枚举
enum CautionLevel {
  normal(0, '运行正常'),
  error(1, '出现错误');
  
  const CautionLevel(this.value, this.description);
  
  final int value;
  final String description;
  
  static CautionLevel fromValue(int value) {
    return CautionLevel.values.firstWhere(
      (e) => e.value == value,
      orElse: () => CautionLevel.normal,
    );
  }
}

// 报告数据模型
class ReportData {
  final WeatherType weather;
  final CautionLevel caution;
  final double time; // 小时.分钟格式
  final DateTime timestamp;
  
  ReportData({
    required this.weather,
    required this.caution,
    required this.time,
    DateTime? timestamp,
  }) : timestamp = timestamp ?? DateTime.now();
  
  factory ReportData.fromJson(Map<String, dynamic> json) {
    return ReportData(
      weather: WeatherType.fromValue(json['weather'] as int),
      caution: CautionLevel.fromValue(json['caution'] as int),
      time: (json['time'] as num).toDouble(),
      timestamp: DateTime.now(),
    );
  }
  
  Map<String, dynamic> toJson() => {
    'weather': weather.value,
    'caution': caution.value,
    'time': time,
  };
  
  // 获取格式化的时间字符串
  String get formattedTime {
    final hours = time.floor();
    final minutes = ((time - hours) * 60).round();
    return '${hours.toString().padLeft(2, '0')}:${minutes.toString().padLeft(2, '0')}';
  }
  
  // 获取时间段描述
  String get timeDescription {
    final hours = time.floor();
    if (hours >= 5 && hours < 12) return '早上';
    if (hours >= 12 && hours < 18) return '下午';
    if (hours >= 18 && hours < 22) return '晚上';
    return '深夜';
  }
  
  // 是否是系统正常状态
  bool get isSystemNormal => caution == CautionLevel.normal;
  
  // 获取天气图标
  String get weatherIcon {
    switch (weather) {
      case WeatherType.cloudy:
        return '☁️';
      case WeatherType.snowy:
        return '❄️';
      case WeatherType.rainy:
        return '🌧️';
      case WeatherType.sunny:
        return '☀️';
      case WeatherType.overcast:
        return '⛅';
      case WeatherType.thunderstorm:
        return '⛈️';
    }
  }
}

// 系统整体状态模型
class SystemStatus {
  final ReportData? reportData;
  final int onlineDevices;
  final int totalDevices;
  final List<String> errorDevices;
  final double overallScore;
  
  SystemStatus({
    this.reportData,
    required this.onlineDevices,
    required this.totalDevices,
    required this.errorDevices,
    required this.overallScore,
  });
  
  // 获取设备在线率
  double get deviceOnlineRate {
    if (totalDevices == 0) return 0.0;
    return (onlineDevices / totalDevices) * 100;
  }
  
  // 获取系统状态描述
  String get systemStatusDescription {
    if (reportData != null && !reportData!.isSystemNormal) {
      return '系统异常';
    }
    if (errorDevices.isNotEmpty) {
      return '设备异常';
    }
    if (deviceOnlineRate < 80) {
      return '设备离线过多';
    }
    return '运行正常';
  }
  
  // 获取状态颜色
  Color get statusColor {
    if (reportData != null && !reportData!.isSystemNormal) {
      return Colors.red;
    }
    if (errorDevices.isNotEmpty) {
      return Colors.orange;
    }
    if (deviceOnlineRate < 80) {
      return Colors.yellow;
    }
    return Colors.green;
  }
}

// 删除自定义颜色类，使用Flutter的Colors 