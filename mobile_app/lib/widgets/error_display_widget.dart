import 'package:flutter/material.dart';

/// Виджет отображения ошибки
class ErrorDisplayWidget extends StatelessWidget {
  final String error;
  final VoidCallback? onDismiss;
  final VoidCallback? onRetry;

  const ErrorDisplayWidget({
    super.key,
    required this.error,
    this.onDismiss,
    this.onRetry,
  });

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;

    return Container(
      decoration: BoxDecoration(
        color: colorScheme.errorContainer.withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: colorScheme.errorContainer.withOpacity(0.3),
          width: 1,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Заголовок
          Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: colorScheme.errorContainer.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Icon(
                    Icons.error_outline,
                    color: colorScheme.onErrorContainer,
                    size: 20,
                  ),
                ),
                const SizedBox(width: 12),
                const Expanded(
                  child: Text(
                    'Ошибка',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                if (onDismiss != null)
                  IconButton(
                    icon: const Icon(Icons.close, size: 20),
                    onPressed: onDismiss,
                    tooltip: 'Закрыть',
                  ),
              ],
            ),
          ),

          // Текст ошибки
          Container(
            margin: const EdgeInsets.symmetric(horizontal: 16),
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.5),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Row(
              children: [
                const Icon(
                  Icons.info_outline,
                  size: 16,
                  color: Colors.grey,
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    error,
                    style: TextStyle(
                      fontSize: 13,
                      color: Colors.grey[800],
                    ),
                  ),
                ),
              ],
            ),
          ),

          // Кнопки действий
          if (onRetry != null || onDismiss != null)
            Padding(
              padding: const EdgeInsets.all(16),
              child: Row(
                children: [
                  if (onRetry != null)
                    Expanded(
                      child: FilledButton.icon(
                        onPressed: onRetry,
                        icon: const Icon(Icons.refresh),
                        label: const Text('Повторить'),
                      ),
                    ),
                  if (onRetry != null && onDismiss != null)
                    const SizedBox(width: 12),
                  if (onDismiss != null)
                    Expanded(
                      child: OutlinedButton.icon(
                        onPressed: onDismiss,
                        icon: const Icon(Icons.check),
                        label: const Text('Понятно'),
                      ),
                    ),
                ],
              ),
            ),
        ],
      ),
    );
  }
}

/// Виджет индикатора загрузки
class LoadingIndicator extends StatelessWidget {
  final String? message;
  final bool isFullScreen;

  const LoadingIndicator({
    super.key,
    this.message,
    this.isFullScreen = false,
  });

  @override
  Widget build(BuildContext context) {
    final widget = Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        const CircularProgressIndicator(),
        if (message != null) ...[
          const SizedBox(height: 16),
          Text(
            message!,
            style: TextStyle(
              color: Colors.grey[600],
              fontSize: 14,
            ),
            textAlign: TextAlign.center,
          ),
        ],
      ],
    );

    if (isFullScreen) {
      return Scaffold(
        body: Center(
          child: widget,
        ),
      );
    }

    return Center(
      child: Card(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: widget,
        ),
      ),
    );
  }
}

/// Виджет состояния "пусто"
class EmptyStateWidget extends StatelessWidget {
  final IconData icon;
  final String title;
  final String? subtitle;
  final String? actionLabel;
  final VoidCallback? onAction;

  const EmptyStateWidget({
    super.key,
    required this.icon,
    required this.title,
    this.subtitle,
    this.actionLabel,
    this.onAction,
  });

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              icon,
              size: 80,
              color: Colors.grey[400],
            ),
            const SizedBox(height: 24),
            Text(
              title,
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.w600,
                color: Colors.grey,
              ),
              textAlign: TextAlign.center,
            ),
            if (subtitle != null) ...[
              const SizedBox(height: 8),
              Text(
                subtitle!,
                style: TextStyle(
                  fontSize: 14,
                  color: Colors.grey[600],
                ),
                textAlign: TextAlign.center,
              ),
            ],
            if (actionLabel != null && onAction != null) ...[
              const SizedBox(height: 24),
              FilledButton.icon(
                onPressed: onAction,
                icon: const Icon(Icons.add),
                label: Text(actionLabel!),
              ),
            ],
          ],
        ),
      ),
    );
  }
}

/// Анимированный индикатор успеха
class SuccessAnimation extends StatefulWidget {
  final String message;
  final VoidCallback? onComplete;

  const SuccessAnimation({
    super.key,
    required this.message,
    this.onComplete,
  });

  @override
  State<SuccessAnimation> createState() => _SuccessAnimationState();
}

class _SuccessAnimationState extends State<SuccessAnimation>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scaleAnimation;
  late Animation<double> _fadeAnimation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );

    _scaleAnimation = Tween<double>(begin: 0.5, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.elasticOut),
    );

    _fadeAnimation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeOut),
    );

    _controller.forward();

    // Автоматическое закрытие через 2 секунды
    Future.delayed(const Duration(seconds: 2), () {
      if (mounted && widget.onComplete != null) {
        widget.onComplete!();
      }
    });
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return FadeTransition(
      opacity: _fadeAnimation,
      child: ScaleTransition(
        scale: _scaleAnimation,
        child: Card(
          color: Colors.green.shade50,
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.green,
                    shape: BoxShape.circle,
                  ),
                  child: const Icon(
                    Icons.check,
                    color: Colors.white,
                    size: 28,
                  ),
                ),
                const SizedBox(width: 16),
                Text(
                  widget.message,
                  style: const TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.w500,
                    color: Colors.green,
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
