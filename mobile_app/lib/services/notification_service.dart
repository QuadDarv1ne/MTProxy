import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Тип уведомления
enum NotificationType {
  info,
  success,
  warning,
  error,
}

/// Модель уведомления
class AppNotification {
  final String id;
  final String title;
  final String message;
  final NotificationType type;
  final DateTime timestamp;
  final Duration? autoDismissDuration;
  final VoidCallback? onTap;
  final bool isRead;

  AppNotification({
    required this.title,
    required this.message,
    this.type = NotificationType.info,
    this.autoDismissDuration,
    this.onTap,
    this.isRead = false,
  })  : id = DateTime.now().millisecondsSinceEpoch.toString(),
        timestamp = DateTime.now();

  /// Иконка для типа уведомления
  IconData get icon {
    switch (type) {
      case NotificationType.info:
        return Icons.info_outline;
      case NotificationType.success:
        return Icons.check_circle_outline;
      case NotificationType.warning:
        return Icons.warning_amber_outlined;
      case NotificationType.error:
        return Icons.error_outline;
    }
  }

  /// Цвет для типа уведомления
  Color get color {
    switch (type) {
      case NotificationType.info:
        return Colors.blue;
      case NotificationType.success:
        return Colors.green;
      case NotificationType.warning:
        return Colors.orange;
      case NotificationType.error:
        return Colors.red;
    }
  }
}

/// Сервис уведомлений
class NotificationService extends ChangeNotifier {
  static final NotificationService _instance = NotificationService._internal();
  factory NotificationService() => _instance;
  NotificationService._internal();

  final List<AppNotification> _notifications = [];
  final List<AppNotification> _history = [];
  Timer? _autoDismissTimer;

  // Настройки
  bool _enabled = true;
  bool _showToast = true;
  bool _playSound = true;
  bool _vibrate = true;
  int _maxHistory = 100;
  int _maxVisible = 5;

  // Getters
  List<AppNotification> get notifications => List.unmodifiable(_notifications);
  List<AppNotification> get history => List.unmodifiable(_history);
  bool get enabled => _enabled;
  int get unreadCount => _notifications.where((n) => !n.isRead).length;

  /// Включение/выключение уведомлений
  void setEnabled(bool enabled) {
    _enabled = enabled;
    notifyListeners();
  }

  /// Показать уведомление
  void show({
    required String title,
    required String message,
    NotificationType type = NotificationType.info,
    Duration? autoDismissDuration,
    VoidCallback? onTap,
  }) {
    if (!_enabled) return;

    final notification = AppNotification(
      title: title,
      message: message,
      type: type,
      autoDismissDuration: autoDismissDuration,
      onTap: onTap,
    );

    _addNotification(notification);
    _showToastNotification(notification);
    _triggerHaptic(type);

    // Авто-скрытие
    if (autoDismissDuration != null) {
      _scheduleAutoDismiss(notification, autoDismissDuration);
    }

    notifyListeners();
  }

  /// Показать info уведомление
  void showInfo(String title, String message) {
    show(title: title, message: message, type: NotificationType.info);
  }

  /// Показать success уведомление
  void showSuccess(String title, String message) {
    show(title: title, message: message, type: NotificationType.success);
  }

  /// Показать warning уведомление
  void showWarning(String title, String message) {
    show(title: title, message: message, type: NotificationType.warning);
  }

  /// Показать error уведомление
  void showError(String title, String message) {
    show(title: title, message: message, type: NotificationType.error);
  }

  /// Уведомление о запуске прокси
  void showProxyStarted() {
    showSuccess(
      'Прокси запущен',
      'MTProxy сервер успешно запущен и готов к работе',
    );
  }

  /// Уведомление об остановке прокси
  void showProxyStopped() {
    showInfo(
      'Прокси остановлен',
      'MTProxy сервер был остановлен',
    );
  }

  /// Уведомление об ошибке прокси
  void showProxyError(String error) {
    showError(
      'Ошибка прокси',
      error,
    );
  }

  /// Уведомление о новом подключении
  void showNewConnection(int count) {
    if (count % 10 == 0) {
      showInfo(
        'Подключения',
        'Активных подключений: $count',
      );
    }
  }

  void _addNotification(AppNotification notification) {
    _notifications.insert(0, notification);
    
    // Ограничение количества видимых уведомлений
    if (_notifications.length > _maxVisible) {
      final removed = _notifications.removeLast();
      _addToHistory(removed);
    }

    // Пометить как прочитанное через 5 секунд
    Timer(const Duration(seconds: 5), () {
      _markAsRead(notification.id);
    });
  }

