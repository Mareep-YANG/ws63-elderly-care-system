import 'dart:convert';
import 'dart:io';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import '../models/sensor_data.dart';
import '../models/control_command.dart';
import '../models/report_data.dart';

class MqttService {
  static final MqttService _instance = MqttService._internal();
  factory MqttService() => _instance;
  MqttService._internal();
  
  MqttServerClient? _client;
  bool _isConnected = false;
  
  // 回调函数
  Function(TemperatureSensorData)? onTemperatureReceived;
  Function(HumiditySensorData)? onHumidityReceived;
  Function(LightSensorData)? onLightReceived;
  Function(AirSensorData)? onAirReceived;
  Function(ReportData)? onReportReceived;
  Function(bool)? onConnectionChanged;
  
  // MQTT连接配置
  static const String _defaultHost = '192.168.1.111';
  static const int _defaultPort = 1883;
  static const String _clientId = 'flutter_elderly_monitoring';
  
  bool get isConnected => _isConnected;
  
  // 初始化并连接MQTT
  Future<bool> connect({
    String host = _defaultHost,
    int port = _defaultPort,
    String? username,
    String? password,
  }) async {
    try {
             _client = MqttServerClient(host, _clientId);
       _client!.port = port;
       _client!.logging(on: false);
       _client!.keepAlivePeriod = 20;
       _client!.connectTimeoutPeriod = 2000;
       _client!.onDisconnected = _onDisconnected;
       _client!.onConnected = _onConnected;
       _client!.onSubscribed = _onSubscribed;
      
      // 设置连接消息
      final connMess = MqttConnectMessage()
          .withClientIdentifier(_clientId)
          .startClean()
          .keepAliveFor(20);
      
      if (username != null && password != null) {
        connMess.authenticateAs(username, password);
      }
      
      _client!.connectionMessage = connMess;
      
      // 连接到MQTT服务器
      await _client!.connect();
      
      if (_client!.connectionStatus!.state == MqttConnectionState.connected) {
        _isConnected = true;
        _setupMessageListener();
        _subscribeToTopics();
        onConnectionChanged?.call(true);
        return true;
      }
    } catch (e) {
      print('MQTT连接失败: $e');
      _isConnected = false;
      onConnectionChanged?.call(false);
    }
    return false;
  }
  
  // 断开连接
  void disconnect() {
    _client?.disconnect();
    _isConnected = false;
    onConnectionChanged?.call(false);
  }
  
  // 订阅所有需要的主题
  void _subscribeToTopics() {
    if (_client == null || !_isConnected) return;
    
    // 订阅设备数据主题
    _client!.subscribe('devices/TemperatureSenser', MqttQos.atMostOnce);
    _client!.subscribe('devices/HumiditySenser', MqttQos.atMostOnce);
    _client!.subscribe('devices/LightSenser', MqttQos.atMostOnce);
    _client!.subscribe('devices/AirSenser', MqttQos.atMostOnce);
    
    // 订阅报告主题
    _client!.subscribe('report', MqttQos.atMostOnce);
    
    print('已订阅所有MQTT主题');
  }
  
  // 设置消息监听器
  void _setupMessageListener() {
    _client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      for (final message in messages) {
        _handleMessage(message.topic, message.payload as MqttPublishMessage);
      }
    });
  }
  
  // 处理接收到的消息
  void _handleMessage(String topic, MqttPublishMessage message) {
    final payload = MqttPublishPayload.bytesToStringAsString(message.payload.message);
    
    try {
      final data = jsonDecode(payload) as Map<String, dynamic>;
      
      switch (topic) {
        case 'devices/TemperatureSenser':
          final tempData = TemperatureSensorData.fromJson(data);
          onTemperatureReceived?.call(tempData);
          break;
          
        case 'devices/HumiditySenser':
          final humidityData = HumiditySensorData.fromJson(data);
          onHumidityReceived?.call(humidityData);
          break;
          
        case 'devices/LightSenser':
          final lightData = LightSensorData.fromJson(data);
          onLightReceived?.call(lightData);
          break;
          
        case 'devices/AirSenser':
          final airData = AirSensorData.fromJson(data);
          onAirReceived?.call(airData);
          break;
          
        case 'report':
          final reportData = ReportData.fromJson(data);
          onReportReceived?.call(reportData);
          break;
          
        default:
          print('未知主题: $topic');
      }
    } catch (e) {
      print('解析消息失败: $e, 主题: $topic, 内容: $payload');
    }
  }
  
  // 发布控制指令
  Future<bool> publishControlCommand(ControlCommand command) async {
    if (_client == null || !_isConnected) {
      print('MQTT未连接，无法发送控制指令');
      return false;
    }
    
    try {
      final payload = jsonEncode(command.toJson());
      final builder = MqttClientPayloadBuilder();
      builder.addString(payload);
      
      _client!.publishMessage(
        command.mqttTopic,
        MqttQos.atMostOnce,
        builder.payload!,
      );
      
      print('发送控制指令: ${command.mqttTopic} -> $payload');
      return true;
    } catch (e) {
      print('发送控制指令失败: $e');
      return false;
    }
  }
  
  // 连接成功回调
  void _onConnected() {
    print('MQTT连接成功');
    _isConnected = true;
    onConnectionChanged?.call(true);
  }
  
  // 断开连接回调
  void _onDisconnected() {
    print('MQTT连接断开');
    _isConnected = false;
    onConnectionChanged?.call(false);
  }
  
  // 订阅成功回调
  void _onSubscribed(String topic) {
    print('订阅成功: $topic');
  }
  
  // 便捷方法：发布窗帘控制
  Future<bool> controlCurtain(int engineId, int percentage) async {
    final command = CommonControlCommands.openCurtain(engineId, percentage: percentage);
    return await publishControlCommand(command);
  }
  
  // 便捷方法：控制风扇
  Future<bool> controlFan(bool isOn) async {
    final command = isOn 
        ? CommonControlCommands.turnOnFan() 
        : CommonControlCommands.turnOffFan();
    return await publishControlCommand(command);
  }
  
  // 便捷方法：控制RGB灯
  Future<bool> controlRGBLight({
    bool? breathingMode,
    int? red,
    int? green,
    int? blue,
  }) async {
    final ControlCommand command;
    
    if (breathingMode == true) {
      command = CommonControlCommands.turnOnBreathingLight();
    } else if (red != null && green != null && blue != null) {
      command = RGBControlCommand.constantColor(red: red, green: green, blue: blue);
    } else {
      command = CommonControlCommands.turnOffLight();
    }
    
    return await publishControlCommand(command);
  }
  
  // 便捷方法：控制蜂鸣器
  Future<bool> controlBuzzer(bool isOn) async {
    final command = isOn 
        ? CommonControlCommands.turnOnBuzz() 
        : CommonControlCommands.turnOffBuzz();
    return await publishControlCommand(command);
  }
  

} 