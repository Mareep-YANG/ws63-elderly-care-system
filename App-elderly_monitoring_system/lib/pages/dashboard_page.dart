import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../theme/app_theme.dart';
import '../widgets/dashboard.dart';
import '../widgets/env_card.dart';
import '../widgets/health_card.dart';
import '../widgets/device_card.dart';
import '../providers/device_provider.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({Key? key}) : super(key: key);

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  final GlobalKey<RefreshIndicatorState> _refreshIndicatorKey = GlobalKey<RefreshIndicatorState>();

  Future<void> _onRefresh() async {
    final deviceProvider = Provider.of<DeviceProvider>(context, listen: false);
    // 模拟刷新数据
    await Future.delayed(const Duration(seconds: 1));
    deviceProvider.connectToMqtt();
  }

  @override
  Widget build(BuildContext context) {
    return RefreshIndicator(
      key: _refreshIndicatorKey,
      onRefresh: _onRefresh,
      color: AppTheme.primaryColor,
      child: SingleChildScrollView(
        physics: const AlwaysScrollableScrollPhysics(),
        padding: EdgeInsets.all(AppTheme.spacing16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // 欢迎卡片
            _buildWelcomeCard(),
            
            SizedBox(height: AppTheme.spacing16),
            
            // 综合仪表板
            Dashboard(),
            
            SizedBox(height: AppTheme.spacing16),
            
            // 环境监测卡片
            EnvCard(),
            
            SizedBox(height: AppTheme.spacing16),
            
            // 健康状态卡片
            HealthCard(),
            
            SizedBox(height: AppTheme.spacing16),
            
            // 设备状态卡片
            DeviceCard(),
            
            SizedBox(height: AppTheme.spacing20),
            
            // 快捷操作卡片
            _buildQuickActionsCard(),
            
            // 底部留白，避免被浮动按钮遮挡
            SizedBox(height: AppTheme.spacing32 * 2),
          ],
        ),
      ),
    );
  }

  Widget _buildWelcomeCard() {
    return Container(
      padding: EdgeInsets.all(AppTheme.spacing20),
      decoration: BoxDecoration(
        gradient: AppTheme.primaryGradient,
        borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        boxShadow: [AppTheme.elevatedShadow],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: EdgeInsets.all(AppTheme.spacing8),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                ),
                child: Icon(
                  Icons.waving_hand,
                  color: Colors.white,
                  size: 24,
                ),
              ),
              SizedBox(width: AppTheme.spacing12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      _getGreeting(),
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 18,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    Text(
                      '今天是${_getCurrentDate()}',
                      style: TextStyle(
                        color: Colors.white.withOpacity(0.9),
                        fontSize: 14,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          SizedBox(height: AppTheme.spacing16),
          Text(
            '系统运行正常，所有传感器已连接',
            style: TextStyle(
              color: Colors.white.withOpacity(0.9),
              fontSize: 14,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildQuickActionsCard() {
    return Card(
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  Icons.flash_on,
                  color: AppTheme.primaryColor,
                  size: 24,
                ),
                SizedBox(width: AppTheme.spacing8),
                Text(
                  '快捷操作',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ],
            ),
            SizedBox(height: AppTheme.spacing16),
            Row(
              children: [
                Expanded(
                  child: _buildQuickActionButton(
                    Icons.lightbulb_outline,
                    '开灯',
                    AppTheme.warningColor,
                    () => _performQuickAction('开灯'),
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: _buildQuickActionButton(
                    Icons.air,
                    '通风',
                    AppTheme.infoColor,
                    () => _performQuickAction('开启通风'),
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: _buildQuickActionButton(
                    Icons.thermostat,
                    '调温',
                    AppTheme.successColor,
                    () => _performQuickAction('调节温度'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildQuickActionButton(
    IconData icon,
    String label,
    Color color,
    VoidCallback onPressed,
  ) {
    return Material(
      color: color.withOpacity(0.1),
      borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
      child: InkWell(
        onTap: onPressed,
        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
        child: Padding(
          padding: EdgeInsets.symmetric(
            vertical: AppTheme.spacing16,
            horizontal: AppTheme.spacing8,
          ),
          child: Column(
            children: [
              Icon(
                icon,
                color: color,
                size: 32,
              ),
              SizedBox(height: AppTheme.spacing8),
              Text(
                label,
                style: TextStyle(
                  color: color,
                  fontWeight: FontWeight.w600,
                  fontSize: 12,
                ),
                textAlign: TextAlign.center,
              ),
            ],
          ),
        ),
      ),
    );
  }

  void _performQuickAction(String action) {
    final deviceProvider = Provider.of<DeviceProvider>(context, listen: false);
    
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Row(
          children: [
            Icon(Icons.check_circle, color: Colors.white),
            SizedBox(width: AppTheme.spacing8),
            Text('$action 指令已发送'),
          ],
        ),
        backgroundColor: AppTheme.successColor,
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
        ),
        margin: EdgeInsets.all(AppTheme.spacing16),
      ),
    );
  }

  String _getGreeting() {
    final hour = DateTime.now().hour;
    if (hour < 12) {
      return '早上好！';
    } else if (hour < 18) {
      return '下午好！';
    } else {
      return '晚上好！';
    }
  }

  String _getCurrentDate() {
    final now = DateTime.now();
    final weekdays = ['星期一', '星期二', '星期三', '星期四', '星期五', '星期六', '星期日'];
    return '${now.month}月${now.day}日 ${weekdays[now.weekday - 1]}';
  }
} 