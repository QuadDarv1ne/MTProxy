import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../services/mtproxy_service.dart';
import '../../services/notification_service.dart';
import '../../widgets/status_card.dart';
import '../../widgets/quick_stats.dart';
import '../../widgets/proxy_control_button.dart';
import '../../widgets/error_display_widget.dart';

/// Главный экран управления прокси
class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('MTProxy Manager'),
        actions: [
          // Кнопка истории ошибок
          Consumer<MTProxyService>(
            builder: (context, service, child) {
              if (service.errorCount > 0) {
                return Stack(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.warning_outlined),
                      onPressed: () => _showErrorHistory(context),
                      tooltip: 'История ошибок',
                    ),
                    Positioned(
                      right: 8,
                      top: 8,
                      child: Container(
                        padding: const EdgeInsets.all(4),
                        decoration: const BoxDecoration(
                          color: Colors.red,
                          shape: BoxShape.circle,
                        ),
                        child: Text(
                          '${service.errorCount}',
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 10,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),
                    ),
                  ],
                );
              }
              return IconButton(
                icon: const Icon(Icons.info_outline),
                onPressed: () => _showAboutDialog(context),
                tooltip: 'О приложении',
              );
            },
          ),
        ],
      ),
      body: Consumer<MTProxyService>(
        builder: (context, service, child) {
          return RefreshIndicator(
            onRefresh: () async {
              // Принудительное обновление статистики
              await Future.delayed(const Duration(milliseconds: 500));
            },
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  // Карточка статуса
                  StatusCard(
                    isRunning: service.isRunning,
                    isInitialized: service.isInitialized,
                    error: service.error,
                  ),
                  
                  const SizedBox(height: 16),
                  
                  // Кнопка управления
                  ProxyControlButton(
                    isRunning: service.isRunning,
                    onStart: () => _handleStart(context, service),
                    onStop: () => _handleStop(context, service),
                  ),
                  
                  // Отображение ошибок
                  if (service.hasError) ...[
                    const SizedBox(height: 16),
                    ErrorDisplayWidget(
                      error: service.error!,
                      onDismiss: () => service.clearErrorHistory(),
                      onRetry: () => _handleStart(context, service),
                    ),
                  ],
                  
                  const SizedBox(height: 24),
                  
                  // Быстрая статистика
                  if (service.isRunning) ...[
                    const Text(
                      'Статистика',
                      style: TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 12),
                    QuickStats(stats: service.stats),
                  ],
                  
                  const SizedBox(height: 24),
                  
                  // Информация о конфигурации
                  _buildConfigInfo(service),
                  
                  const SizedBox(height: 16),
                  
                  // Информация о версии
                  _buildVersionInfo(service),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  void _handleStart(BuildContext context, MTProxyService service) async {
    final notificationService = context.read<NotificationService>();
    
    final success = await service.start();
    
    if (success && context.mounted) {
      notificationService.showProxyStarted();
    } else if (context.mounted) {
      notificationService.showProxyError(service.error ?? 'Неизвестная ошибка');
    }
  }

  void _handleStop(BuildContext context, MTProxyService service) {
    final notificationService = context.read<NotificationService>();
    
    service.stop();
    
    if (context.mounted) {
      notificationService.showProxyStopped();
    }
  }

  Widget _buildConfigInfo(MTProxyService service) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Конфигурация',
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 12),
            _buildInfoRow('Порт', '${service.config.port}'),
            _buildInfoRow(
              'Секретов',
              '${service.config.secrets.length}',
            ),
            _buildInfoRow(
              'Max подключения',
              '${service.config.maxConnections}',
            ),
            _buildInfoRow(
              'IPv6',
              service.config.enableIpv6 ? 'Включено' : 'Выключено',
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildVersionInfo(MTProxyService service) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            const Icon(Icons.info_outline, size: 20),
            const SizedBox(width: 12),
            Text(
              'Версия библиотеки: ${service.getVersion()}',
              style: TextStyle(
                fontSize: 12,
                color: Colors.grey[600],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(
              color: Colors.grey[600],
              fontSize: 14,
            ),
          ),
          Text(
            value,
            style: const TextStyle(
              fontWeight: FontWeight.w500,
              fontSize: 14,
            ),
          ),
        ],
      ),
    );
  }

  void _showErrorHistory(BuildContext context) {
    final service = context.read<MTProxyService>();
    
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      builder: (context) => DraggableScrollableSheet(
        initialChildSize: 0.6,
        minChildSize: 0.3,
        maxChildSize: 0.9,
        builder: (context, scrollController) => Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Center(
                child: Container(
                  width: 40,
                  height: 4,
                  decoration: BoxDecoration(
                    color: Colors.grey[300],
                    borderRadius: BorderRadius.circular(2),
                  ),
                ),
              ),
              const SizedBox(height: 20),
              const Text(
                'История ошибок',
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 16),
              if (service.errorHistory.isEmpty)
                const Center(
                  child: Text('Нет ошибок в истории'),
                )
              else
                Expanded(
                  child: ListView.separated(
                    controller: scrollController,
                    itemCount: service.errorHistory.length,
                    separatorBuilder: (_, __) => const SizedBox(height: 8),
                    itemBuilder: (context, index) {
                      final error = service.errorHistory[index];
                      return Card(
                        child: ListTile(
                          leading: Icon(
                            _getErrorIcon(error.type),
                            color: _getErrorColor(error.type),
                          ),
                          title: Text(error.message),
                          subtitle: Text(
                            '${error.timestamp} • ${error.type.name}',
                            style: TextStyle(
                              fontSize: 11,
                              color: Colors.grey[600],
                            ),
                          ),
                          isThreeLine: error.source != null,
                        ),
                      );
                    },
                  ),
                ),
              const SizedBox(height: 16),
              SizedBox(
                width: double.infinity,
                child: OutlinedButton.icon(
                  onPressed: () {
                    service.clearErrorHistory();
                    Navigator.pop(context);
                  },
                  icon: const Icon(Icons.delete_outline),
                  label: const Text('Очистить историю'),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  IconData _getErrorIcon(dynamic type) {
    return Icons.error_outline;
  }

  Color _getErrorColor(dynamic type) {
    return Colors.red;
  }

  void _showAboutDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AboutDialog(
        applicationName: 'MTProxy Manager',
        applicationVersion: '1.0.0',
        applicationIcon: const Icon(Icons.cloud_sync, size: 48),
        children: [
          const Text('Приложение для управления MTProxy сервером'),
          const SizedBox(height: 8),
          const Text('Поддерживаемые платформы:'),
          const Text('• Windows'),
          const Text('• Linux'),
          const Text('• macOS'),
          const Text('• iOS'),
          const Text('• Android'),
        ],
      ),
    );
  }
}