  void _addToHistory(AppNotification notification) {
    _history.insert(0, notification);
    
    // Ограничение истории
    if (_history.length > _maxHistory) {
      _history.removeLast();
    }
    
    notifyListeners();
  }

  void _markAsRead(String id) {
    final index = _notifications.indexWhere((n) => n.id == id);
    if (index != -1) {
      _notifications[index] = AppNotification(
        title: _notifications[index].title,
        message: _notifications[index].message,
        type: _notifications[index].type,
        autoDismissDuration: _notifications[index].autoDismissDuration,
        onTap: _notifications[index].onTap,
        isRead: true,
      );
      notifyListeners();
    }
  }

  void _showToastNotification(AppNotification notification) {
    if (!_showToast) return;

    // Используем Snackbar для отображения
    // (должно вызываться из контекста виджета)
  }

  void _triggerHaptic(NotificationType type) {
    if (!_vibrate) return;

    switch (type) {
      case NotificationType.info:
        HapticFeedback.lightImpact();
        break;
      case NotificationType.success:
        HapticFeedback.mediumImpact();
        break;
      case NotificationType.warning:
        HapticFeedback.heavyImpact();
        break;
      case NotificationType.error:
        HapticFeedback.vibrate();
        break;
    }
  }

  void _scheduleAutoDismiss(
    AppNotification notification,
    Duration duration,
  ) {
    Timer(duration, () {
      dismiss(notification.id);
    });
  }

  /// Закрыть уведомление
  void dismiss(String id) {
    final index = _notifications.indexWhere((n) => n.id == id);
    if (index != -1) {
      final removed = _notifications.removeAt(index);
      _addToHistory(removed);
      notifyListeners();
    }
  }

  /// Закрыть все уведомления
  void dismissAll() {
    _notifications.forEach(_addToHistory);
    _notifications.clear();
    notifyListeners();
  }

  /// Очистить историю
  void clearHistory() {
    _history.clear();
    notifyListeners();
  }

  /// Обработчик нажатия на уведомление
  void handleTap(String id) {
    final notification = _notifications.firstWhere(
      (n) => n.id == id,
      orElse: () => AppNotification(title: '', message: ''),
    );
    
    if (notification.id.isNotEmpty) {
      notification.onTap?.call();
      _markAsRead(id);
    }
  }

  @override
  void dispose() {
    _autoDismissTimer?.cancel();
    super.dispose();
  }
}

/// Виджет для отображения уведомлений
class NotificationOverlay extends StatelessWidget {
  const NotificationOverlay({super.key});

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: NotificationService(),
      builder: (context, _) {
        final notifications = NotificationService().notifications;
        
        if (notifications.isEmpty) {
          return const SizedBox.shrink();
        }

        return Positioned(
          top: MediaQuery.of(context).padding.top + 16,
          left: 16,
          right: 16,
          child: Column(
            children: notifications
                .take(3)
                .map((notification) => _NotificationCard(notification))
                .toList(),
          ),
        );
      },
    );
  }
}

class _NotificationCard extends StatelessWidget {
  final AppNotification notification;

  const _NotificationCard(this.notification);

  @override
  Widget build(BuildContext context) {
    return Dismissible(
      key: Key(notification.id),
      direction: DismissDirection.endToStart,
      onDismissed: (_) {
        NotificationService().dismiss(notification.id);
      },
      background: Container(
        padding: const EdgeInsets.symmetric(horizontal: 20),
        decoration: BoxDecoration(
          color: Colors.red,
          borderRadius: BorderRadius.circular(12),
        ),
        alignment: Alignment.centerRight,
        child: const Icon(Icons.delete_outline, color: Colors.white),
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: () {
            NotificationService().handleTap(notification.id);
          },
          borderRadius: BorderRadius.circular(12),
          child: Container(
            margin: const EdgeInsets.only(bottom: 8),
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Theme.of(context).cardColor,
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: notification.color.withOpacity(0.3),
                width: 1,
              ),
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withOpacity(0.1),
                  blurRadius: 8,
                  offset: const Offset(0, 2),
                ),
              ],
            ),
            child: Row(
              children: [
                Icon(
                  notification.icon,
                  color: notification.color,
                  size: 24,
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        notification.title,
                        style: const TextStyle(
                          fontWeight: FontWeight.bold,
                          fontSize: 14,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        notification.message,
                        style: TextStyle(
                          fontSize: 12,
                          color: Colors.grey[600],
                        ),
                        maxLines: 2,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ],
                  ),
                ),
                if (!notification.isRead) ...[
                  Container(
                    width: 8,
                    height: 8,
                    decoration: BoxDecoration(
                      color: notification.color,
                      shape: BoxShape.circle,
                    ),
                  ),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}
