import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// Экран настроек приложения
class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  bool _darkMode = false;
  bool _notifications = true;
  bool _autoStart = false;
  String _language = 'ru';

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _darkMode = prefs.getBool('dark_mode') ?? false;
      _notifications = prefs.getBool('notifications') ?? true;
      _autoStart = prefs.getBool('auto_start') ?? false;
      _language = prefs.getString('language') ?? 'ru';
    });
  }

  Future<void> _saveSetting(String key, dynamic value) async {
    final prefs = await SharedPreferences.getInstance();
    if (value is bool) {
      await prefs.setBool(key, value);
    } else if (value is String) {
      await prefs.setString(key, value);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Настройки'),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          // Тема
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  title: const Text('Тёмная тема'),
                  subtitle: const Text('Использовать тёмную тему оформления'),
                  value: _darkMode,
                  onChanged: (value) {
                    setState(() => _darkMode = value);
                    _saveSetting('dark_mode', value);
                  },
                  secondary: Icon(
                    _darkMode ? Icons.dark_mode : Icons.light_mode,
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Уведомления
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  title: const Text('Уведомления'),
                  subtitle: const Text('Показывать уведомления о событиях'),
                  value: _notifications,
                  onChanged: (value) {
                    setState(() => _notifications = value);
                    _saveSetting('notifications', value);
                  },
                  secondary: const Icon(Icons.notifications),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Автозапуск
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  title: const Text('Автозапуск'),
                  subtitle: const Text('Запускать прокси при старте системы'),
                  value: _autoStart,
                  onChanged: (value) {
                    setState(() => _autoStart = value);
                    _saveSetting('auto_start', value);
                  },
                  secondary: const Icon(Icons.rocket_launch),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Язык
          Card(
            child: ListTile(
              title: const Text('Язык'),
              subtitle: Text(_getLanguageName(_language)),
              leading: const Icon(Icons.language),
              trailing: const Icon(Icons.chevron_right),
              onTap: () => _showLanguageDialog(),
            ),
          ),

          const SizedBox(height: 16),

          // О приложении
          Card(
            child: Column(
              children: [
                ListTile(
                  title: const Text('Версия приложения'),
                  subtitle: const Text('1.0.0'),
                  leading: const Icon(Icons.info_outline),
                ),
                const Divider(),
                ListTile(
                  title: const Text('Проверить обновления'),
                  leading: const Icon(Icons.update),
                  onTap: () {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('Установлена последняя версия')),
                    );
                  },
                ),
                const Divider(),
                ListTile(
                  title: const Text('Лицензия'),
                  subtitle: const Text('MIT License'),
                  leading: const Icon(Icons.description),
                  onTap: () {
                    showLicensePage(
                      context: context,
                      applicationName: 'MTProxy Manager',
                      applicationVersion: '1.0.0',
                    );
                  },
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Документация
          Card(
            child: Column(
              children: [
                ListTile(
                  title: const Text('Документация'),
                  subtitle: const Text('Руководство пользователя'),
                  leading: const Icon(Icons.menu_book),
                  trailing: const Icon(Icons.open_in_new),
                  onTap: () {
                    // Открыть документацию
                  },
                ),
                const Divider(),
                ListTile(
                  title: const Text('GitHub репозиторий'),
                  subtitle: const Text('Исходный код проекта'),
                  leading: const Icon(Icons.code),
                  trailing: const Icon(Icons.open_in_new),
                  onTap: () {
                    // Открыть GitHub
                  },
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Сброс настроек
          Card(
            child: ListTile(
              title: const Text(
                'Сбросить все настройки',
                style: TextStyle(color: Colors.red),
              ),
              leading: const Icon(Icons.delete_forever, color: Colors.red),
              onTap: () => _confirmReset(),
            ),
          ),

          const SizedBox(height: 32),

          // Информация о платформах
          Center(
            child: Column(
              children: [
                Text(
                  'Доступно на:',
                  style: TextStyle(color: Colors.grey[600]),
                ),
                const SizedBox(height: 8),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _buildPlatformBadge('Windows', Colors.blue),
                    const SizedBox(width: 8),
                    _buildPlatformBadge('Linux', Colors.orange),
                    const SizedBox(width: 8),
                    _buildPlatformBadge('macOS', Colors.purple),
                    const SizedBox(width: 8),
                    _buildPlatformBadge('iOS', Colors.grey),
                    const SizedBox(width: 8),
                    _buildPlatformBadge('Android', Colors.green),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildPlatformBadge(String name, Color color) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(4),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Text(
        name,
        style: TextStyle(
          fontSize: 10,
          color: color,
          fontWeight: FontWeight.bold,
        ),
      ),
    );
  }

  String _getLanguageName(String code) {
    switch (code) {
      case 'ru':
        return 'Русский';
      case 'en':
        return 'English';
      case 'zh':
        return '中文';
      default:
        return 'Русский';
    }
  }

  void _showLanguageDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Выберите язык'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            RadioListTile<String>(
              title: const Text('Русский'),
              value: 'ru',
              groupValue: _language,
              onChanged: (value) {
                setState(() => _language = value!);
                _saveSetting('language', value);
                Navigator.pop(context);
              },
            ),
            RadioListTile<String>(
              title: const Text('English'),
              value: 'en',
              groupValue: _language,
              onChanged: (value) {
                setState(() => _language = value!);
                _saveSetting('language', value);
                Navigator.pop(context);
              },
            ),
            RadioListTile<String>(
              title: const Text('中文'),
              value: 'zh',
              groupValue: _language,
              onChanged: (value) {
                setState(() => _language = value!);
                _saveSetting('language', value);
                Navigator.pop(context);
              },
            ),
          ],
        ),
      ),
    );
  }

  void _confirmReset() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        icon: const Icon(Icons.warning_amber, color: Colors.orange, size: 48),
        title: const Text('Сброс настроек'),
        content: const Text(
          'Вы уверены? Все настройки будут сброшены к значениям по умолчанию.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Отмена'),
          ),
          FilledButton(
            onPressed: () async {
              final prefs = await SharedPreferences.getInstance();
              await prefs.clear();
              if (mounted) {
                Navigator.pop(context);
                _loadSettings();
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(
                    content: Text('Настройки сброшены'),
                    backgroundColor: Colors.green,
                  ),
                );
              }
            },
            style: FilledButton.styleFrom(
              backgroundColor: Colors.red,
            ),
            child: const Text('Сбросить'),
          ),
        ],
      ),
    );
  }
}
