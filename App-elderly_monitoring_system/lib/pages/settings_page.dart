import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../theme/app_theme.dart';
import '../providers/device_provider.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({Key? key}) : super(key: key);

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  bool _notificationsEnabled = true;
  bool _emergencyAlertsEnabled = true;
  bool _dataSync = true;
  double _alertSensitivity = 0.7;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: EdgeInsets.all(AppTheme.spacing16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 用户信息卡片
          _buildUserInfoCard(),
          
          SizedBox(height: AppTheme.spacing20),
          
          // 连接设置
          _buildConnectionSection(),
          
          SizedBox(height: AppTheme.spacing20),
          
          // 通知设置
          _buildNotificationSection(),
          
          SizedBox(height: AppTheme.spacing20),
          
          // 监护设置
          _buildMonitoringSection(),
          
          SizedBox(height: AppTheme.spacing20),
          
          // 系统设置
          _buildSystemSection(),
          
          SizedBox(height: AppTheme.spacing20),
          
          // 关于应用
          _buildAboutSection(),
        ],
      ),
    );
  }

  Widget _buildUserInfoCard() {
    return Card(
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing20),
        child: Row(
          children: [
            CircleAvatar(
              radius: 30,
              backgroundColor: AppTheme.primaryColor.withOpacity(0.1),
              child: Icon(
                Icons.person,
                size: 32,
                color: AppTheme.primaryColor,
              ),
            ),
            SizedBox(width: AppTheme.spacing16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    '张奶奶',
                    style: TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  SizedBox(height: AppTheme.spacing4),
                  Text(
                    '83岁 · XX小区1栋302',
                    style: TextStyle(
                      color: AppTheme.textSecondary,
                      fontSize: 14,
                    ),
                  ),
                  SizedBox(height: AppTheme.spacing8),
                  Container(
                    padding: EdgeInsets.symmetric(
                      horizontal: AppTheme.spacing8,
                      vertical: AppTheme.spacing4,
                    ),
                    decoration: BoxDecoration(
                      color: AppTheme.successColor.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                    ),
                    child: Text(
                      '健康状况良好',
                      style: TextStyle(
                        color: AppTheme.successColor,
                        fontSize: 12,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                  ),
                ],
              ),
            ),
            Icon(
              Icons.edit,
              color: AppTheme.textHint,
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConnectionSection() {
    return _buildSection(
      '连接设置',
      Icons.wifi,
      [
        Consumer<DeviceProvider>(
          builder: (context, deviceProvider, child) {
            return _buildSettingTile(
              'MQTT连接',
              deviceProvider.isConnected ? '已连接' : '未连接',
              Icons.router,
              trailing: Switch(
                value: deviceProvider.isConnected,
                onChanged: (value) {
                  if (value) {
                    deviceProvider.connectToMqtt();
                  } else {
                    deviceProvider.disconnectMqtt();
                  }
                },
                activeColor: AppTheme.successColor,
              ),
            );
          },
        ),
        _buildSettingTile(
          '服务器设置',
          'localhost:1883',
          Icons.dns,
          onTap: _showMqttSettings,
        ),
        _buildSettingTile(
          '数据同步',
          _dataSync ? '已启用' : '已禁用',
          Icons.sync,
          trailing: Switch(
            value: _dataSync,
            onChanged: (value) {
              setState(() {
                _dataSync = value;
              });
            },
            activeColor: AppTheme.successColor,
          ),
        ),
      ],
    );
  }

  Widget _buildNotificationSection() {
    return _buildSection(
      '通知设置',
      Icons.notifications,
      [
        _buildSettingTile(
          '推送通知',
          _notificationsEnabled ? '已启用' : '已禁用',
          Icons.notifications_outlined,
          trailing: Switch(
            value: _notificationsEnabled,
            onChanged: (value) {
              setState(() {
                _notificationsEnabled = value;
              });
            },
            activeColor: AppTheme.successColor,
          ),
        ),
        _buildSettingTile(
          '紧急警报',
          _emergencyAlertsEnabled ? '已启用' : '已禁用',
          Icons.warning,
          trailing: Switch(
            value: _emergencyAlertsEnabled,
            onChanged: (value) {
              setState(() {
                _emergencyAlertsEnabled = value;
              });
            },
            activeColor: AppTheme.errorColor,
          ),
        ),
        _buildSettingTile(
          '通知时间',
          '8:00 - 22:00',
          Icons.schedule,
          onTap: () => _showTimePicker(),
        ),
      ],
    );
  }

  Widget _buildMonitoringSection() {
    return _buildSection(
      '监护设置',
      Icons.health_and_safety,
      [
        _buildSettingTile(
          '监护灵敏度',
          '${(_alertSensitivity * 100).round()}%',
          Icons.tune,
          trailing: SizedBox(
            width: 100,
            child: Slider(
              value: _alertSensitivity,
              onChanged: (value) {
                setState(() {
                  _alertSensitivity = value;
                });
              },
              activeColor: AppTheme.primaryColor,
            ),
          ),
        ),
        _buildSettingTile(
          '紧急联系人',
          '2个联系人',
          Icons.contact_phone,
          onTap: () => _showEmergencyContacts(),
        ),
        _buildSettingTile(
          '健康报告',
          '每周生成',
          Icons.assessment,
          onTap: () => _showReportSettings(),
        ),
      ],
    );
  }

  Widget _buildSystemSection() {
    return _buildSection(
      '系统设置',
      Icons.settings,
      [
        _buildSettingTile(
          '清除缓存',
          '释放存储空间',
          Icons.cleaning_services,
          onTap: _clearCache,
        ),
        _buildSettingTile(
          '导出数据',
          '备份健康数据',
          Icons.download,
          onTap: _exportData,
        ),
        _buildSettingTile(
          '检查更新',
          '版本 1.0.0',
          Icons.system_update,
          onTap: _checkUpdates,
        ),
      ],
    );
  }

  Widget _buildAboutSection() {
    return _buildSection(
      '关于应用',
      Icons.info,
      [
        _buildSettingTile(
          '用户协议',
          '查看服务条款',
          Icons.description,
          onTap: () => _showTerms(),
        ),
        _buildSettingTile(
          '隐私政策',
          '数据保护说明',
          Icons.privacy_tip,
          onTap: () => _showPrivacy(),
        ),
        _buildSettingTile(
          '联系我们',
          '技术支持',
          Icons.support_agent,
          onTap: () => _showSupport(),
        ),
      ],
    );
  }

  Widget _buildSection(String title, IconData icon, List<Widget> children) {
    return Card(
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: AppTheme.primaryColor),
                SizedBox(width: AppTheme.spacing8),
                Text(
                  title,
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ],
            ),
            SizedBox(height: AppTheme.spacing12),
            ...children,
          ],
        ),
      ),
    );
  }

  Widget _buildSettingTile(
    String title,
    String subtitle,
    IconData icon, {
    Widget? trailing,
    VoidCallback? onTap,
  }) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
        child: Padding(
          padding: EdgeInsets.symmetric(
            vertical: AppTheme.spacing8,
            horizontal: AppTheme.spacing4,
          ),
          child: Row(
            children: [
              Icon(
                icon,
                color: AppTheme.textSecondary,
                size: 20,
              ),
              SizedBox(width: AppTheme.spacing12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      title,
                      style: TextStyle(
                        fontWeight: FontWeight.w500,
                        fontSize: 16,
                      ),
                    ),
                    Text(
                      subtitle,
                      style: TextStyle(
                        color: AppTheme.textSecondary,
                        fontSize: 14,
                      ),
                    ),
                  ],
                ),
              ),
              trailing ?? Icon(
                Icons.arrow_forward_ios,
                size: 16,
                color: AppTheme.textHint,
              ),
            ],
          ),
        ),
      ),
    );
  }

  void _showMqttSettings() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        ),
        title: Text('MQTT连接设置'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              decoration: InputDecoration(
                labelText: '服务器地址',
                hintText: 'localhost',
                prefixIcon: Icon(Icons.dns),
              ),
            ),
            SizedBox(height: AppTheme.spacing16),
                         TextField(
               decoration: InputDecoration(
                 labelText: '端口',
                 hintText: '1883',
                 prefixIcon: Icon(Icons.numbers),
               ),
               keyboardType: TextInputType.number,
             ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: Text('取消'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _saveSettings();
            },
            child: Text('保存'),
          ),
        ],
      ),
    );
  }

  void _showTimePicker() {
    _showInfoDialog('通知时间设置', '可以设置接收通知的时间段，避免夜间打扰');
  }

  void _showEmergencyContacts() {
    _showInfoDialog('紧急联系人', '紧急情况下将自动联系这些人员');
  }

  void _showReportSettings() {
    _showInfoDialog('健康报告', '系统将定期生成健康状况分析报告');
  }

  void _clearCache() {
    _showInfoDialog('清除缓存', '缓存已清除，释放了50MB存储空间');
  }

  void _exportData() {
    _showInfoDialog('导出数据', '数据导出功能正在开发中');
  }

  void _checkUpdates() {
    _showInfoDialog('检查更新', '当前已是最新版本');
  }

  void _showTerms() {
    _showInfoDialog('用户协议', '用户协议详情页面正在开发中');
  }

  void _showPrivacy() {
    _showInfoDialog('隐私政策', '隐私政策详情页面正在开发中');
  }

  void _showSupport() {
    _showInfoDialog('联系我们', '技术支持：400-123-456\n邮箱：support@example.com');
  }

  void _saveSettings() {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Row(
          children: [
            Icon(Icons.check_circle, color: Colors.white),
            SizedBox(width: AppTheme.spacing8),
            Text('设置已保存'),
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

  void _showInfoDialog(String title, String content) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        ),
        title: Text(title),
        content: Text(content),
        actions: [
          ElevatedButton(
            onPressed: () => Navigator.pop(context),
            child: Text('确定'),
          ),
        ],
      ),
    );
  }
} 