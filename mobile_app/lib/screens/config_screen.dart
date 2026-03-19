import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../services/mtproxy_service.dart';
import '../../widgets/secret_list_editor.dart';

/// Экран конфигурации MTProxy
class ConfigScreen extends StatefulWidget {
  const ConfigScreen({super.key});

  @override
  State<ConfigScreen> createState() => _ConfigScreenState();
}

class _ConfigScreenState extends State<ConfigScreen> {
  final _formKey = GlobalKey<FormState>();
  
  late TextEditingController _portController;
  late TextEditingController _maxConnectionsController;
  late List<String> _secrets;
  bool _enableIpv6 = false;
  bool _enableStats = true;
  
  bool _hasChanges = false;

  @override
  void initState() {
    super.initState();
    _loadConfig();
  }

  void _loadConfig() {
    final service = context.read<MTProxyService>();
    _portController = TextEditingController(
      text: service.config.port.toString(),
    );
    _maxConnectionsController = TextEditingController(
      text: service.config.maxConnections.toString(),
    );
    _secrets = List.from(service.config.secrets);
    _enableIpv6 = service.config.enableIpv6;
    _enableStats = service.config.enableStats;
  }

  @override
  void dispose() {
    _portController.dispose();
    _maxConnectionsController.dispose();
    super.dispose();
  }

  void _markChanged() {
    if (!_hasChanges) {
      setState(() => _hasChanges = true);
    }
  }

  Future<void> _saveConfig() async {
    if (!_formKey.currentState!.validate()) return;

    final service = context.read<MTProxyService>();
    
    final newSecrets = _secrets.where((s) => s.isNotEmpty).toList();
    
    await service.updateConfig(
      port: int.parse(_portController.text),
      secrets: newSecrets,
      maxConnections: int.parse(_maxConnectionsController.text),
      enableIpv6: _enableIpv6,
      enableStats: _enableStats,
    );

    setState(() => _hasChanges = false);

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Конфигурация сохранена'),
          backgroundColor: Colors.green,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Конфигурация'),
        actions: [
          if (_hasChanges)
            IconButton(
              icon: const Icon(Icons.save),
              onPressed: _saveConfig,
              tooltip: 'Сохранить',
            ),
        ],
      ),
      body: Form(
        key: _formKey,
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            // Порт
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Порт',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    TextFormField(
                      controller: _portController,
                      keyboardType: TextInputType.number,
                      decoration: const InputDecoration(
                        labelText: 'Порт сервера',
                        hintText: '443',
                        prefixIcon: Icon(Icons.dns),
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Введите порт';
                        }
                        final port = int.tryParse(value);
                        if (port == null || port < 1 || port > 65535) {
                          return 'Порт должен быть от 1 до 65535';
                        }
                        return null;
                      },
                      onChanged: (_) => _markChanged(),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Рекомендуется использовать порт 443 для обхода блокировок',
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 16),

            // Секретные ключи
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        const Text(
                          'Секретные ключи',
                          style: TextStyle(
                            fontSize: 18,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        const Spacer(),
                        IconButton(
                          icon: const Icon(Icons.add_circle_outline),
                          onPressed: () {
                            setState(() {
                              _secrets.add('');
                              _markChanged();
                            });
                          },
                          tooltip: 'Добавить ключ',
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    SecretListEditor(
                      secrets: _secrets,
                      onChanged: (secrets) {
                        setState(() => _secrets = secrets);
                        _markChanged();
                      },
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Hex-ключи для аутентификации клиентов (32 байта в hex)',
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 16),

            // Максимум подключений
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Лимит подключений',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    TextFormField(
                      controller: _maxConnectionsController,
                      keyboardType: TextInputType.number,
                      decoration: const InputDecoration(
                        labelText: 'Макс. подключений',
                        hintText: '10000',
                        prefixIcon: Icon(Icons.people),
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Введите значение';
                        }
                        final val = int.tryParse(value);
                        if (val == null || val < 1) {
                          return 'Значение должно быть больше 0';
                        }
                        return null;
                      },
                      onChanged: (_) => _markChanged(),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 16),

            // Дополнительные настройки
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Дополнительно',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    SwitchListTile(
                      title: const Text('IPv6'),
                      subtitle: const Text('Включить поддержку IPv6'),
                      value: _enableIpv6,
                      onChanged: (value) {
                        setState(() => _enableIpv6 = value);
                        _markChanged();
                      },
                    ),
                    SwitchListTile(
                      title: const Text('Статистика'),
                      subtitle: const Text('Включить сбор статистики'),
                      value: _enableStats,
                      onChanged: (value) {
                        setState(() => _enableStats = value);
                        _markChanged();
                      },
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 80),

            // Кнопка сохранения (если есть изменения)
            if (_hasChanges)
              Padding(
                padding: const EdgeInsets.only(bottom: 16),
                child: FilledButton.icon(
                  onPressed: _saveConfig,
                  icon: const Icon(Icons.save),
                  label: const Text('Сохранить конфигурацию'),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
