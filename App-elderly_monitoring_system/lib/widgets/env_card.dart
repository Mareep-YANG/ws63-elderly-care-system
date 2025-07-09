import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/device_provider.dart';
import '../theme/app_theme.dart';

class EnvCard extends StatefulWidget {
  @override
  State<EnvCard> createState() => _EnvCardState();
}

class _EnvCardState extends State<EnvCard> with TickerProviderStateMixin {
  late AnimationController _animationController;
  late Animation<double> _fadeAnimation;
  late Animation<Offset> _slideAnimation;

  @override
  void initState() {
    super.initState();
    _animationController = AnimationController(
      duration: AppAnimations.normal,
      vsync: this,
    );
    
    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _animationController,
      curve: AppAnimations.defaultCurve,
    ));
    
    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, 0.3),
      end: Offset.zero,
    ).animate(CurvedAnimation(
      parent: _animationController,
      curve: AppAnimations.defaultCurve,
    ));
    
    _animationController.forward();
  }

  @override
  void dispose() {
    _animationController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceProvider>(
      builder: (context, deviceProvider, child) {
        final temperatureData = deviceProvider.temperatureData;
        final humidityData = deviceProvider.humidityData;
        final airData = deviceProvider.airData;
        final lightData = deviceProvider.lightData;
        
        return SlideTransition(
          position: _slideAnimation,
          child: FadeTransition(
            opacity: _fadeAnimation,
            child: Container(
              decoration: BoxDecoration(
                color: Colors.white,
                borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
                border: Border.all(
                  color: Colors.grey.shade200,
                  width: 1,
                ),
                boxShadow: [AppTheme.cardShadow],
              ),
              child: Padding(
                padding: EdgeInsets.all(AppTheme.spacing20),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    // 标题行
                    Row(
                      children: [
                        Container(
                          padding: EdgeInsets.all(AppTheme.spacing8),
                          decoration: BoxDecoration(
                            color: AppTheme.infoColor.withOpacity(0.1),
                            borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                          ),
                          child: Icon(
                            Icons.eco,
                            color: AppTheme.infoColor,
                            size: 24,
                          ),
                        ),
                        SizedBox(width: AppTheme.spacing12),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                '环境监测',
                                style: TextStyle(
                                  fontSize: 18,
                                  fontWeight: FontWeight.bold,
                                  color: AppTheme.textPrimary,
                                ),
                              ),
                              Text(
                                '实时环境数据',
                                style: TextStyle(
                                  color: AppTheme.textSecondary,
                                  fontSize: 14,
                                ),
                              ),
                            ],
                          ),
                        ),
                        _buildDataStatus(temperatureData, humidityData, airData, lightData),
                      ],
                    ),
                    
                    SizedBox(height: AppTheme.spacing20),
                    
                    // 主要指标网格
                    Row(
                      children: [
                        Expanded(
                          child: _buildMetricCard(
                            icon: Icons.device_thermostat,
                            title: '温度',
                            value: temperatureData?.temperature?.toStringAsFixed(1) ?? '--',
                            unit: '°C',
                            color: StatusColors.getTemperatureColor(temperatureData?.temperature),
                            subtitle: _getTemperatureStatus(temperatureData?.temperature),
                          ),
                        ),
                        SizedBox(width: AppTheme.spacing12),
                        Expanded(
                          child: _buildMetricCard(
                            icon: Icons.water_drop,
                            title: '湿度',
                            value: humidityData?.humidity?.toStringAsFixed(1) ?? '--',
                            unit: '%',
                            color: _getHumidityColor(humidityData?.humidity),
                            subtitle: _getHumidityStatus(humidityData?.humidity),
                          ),
                        ),
                      ],
                    ),
                    
                    SizedBox(height: AppTheme.spacing16),
                    
                    // 空气质量指标
                    Container(
                      padding: EdgeInsets.all(AppTheme.spacing16),
                      decoration: BoxDecoration(
                        color: StatusColors.getAirQualityColor(airData?.air).withOpacity(0.05),
                        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                        border: Border.all(
                          color: StatusColors.getAirQualityColor(airData?.air).withOpacity(0.2),
                        ),
                      ),
                      child: Row(
                        children: [
                          Container(
                            padding: EdgeInsets.all(AppTheme.spacing8),
                            decoration: BoxDecoration(
                              color: StatusColors.getAirQualityColor(airData?.air).withOpacity(0.1),
                              borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                            ),
                            child: Icon(
                              Icons.air,
                              color: StatusColors.getAirQualityColor(airData?.air),
                              size: 24,
                            ),
                          ),
                          SizedBox(width: AppTheme.spacing12),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Row(
                                  children: [
                                    Text(
                                      '空气质量',
                                      style: TextStyle(
                                        fontWeight: FontWeight.w600,
                                        fontSize: 16,
                                      ),
                                    ),
                                    SizedBox(width: AppTheme.spacing8),
                                    Container(
                                      padding: EdgeInsets.symmetric(
                                        horizontal: AppTheme.spacing8,
                                        vertical: AppTheme.spacing4,
                                      ),
                                      decoration: BoxDecoration(
                                        color: StatusColors.getAirQualityColor(airData?.air),
                                        borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                                      ),
                                      child: Text(
                                        airData?.airQualityDescription ?? '--',
                                        style: TextStyle(
                                          color: Colors.white,
                                          fontSize: 12,
                                          fontWeight: FontWeight.w600,
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                                SizedBox(height: AppTheme.spacing4),
                                if (airData != null)
                                  Text(
                                    '浓度: ${airData.air.toStringAsFixed(1)}%',
                                    style: TextStyle(
                                      color: AppTheme.textSecondary,
                                      fontSize: 14,
                                    ),
                                  ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                    
                    // 光照强度（如果有数据）
                    if (lightData != null) ...[
                      SizedBox(height: AppTheme.spacing16),
                      Container(
                        padding: EdgeInsets.all(AppTheme.spacing16),
                        decoration: BoxDecoration(
                          color: AppTheme.warningColor.withOpacity(0.05),
                          borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                          border: Border.all(
                            color: AppTheme.warningColor.withOpacity(0.2),
                          ),
                        ),
                        child: Row(
                          children: [
                            Icon(
                              Icons.light_mode,
                              color: AppTheme.warningColor,
                              size: 24,
                            ),
                            SizedBox(width: AppTheme.spacing12),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    '光照强度',
                                    style: TextStyle(
                                      fontWeight: FontWeight.w600,
                                      fontSize: 16,
                                    ),
                                  ),
                                  Text(
                                    '${lightData.light} cd',
                                    style: TextStyle(
                                      color: AppTheme.textSecondary,
                                      fontSize: 14,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                            Container(
                              padding: EdgeInsets.symmetric(
                                horizontal: AppTheme.spacing12,
                                vertical: AppTheme.spacing8,
                              ),
                              decoration: BoxDecoration(
                                color: AppTheme.warningColor.withOpacity(0.1),
                                borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                              ),
                              child: Text(
                                _getLightStatus(lightData.light),
                                style: TextStyle(
                                  color: AppTheme.warningColor,
                                  fontWeight: FontWeight.w600,
                                  fontSize: 12,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ],
                    
                    SizedBox(height: AppTheme.spacing16),
                    
                    // 数据更新时间
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(
                          '环境舒适度评估',
                          style: TextStyle(
                            fontSize: 14,
                            fontWeight: FontWeight.w500,
                            color: AppTheme.textSecondary,
                          ),
                        ),
                        Row(
                          children: [
                            Icon(
                              Icons.access_time,
                              size: 14,
                              color: AppTheme.textHint,
                            ),
                            SizedBox(width: AppTheme.spacing4),
                            Text(
                              '${_formatTime(DateTime.now())}',
                              style: TextStyle(
                                fontSize: 12,
                                color: AppTheme.textHint,
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                    
                    SizedBox(height: AppTheme.spacing8),
                    
                    // 舒适度指示器
                    _buildComfortIndicator(temperatureData, humidityData, airData),
                  ],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildDataStatus(temperatureData, humidityData, airData, lightData) {
    int activeCount = 0;
    if (temperatureData != null) activeCount++;
    if (humidityData != null) activeCount++;
    if (airData != null) activeCount++;
    if (lightData != null) activeCount++;
    
    Color statusColor = activeCount > 2 ? AppTheme.successColor : AppTheme.warningColor;
    
    return Container(
      padding: EdgeInsets.symmetric(
        horizontal: AppTheme.spacing8,
        vertical: AppTheme.spacing4,
      ),
      decoration: BoxDecoration(
        color: statusColor.withOpacity(0.1),
        borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
      ),
      child: Text(
        '$activeCount/4 在线',
        style: TextStyle(
          color: statusColor,
          fontSize: 12,
          fontWeight: FontWeight.w600,
        ),
      ),
    );
  }

  Widget _buildMetricCard({
    required IconData icon,
    required String title,
    required String value,
    required String unit,
    required Color color,
    required String subtitle,
  }) {
    return Container(
      padding: EdgeInsets.all(AppTheme.spacing16),
      decoration: BoxDecoration(
        color: color.withOpacity(0.05),
        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
        border: Border.all(
          color: color.withOpacity(0.2),
        ),
      ),
      child: Column(
        children: [
          Icon(
            icon,
            color: color,
            size: 28,
          ),
          SizedBox(height: AppTheme.spacing8),
          Text(
            title,
            style: TextStyle(
              fontSize: 12,
              color: AppTheme.textSecondary,
              fontWeight: FontWeight.w500,
            ),
          ),
          SizedBox(height: AppTheme.spacing4),
          RichText(
            text: TextSpan(
              text: value,
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: color,
              ),
              children: [
                TextSpan(
                  text: unit,
                  style: TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.normal,
                    color: AppTheme.textSecondary,
                  ),
                ),
              ],
            ),
          ),
          SizedBox(height: AppTheme.spacing4),
          Text(
            subtitle,
            style: TextStyle(
              fontSize: 10,
              color: color,
              fontWeight: FontWeight.w500,
            ),
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }

  Widget _buildComfortIndicator(temperatureData, humidityData, airData) {
    double comfort = _calculateComfortScore(temperatureData, humidityData, airData);
    Color comfortColor = AppTheme.successColor;
    String comfortText = '舒适';
    
    if (comfort < 0.4) {
      comfortColor = AppTheme.errorColor;
      comfortText = '不适';
    } else if (comfort < 0.7) {
      comfortColor = AppTheme.warningColor;
      comfortText = '一般';
    }
    
    return Column(
      children: [
        Row(
          children: [
            Expanded(
              child: Container(
                height: 8,
                decoration: BoxDecoration(
                  color: Colors.grey.shade200,
                  borderRadius: BorderRadius.circular(4),
                ),
                child: FractionallySizedBox(
                  alignment: Alignment.centerLeft,
                  widthFactor: comfort,
                  child: Container(
                    decoration: BoxDecoration(
                      color: comfortColor,
                      borderRadius: BorderRadius.circular(4),
                    ),
                  ),
                ),
              ),
            ),
            SizedBox(width: AppTheme.spacing12),
            Text(
              comfortText,
              style: TextStyle(
                color: comfortColor,
                fontWeight: FontWeight.w600,
                fontSize: 14,
              ),
            ),
          ],
        ),
      ],
    );
  }

  double _calculateComfortScore(temperatureData, humidityData, airData) {
    double score = 0.5; // 基础分数
    
    // 温度评分 (18-26°C为最佳)
    if (temperatureData?.temperature != null) {
      double temp = temperatureData.temperature;
      if (temp >= 18 && temp <= 26) {
        score += 0.3;
      } else if (temp >= 16 && temp <= 28) {
        score += 0.1;
      }
    }
    
    // 湿度评分 (40-60%为最佳)
    if (humidityData?.humidity != null) {
      double humidity = humidityData.humidity;
      if (humidity >= 40 && humidity <= 60) {
        score += 0.15;
      } else if (humidity >= 30 && humidity <= 70) {
        score += 0.05;
      }
    }
    
    // 空气质量评分
    if (airData?.air != null) {
      double air = airData.air;
      if (air <= 5.0) {
        score += 0.05;
      } else if (air > 20.0) {
        score -= 0.2;
      }
    }
    
    return score.clamp(0.0, 1.0);
  }

  Color _getHumidityColor(double? humidity) {
    if (humidity == null) return AppTheme.textSecondary;
    if (humidity >= 40 && humidity <= 60) return AppTheme.successColor;
    if (humidity < 30 || humidity > 70) return AppTheme.errorColor;
    return AppTheme.warningColor;
  }

  String _getTemperatureStatus(double? temp) {
    if (temp == null) return '无数据';
    if (temp < 18) return '偏冷';
    if (temp > 26) return '偏热';
    return '适宜';
  }

  String _getHumidityStatus(double? humidity) {
    if (humidity == null) return '无数据';
    if (humidity < 30) return '过干';
    if (humidity > 70) return '过湿';
    if (humidity >= 40 && humidity <= 60) return '适宜';
    return '一般';
  }

  String _getLightStatus(num light) {
    if (light < 50) return '昏暗';
    if (light > 500) return '强烈';
    return '适中';
  }

  String _formatTime(DateTime time) {
    return '${time.hour.toString().padLeft(2, '0')}:${time.minute.toString().padLeft(2, '0')}';
  }
} 