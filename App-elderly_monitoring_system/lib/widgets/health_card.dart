import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'dart:math';

class HealthCard extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.health_and_safety, color: Colors.blue),
                const SizedBox(width: 8),
                Text('健康状态概览', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                const Spacer(),
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: Colors.green.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Text('正常', style: TextStyle(color: Colors.green, fontSize: 12, fontWeight: FontWeight.bold)),
                ),
              ],
            ),
            const SizedBox(height: 16),
            
            // 关键指标行
            Row(
              children: [
                Expanded(
                  child: _buildQuickMetric('心率', '78', 'bpm', Colors.red, Icons.favorite),
                ),
                Container(width: 1, height: 40, color: Colors.grey[300]),
                Expanded(
                  child: _buildQuickMetric('血压', '125/82', '', Colors.blue, Icons.water_drop),
                ),
                Container(width: 1, height: 40, color: Colors.grey[300]),
                Expanded(
                  child: _buildQuickMetric('血氧', '98%', '', Colors.purple, Icons.air),
                ),
              ],
            ),
            
            const SizedBox(height: 16),
            
            // 心率趋势小图表
            Row(
              children: [
                Expanded(
                  flex: 2,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text('今日心率趋势', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                      const SizedBox(height: 8),
                      Container(
                        height: 60,
                        child: LineChart(
                          LineChartData(
                            gridData: FlGridData(show: false),
                            titlesData: FlTitlesData(show: false),
                            borderData: FlBorderData(show: false),
                            lineBarsData: [
                              LineChartBarData(
                                spots: _generateMiniHeartRateData(),
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
                    ],
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  flex: 1,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          Icon(Icons.medication, color: Colors.green, size: 16),
                          const SizedBox(width: 4),
                          Text('用药', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                        ],
                      ),
                      const SizedBox(height: 4),
                      Text('08:00 ✓', style: TextStyle(fontSize: 11, color: Colors.green)),
                      Text('20:00 ○', style: TextStyle(fontSize: 11, color: Colors.orange)),
                      const SizedBox(height: 8),
                      Row(
                        children: [
                          Icon(Icons.bedtime, color: Colors.blue, size: 16),
                          const SizedBox(width: 4),
                          Text('睡眠', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                        ],
                      ),
                      const SizedBox(height: 4),
                      Text('7h 15min', style: TextStyle(fontSize: 11, color: Colors.blue)),
                      Text('质量良好', style: TextStyle(fontSize: 11, color: Colors.green)),
                    ],
                  ),
                ),
              ],
            ),
            
            const SizedBox(height: 12),
            
            // 预警信息
            Container(
              padding: EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: Colors.orange.withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(color: Colors.orange.withOpacity(0.3)),
              ),
              child: Row(
                children: [
                  Icon(Icons.warning_amber_rounded, color: Colors.orange, size: 16),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      '夜间心率偏高，建议关注睡眠质量',
                      style: TextStyle(fontSize: 11, color: Colors.orange[800]),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildQuickMetric(String title, String value, String unit, Color color, IconData icon) {
    return Column(
      children: [
        Icon(icon, color: color, size: 20),
        const SizedBox(height: 4),
        Text(title, style: TextStyle(fontSize: 10, color: Colors.grey[600])),
        const SizedBox(height: 2),
        Text(value, style: TextStyle(fontSize: 14, fontWeight: FontWeight.bold)),
        if (unit.isNotEmpty)
          Text(unit, style: TextStyle(fontSize: 8, color: Colors.grey[500])),
      ],
    );
  }

  List<FlSpot> _generateMiniHeartRateData() {
    final random = Random();
    List<FlSpot> spots = [];
    for (int i = 0; i < 12; i++) {
      // 生成12个数据点模拟半天的心率
      int baseValue = 70;
      if (i >= 2 && i <= 8) baseValue = 80; // 活动时间心率稍高
      int value = baseValue + random.nextInt(16); // 0-15的随机整数
      spots.add(FlSpot(i.toDouble(), value.toDouble()));
    }
    return spots;
  }
} 