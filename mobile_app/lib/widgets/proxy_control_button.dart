import 'package:flutter/material.dart';

/// Кнопка управления прокси (старт/стоп)
class ProxyControlButton extends StatelessWidget {
  final bool isRunning;
  final VoidCallback onStart;
  final VoidCallback onStop;

  const ProxyControlButton({
    super.key,
    required this.isRunning,
    required this.onStart,
    required this.onStop,
  });

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;
    
    return SizedBox(
      height: 60,
      child: isRunning
          ? FilledButton.icon(
              onPressed: () => _confirmStop(context),
              icon: const Icon(Icons.stop, size: 24),
              label: const Text(
                'Остановить прокси',
                style: TextStyle(fontSize: 16),
              ),
              style: FilledButton.styleFrom(
                backgroundColor: Colors.red,
                foregroundColor: Colors.white,
              ),
            )
          : FilledButton.icon(
              onPressed: onStart,
              icon: const Icon(Icons.play_arrow, size: 24),
              label: const Text(
                'Запустить прокси',
                style: TextStyle(fontSize: 16),
              ),
              style: FilledButton.styleFrom(
                backgroundColor: colorScheme.primary,
                foregroundColor: colorScheme.onPrimary,
              ),
            ),
    );
  }

  void _confirmStop(BuildContext context) {
    showDialog(
      context: context,
      builder: (dialogContext) => AlertDialog(
        icon: Icon(Icons.warning_amber, color: Colors.orange[700], size: 48),
        title: const Text('Остановить прокси?'),
        content: const Text(
          'Все активные подключения будут разорваны. Вы уверены?',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(dialogContext),
            child: const Text('Отмена'),
          ),
          FilledButton(
            onPressed: () {
              Navigator.pop(dialogContext);
              onStop();
            },
            style: FilledButton.styleFrom(
              backgroundColor: Colors.red,
            ),
            child: const Text('Остановить'),
          ),
        ],
      ),
    );
  }
}
