import 'package:flutter/material.dart';
import '../models/proxy_stats.dart';

/// Виджет быстрой статистики
class QuickStats extends StatelessWidget {
  final ProxyStats stats;

  const QuickStats({super.key, required this.stats});

  @override
  Widget build(BuildContext context) {
    return GridView.count(
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      crossAxisCount: 2,
      mainAxisSpacing: 12,
      crossAxisSpacing: 12,
      childAspectRatio: 1.5,
      children: [
        _StatItem(
          icon: Icons.people_outline,
          label: 'Подключения',
          value: '${stats.activeConnections}',
          subtitle: 'из ${stats.totalConnections} всего',
          color: Colors.blue,
        ),
        _StatItem(
          icon: Icons.upload_outlined,
          label: 'Отправлено',
          value: stats.formattedSent,
          subtitle: 'Трафик',
          color: Colors.green,
        ),
        _StatItem(
          icon: Icons.download_outlined,
          label: 'Получено',
          value: stats.formattedReceived,
          subtitle: 'Трафик',
          color: Colors.orange,
        ),
        _StatItem(
          icon: Icons.timer_outlined,
          label: 'Время работы',
          value: stats.formattedUptime,
          subtitle: 'Аптайм',
          color: Colors.purple,
        ),
      ],
    );
  }
}

class _StatItem extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final String subtitle;
  final Color color;

  const _StatItem({
    required this.icon,
    required this.label,
    required this.value,
    required this.subtitle,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(6),
                  decoration: BoxDecoration(
                    color: color.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(6),
                  ),
                  child: Icon(
                    icon,
                    size: 18,
                    color: color,
                  ),
                ),
                const Spacer(),
              ],
            ),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  value,
                  style: const TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  label,
                  style: const TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w500,
                  ),
                ),
                Text(
                  subtitle,
                  style: TextStyle(
                    fontSize: 10,
                    color: Colors.grey[600],
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
