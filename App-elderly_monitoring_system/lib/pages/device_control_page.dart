import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'dart:math' as math;
import '../providers/device_provider.dart';
import '../theme/app_theme.dart';

class DeviceControlPage extends StatefulWidget {
  const DeviceControlPage({Key? key}) : super(key: key);

  @override
  State<DeviceControlPage> createState() => _DeviceControlPageState();
}

class _DeviceControlPageState extends State<DeviceControlPage> with TickerProviderStateMixin {
  // 设备控制参数
  int _curtainAngle = 50;
  int _rgbMode = 0;
  int _rgbRed = 255;
  int _rgbGreen = 255;
  int _rgbBlue = 255;

  late AnimationController _fadeController;
  late AnimationController _slideController;
  late Animation<double> _fadeAnimation;
  late Animation<Offset> _slideAnimation;

  @override
  void initState() {
    super.initState();
    
    _fadeController = AnimationController(
      duration: AppAnimations.normal,
      vsync: this,
    );
    
    _slideController = AnimationController(
      duration: AppAnimations.slow,
      vsync: this,
    );
    
    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _fadeController,
      curve: AppAnimations.defaultCurve,
    ));
    
    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, 0.3),
      end: Offset.zero,
    ).animate(CurvedAnimation(
      parent: _slideController,
      curve: AppAnimations.defaultCurve,
    ));
    
    _fadeController.forward();
    _slideController.forward();
  }

  @override
  void dispose() {
    _fadeController.dispose();
    _slideController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppTheme.backgroundColor,
      body: Consumer<DeviceProvider>(
        builder: (context, deviceProvider, child) {
          final isConnected = deviceProvider.isConnected;

          return FadeTransition(
            opacity: _fadeAnimation,
            child: Column(
              children: [
                // 连接状态和场景控制
                _buildHeaderSection(isConnected, deviceProvider),
                
                // 设备控制区域
                Expanded(
                  child: SlideTransition(
                    position: _slideAnimation,
                    child: ListView(
                      padding: EdgeInsets.all(AppTheme.spacing16),
                      children: [
                        // 窗帘控制
                        _buildCurtainControl(isConnected, deviceProvider),
                        
                        SizedBox(height: AppTheme.spacing16),
                        
                        // 风扇控制
                        _buildFanControl(isConnected, deviceProvider),
                        
                        SizedBox(height: AppTheme.spacing16),
                        
                        // RGB灯光控制 - 保留完整的调色盘
                        _buildRGBLightControl(isConnected, deviceProvider),
                        
                        SizedBox(height: AppTheme.spacing32),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildHeaderSection(bool isConnected, DeviceProvider deviceProvider) {
    return Container(
      margin: EdgeInsets.all(AppTheme.spacing16),
      child: Column(
        children: [
          // 连接状态指示器
          Container(
            width: double.infinity,
            padding: EdgeInsets.all(AppTheme.spacing16),
            decoration: BoxDecoration(
              gradient: isConnected ? AppTheme.successGradient : 
                       LinearGradient(
                         colors: [AppTheme.errorColor, AppTheme.errorColor.withOpacity(0.8)],
                       ),
              borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
              boxShadow: [AppTheme.elevatedShadow],
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  isConnected ? Icons.wifi : Icons.wifi_off,
                  color: Colors.white,
                  size: 24,
                ),
                SizedBox(width: AppTheme.spacing8),
                Text(
                  isConnected ? 'MQTT已连接' : 'MQTT未连接',
                  style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 16,
                  ),
                ),
              ],
            ),
          ),

          SizedBox(height: AppTheme.spacing16),

          // 场景卡片
          Row(
            children: [
              Expanded(
                child: _buildSceneCard(
                  title: '起床模式',
                  subtitle: '自动开窗帘+开灯+通风',
                  icon: Icons.wb_sunny,
                  gradient: AppTheme.successGradient,
                  onTap: () => _executeWakeUpMode(deviceProvider),
                ),
              ),
              SizedBox(width: AppTheme.spacing12),
              Expanded(
                child: _buildSceneCard(
                  title: '紧急模式',
                  subtitle: '全屋灯光高亮+紧急呼救',
                  icon: Icons.emergency,
                  gradient: LinearGradient(
                    colors: [AppTheme.errorColor, Colors.red.shade600],
                  ),
                  onTap: () => _executeEmergencyMode(deviceProvider),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSceneCard({
    required String title,
    required String subtitle,
    required IconData icon,
    required LinearGradient gradient,
    required VoidCallback onTap,
  }) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        child: Container(
          padding: EdgeInsets.all(AppTheme.spacing20),
          decoration: BoxDecoration(
            gradient: gradient,
            borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
            boxShadow: [AppTheme.cardShadow],
          ),
          child: Column(
            children: [
              Icon(
                icon,
                color: Colors.white,
                size: 36,
              ),
              SizedBox(height: AppTheme.spacing12),
              Text(
                title,
                style: TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.bold,
                  fontSize: 16,
                ),
              ),
              SizedBox(height: AppTheme.spacing4),
              Text(
                subtitle,
                style: TextStyle(
                  color: Colors.white.withOpacity(0.9),
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

  Widget _buildCurtainControl(bool isConnected, DeviceProvider deviceProvider) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        boxShadow: [AppTheme.cardShadow],
      ),
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  padding: EdgeInsets.all(AppTheme.spacing8),
                  decoration: BoxDecoration(
                    color: Colors.brown.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                  ),
                  child: Icon(
                    Icons.curtains_closed,
                    color: Colors.brown.shade700,
                    size: 24,
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        '窗帘控制',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.textPrimary,
                        ),
                      ),
                      Text(
                        '调节室内光线',
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
                    vertical: AppTheme.spacing6,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.brown.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
                  ),
                  child: Text(
                    '$_curtainAngle%',
                    style: TextStyle(
                      color: Colors.brown.shade700,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ],
            ),
            
            SizedBox(height: AppTheme.spacing20),
            
            Text(
              '开启角度',
              style: TextStyle(
                fontWeight: FontWeight.w500,
                color: AppTheme.textSecondary,
              ),
            ),
            
            SizedBox(height: AppTheme.spacing8),
            
            SliderTheme(
              data: SliderTheme.of(context).copyWith(
                activeTrackColor: Colors.brown.shade700,
                inactiveTrackColor: Colors.brown.shade200,
                thumbColor: Colors.brown.shade700,
                overlayColor: Colors.brown.withOpacity(0.1),
                trackHeight: 6,
                thumbShape: RoundSliderThumbShape(enabledThumbRadius: 12),
              ),
              child: Slider(
                value: _curtainAngle.toDouble(),
                min: 0,
                max: 100,
                divisions: 20,
                label: '$_curtainAngle%',
                onChanged: (value) {
                  setState(() {
                    _curtainAngle = value.toInt();
                  });
                },
              ),
            ),
            
            SizedBox(height: AppTheme.spacing16),
            
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                _buildCurtainButton('窗帘1', 1, isConnected, deviceProvider),
                _buildCurtainButton('窗帘2', 2, isConnected, deviceProvider),
                _buildCurtainButton('窗帘3', 3, isConnected, deviceProvider),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildCurtainButton(String label, int curtainId, bool isConnected, DeviceProvider deviceProvider) {
    return Expanded(
      child: Container(
        margin: EdgeInsets.symmetric(horizontal: AppTheme.spacing4),
        child: ElevatedButton(
          onPressed: isConnected
              ? () {
                deviceProvider.controlCurtain(curtainId, _curtainAngle);
                _showControlFeedback('$label已设置为$_curtainAngle%');
              }
              : null,
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.brown.shade50,
            foregroundColor: Colors.brown.shade700,
            elevation: 0,
            padding: EdgeInsets.symmetric(vertical: AppTheme.spacing12),
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
              side: BorderSide(color: Colors.brown.shade200),
            ),
          ),
          child: Text(
            label,
            style: TextStyle(fontWeight: FontWeight.w600),
          ),
        ),
      ),
    );
  }

  Widget _buildFanControl(bool isConnected, DeviceProvider deviceProvider) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        boxShadow: [AppTheme.cardShadow],
      ),
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  padding: EdgeInsets.all(AppTheme.spacing8),
                  decoration: BoxDecoration(
                    color: AppTheme.infoColor.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                  ),
                  child: Icon(
                    Icons.air,
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
                        '风扇控制',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.textPrimary,
                        ),
                      ),
                      Text(
                        '空气循环系统',
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
            
            SizedBox(height: AppTheme.spacing20),
            
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: isConnected
                        ? () {
                          deviceProvider.controlFan(true);
                          _showControlFeedback('风扇已开启');
                        }
                        : null,
                    icon: Icon(Icons.power_settings_new),
                    label: Text('开启'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppTheme.successColor.withOpacity(0.1),
                      foregroundColor: AppTheme.successColor,
                      elevation: 0,
                      padding: EdgeInsets.symmetric(vertical: AppTheme.spacing16),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                        side: BorderSide(color: AppTheme.successColor.withOpacity(0.3)),
                      ),
                    ),
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: isConnected
                        ? () {
                          deviceProvider.controlFan(false);
                          _showControlFeedback('风扇已关闭');
                        }
                        : null,
                    icon: Icon(Icons.power_settings_new),
                    label: Text('关闭'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppTheme.errorColor.withOpacity(0.1),
                      foregroundColor: AppTheme.errorColor,
                      elevation: 0,
                      padding: EdgeInsets.symmetric(vertical: AppTheme.spacing16),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                        side: BorderSide(color: AppTheme.errorColor.withOpacity(0.3)),
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildRGBLightControl(bool isConnected, DeviceProvider deviceProvider) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
        boxShadow: [AppTheme.cardShadow],
      ),
      child: Padding(
        padding: EdgeInsets.all(AppTheme.spacing20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  padding: EdgeInsets.all(AppTheme.spacing8),
                  decoration: BoxDecoration(
                    color: AppTheme.warningColor.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                  ),
                  child: Icon(
                    Icons.lightbulb,
                    color: AppTheme.warningColor,
                    size: 24,
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'RGB灯光控制',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.textPrimary,
                        ),
                      ),
                      Text(
                        '智能调色系统',
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

            SizedBox(height: AppTheme.spacing24),

            // 颜色预览 - 现代化设计
            Container(
              width: double.infinity,
              height: 80,
              decoration: BoxDecoration(
                color: Color.fromARGB(255, _rgbRed, _rgbGreen, _rgbBlue),
                borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                border: Border.all(color: Colors.grey.shade300, width: 2),
                boxShadow: [
                  BoxShadow(
                    color: Color.fromARGB(255, _rgbRed, _rgbGreen, _rgbBlue).withOpacity(0.3),
                    blurRadius: 12,
                    offset: Offset(0, 4),
                  ),
                ],
              ),
              child: Center(
                child: Container(
                  padding: EdgeInsets.symmetric(
                    horizontal: AppTheme.spacing16,
                    vertical: AppTheme.spacing8,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.black.withOpacity(0.5),
                    borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
                  ),
                  child: Text(
                    'RGB($_rgbRed, $_rgbGreen, $_rgbBlue)',
                    style: TextStyle(
                      color: Colors.white,
                      fontWeight: FontWeight.bold,
                      fontSize: 16,
                    ),
                  ),
                ),
              ),
            ),
            
            SizedBox(height: AppTheme.spacing24),

            // 调色盘 - 保留完整功能
            Center(
              child: Container(
                padding: EdgeInsets.all(AppTheme.spacing16),
                decoration: BoxDecoration(
                  color: AppTheme.surfaceColor,
                  borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
                  border: Border.all(color: Colors.grey.shade200),
                ),
                child: Column(
                  children: [
                    Text(
                      '调色盘',
                      style: TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 16,
                        color: AppTheme.textPrimary,
                      ),
                    ),
                    SizedBox(height: AppTheme.spacing16),
                    Container(
                      width: 200,
                      height: 200,
                      child: _buildColorWheel(),
                    ),
                  ],
                ),
              ),
            ),
            
            SizedBox(height: AppTheme.spacing24),

            // HSV滑块 - 现代化设计
            _buildModernHSVControls(),
            
            SizedBox(height: AppTheme.spacing24),

            // 预设颜色 - 现代化设计
            Text(
              '快速选择',
              style: TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: 16,
                color: AppTheme.textPrimary,
              ),
            ),
            SizedBox(height: AppTheme.spacing12),
            Container(
              padding: EdgeInsets.all(AppTheme.spacing12),
              decoration: BoxDecoration(
                color: AppTheme.surfaceColor,
                borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
              ),
              child: Wrap(
                spacing: AppTheme.spacing12,
                runSpacing: AppTheme.spacing12,
                children: [
                  _buildModernPresetColor('红色', 255, 0, 0),
                  _buildModernPresetColor('绿色', 0, 255, 0),
                  _buildModernPresetColor('蓝色', 0, 0, 255),
                  _buildModernPresetColor('黄色', 255, 255, 0),
                  _buildModernPresetColor('青色', 0, 255, 255),
                  _buildModernPresetColor('紫色', 255, 0, 255),
                  _buildModernPresetColor('橙色', 255, 165, 0),
                  _buildModernPresetColor('粉色', 255, 192, 203),
                  _buildModernPresetColor('白色', 255, 255, 255),
                  _buildModernPresetColor('关闭', 0, 0, 0),
                ],
              ),
            ),
            
            SizedBox(height: AppTheme.spacing24),

            // 控制按钮 - 现代化设计
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: isConnected
                        ? () {
                          deviceProvider.controlRGBLight(
                            breathingMode: true,
                            red: _rgbRed,
                            green: _rgbGreen,
                            blue: _rgbBlue,
                          );
                          _showControlFeedback('炫彩呼吸灯已启用');
                        }
                        : null,
                    icon: Icon(Icons.auto_awesome),
                    label: Text('炫彩呼吸灯'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.purple.withOpacity(0.1),
                      foregroundColor: Colors.purple.shade700,
                      elevation: 0,
                      padding: EdgeInsets.symmetric(vertical: AppTheme.spacing16),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                        side: BorderSide(color: Colors.purple.withOpacity(0.3)),
                      ),
                    ),
                  ),
                ),
                SizedBox(width: AppTheme.spacing12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: isConnected
                        ? () {
                          deviceProvider.controlRGBLight(
                            breathingMode: false,
                            red: _rgbRed,
                            green: _rgbGreen,
                            blue: _rgbBlue,
                          );
                          _showControlFeedback('RGB灯光已设置为 R:$_rgbRed G:$_rgbGreen B:$_rgbBlue');
                        }
                        : null,
                    icon: Icon(Icons.palette),
                    label: Text('应用颜色'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppTheme.primaryColor.withOpacity(0.1),
                      foregroundColor: AppTheme.primaryColor,
                      elevation: 0,
                      padding: EdgeInsets.symmetric(vertical: AppTheme.spacing16),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
                        side: BorderSide(color: AppTheme.primaryColor.withOpacity(0.3)),
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildModernHSVControls() {
    HSVColor hsvColor = HSVColor.fromColor(Color.fromARGB(255, _rgbRed, _rgbGreen, _rgbBlue));
    
    return Container(
      padding: EdgeInsets.all(AppTheme.spacing16),
      decoration: BoxDecoration(
        color: AppTheme.surfaceColor,
        borderRadius: BorderRadius.circular(AppTheme.radiusMedium),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '精细调节',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 16,
              color: AppTheme.textPrimary,
            ),
          ),
          SizedBox(height: AppTheme.spacing16),
          
          // 饱和度控制
          Row(
            children: [
              Container(
                width: 60,
                child: Text(
                  '饱和度',
                  style: TextStyle(
                    fontWeight: FontWeight.w500,
                    color: AppTheme.textSecondary,
                  ),
                ),
              ),
              Expanded(
                child: SliderTheme(
                  data: SliderTheme.of(context).copyWith(
                    activeTrackColor: HSVColor.fromAHSV(1.0, hsvColor.hue, 1.0, hsvColor.value).toColor(),
                    inactiveTrackColor: Colors.grey.shade300,
                    thumbColor: HSVColor.fromAHSV(1.0, hsvColor.hue, 1.0, hsvColor.value).toColor(),
                    trackHeight: 6,
                    thumbShape: RoundSliderThumbShape(enabledThumbRadius: 12),
                  ),
                  child: Slider(
                    value: hsvColor.saturation,
                    min: 0,
                    max: 1,
                    onChanged: (value) {
                      _updateFromHSV(hsvColor.withSaturation(value));
                    },
                  ),
                ),
              ),
              Container(
                width: 40,
                child: Text(
                  '${(hsvColor.saturation * 100).toInt()}%',
                  style: TextStyle(
                    fontWeight: FontWeight.w600,
                    color: AppTheme.textPrimary,
                  ),
                  textAlign: TextAlign.end,
                ),
              ),
            ],
          ),
          
          SizedBox(height: AppTheme.spacing8),
          
          // 亮度控制
          Row(
            children: [
              Container(
                width: 60,
                child: Text(
                  '亮度',
                  style: TextStyle(
                    fontWeight: FontWeight.w500,
                    color: AppTheme.textSecondary,
                  ),
                ),
              ),
              Expanded(
                child: SliderTheme(
                  data: SliderTheme.of(context).copyWith(
                    activeTrackColor: HSVColor.fromAHSV(1.0, hsvColor.hue, hsvColor.saturation, 1.0).toColor(),
                    inactiveTrackColor: Colors.grey.shade300,
                    thumbColor: HSVColor.fromAHSV(1.0, hsvColor.hue, hsvColor.saturation, 1.0).toColor(),
                    trackHeight: 6,
                    thumbShape: RoundSliderThumbShape(enabledThumbRadius: 12),
                  ),
                  child: Slider(
                    value: hsvColor.value,
                    min: 0,
                    max: 1,
                    onChanged: (value) {
                      _updateFromHSV(hsvColor.withValue(value));
                    },
                  ),
                ),
              ),
              Container(
                width: 40,
                child: Text(
                  '${(hsvColor.value * 100).toInt()}%',
                  style: TextStyle(
                    fontWeight: FontWeight.w600,
                    color: AppTheme.textPrimary,
                  ),
                  textAlign: TextAlign.end,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildModernPresetColor(String label, int r, int g, int b) {
    final isSelected = _rgbRed == r && _rgbGreen == g && _rgbBlue == b;
    
    return GestureDetector(
      onTap: () {
        setState(() {
          _rgbRed = r;
          _rgbGreen = g;
          _rgbBlue = b;
        });
      },
      child: Column(
        children: [
          AnimatedContainer(
            duration: AppAnimations.fast,
            width: 50,
            height: 50,
            decoration: BoxDecoration(
              color: Color.fromARGB(255, r, g, b),
              borderRadius: BorderRadius.circular(AppTheme.radiusLarge),
              border: Border.all(
                color: isSelected ? AppTheme.primaryColor : Colors.grey.shade300,
                width: isSelected ? 3 : 2,
              ),
              boxShadow: isSelected ? [
                BoxShadow(
                  color: AppTheme.primaryColor.withOpacity(0.3),
                  blurRadius: 8,
                  offset: Offset(0, 2),
                ),
              ] : [AppTheme.cardShadow],
            ),
            child: r == 0 && g == 0 && b == 0 ? 
              Icon(Icons.power_settings_new, color: Colors.white, size: 24) : 
              null,
          ),
          SizedBox(height: AppTheme.spacing4),
          Text(
            label,
            style: TextStyle(
              fontSize: 10,
              fontWeight: FontWeight.w500,
              color: isSelected ? AppTheme.primaryColor : AppTheme.textSecondary,
            ),
          ),
        ],
      ),
    );
  }

  // 保留原有的调色盘逻辑
  Widget _buildColorWheel() {
    return GestureDetector(
      onPanUpdate: (details) => _updateColorFromPosition(details.localPosition),
      onTapDown: (details) => _updateColorFromPosition(details.localPosition),
      child: CustomPaint(
        painter: ColorWheelPainter(),
        size: Size(200, 200),
      ),
    );
  }

  void _updateColorFromPosition(Offset position) {
    final center = Offset(100, 100);
    final distance = (position - center).distance;
    
    if (distance <= 100) {
      final angle = math.atan2(position.dy - center.dy, position.dx - center.dx);
      final hue = (angle * 180 / math.pi + 360) % 360;
      final saturation = math.min(distance / 100, 1.0);
      
      HSVColor currentHSV = HSVColor.fromColor(Color.fromARGB(255, _rgbRed, _rgbGreen, _rgbBlue));
      HSVColor newHSV = HSVColor.fromAHSV(1.0, hue, saturation, currentHSV.value);
      
      _updateFromHSV(newHSV);
    }
  }

  void _updateFromHSV(HSVColor hsvColor) {
    Color rgbColor = hsvColor.toColor();
    setState(() {
      _rgbRed = rgbColor.red;
      _rgbGreen = rgbColor.green;
      _rgbBlue = rgbColor.blue;
    });
  }

  void _executeWakeUpMode(DeviceProvider deviceProvider) {
    if (!deviceProvider.isConnected) {
      _showControlFeedback('设备未连接，无法执行起床模式');
      return;
    }

    // 起床模式：开启窗帘、开启灯光、开启风扇
    deviceProvider.controlCurtain(1, 100); // 窗帘1全开
    deviceProvider.controlCurtain(2, 100); // 窗帘2全开
    deviceProvider.controlCurtain(3, 100); // 窗帘3全开
    deviceProvider.controlRGBLight(red: 255, green: 255, blue: 255); // 白色灯光
    deviceProvider.controlFan(true); // 开启风扇

    _showControlFeedback('起床模式已激活：所有窗帘已打开，灯光已开启，风扇已开启');
  }

  void _executeEmergencyMode(DeviceProvider deviceProvider) {
    if (!deviceProvider.isConnected) {
      _showControlFeedback('设备未连接，无法执行紧急模式');
      return;
    }

    // 紧急模式：高亮灯光、发送紧急呼救
    deviceProvider.controlRGBLight(red: 255, green: 0, blue: 0); // 红色警报灯
    deviceProvider.emergencyCall(); // 发送紧急呼救

    _showControlFeedback('紧急模式已激活：警报灯已开启，紧急呼救信号已发送！');
  }

  void _showControlFeedback(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Row(
          children: [
            Icon(Icons.check_circle, color: Colors.white),
            SizedBox(width: AppTheme.spacing8),
            Expanded(child: Text(message)),
          ],
        ),
        backgroundColor: AppTheme.successColor,
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusSmall),
        ),
        margin: EdgeInsets.all(AppTheme.spacing16),
        duration: Duration(seconds: 3),
      ),
    );
  }
}

// 保留原有的ColorWheelPainter类
class ColorWheelPainter extends CustomPainter {
  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    // 绘制色相环
    for (int i = 0; i < 360; i++) {
      final paint = Paint()
        ..shader = SweepGradient(
          colors: [
            HSVColor.fromAHSV(1, 0, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 60, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 120, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 180, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 240, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 300, 1, 1).toColor(),
            HSVColor.fromAHSV(1, 360, 1, 1).toColor(),
          ],
        ).createShader(Rect.fromCircle(center: center, radius: radius));

      canvas.drawCircle(center, radius, paint);
    }

    // 绘制饱和度渐变（从中心到边缘）
    final saturationPaint = Paint()
      ..shader = RadialGradient(
        colors: [Colors.white, Colors.transparent],
        stops: [0.0, 1.0],
      ).createShader(Rect.fromCircle(center: center, radius: radius));

    canvas.drawCircle(center, radius, saturationPaint);

    // 绘制现代化边框
    final borderPaint = Paint()
      ..color = Colors.grey.shade400
      ..style = PaintingStyle.stroke
      ..strokeWidth = 3;

    canvas.drawCircle(center, radius, borderPaint);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
