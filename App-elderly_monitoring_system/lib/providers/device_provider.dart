import 'package:flutter/foundation.dart';
import '../models/sensor_data.dart';
import '../models/control_command.dart';
import '../models/report_data.dart';
import '../services/mqtt_service.dart';

class DeviceProvider with ChangeNotifier {
  final MqttService _mqttService = MqttService();
  
  // 传感器数据
  TemperatureSensorData? _temperatureData;
  HumiditySensorData? _humidityData;
  LightSensorData? _lightData;
  AirSensorData? _airData;
  ReportData? _reportData;
  
  // 连接状态
  bool _isConnected = false;
  bool _isConnecting = false;
  
  // 设备状态
  final Map<String, DeviceStatus> _deviceStatus = {
    'curtain_1': DeviceStatus(
      deviceId: 'curtain_1',
      deviceName: '卧室窗帘',
      isOnline: true,
    ),
    'curtain_2': DeviceStatus(
      deviceId: 'curtain_2',
      deviceName: '客厅窗帘',
      isOnline: true,
    ),
    'curtain_3': DeviceStatus(
      deviceId: 'curtain_3',
      deviceName: '厨房窗帘',
      isOnline: true,
    ),
    'fan': DeviceStatus(
      deviceId: 'fan',
      deviceName: '风扇',
      isOnline: true,
    ),
    'rgb_light': DeviceStatus(
      deviceId: 'rgb_light',
      deviceName: 'RGB灯',
      isOnline: true,
    ),
    'buzzer': DeviceStatus(
      deviceId: 'buzzer',
      deviceName: '蜂鸣器',
      isOnline: true,
    ),
    'temp_sensor': DeviceStatus(
      deviceId: 'temp_sensor',
      deviceName: '温度传感器',
      isOnline: false,
      hasError: false,
      errorMessage: '等待数据',
    ),
    'humidity_sensor': DeviceStatus(
      deviceId: 'humidity_sensor',
      deviceName: '湿度传感器',
      isOnline: false,
    ),
    'air_sensor': DeviceStatus(
      deviceId: 'air_sensor',
      deviceName: '空气质量传感器',
      isOnline: false,
    ),
    'light_sensor': DeviceStatus(
      deviceId: 'light_sensor',
      deviceName: '光照传感器',
      isOnline: false,
    ),
  };
  
  // Getters
  TemperatureSensorData? get temperatureData => _temperatureData;
  HumiditySensorData? get humidityData => _humidityData;
  LightSensorData? get lightData => _lightData;
  AirSensorData? get airData => _airData;
  ReportData? get reportData => _reportData;
  bool get isConnected => _isConnected;
  bool get isConnecting => _isConnecting;
  
  // 获取环境数据
  EnvironmentData get environmentData => EnvironmentData(
    temperature: _temperatureData,
    humidity: _humidityData,
    light: _lightData,
    air: _airData,
  );
  
  // 获取设备状态列表
  List<DeviceStatus> get deviceStatusList => _deviceStatus.values.toList();
  
  // 获取在线设备数量
  int get onlineDeviceCount => _deviceStatus.values.where((d) => d.isOnline).length;
  
  // 获取总设备数量
  int get totalDeviceCount => _deviceStatus.length;
  
  // 获取异常设备列表
  List<DeviceStatus> get errorDevices => _deviceStatus.values.where((d) => d.hasError).toList();
  
  // 获取系统状态
  SystemStatus get systemStatus => SystemStatus(
    reportData: _reportData,
    onlineDevices: onlineDeviceCount,
    totalDevices: totalDeviceCount,
    errorDevices: errorDevices.map((d) => d.deviceName).toList(),
    overallScore: environmentData.safetyScore,
  );
  
  // 初始化Provider
  DeviceProvider() {
    _initializeMqttService();
  }
  
  // 初始化MQTT服务
  void _initializeMqttService() {
    _mqttService.onTemperatureReceived = _onTemperatureReceived;
    _mqttService.onHumidityReceived = _onHumidityReceived;
    _mqttService.onLightReceived = _onLightReceived;
    _mqttService.onAirReceived = _onAirReceived;
    _mqttService.onReportReceived = _onReportReceived;
    _mqttService.onConnectionChanged = _onConnectionChanged;
  }
  
  // 连接到MQTT服务器
  Future<bool> connectToMqtt({
    String host = '192.168.1.111',
    int port = 1883,
    String? username,
    String? password,
  }) async {
    if (_isConnecting) return false;
    
    _isConnecting = true;
    notifyListeners();
    
    try {
      final success = await _mqttService.connect(
        host: host,
        port: port,
        username: username,
        password: password,
      );
      
      // 连接结果处理，不再启动模拟数据
      
      _isConnecting = false;
      notifyListeners();
      
      return success;
    } catch (e) {
      _isConnecting = false;
      notifyListeners();
      return false;
    }
  }
  
