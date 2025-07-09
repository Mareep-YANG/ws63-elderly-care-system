import 'dart:math';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/device_provider.dart';

class DeviceCard extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceProvider>(
      builder: (context, deviceProvider, child) {
        final deviceChips = deviceProvider.getDeviceChips();
        final statusText = deviceProvider.getDeviceStatusText();
        final errorCount = deviceProvider.errorDevices.length;

        return Card(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(
                      Icons.power,
                      color: errorCount > 0 ? Colors.orange : Colors.green,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      '设备在线状态',
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    Text(
                      '在线设备数: ',
                      style: TextStyle(fontWeight: FontWeight.bold),
                    ),
                    Text(
                      statusText,
                      style: TextStyle(
                        color: errorCount > 0 ? Colors.red : Colors.green,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                    if (errorCount > 0) ...[
                      const SizedBox(width: 8),
                      Text(
                        '（异常设备标红）',
                        style: TextStyle(color: Colors.red, fontSize: 12),
                      ),
                    ],
                  ],
                ),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 4,
                  children:
                      deviceChips.map((chip) {
                        Color chipColor = Colors.grey[200]!;
                        Color textColor = Colors.black;

                        if (chip.hasError) {
                          chipColor = Colors.red[100]!;
                          textColor = Colors.red[700]!;
                        } else if (!chip.isOnline) {
                          chipColor = Colors.grey[300]!;
                          textColor = Colors.grey[600]!;
                        } else {
                          chipColor = Colors.green[100]!;
                          textColor = Colors.green[700]!;
                        }

                        return Chip(
                          label: Text(
                            '${chip.label}(${chip.status})',
                            style: TextStyle(color: textColor, fontSize: 12),
                          ),
                          backgroundColor: chipColor,
                          materialTapTargetSize:
                              MaterialTapTargetSize.shrinkWrap,
                        );
                      }).toList(),
                ),

              ],
            ),
          ),
        );
      },
    );
  }

  void _showColorPicker(
    BuildContext context,
    DeviceProvider deviceProvider,
  ) {
    Color selectedColor = Colors.white;
    bool breathingMode = false;
    
    showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setState) => AlertDialog(
          title: Text('RGB灯光颜色选择'),
                     content: Container(
             width: 300,
             height: 350,
            child: Column(
              children: [
                // 颜色预览
                Container(
                  width: double.infinity,
                  height: 60,
                  decoration: BoxDecoration(
                    color: selectedColor,
                    border: Border.all(color: Colors.grey),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Center(
                    child: Text(
                      'RGB(${selectedColor.red}, ${selectedColor.green}, ${selectedColor.blue})',
                      style: TextStyle(
                        color: selectedColor.computeLuminance() > 0.5 
                            ? Colors.black 
                            : Colors.white,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
                ),
                SizedBox(height: 16),
                
                // HSV颜色选择器
                Expanded(
                  child: Container(
                    child: _buildColorPicker(context, selectedColor, (color) {
                      setState(() {
                        selectedColor = color;
                      });
                    }),
                  ),
                ),
                
                SizedBox(height: 16),
                
                                 // 呼吸模式开关
                 Row(
                   mainAxisAlignment: MainAxisAlignment.spaceBetween,
                   children: [
                     Text('呼吸模式:', style: TextStyle(fontWeight: FontWeight.bold)),
                     Switch(
                       value: breathingMode,
                       onChanged: (value) {
                         setState(() {
                           breathingMode = value;
                         });
                       },
                     ),
                   ],
                 ),
                 SizedBox(height: 8),
                 
                 // 预设颜色
                 Text('快速选择:', style: TextStyle(fontWeight: FontWeight.bold)),
                 SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    Colors.red,
                    Colors.green,
                    Colors.blue,
                    Colors.yellow,
                    Colors.purple,
                    Colors.orange,
                    Colors.pink,
                    Colors.cyan,
                    Colors.white,
                    Colors.black,
                  ].map((color) => GestureDetector(
                    onTap: () {
                      setState(() {
                        selectedColor = color;
                      });
                    },
                    child: Container(
                      width: 30,
                      height: 30,
                      decoration: BoxDecoration(
                        color: color,
                        border: Border.all(
                          color: selectedColor == color ? Colors.black : Colors.grey,
                          width: selectedColor == color ? 3 : 1,
                        ),
                        borderRadius: BorderRadius.circular(15),
                      ),
                    ),
                  )).toList(),
                ),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: Text('取消'),
            ),
                         ElevatedButton(
               onPressed: () {
                 deviceProvider.controlRGBLight(
                   breathingMode: breathingMode,
                   red: selectedColor.red,
                   green: selectedColor.green,
                   blue: selectedColor.blue,
                 );
                 Navigator.pop(context);
                 ScaffoldMessenger.of(context).showSnackBar(
                   SnackBar(
                     content: Text(breathingMode 
                         ? 'RGB灯光呼吸模式已启用' 
                         : 'RGB灯光颜色已设置'),
                     backgroundColor: selectedColor,
                   ),
                 );
               },
               child: Text('确定'),
             ),
                         ElevatedButton(
               onPressed: () {
                 deviceProvider.controlRGBLight(
                   breathingMode: false,
                   red: 0,
                   green: 0,
                   blue: 0,
                 );
                Navigator.pop(context);
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text('RGB灯光已关闭'),
                    backgroundColor: Colors.grey,
                  ),
                );
              },
              child: Text('关闭'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.grey[200],
                foregroundColor: Colors.black,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildColorPicker(BuildContext context, Color currentColor, Function(Color) onColorChanged) {
    return Column(
      children: [
        // 色相条
        Container(
          height: 30,
          child: _buildHueSlider(context, currentColor, onColorChanged),
        ),
        SizedBox(height: 16),
        
        // 饱和度和亮度选择区域
        Expanded(
          child: _buildSaturationValuePicker(currentColor, onColorChanged),
        ),
      ],
    );
  }

  Widget _buildHueSlider(BuildContext context, Color currentColor, Function(Color) onColorChanged) {
    HSVColor hsvColor = HSVColor.fromColor(currentColor);
    
    return Container(
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(15),
        gradient: LinearGradient(
          colors: [
            Colors.red,
            Colors.yellow,
            Colors.green,
            Colors.cyan,
            Colors.blue,
            Colors.purple,
            Colors.red,
          ],
        ),
      ),
      child: SliderTheme(
        data: SliderTheme.of(context).copyWith(
          trackHeight: 30,
          thumbShape: RoundSliderThumbShape(enabledThumbRadius: 15),
          overlayShape: RoundSliderOverlayShape(overlayRadius: 20),
          activeTrackColor: Colors.transparent,
          inactiveTrackColor: Colors.transparent,
          thumbColor: Colors.white,
          overlayColor: Colors.white.withOpacity(0.2),
        ),
        child: Slider(
          value: hsvColor.hue,
          min: 0,
          max: 360,
          onChanged: (value) {
            final newColor = hsvColor.withHue(value).toColor();
            onColorChanged(newColor);
          },
        ),
      ),
    );
  }

  Widget _buildSaturationValuePicker(Color currentColor, Function(Color) onColorChanged) {
    HSVColor hsvColor = HSVColor.fromColor(currentColor);
    
    return Column(
      children: [
        // 饱和度滑块
        Text('饱和度', style: TextStyle(fontSize: 12)),
        Slider(
          value: hsvColor.saturation,
          min: 0,
          max: 1,
          onChanged: (value) {
            final newColor = hsvColor.withSaturation(value).toColor();
            onColorChanged(newColor);
          },
          activeColor: HSVColor.fromAHSV(1.0, hsvColor.hue, 1.0, hsvColor.value).toColor(),
        ),
        SizedBox(height: 8),
        
        // 亮度滑块
        Text('亮度', style: TextStyle(fontSize: 12)),
        Slider(
          value: hsvColor.value,
          min: 0,
          max: 1,
          onChanged: (value) {
            final newColor = hsvColor.withValue(value).toColor();
            onColorChanged(newColor);
          },
          activeColor: HSVColor.fromAHSV(1.0, hsvColor.hue, hsvColor.saturation, 1.0).toColor(),
        ),
      ],
    );
  }

  void _showDeviceControlDialog(
    BuildContext context,
    DeviceProvider deviceProvider,
  ) {
    showDialog(
      context: context,
      builder:
          (context) => AlertDialog(
            title: Text('设备控制'),
            content: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                ListTile(
                  title: Text('控制窗帘'),
                  trailing: ElevatedButton(
                    onPressed: () {
                      int angle = Random().nextInt(100);
                      deviceProvider.controlCurtain(1, angle); // 窗帘1开启50%
                      deviceProvider.controlCurtain(2, angle); // 窗帘1开启50%
                      deviceProvider.controlCurtain(3, angle); // 窗帘1开启50%
                      Navigator.pop(context);
                    },
                    child: Text('开启'),
                  ),
                ),
                ListTile(
                  title: Text('控制风扇'),
                  trailing: ElevatedButton(
                    onPressed: () {
                      deviceProvider.controlFan(true);
                      Navigator.pop(context);
                    },
                    child: Text('开启'),
                  ),
                ),
                ListTile(
                  title: Text('控制灯光'),
                  trailing: ElevatedButton(
                    onPressed: () {
                      Navigator.pop(context);
                      _showColorPicker(context, deviceProvider);
                    },
                    child: Text('选色'),
                  ),
                ),
              ],
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(context),
                child: Text('关闭'),
              ),
            ],
          ),
    );
  }
}
