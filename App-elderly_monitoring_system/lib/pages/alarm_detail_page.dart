import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class AlarmDetailPage extends StatefulWidget {
  const AlarmDetailPage({Key? key}) : super(key: key);

  @override
  State<AlarmDetailPage> createState() => _AlarmDetailPageState();
}

class _AlarmDetailPageState extends State<AlarmDetailPage> {
  @override
  void initState() {
    super.initState();
    _vibrate();
  }

  void _vibrate() async {
    // 持续震动（简单实现，实际可用 vibration 插件增强）
    for (int i = 0; i < 10; i++) {
      await Future.delayed(const Duration(milliseconds: 500));
      HapticFeedback.heavyImpact();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        // 全屏红色遮罩
        Container(color: Colors.red.withOpacity(0.92)),
        SafeArea(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.warning, color: Colors.white, size: 64),
              const SizedBox(height: 16),
              const Text('警报触发', style: TextStyle(color: Colors.white, fontSize: 28, fontWeight: FontWeight.bold)),
              const SizedBox(height: 32),
              // 紧急操作按钮
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white,
                      foregroundColor: Colors.red,
                      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    icon: const Icon(Icons.local_hospital),
                    label: const Text('呼叫120'),
                    onPressed: () {
                      // 拨打医院电话
                    },
                  ),
                  const SizedBox(width: 24),
                  ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white,
                      foregroundColor: Colors.red,
                      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    icon: const Icon(Icons.phone),
                    label: const Text('联系监护人'),
                    onPressed: () {
                      // 拨打监护人电话
                    },
                  ),
                ],
              ),
              const SizedBox(height: 40),
              // 智能诊断建议
              Card(
                color: Colors.white.withOpacity(0.95),
                margin: const EdgeInsets.symmetric(horizontal: 32),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
                child: Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Row(
                    children: [
                      Icon(Icons.camera_alt, color: Colors.red[400]),
                      const SizedBox(width: 12),
                      Expanded(
                        child: Text(
                          '识别到跌倒动作，已抓拍现场图像',
                          style: TextStyle(color: Colors.red[900], fontWeight: FontWeight.bold),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
} 