import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/device_provider.dart';
import '../theme/app_theme.dart';

class Dashboard extends StatefulWidget {
  @override
  State<Dashboard> createState() => _DashboardState();
}

class _DashboardState extends State<Dashboard> with TickerProviderStateMixin {
  late AnimationController _scoreAnimationController;
  late AnimationController _pulseAnimationController;
  late Animation<double> _scoreAnimation;
  late Animation<double> _pulseAnimation;

  @override
  void initState() {
    super.initState();
    
    // 分数动画控制器
    _scoreAnimationController = AnimationController(
      duration: AppAnimations.slow,
      vsync: this,
    );
    
    // 脉冲动画控制器
    _pulseAnimationController = AnimationController(
      duration: const Duration(seconds: 2),
      vsync: this,
    );
    
    _scoreAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _scoreAnimationController,
      curve: AppAnimations.defaultCurve,
    ));
    
    _pulseAnimation = Tween<double>(
      begin: 1.0,
      end: 1.1,
    ).animate(CurvedAnimation(
      parent: _pulseAnimationController,
      curve: Curves.easeInOut,
    ));
    
    // 启动动画
    _scoreAnimationController.forward();
    _pulseAnimationController.repeat(reverse: true);
  }

  @override
  void dispose() {
    _scoreAnimationController.dispose();
    _pulseAnimationController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceProvider>(
      builder: (context, deviceProvider, child) {
        final systemStatus = deviceProvider.systemStatus;
        final score = systemStatus.overallScore / 100;
        final scoreInt = systemStatus.overallScore.round();
        
        // 根据分数确定颜色和状态
        Color scoreColor = AppTheme.successColor;
        String statusText = '优秀';
        IconData statusIcon = Icons.check_circle;
        
        if (scoreInt < 60) {
          scoreColor = AppTheme.errorColor;
          statusText = '需要关注';
          statusIcon = Icons.warning;
        } else if (scoreInt < 80) {
          scoreColor = AppTheme.warningColor;
          statusText = '良好';
          statusIcon = Icons.info;
        }
        
        return Container(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [
                Colors.white,
                scoreColor.withOpacity(0.02),
              ],
            ),
            borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
            border: Border.all(
              color: scoreColor.withOpacity(0.2),
              width: 2,
            ),
            boxShadow: [
              BoxShadow(
                color: scoreColor.withOpacity(0.1),
                blurRadius: 20,
                offset: const Offset(0, 8),
              ),
            ],
          ),
          child: Padding(
            padding: EdgeInsets.all(AppTheme.spacing24),
            child: Column(
              children: [
                // 标题行
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          '综合安全评分',
                          style: TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                            color: AppTheme.textPrimary,
                          ),
                        ),
                        SizedBox(height: AppTheme.spacing4),
                        Text(
                          '基于多维度数据分析',
                          style: TextStyle(
                            color: AppTheme.textSecondary,
                            fontSize: 14,
                          ),
                        ),
                      ],
                    ),
                    // 连接状态指示器
                    ScaleTransition(
                      scale: deviceProvider.isConnected ? _pulseAnimation : 
                             const AlwaysStoppedAnimation(1.0),
                      child: Container(
                        padding: EdgeInsets.all(AppTheme.spacing8),
                        decoration: BoxDecoration(
                          color: deviceProvider.isConnected 
                            ? AppTheme.successColor.withOpacity(0.1)
                            : AppTheme.errorColor.withOpacity(0.1),
                          borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                        ),
                        child: Icon(
                          deviceProvider.isConnected ? Icons.wifi : Icons.wifi_off,
                          color: deviceProvider.isConnected 
                            ? AppTheme.successColor 
                            : AppTheme.errorColor,
                          size: 20,
                        ),
                      ),
                    ),
                  ],
                ),
                
                SizedBox(height: AppTheme.spacing24),
                
                // 分数圆环
                Row(
                  children: [
                    Expanded(
                      flex: 2,
                      child: AspectRatio(
                        aspectRatio: 1.0,
                        child: Stack(
                          alignment: Alignment.center,
                          children: [
                            // 背景圆环
                            SizedBox(
                              width: 160,
                              height: 160,
                              child: CircularProgressIndicator(
                                value: 1.0,
                                strokeWidth: 12,
                                backgroundColor: Colors.transparent,
                                valueColor: AlwaysStoppedAnimation<Color>(
                                  Colors.grey.shade200,
                                ),
                              ),
                            ),
                            // 分数圆环
                            SizedBox(
                              width: 160,
                              height: 160,
                              child: AnimatedBuilder(
                                animation: _scoreAnimation,
                                builder: (context, child) {
                                  return CircularProgressIndicator(
                                    value: score * _scoreAnimation.value,
                                    strokeWidth: 12,
                                    backgroundColor: Colors.transparent,
                                    valueColor: AlwaysStoppedAnimation<Color>(scoreColor),
                                    strokeCap: StrokeCap.round,
                                  );
                                },
                              ),
                            ),
                            // 中心内容
                            Column(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                AnimatedBuilder(
                                  animation: _scoreAnimation,
                                  builder: (context, child) {
                                    final animatedScore = (scoreInt * _scoreAnimation.value).round();
                                    return Text(
                                      '$animatedScore',
                                      style: TextStyle(
                                        fontSize: 42,
                                        fontWeight: FontWeight.bold,
                                        color: scoreColor,
                                        height: 1.0,
                                      ),
                                    );
                                  },
                                ),
                                Text(
                                  'SCORE',
                                  style: TextStyle(
                                    fontSize: 12,
                                    fontWeight: FontWeight.w600,
                                    color: AppTheme.textSecondary,
                                    letterSpacing: 1.5,
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                    ),
                    
                    SizedBox(width: AppTheme.spacing20),
                    
                    // 状态信息
                    Expanded(
                      flex: 3,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          // 状态标签
                          Container(
                            padding: EdgeInsets.symmetric(
                              horizontal: AppTheme.spacing12,
                              vertical: AppTheme.spacing8,
                            ),
                            decoration: BoxDecoration(
                              color: scoreColor.withOpacity(0.1),
                              borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
                            ),
                            child: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                Icon(
                                  statusIcon,
                                  color: scoreColor,
                                  size: 16,
                                ),
                                SizedBox(width: AppTheme.spacing4),
                                Text(
                                  statusText,
                                  style: TextStyle(
                                    color: scoreColor,
                                    fontWeight: FontWeight.w600,
                                    fontSize: 14,
                                  ),
                                ),
                              ],
                            ),
                          ),
                          
                          SizedBox(height: AppTheme.spacing16),
                          
                          // 系统状态描述
                          Text(
                            systemStatus.systemStatusDescription,
                            style: TextStyle(
                              color: AppTheme.textPrimary,
                              fontWeight: FontWeight.w500,
                              fontSize: 16,
                            ),
                          ),
                          
                          SizedBox(height: AppTheme.spacing12),
                          
                          // 天气信息
                          if (deviceProvider.reportData != null) ...[
                            Container(
                              padding: EdgeInsets.all(AppTheme.spacing12),
                              decoration: BoxDecoration(
                                color: AppTheme.infoColor.withOpacity(0.05),
                                borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                                border: Border.all(
                                  color: AppTheme.infoColor.withOpacity(0.2),
                                ),
                              ),
                              child: Row(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Text(
                                    deviceProvider.reportData!.weatherIcon,
                                    style: TextStyle(fontSize: 20),
                                  ),
                                  SizedBox(width: AppTheme.spacing8),
                                  Expanded(
                                    child: Column(
                                      crossAxisAlignment: CrossAxisAlignment.start,
                                      children: [
                                        Text(
                                          deviceProvider.reportData!.weather.description,
                                          style: TextStyle(
                                            fontSize: 14,
                                            fontWeight: FontWeight.w500,
                                            color: AppTheme.textPrimary,
                                          ),
                                        ),
                                        Text(
                                          deviceProvider.reportData!.timeDescription,
                                          style: TextStyle(
                                            fontSize: 12,
                                            color: AppTheme.textSecondary,
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ],
                      ),
                    ),
                  ],
                ),
                
                SizedBox(height: AppTheme.spacing20),
                
                // 底部快速指标
                _buildQuickMetrics(deviceProvider),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildQuickMetrics(DeviceProvider deviceProvider) {
    return Container(
      padding: EdgeInsets.all(AppTheme.spacing16),
      decoration: BoxDecoration(
        color: AppTheme.surfaceColor,
        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          _buildMetricItem(
            Icons.sensors,
            '传感器',
            '${_getActiveSensorCount(deviceProvider)}/8',
            AppTheme.infoColor,
          ),
          Container(
            width: 1,
            height: 40,
            color: Colors.grey.shade300,
          ),
          _buildMetricItem(
            Icons.schedule,
            '运行时间',
            _getUptime(),
            AppTheme.successColor,
          ),
          Container(
            width: 1,
            height: 40,
            color: Colors.grey.shade300,
          ),
          _buildMetricItem(
            Icons.update,
            '最后更新',
            _getLastUpdate(),
            AppTheme.warningColor,
          ),
        ],
      ),
    );
  }

  Widget _buildMetricItem(IconData icon, String title, String value, Color color) {
    return Column(
      children: [
        Icon(
          icon,
          color: color,
          size: 20,
        ),
        SizedBox(height: AppTheme.spacing4),
        Text(
          title,
          style: TextStyle(
            fontSize: 12,
            color: AppTheme.textSecondary,
          ),
        ),
        SizedBox(height: AppTheme.spacing4),
        Text(
          value,
          style: TextStyle(
            fontSize: 14,
            fontWeight: FontWeight.w600,
            color: AppTheme.textPrimary,
          ),
        ),
      ],
    );
  }

  int _getActiveSensorCount(DeviceProvider deviceProvider) {
    int count = 0;
    if (deviceProvider.temperatureData != null) count++;
    if (deviceProvider.humidityData != null) count++;
    if (deviceProvider.airData != null) count++;
    if (deviceProvider.lightData != null) count++;
    // 可以添加更多传感器检查
    return count;
  }

  String _getUptime() {
    // 这里应该从deviceProvider获取实际的运行时间
    return '24h 35m';
  }

  String _getLastUpdate() {
    final now = DateTime.now();
    return '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}';
  }
} 