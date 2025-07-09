import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'dart:math';

class HealthDataPage extends StatefulWidget {
  const HealthDataPage({Key? key}) : super(key: key);

  @override
  State<HealthDataPage> createState() => _HealthDataPageState();
}

class _HealthDataPageState extends State<HealthDataPage> {
  int _viewType = 0; // 0: 日, 1: 周, 2: 月
  bool _isLineChart = true;

  // 生成心率变异数据
  List<FlSpot> _generateHRVData() {
    final random = Random();
    List<FlSpot> spots = [];
    for (int i = 0; i < 24; i++) {
      // 模拟一天24小时的HRV数据，夜间会有所升高
      double baseValue = 30;
      if (i >= 22 || i <= 6) baseValue = 40; // 夜间HRV较高
      double value = baseValue + random.nextDouble() * 20;
      // 保留一位小数
      value = double.parse(value.toStringAsFixed(1));
      spots.add(FlSpot(i.toDouble(), value));
    }
    return spots;
  }

  // 生成心率数据
  List<FlSpot> _generateHeartRateData() {
    final random = Random();
    List<FlSpot> spots = [];
    for (int i = 0; i < 24; i++) {
      // 模拟一天24小时的心率数据
      int baseValue = 70;
      if (i >= 6 && i <= 18) baseValue = 80; // 白天心率稍高
      if (i >= 22 || i <= 6) baseValue = 60; // 夜间心率较低
      int value = baseValue + random.nextInt(21); // 0-20的随机整数
      spots.add(FlSpot(i.toDouble(), value.toDouble()));
    }
    return spots;
  }

