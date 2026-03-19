import 'package:flutter/material.dart';

/// Карточка статуса прокси сервера
class StatusCard extends StatelessWidget {
  final bool isRunning;
  final bool isInitialized;
  final String? error;

  const StatusCard({
    super.key,
    required this.isRunning,
    required this.isInitialized,
    this.error,
  });

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;
    
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            // Индикатор статуса
            Container(
              width: 80,
              height: 80,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: _getStatusColor().withOpacity(0.2),
              ),
              child: Center(
                child: Icon(
                  _getStatusIcon(),
                  size: 40,
                  color: _getStatusColor(),
                ),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Текст статуса
            Text(
              _getStatusText(),
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
                color: _getStatusColor(),
              ),
            ),
            
            const SizedBox(height: 8),
            
            // Дополнительная информация
            Text(
              _getSubStatusText(),
              style: TextStyle(
                fontSize: 14,
                color: Colors.grey[600],
              ),
              textAlign: TextAlign.center,
            ),
            
            // Ошибка
            if (error != null) ...[
              const SizedBox(height: 12),
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 12,
                  vertical: 8,
                ),
                decoration: BoxDecoration(
                  color: colorScheme.errorContainer,
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      Icons.error_outline,
                      size: 16,
                      color: colorScheme.onErrorContainer,
                    ),
                    const SizedBox(width: 8),
                    Flexible(
                      child: Text(
                        error!,
                        style: TextStyle(
                          fontSize: 12,
                          color: colorScheme.onErrorContainer,
                        ),
                        maxLines: 2,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Color _getStatusColor() {
    if (!isInitialized) return Colors.orange;
    if (error != null) return Colors.red;
    if (isRunning) return Colors.green;
    return Colors.grey;
  }

  IconData _getStatusIcon() {
    if (!isInitialized) return Icons.sync;
    if (error != null) return Icons.error;
    if (isRunning) return Icons.check_circle;
    return Icons.stop_circle;
  }

  String _getStatusText() {
    if (!isInitialized) return 'Инициализация...';
    if (error != null) return 'Ошибка';
    if (isRunning) return 'Работает';
    return 'Остановлен';
  }

  String _getSubStatusText() {
    if (!isInitialized) return 'Загрузка модулей...';
    if (error != null) return 'Требуется внимание';
    if (isRunning) return 'Прокси сервер активен';
    return 'Нажмите старт для запуска';
  }
}
