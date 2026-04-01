import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/mtproxy_service.dart';
import '../services/notification_service.dart';
import '../utils/secret_generator.dart';
import '../utils/validation_utils.dart';

/// Мастер первоначальной настройки
class SetupWizard extends StatefulWidget {
  const SetupWizard({super.key});

  @override
  State<SetupWizard> createState() => _SetupWizardState();
}

class _SetupWizardState extends State<SetupWizard> {
  int _currentStep = 0;
  final PageController _pageController = PageController();

  // Данные настройки
  int _port = 443;
  final List<String> _secrets = [];
  int _maxConnections = 10000;
  bool _enableIpv6 = false;
  bool _enableStats = true;

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }

  void _nextStep() {
    if (_currentStep < 4) {
      setState(() => _currentStep++);
      _pageController.animateToPage(
        _currentStep,
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeInOut,
      );
    }
  }

  void _prevStep() {
    if (_currentStep > 0) {
      setState(() => _currentStep--);
      _pageController.animateToPage(
        _currentStep,
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeInOut,
      );
    }
  }

  void _finishSetup() async {
    final proxyService = context.read<MTProxyService>();
    final notificationService = context.read<NotificationService>();

    await proxyService.updateConfig(
      port: _port,
      secrets: _secrets,
      maxConnections: _maxConnections,
      enableIpv6: _enableIpv6,
      enableStats: _enableStats,
    );

    if (mounted) {
      notificationService.showSuccess(
        'Настройка завершена',
        'MTProxy готов к работе!',
      );
      Navigator.pop(context);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Настройка MTProxy'),
        leading: _currentStep > 0
            ? IconButton(
                icon: const Icon(Icons.arrow_back),
                onPressed: _prevStep,
              )
            : IconButton(
                icon: const Icon(Icons.close),
                onPressed: () => Navigator.pop(context),
              ),
        actions: [
          if (_currentStep == 4)
            FilledButton(
              onPressed: _finishSetup,
              child: const Text('Готово'),
            ),
        ],
      ),
      body: Column(
        children: [
          // Индикатор прогресса
          LinearProgressIndicator(
            value: (_currentStep + 1) / 5,
            minHeight: 4,
          ),
          const SizedBox(height: 24),
          // Шаги
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: List.generate(
              5,
              (index) => _StepIndicator(
                isActive: index == _currentStep,
                isCompleted: index < _currentStep,
              ),
            ),
          ),
          const SizedBox(height: 32),
          // Контент
          Expanded(
            child: PageView(
              controller: _pageController,
              physics: const NeverScrollableScrollPhysics(),
              children: [
                _WelcomeStep(onContinue: _nextStep),
                _PortStep(
                  port: _port,
                  onPortChanged: (port) => setState(() => _port = port),
                  onNext: _nextStep,
                ),
                _SecretsStep(
                  secrets: _secrets,
                  onSecretsChanged: (secrets) =>
                      setState(() => _secrets.clear()..addAll(secrets)),
                  onNext: _nextStep,
                ),
                _AdvancedStep(
                  maxConnections: _maxConnections,
                  enableIpv6: _enableIpv6,
                  enableStats: _enableStats,
                  onMaxConnectionsChanged: (value) =>
                      setState(() => _maxConnections = value),
                  onIpv6Changed: (value) =>
                      setState(() => _enableIpv6 = value),
                  onStatsChanged: (value) =>
                      setState(() => _enableStats = value),
                  onNext: _nextStep,
                ),
                _SummaryStep(
                  port: _port,
                  secretsCount: _secrets.length,
                  maxConnections: _maxConnections,
                  enableIpv6: _enableIpv6,
                  enableStats: _enableStats,
                  onFinish: _finishSetup,
                ),
              ],
            ),
          ),
          // Навигация
          if (_currentStep < 4)
            Padding(
              padding: const EdgeInsets.all(24),
              child: Row(
                children: [
                  if (_currentStep > 0)
                    OutlinedButton.icon(
                      onPressed: _prevStep,
                      icon: const Icon(Icons.arrow_back),
                      label: const Text('Назад'),
                    ),
                  const Spacer(),
                  FilledButton.icon(
                    onPressed: _nextStep,
                    icon: const Icon(Icons.arrow_forward),
                    label: const Text('Далее'),
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }
}

// ============================================================================
// Шаги мастера
// ============================================================================

class _WelcomeStep extends StatelessWidget {
  final VoidCallback onContinue;

  const _WelcomeStep({required this.onContinue});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.cloud_sync,
            size: 120,
            color: Theme.of(context).colorScheme.primary,
          ),
          const SizedBox(height: 32),
          const Text(
            'Добро пожаловать!',
            style: TextStyle(
              fontSize: 28,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 16),
          Text(
            'Мастер настройки поможет вам быстро настроить MTProxy',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 16,
              color: Colors.grey[600],
            ),
          ),
          const SizedBox(height: 48),
          _buildFeature(Icons.speed, 'Высокая производительность'),
          const SizedBox(height: 12),
          _buildFeature(Icons.security, 'Безопасное соединение'),
          const SizedBox(height: 12),
          _buildFeature(Icons.tune, 'Гибкая настройка'),
          const SizedBox(height: 48),
          FilledButton.icon(
            onPressed: onContinue,
            icon: const Icon(Icons.arrow_forward),
            label: const Text('Начать настройку'),
            style: FilledButton.styleFrom(
              padding: const EdgeInsets.symmetric(
                horizontal: 32,
                vertical: 16,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildFeature(IconData icon, String text) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Icon(icon, size: 20, color: Colors.green),
        const SizedBox(width: 8),
        Text(text),
      ],
    );
  }
}

class _PortStep extends StatelessWidget {
  final int port;
  final Function(int) onPortChanged;
  final VoidCallback onNext;

  const _PortStep({
    required this.port,
    required this.onPortChanged,
    required this.onNext,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Настройка порта',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Укажите порт, на котором будет работать MTProxy',
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[600],
            ),
          ),
          const SizedBox(height: 32),
          TextField(
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(
              labelText: 'Порт',
              hintText: '443',
              prefixIcon: Icon(Icons.dns),
            ),
            controller: TextEditingController(text: port.toString()),
            onChanged: (value) {
              final port = int.tryParse(value);
              if (port != null) {
                onPortChanged(port);
              }
            },
          ),
          const SizedBox(height: 16),
          _buildRecommendedPorts(context),
          const SizedBox(height: 24),
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.blue.withOpacity(0.1),
              borderRadius: BorderRadius.circular(8),
              border: Border.all(color: Colors.blue.withOpacity(0.3)),
            ),
            child: Row(
              children: [
                const Icon(Icons.info_outline, color: Colors.blue),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    'Порт 443 рекомендуется для обхода блокировок',
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.grey[800],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildRecommendedPorts(BuildContext context) {
    final ports = [443, 8443, 8080, 9000];
    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: ports.map((p) {
        final isSelected = port == p;
        return ChoiceChip(
          label: Text('$p'),
          selected: isSelected,
          onSelected: (selected) {
            if (selected) {
              onPortChanged(p);
            }
          },
        );
      }).toList(),
    );
  }
}

class _SecretsStep extends StatefulWidget {
  final List<String> secrets;
  final Function(List<String>) onSecretsChanged;
  final VoidCallback onNext;

  const _SecretsStep({
    required this.secrets,
    required this.onSecretsChanged,
    required this.onNext,
  });

  @override
  State<_SecretsStep> createState() => _SecretsStepState();
}

class _SecretsStepState extends State<_SecretsStep> {
  void _addSecret() {
    final secret = SecretGenerator.generateHexSecret();
    widget.secrets.add(secret);
    widget.onSecretsChanged(widget.secrets);
  }

  void _removeSecret(int index) {
    widget.secrets.removeAt(index);
    widget.onSecretsChanged(widget.secrets);
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Секретные ключи',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Добавьте хотя бы один секретный ключ для аутентификации клиентов',
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[600],
            ),
          ),
          const SizedBox(height: 24),
          if (widget.secrets.isEmpty)
            Center(
              child: Column(
                children: [
                  Icon(
                    Icons.key_outlined,
                    size: 64,
                    color: Colors.grey[400],
                  ),
                  const SizedBox(height: 16),
                  Text(
                    'Нет секретных ключей',
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.grey[600],
                    ),
                  ),
                ],
              ),
            )
          else
            Expanded(
              child: ListView.separated(
                itemCount: widget.secrets.length,
                separatorBuilder: (_, __) => const SizedBox(height: 8),
                itemBuilder: (context, index) {
                  return Card(
                    child: ListTile(
                      title: Text(
                        '${widget.secrets[index].substring(0, 16)}...',
                        style: const TextStyle(fontFamily: 'monospace'),
                      ),
                      subtitle: const Text('Секретный ключ'),
                      trailing: IconButton(
                        icon: const Icon(Icons.delete_outline),
                        onPressed: () => _removeSecret(index),
                      ),
                    ),
                  );
                },
              ),
            ),
          const SizedBox(height: 16),
          Row(
            children: [
              FilledButton.icon(
                onPressed: _addSecret,
                icon: const Icon(Icons.add),
                label: const Text('Сгенерировать ключ'),
              ),
              const Spacer(),
              Text(
                '${widget.secrets.length} ключ(ей)',
                style: TextStyle(
                  color: Colors.grey[600],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _AdvancedStep extends StatelessWidget {
  final int maxConnections;
  final bool enableIpv6;
  final bool enableStats;
  final Function(int) onMaxConnectionsChanged;
  final Function(bool) onIpv6Changed;
  final Function(bool) onStatsChanged;
  final VoidCallback onNext;

  const _AdvancedStep({
    required this.maxConnections,
    required this.enableIpv6,
    required this.enableStats,
    required this.onMaxConnectionsChanged,
    required this.onIpv6Changed,
    required this.onStatsChanged,
    required this.onNext,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Дополнительные настройки',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Настройте дополнительные параметры MTProxy',
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[600],
            ),
          ),
          const SizedBox(height: 24),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Макс. подключений',
                    style: TextStyle(fontWeight: FontWeight.w500),
                  ),
                  const SizedBox(height: 8),
                  Slider(
                    value: maxConnections.toDouble(),
                    min: 100,
                    max: 100000,
                    divisions: 99,
                    label: '$maxConnections',
                    onChanged: (value) {
                      onMaxConnectionsChanged(value.round());
                    },
                  ),
                  Text('$maxConnections подключений'),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  title: const Text('IPv6'),
                  subtitle: const Text('Включить поддержку IPv6'),
                  value: enableIpv6,
                  onChanged: onIpv6Changed,
                ),
                const Divider(height: 1),
                SwitchListTile(
                  title: const Text('Статистика'),
                  subtitle: const Text('Сбор статистики подключений'),
                  value: enableStats,
                  onChanged: onStatsChanged,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _SummaryStep extends StatelessWidget {
  final int port;
  final int secretsCount;
  final int maxConnections;
  final bool enableIpv6;
  final bool enableStats;
  final VoidCallback onFinish;

  const _SummaryStep({
    required this.port,
    required this.secretsCount,
    required this.maxConnections,
    required this.enableIpv6,
    required this.enableStats,
    required this.onFinish,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        children: [
          Icon(
            Icons.check_circle_outline,
            size: 80,
            color: Theme.of(context).colorScheme.primary,
          ),
          const SizedBox(height: 24),
          const Text(
            'Готово к запуску!',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Проверьте настройки перед запуском',
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[600],
            ),
          ),
          const SizedBox(height: 32),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
                  _buildSummaryRow('Порт', '$port'),
                  const Divider(),
                  _buildSummaryRow('Секретов', '$secretsCount'),
                  const Divider(),
                  _buildSummaryRow('Макс. подключений', '$maxConnections'),
                  const Divider(),
                  _buildSummaryRow(
                    'IPv6',
                    enableIpv6 ? 'Включено' : 'Выключено',
                  ),
                  const Divider(),
                  _buildSummaryRow(
                    'Статистика',
                    enableStats ? 'Включена' : 'Выключена',
                  ),
                ],
              ),
            ),
          ),
          const Spacer(),
          FilledButton.icon(
            onPressed: onFinish,
            icon: const Icon(Icons.play_arrow),
            label: const Text('Запустить MTProxy'),
            style: FilledButton.styleFrom(
              padding: const EdgeInsets.symmetric(
                horizontal: 48,
                vertical: 16,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSummaryRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(
              color: Colors.grey[600],
            ),
          ),
          Text(
            value,
            style: const TextStyle(
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }
}

// ============================================================================
// Индикатор шага
// ============================================================================

class _StepIndicator extends StatelessWidget {
  final bool isActive;
  final bool isCompleted;

  const _StepIndicator({
    required this.isActive,
    required this.isCompleted,
  });

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;

    return Container(
      width: 12,
      height: 12,
      margin: const EdgeInsets.symmetric(horizontal: 4),
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: isCompleted
            ? colorScheme.primary
            : isActive
                ? colorScheme.primary
                : Colors.grey.shade300,
      ),
    );
  }
}