  // 生成步数数据
  List<BarChartGroupData> _generateStepsData() {
    final random = Random();
    List<BarChartGroupData> data = [];
    List<String> days = ['周一', '周二', '周三', '周四', '周五', '周六', '周日'];
    
    for (int i = 0; i < 7; i++) {
      double steps = 3000 + random.nextDouble() * 7000; // 3000-10000步
      data.add(
        BarChartGroupData(
          x: i,
          barRods: [
            BarChartRodData(
              toY: steps,
              color: steps < 5000 ? Colors.red : steps < 8000 ? Colors.orange : Colors.green,
              width: 16,
              borderRadius: BorderRadius.circular(4),
            ),
          ],
        ),
      );
    }
    return data;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('健康数据'),
        backgroundColor: Colors.blue.shade50,
        elevation: 0,
      ),
      body: Column(
        children: [
          // 导航栏
          Container(
            color: Colors.blue.shade50,
            padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
            child: Row(
              children: [
                ToggleButtons(
                  isSelected: [0, 1, 2].map((i) => i == _viewType).toList(),
                  borderRadius: BorderRadius.circular(8),
                  selectedColor: Colors.white,
                  fillColor: Colors.blue,
                  color: Colors.blue,
                  children: const [Text('日视图'), Text('周视图'), Text('月视图')],
                  onPressed: (idx) {
                    setState(() => _viewType = idx);
                  },
                ),
                const Spacer(),
                IconButton(
                  icon: Icon(_isLineChart ? Icons.show_chart : Icons.bar_chart),
                  tooltip: _isLineChart ? '切换为柱状图' : '切换为折线图',
                  onPressed: () {
                    setState(() => _isLineChart = !_isLineChart);
                  },
                ),
              ],
            ),
          ),
          Expanded(
            child: ListView(
              padding: const EdgeInsets.all(16),
              children: [
                // 关键指标卡片
                _buildKeyMetricsCard(),
                const SizedBox(height: 16),
                
                // 心率变异分析
                _buildHRVCard(),
                const SizedBox(height: 16),
                
                // 心率监测
                _buildHeartRateCard(),
                const SizedBox(height: 16),
                
                // 步数统计
                _buildStepsCard(),
                const SizedBox(height: 16),
                
                // 睡眠阶段分解
                _buildSleepCard(),
                const SizedBox(height: 16),
                
                // 用药提醒系统
                _buildMedicationCard(),
                const SizedBox(height: 16),
                
                // AI建议
                _buildAIAdviceCard(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildKeyMetricsCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('今日健康概览', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: _buildMetricItem('心率', '78', 'bpm', Colors.red, Icons.favorite),
                ),
                Expanded(
                  child: _buildMetricItem('血压', '125/82', 'mmHg', Colors.blue, Icons.water_drop),
                ),
                Expanded(
                  child: _buildMetricItem('步数', '7,842', '步', Colors.green, Icons.directions_walk),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildMetricItem(String title, String value, String unit, Color color, IconData icon) {
    return Column(
      children: [
        Icon(icon, color: color, size: 24),
        const SizedBox(height: 4),
        Text(title, style: TextStyle(fontSize: 12, color: Colors.grey[600])),
        const SizedBox(height: 2),
        Text(value, style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
        Text(unit, style: TextStyle(fontSize: 10, color: Colors.grey[500])),
      ],
    );
  }

  Widget _buildHRVCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.timeline, color: Colors.purple),
                const SizedBox(width: 8),
                Text('心率变异分析（HRV）', style: TextStyle(fontWeight: FontWeight.bold)),
              ],
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 200,
              child: LineChart(
                LineChartData(
                  gridData: FlGridData(show: true, drawVerticalLine: false),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 40,
                        getTitlesWidget: (value, meta) {
                          return Text('${value.toInt()}ms', style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        getTitlesWidget: (value, meta) {
                          return Text('${value.toInt()}:00', style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  ),
                  borderData: FlBorderData(show: false),
                  lineBarsData: [
                    LineChartBarData(
                      spots: _generateHRVData(),
                      isCurved: true,
                      color: Colors.purple,
                      barWidth: 2,
                      dotData: FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: Colors.purple.withOpacity(0.1),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 8),
            Text('平均HRV: 35ms | 范围正常', style: TextStyle(color: Colors.grey[700], fontSize: 12)),
          ],
        ),
      ),
    );
  }

  Widget _buildHeartRateCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.favorite, color: Colors.red),
                const SizedBox(width: 8),
                Text('24小时心率监测', style: TextStyle(fontWeight: FontWeight.bold)),
              ],
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 200,
              child: LineChart(
                LineChartData(
                  gridData: FlGridData(show: true, drawVerticalLine: false),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 40,
                        getTitlesWidget: (value, meta) {
                          return Text('${value.toInt()}', style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        getTitlesWidget: (value, meta) {
                          return Text('${value.toInt()}:00', style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  ),
                  borderData: FlBorderData(show: false),
                  lineBarsData: [
                    LineChartBarData(
                      spots: _generateHeartRateData(),
                      isCurved: true,
                      color: Colors.red,
                      barWidth: 2,
                      dotData: FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: Colors.red.withOpacity(0.1),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 8),
            Text('平均心率: 74 bpm | 静息心率: 62 bpm', style: TextStyle(color: Colors.grey[700], fontSize: 12)),
          ],
        ),
      ),
    );
  }

  Widget _buildStepsCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.directions_walk, color: Colors.green),
                const SizedBox(width: 8),
                Text('本周步数统计', style: TextStyle(fontWeight: FontWeight.bold)),
              ],
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 200,
              child: BarChart(
                BarChartData(
                  gridData: FlGridData(show: false),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 40,
                        getTitlesWidget: (value, meta) {
                          return Text('${(value/1000).toInt()}k', style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        getTitlesWidget: (value, meta) {
                          List<String> days = ['周一', '周二', '周三', '周四', '周五', '周六', '周日'];
                          return Text(days[value.toInt() % 7], style: TextStyle(fontSize: 10));
                        },
                      ),
                    ),
                    topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  ),
                  borderData: FlBorderData(show: false),
                  barGroups: _generateStepsData(),
                  maxY: 12000,
                ),
              ),
            ),
            const SizedBox(height: 8),
            Text('本周目标: 8000步/天 | 完成度: 85%', style: TextStyle(color: Colors.grey[700], fontSize: 12)),
          ],
        ),
      ),
    );
  }

  Widget _buildSleepCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.bedtime, color: Colors.blue),
                const SizedBox(width: 8),
                Text('睡眠阶段分解', style: TextStyle(fontWeight: FontWeight.bold)),
              ],
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 150,
              child: Row(
                children: [
                  Expanded(
                    flex: 1,
                    child: PieChart(
                      PieChartData(
                        sections: [
                          PieChartSectionData(
                            value: 4,
                            color: Colors.blue.shade800,
                            title: '深睡\n4h',
                            titleStyle: TextStyle(color: Colors.white, fontSize: 10, fontWeight: FontWeight.bold),
                            radius: 50,
                          ),
                          PieChartSectionData(
                            value: 2,
                            color: Colors.blue.shade400,
                            title: '浅睡\n2h',
                            titleStyle: TextStyle(color: Colors.white, fontSize: 10, fontWeight: FontWeight.bold),
                            radius: 50,
                          ),
                          PieChartSectionData(
                            value: 1,
                            color: Colors.blue.shade200,
                            title: 'REM\n1h',
                            titleStyle: TextStyle(color: Colors.black, fontSize: 10, fontWeight: FontWeight.bold),
                            radius: 50,
                          ),
                        ],
                        centerSpaceRadius: 30,
                      ),
                    ),
                  ),
                  Expanded(
                    flex: 1,
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        _buildSleepLegend('深睡眠', '4小时', Colors.blue.shade800),
                        _buildSleepLegend('浅睡眠', '2小时', Colors.blue.shade400),
                        _buildSleepLegend('快速眼动', '1小时', Colors.blue.shade200),
                        const SizedBox(height: 10),
                        Text('总睡眠: 7小时', style: TextStyle(fontWeight: FontWeight.bold)),
                        Text('睡眠质量: 良好', style: TextStyle(color: Colors.green)),
                      ],
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 8),
            Text('入睡时间: 22:30 | 起床时间: 06:30', style: TextStyle(color: Colors.grey[700], fontSize: 12)),
          ],
        ),
      ),
    );
  }

  Widget _buildSleepLegend(String title, String duration, Color color) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        children: [
          Container(
            width: 12,
            height: 12,
            decoration: BoxDecoration(
              color: color,
              shape: BoxShape.circle,
            ),
          ),
          const SizedBox(width: 8),
          Text('$title: $duration', style: TextStyle(fontSize: 12)),
        ],
      ),
    );
  }

  Widget _buildMedicationCard() {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.medication, color: Colors.green),
                const SizedBox(width: 8),
                Text('用药提醒系统', style: TextStyle(fontWeight: FontWeight.bold)),
                const Spacer(),
                Icon(Icons.nfc, color: Colors.blue),
                Text(' 支持NFC扫码', style: TextStyle(color: Colors.blue, fontSize: 12)),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Icon(Icons.alarm, color: Colors.orange),
                const SizedBox(width: 4),
                Text('08:00 降压药'),
                const SizedBox(width: 16),
                Icon(Icons.check_circle, color: Colors.green, size: 16),
                Text(' 已服用', style: TextStyle(color: Colors.green, fontSize: 12)),
              ],
            ),
            const SizedBox(height: 4),
            Row(
              children: [
                Icon(Icons.alarm, color: Colors.orange),
                const SizedBox(width: 4),
                Text('20:00 睡前药'),
                const SizedBox(width: 16),
                Icon(Icons.pending, color: Colors.orange, size: 16),
                Text(' 待服用', style: TextStyle(color: Colors.orange, fontSize: 12)),
              ],
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () {
                // NFC扫码逻辑
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('请将手机靠近药瓶上的NFC标签')),
                );
              },
              icon: Icon(Icons.nfc),
              label: Text('NFC扫码记录服药'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.green,
                foregroundColor: Colors.white,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAIAdviceCard() {
    return Card(
      color: Colors.yellow[50],
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.psychology, color: Colors.orange),
                const SizedBox(width: 8),
                Text('AI健康建议', style: TextStyle(fontWeight: FontWeight.bold)),
              ],
            ),
            const SizedBox(height: 8),
            Text('• 连续3天夜间心率偏高，建议联系主治医师', style: TextStyle(color: Colors.orange[900])),
            Text('• 本周步数较上周下降15%，建议增加户外活动', style: TextStyle(color: Colors.blue[900])),
            Text('• 睡眠质量良好，请保持规律作息', style: TextStyle(color: Colors.green[900])),
          ],
        ),
      ),
    );
  }
} 