  // 断开MQTT连接
  void disconnectMqtt() {
    _mqttService.disconnect();
  }
  
  // 传感器数据回调
  void _onTemperatureReceived(TemperatureSensorData data) {
    _temperatureData = data;
    _updateDeviceStatus('temp_sensor', isOnline: true, hasError: false);
    notifyListeners();
  }
  
  void _onHumidityReceived(HumiditySensorData data) {
    _humidityData = data;
    _updateDeviceStatus('humidity_sensor', isOnline: true, hasError: false);
    notifyListeners();
  }
  
  void _onLightReceived(LightSensorData data) {
    _lightData = data;
    _updateDeviceStatus('light_sensor', isOnline: true, hasError: false);
    notifyListeners();
  }
  
  void _onAirReceived(AirSensorData data) {
    _airData = data;
    _updateDeviceStatus('air_sensor', isOnline: true, hasError: false);
    notifyListeners();
  }
  
  void _onReportReceived(ReportData data) {
    _reportData = data;
    notifyListeners();
  }
  
  void _onConnectionChanged(bool connected) {
    _isConnected = connected;
    notifyListeners();
  }
  
  // 更新设备状态
  void _updateDeviceStatus(String deviceId, {
    bool? isOnline,
    bool? hasError,
    String? errorMessage,
  }) {
    final device = _deviceStatus[deviceId];
    if (device != null) {
      _deviceStatus[deviceId] = DeviceStatus(
        deviceId: device.deviceId,
        deviceName: device.deviceName,
        isOnline: isOnline ?? device.isOnline,
        hasError: hasError ?? device.hasError,
        errorMessage: errorMessage ?? device.errorMessage,
      );
    }
  }
  
  // 控制设备方法
  Future<bool> controlCurtain(int engineId, int percentage) async {
    final success = await _mqttService.controlCurtain(engineId, percentage);
    if (success) {
      _updateDeviceStatus('curtain_$engineId', isOnline: true, hasError: false);
      notifyListeners();
    }
    return success;
  }
  
  Future<bool> controlFan(bool isOn) async {
    final success = await _mqttService.controlFan(isOn);
    if (success) {
      _updateDeviceStatus('fan', isOnline: true, hasError: false);
      notifyListeners();
    }
    return success;
  }
  
  Future<bool> controlRGBLight({
    bool? breathingMode,
    int? red,
    int? green,
    int? blue,
  }) async {
    final success = await _mqttService.controlRGBLight(
      breathingMode: breathingMode,
      red: red,
      green: green,
      blue: blue,
    );
    if (success) {
      _updateDeviceStatus('rgb_light', isOnline: true, hasError: false);
      notifyListeners();
    }
    return success;
  }
  
  Future<bool> controlBuzzer(bool isOn) async {
    final success = await _mqttService.controlBuzzer(isOn);
    if (success) {
      _updateDeviceStatus('buzzer', isOnline: true, hasError: false);
      notifyListeners();
    }
    return success;
  }
  
  // 紧急呼救功能
  Future<bool> emergencyCall() async {
    // 同时触发蜂鸣器和RGB灯警告
    final buzzerSuccess = await controlBuzzer(true);
    final lightSuccess = await controlRGBLight(red: 255, green: 0, blue: 0);
    
    return buzzerSuccess && lightSuccess;
  }
  
  // 获取设备状态的文本描述
  String getDeviceStatusText() {
    final online = onlineDeviceCount;
    final total = totalDeviceCount;
    final errors = errorDevices.length;
    
    if (errors > 0) {
      return '$online/$total（${errors}个设备异常）';
    }
    return '$online/$total';
  }
  
  // 获取设备状态chips
  List<DeviceChip> getDeviceChips() {
    return _deviceStatus.values.map((device) {
      return DeviceChip(
        label: device.deviceName,
        isOnline: device.isOnline,
        hasError: device.hasError,
        status: device.isOnline ? (device.hasError ? '异常' : '正常') : '离线',
      );
    }).toList();
  }
  
  @override
  void dispose() {
    _mqttService.disconnect();
    super.dispose();
  }
}

// 设备状态Chip数据模型
class DeviceChip {
  final String label;
  final bool isOnline;
  final bool hasError;
  final String status;
  
  DeviceChip({
    required this.label,
    required this.isOnline,
    required this.hasError,
    required this.status,
  });
} 