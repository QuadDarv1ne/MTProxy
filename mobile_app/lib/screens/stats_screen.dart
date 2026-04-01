import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:fl_chart/fl_chart.dart';
import '../../services/mtproxy_service.dart';
import '../../models/proxy_stats.dart';

/// Экран статистики
class StatsScreen extends StatefulWidget {
  const StatsScreen({super.key});

  @override
  State<StatsScreen> createState() => _StatsScreenState();
}

class _StatsScreenState extends State<StatsScreen> {
  int _selectedTab = 0;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Статистика'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              // Принудительное обновление
              setState(() {});
            },
            tooltip: 'Обновить',
          ),
        ],
      ),
      body: Consumer<MTProxyService>(
        builder: (context, service, child) {
          if (!service.isRunning) {
            return Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(
                    Icons.bar_chart_outlined,
                    size: 64,
                    color: Colors.grey[400],
                  ),
                  const SizedBox(height: 16),
                  Text(
                    'Прокси сервер остановлен',
                    style: TextStyle(
                      fontSize: 18,
                      color: Colors.grey[600],
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Запустите прокси для просмотра статистики',
                    style: TextStyle(
                      fontSize: 14,
                      color: Colors.grey[500],
                    ),
                  ),
                ],
              ),
            );
          }

          return Column(
            children: [
              // Табы
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: SegmentedButton<int>(
                  segments: const [
                    ButtonSegment(
                      value: 0,
                      label: Text('Обзор'),
                      icon: Icon(Icons.dashboard),
                    ),
                    ButtonSegment(
                      value: 1,
                      label: Text('Трафик'),
                      icon: Icon(Icons.show_chart),
                    ),
                    ButtonSegment(
                      value: 2,
                      label: Text('Подключения'),
                      icon: Icon(Icons.people),
                    ),
                  ],
                  selected: {_selectedTab},
                  onSelectionChanged: (selected) {
                    setState(() => _selectedTab = selected.first);
                  },
                ),
              ),

              const SizedBox(height: 16),

              // Контент
              Expanded(
                child: IndexedStack(
                  index: _selectedTab,
                  children: [
                    _buildOverviewTab(service.stats),
                    _buildTrafficTab(service.stats),
                    _buildConnectionsTab(service.stats),
                  ],
                ),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildOverviewTab(ProxyStats stats) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // Основные метрики
          _buildMetricCard(
            'Активные подключения',
            '${stats.activeConnections}',
            Icons.people,
            Colors.blue,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Всего подключений',
            '${stats.totalConnections}',
            Icons.groups,
            Colors.green,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Отправлено',
            stats.formattedSent,
            Icons.upload,
            Colors.orange,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Получено',
            stats.formattedReceived,
            Icons.download,
            Colors.purple,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Время работы',
            stats.formattedUptime,
            Icons.timer,
            Colors.teal,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Использование CPU',
            '${stats.cpuUsage.toStringAsFixed(1)}%',
            Icons.memory,
            Colors.red,
          ),
          const SizedBox(height: 12),
          _buildMetricCard(
            'Использование памяти',
            stats.formattedMemory,
            Icons.storage,
            Colors.indigo,
          ),

          const SizedBox(height: 24),

          // График (заглушка)
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Динамика подключений',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 16),
                  SizedBox(
                    height: 200,
                    child: stats.activeConnections > 0
                        ? LineChart(
                            LineChartData(
                              gridData: FlGridData(show: true),
                              titlesData: FlTitlesData(show: true),
                              borderData: FlBorderData(show: true),
                              lineBarsData: [
                                LineChartBarData(
                                  spots: _generateSampleSpots(),
                                  isCurved: true,
                                  color: Colors.blue,
                                  barWidth: 3,
                                  dotData: FlDotData(show: false),
                                  belowBarData: BarAreaData(
                                    show: true,
                                    color: Colors.blue.withOpacity(0.1),
                                  ),
                                ),
                              ],
                            ),
                          )
                        : Center(
                            child: Text(
                              'Нет данных для отображения',
                              style: TextStyle(color: Colors.grey[500]),
                            ),
                          ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTrafficTab(ProxyStats stats) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
                  const Text(
                    'Общий трафик',
                    style: TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 24),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      _buildTrafficItem(
                        'Отправлено',
                        stats.formattedSent,
                        Colors.orange,
                        Icons.arrow_upward,
                      ),
                      Container(
                        width: 1,
                        height: 60,
                        color: Colors.grey[300],
                      ),
                      _buildTrafficItem(
                        'Получено',
                        stats.formattedReceived,
                        Colors.green,
                        Icons.arrow_downward,
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'Всего: ${_formatTotalTraffic(stats)}',
                    style: const TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Трафик в реальном времени',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 16),
                  SizedBox(
                    height: 200,
                    child: Center(
                      child: Text(
                        'График будет доступен после сбора данных',
                        style: TextStyle(color: Colors.grey[500]),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConnectionsTab(ProxyStats stats) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'Подключения',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 16),
                _buildConnectionStatRow(
                  'Активные',
                  '${stats.activeConnections}',
                  Colors.green,
                ),
                const Divider(),
                _buildConnectionStatRow(
                  'Всего',
                  '${stats.totalConnections}',
                  Colors.blue,
                ),
                const Divider(),
                _buildConnectionStatRow(
                  'В среднем в час',
                  '${_calcAveragePerHour(stats)}',
                  Colors.grey,
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 16),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'Информация',
                  style: TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 12),
                Text(
                  'Данные обновляются каждые 2 секунды',
                  style: TextStyle(
                    fontSize: 12,
                    color: Colors.grey[600],
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildMetricCard(
    String label,
    String value,
    IconData icon,
    Color color,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: color.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(icon, color: color, size: 28),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    label,
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.grey[600],
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    value,
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
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

  Widget _buildTrafficItem(
    String label,
    String value,
    Color color,
    IconData icon,
  ) {
    return Column(
      children: [
        Icon(icon, color: color, size: 32),
        const SizedBox(height: 8),
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: Colors.grey[600],
          ),
        ),
        const SizedBox(height: 4),
        Text(
          value,
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
      ],
    );
  }

  Widget _buildConnectionStatRow(
    String label,
    String value,
    Color color,
  ) {
    return Row(
      children: [
        Container(
          width: 8,
          height: 8,
          decoration: BoxDecoration(
            color: color,
            shape: BoxShape.circle,
          ),
        ),
        const SizedBox(width: 12),
        Text(
          label,
          style: const TextStyle(fontSize: 14),
        ),
        const Spacer(),
        Text(
          value,
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
      ],
    );
  }

  String _formatTotalTraffic(ProxyStats stats) {
    final total = stats.bytesSent + stats.bytesReceived;
    if (total < 1024) {
      return '$total B';
    } else if (total < 1024 * 1024) {
      return '${(total / 1024).toStringAsFixed(1)} KB';
    } else if (total < 1024 * 1024 * 1024) {
      return '${(total / (1024 * 1024)).toStringAsFixed(1)} MB';
    } else {
      return '${(total / (1024 * 1024 * 1024)).toStringAsFixed(2)} GB';
    }
  }

  String _calcAveragePerHour(ProxyStats stats) {
    final hours = stats.uptime.inHours;
    if (hours == 0) return '0';
    final avg = stats.totalConnections ~/ hours;
    return '$avg';
  }

  List<FlSpot> _generateSampleSpots() {
    final now = DateTime.now();
    return List.generate(10, (i) {
      return FlSpot(
        i.toDouble(),
        (10 + (i * 5) % 30).toDouble(),
      );
    });
  }
}
