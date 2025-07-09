import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../widgets/dashboard.dart';
import '../widgets/env_card.dart';
import '../widgets/health_card.dart';
import '../widgets/device_card.dart';
import '../providers/device_provider.dart';
import 'device_control_page.dart';
import 'health_data_page.dart';

class HomePage extends StatefulWidget {
  const HomePage({Key? key}) : super(key: key);

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int _selectedIndex = 0;

  @override
  void initState() {
    super.initState();
    // 启动时自动连接MQTT
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _connectToMqtt();
    });
  }

  void _connectToMqtt() {
    final deviceProvider = Provider.of<DeviceProvider>(context, listen: false);
    deviceProvider.connectToMqtt();
  }

  void _onBottomNavTapped(int index) {
    switch (index) {
      case 0:
        // 首页，设置选中状态
        setState(() {
          _selectedIndex = 0;
        });
        break;
      case 1:
        Navigator.push(
          context,
          MaterialPageRoute(builder: (context) => const DeviceControlPage()),
        ).then((_) {
          // 返回时重置为首页选中状态
          setState(() {
            _selectedIndex = 0;
          });
        });
        break;
      case 2:
        Navigator.push(
          context,
          MaterialPageRoute(builder: (context) => const HealthDataPage()),
        ).then((_) {
          // 返回时重置为首页选中状态
          setState(() {
            _selectedIndex = 0;
          });
        });
        break;
      case 3:
        // 更多设置
        _showMoreOptions();
        // 保持首页选中状态
        setState(() {
          _selectedIndex = 0;
        });
        break;
    }
  }

  void _showMoreOptions() {
    showModalBottomSheet(
      context: context,
      builder: (context) => Container(
        padding: EdgeInsets.all(16),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              leading: Icon(Icons.videocam),
              title: Text('摄像头'),
              onTap: () {
                Navigator.pop(context);
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('摄像头功能开发中...')),
                );
              },
            ),
            ListTile(
              leading: Icon(Icons.person),
              title: Text('用户切换'),
              onTap: () {
                Navigator.pop(context);
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('用户切换功能开发中...')),
                );
              },
            ),
            ListTile(
              leading: Icon(Icons.connect_without_contact),
              title: Text('连接MQTT'),
              onTap: () {
                Navigator.pop(context);
                _connectToMqtt();
              },
            ),
            ListTile(
              leading: Icon(Icons.link_off),
              title: Text('断开MQTT'),
              onTap: () {
                Navigator.pop(context);
                Provider.of<DeviceProvider>(context, listen: false).disconnectMqtt();
              },
            ),
            ListTile(
              leading: Icon(Icons.settings),
              title: Text('MQTT设置'),
              onTap: () {
                Navigator.pop(context);
                _showMqttSettings();
              },
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceProvider>(
      builder: (context, deviceProvider, child) {
        return Scaffold(
          appBar: AppBar(
            title: Row(
              children: [
                Icon(Icons.gps_fixed, color: Colors.blue),
                const SizedBox(width: 8),
                const Text('XX小区1栋302'),
                const Spacer(),
                // MQTT连接状态指示器
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: deviceProvider.isConnected ? Colors.green : Colors.red,
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Text(
                    deviceProvider.isConnected ? 'MQTT已连接' : 'MQTT未连接',
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ],
            ),
            backgroundColor: Colors.white,
            elevation: 1,
          ),
          body: SingleChildScrollView(
            child: Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Dashboard(),
                  const SizedBox(height: 16),
                  EnvCard(),
                  const SizedBox(height: 12),
                  HealthCard(),
                  const SizedBox(height: 12),
                  DeviceCard(),
                ],
              ),
            ),
          ),
          bottomNavigationBar: BottomNavigationBar(
            type: BottomNavigationBarType.fixed,
            currentIndex: _selectedIndex,
            onTap: _onBottomNavTapped,
            selectedItemColor: Colors.blue,
            unselectedItemColor: Colors.grey,
            items: const [
              BottomNavigationBarItem(
                icon: Icon(Icons.home),
                label: '首页',
              ),
              BottomNavigationBarItem(
                icon: Icon(Icons.settings),
                label: '设备管理',
              ),
              BottomNavigationBarItem(
                icon: Icon(Icons.favorite),
                label: '健康数据',
              ),
              BottomNavigationBarItem(
                icon: Icon(Icons.more_horiz),
                label: '更多',
              ),
            ],
          ),
          floatingActionButton: FloatingActionButton(
            backgroundColor: Colors.red,
            child: const Icon(Icons.sos, size: 32),
            onPressed: () {
              // 紧急呼救逻辑
              deviceProvider.emergencyCall();
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(
                  content: Text('紧急呼救已发送！'),
                  backgroundColor: Colors.red,
                ),
              );
            },
            tooltip: '紧急呼救',
          ),
          floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
        );
      },
    );
  }

  void _showMqttSettings() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('MQTT设置'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              decoration: InputDecoration(
                labelText: '服务器地址',
                hintText: 'localhost',
              ),
            ),
            TextField(
              decoration: InputDecoration(
                labelText: '端口',
                hintText: '1883',
              ),
              keyboardType: TextInputType.number,
            ),
            TextField(
              decoration: InputDecoration(
                labelText: '用户名（可选）',
              ),
            ),
            TextField(
              decoration: InputDecoration(
                labelText: '密码（可选）',
              ),
              obscureText: true,
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
              // 这里可以添加自定义连接逻辑
              _connectToMqtt();
            },
            child: Text('连接'),
          ),
        ],
      ),
    );
  }
} 