import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../theme/app_theme.dart';
import '../providers/device_provider.dart';
import '../widgets/custom_app_bar.dart';
import 'dashboard_page.dart';
import 'device_control_page.dart';
import 'health_data_page.dart';
import 'settings_page.dart';

class MainNavigation extends StatefulWidget {
  const MainNavigation({Key? key}) : super(key: key);

  @override
  State<MainNavigation> createState() => _MainNavigationState();
}

class _MainNavigationState extends State<MainNavigation> with TickerProviderStateMixin {
  late PageController _pageController;
  late AnimationController _fabAnimationController;
  int _currentIndex = 0;

  final List<String> _pageTitles = [
    'XX小区1栋302',
    '设备管理',
    '健康数据',
    '系统设置',
  ];

  @override
  void initState() {
    super.initState();
    _pageController = PageController();
    _fabAnimationController = AnimationController(
      duration: AppAnimations.normal,
      vsync: this,
    );
    _fabAnimationController.forward();
    
    // 启动时自动连接MQTT
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _connectToMqtt();
    });
  }

  @override
  void dispose() {
    _pageController.dispose();
    _fabAnimationController.dispose();
    super.dispose();
  }

  void _connectToMqtt() {
    final deviceProvider = Provider.of<DeviceProvider>(context, listen: false);
    deviceProvider.connectToMqtt();
  }

  void _onPageChanged(int index) {
    setState(() {
      _currentIndex = index;
    });
    
    // 控制紧急按钮的显示/隐藏
    if (index == 0) {
      _fabAnimationController.forward();
    } else {
      _fabAnimationController.reverse();
    }
  }

  void _onBottomNavTapped(int index) {
    _pageController.animateToPage(
      index,
      duration: AppAnimations.normal,
      curve: AppAnimations.defaultCurve,
    );
  }

  void _showEmergencyDialog() {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        ),
        backgroundColor: Colors.white,
        title: Row(
          children: [
            Container(
              padding: EdgeInsets.all(AppTheme.spacing8),
              decoration: BoxDecoration(
                color: AppTheme.errorColor.withOpacity(0.1),
                borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
              ),
              child: Icon(
                Icons.emergency,
                color: AppTheme.errorColor,
                size: 24,
              ),
            ),
            SizedBox(width: AppTheme.spacing12),
            Expanded(
              child: Text(
                '紧急呼救',
                style: TextStyle(
                  color: AppTheme.errorColor,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ),
          ],
        ),
        content: Text(
          '确认发送紧急呼救信号？\n将自动联系紧急联系人并触发报警。',
          style: TextStyle(fontSize: 16),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: Text('取消'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _triggerEmergency();
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.errorColor,
            ),
            child: Text('确认呼救'),
          ),
        ],
      ),
    );
  }

  void _triggerEmergency() {
    final deviceProvider = Provider.of<DeviceProvider>(context, listen: false);
    deviceProvider.emergencyCall();
    
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Row(
          children: [
            Icon(Icons.check_circle, color: Colors.white),
            SizedBox(width: AppTheme.spacing8),
            Text('紧急呼救已发送！'),
          ],
        ),
        backgroundColor: AppTheme.errorColor,
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
        ),
        margin: EdgeInsets.all(AppTheme.spacing16),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppTheme.backgroundColor,
      appBar: CustomAppBar(
        title: _pageTitles[_currentIndex],
        actions: [
          IconButton(
            onPressed: () => _showConnectionMenu(),
            icon: Icon(
              Icons.more_vert,
              color: AppTheme.textSecondary,
            ),
          ),
        ],
      ),
      body: PageView(
        controller: _pageController,
        onPageChanged: _onPageChanged,
        children: const [
          DashboardPage(),
          DeviceControlPage(),
          HealthDataPage(),
          SettingsPage(),
        ],
      ),
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          color: Colors.white,
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.08),
              blurRadius: 12,
              offset: const Offset(0, -2),
            ),
          ],
        ),
        child: SafeArea(
          child: Padding(
            padding: EdgeInsets.symmetric(
              horizontal: AppTheme.spacing8,
              vertical: AppTheme.spacing4,
            ),
            child: BottomNavigationBar(
              type: BottomNavigationBarType.fixed,
              currentIndex: _currentIndex,
              onTap: _onBottomNavTapped,
              selectedItemColor: AppTheme.primaryColor,
              unselectedItemColor: AppTheme.textSecondary,
              backgroundColor: Colors.transparent,
              elevation: 0,
              selectedLabelStyle: TextStyle(
                fontWeight: FontWeight.w600,
                fontSize: 12,
              ),
              unselectedLabelStyle: TextStyle(
                fontWeight: FontWeight.w500,
                fontSize: 12,
              ),
              items: [
                BottomNavigationBarItem(
                  icon: _buildNavIcon(Icons.home_outlined, Icons.home, 0),
                  label: '首页',
                ),
                BottomNavigationBarItem(
                  icon: _buildNavIcon(Icons.devices_outlined, Icons.devices, 1),
                  label: '设备',
                ),
                BottomNavigationBarItem(
                  icon: _buildNavIcon(Icons.favorite_outline, Icons.favorite, 2),
                  label: '健康',
                ),
                BottomNavigationBarItem(
                  icon: _buildNavIcon(Icons.settings_outlined, Icons.settings, 3),
                  label: '设置',
                ),
              ],
            ),
          ),
        ),
      ),
      floatingActionButton: ScaleTransition(
        scale: _fabAnimationController,
        child: FloatingActionButton.extended(
          onPressed: _showEmergencyDialog,
          backgroundColor: AppTheme.errorColor,
          foregroundColor: Colors.white,
          elevation: 8,
          label: Text(
            'SOS',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 16,
            ),
          ),
          icon: Icon(Icons.sos, size: 24),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
          ),
        ),
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
    );
  }

  Widget _buildNavIcon(IconData outlinedIcon, IconData filledIcon, int index) {
    final isSelected = _currentIndex == index;
    return AnimatedContainer(
      duration: AppAnimations.fast,
      padding: EdgeInsets.all(AppTheme.spacing4),
      decoration: BoxDecoration(
        color: isSelected 
          ? AppTheme.primaryColor.withOpacity(0.1) 
          : Colors.transparent,
        borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
      ),
      child: Icon(
        isSelected ? filledIcon : outlinedIcon,
        size: 24,
      ),
    );
  }

  void _showConnectionMenu() {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => Container(
        margin: EdgeInsets.all(AppTheme.spacing16),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(
              margin: EdgeInsets.only(top: AppTheme.spacing12),
              width: 40,
              height: 4,
              decoration: BoxDecoration(
                color: Colors.grey[300],
                borderRadius: BorderRadius.circular(2),
              ),
            ),
            Padding(
              padding: EdgeInsets.all(AppTheme.spacing20),
              child: Column(
                children: [
                  _buildMenuTile(
                    Icons.videocam_outlined,
                    '摄像头',
                    '查看实时监控',
                    () => Navigator.pop(context),
                  ),
                  _buildMenuTile(
                    Icons.person_outline,
                    '用户切换',
                    '切换监护对象',
                    () => Navigator.pop(context),
                  ),
                  _buildMenuTile(
                    Icons.wifi,
                    '连接设置',
                    '管理MQTT连接',
                    () {
                      Navigator.pop(context);
                      _showMqttSettings();
                    },
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildMenuTile(IconData icon, String title, String subtitle, VoidCallback onTap) {
    return Container(
      margin: EdgeInsets.only(bottom: AppTheme.spacing8),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
          child: Padding(
            padding: EdgeInsets.all(AppTheme.spacing12),
            child: Row(
              children: [
                Container(
                  padding: EdgeInsets.all(AppTheme.spacing8),
                  decoration: BoxDecoration(
                    color: AppTheme.primaryColor.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                  ),
                  child: Icon(icon, color: AppTheme.primaryColor),
                ),
                SizedBox(width: AppTheme.spacing16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        title,
                        style: TextStyle(
                          fontWeight: FontWeight.w600,
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
                Icon(
                  Icons.arrow_forward_ios,
                  size: 16,
                  color: AppTheme.textHint,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  void _showMqttSettings() {
    // 实现MQTT设置对话框
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
              _connectToMqtt();
            },
            child: Text('连接'),
          ),
        ],
      ),
    );
  }
} 