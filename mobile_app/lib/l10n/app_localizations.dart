import 'package:flutter/material.dart';

/// Класс локализации приложения
class AppLocalizations {
  final Locale locale;

  AppLocalizations(this.locale);

  static AppLocalizations of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations)!;
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  // Статические переводы для доступа без контекста
  static Map<String, String> _localizedStrings = {};

  // Getter'ы для переводов
  late final Map<String, String> _strings;

  String get appTitle => _strings['app_title'] ?? 'MTProxy Manager';
  String get home => _strings['home'] ?? 'Главная';
  String get config => _strings['config'] ?? 'Конфигурация';
  String get statistics => _strings['statistics'] ?? 'Статистика';
  String get settings => _strings['settings'] ?? 'Настройки';

  // Главная страница
  String get proxyStatus => _strings['proxy_status'] ?? 'Статус прокси';
  String get running => _strings['running'] ?? 'Работает';
  String get stopped => _strings['stopped'] ?? 'Остановлен';
  String get initializing => _strings['initializing'] ?? 'Инициализация...';
  String get error => _strings['error'] ?? 'Ошибка';
  
  String get startProxy => _strings['start_proxy'] ?? 'Запустить прокси';
  String get stopProxy => _strings['stop_proxy'] ?? 'Остановить прокси';
  String get confirmStop => _strings['confirm_stop'] ?? 'Остановить прокси?';
  String get confirmStopMessage => _strings['confirm_stop_message'] ?? 
      'Все активные подключения будут разорваны. Вы уверены?';
  String get cancel => _strings['cancel'] ?? 'Отмена';
  String get confirm => _strings['confirm'] ?? 'Подтвердить';

  // Статистика
  String get activeConnections => _strings['active_connections'] ?? 'Активные подключения';
  String get totalConnections => _strings['total_connections'] ?? 'Всего подключений';
  String get bytesSent => _strings['bytes_sent'] ?? 'Отправлено';
  String get bytesReceived => _strings['bytes_received'] ?? 'Получено';
  String get uptime => _strings['uptime'] ?? 'Время работы';
  String get cpuUsage => _strings['cpu_usage'] ?? 'Использование CPU';
  String get memoryUsage => _strings['memory_usage'] ?? 'Использование памяти';
  String get traffic => _strings['traffic'] ?? 'Трафик';
  String get connections => _strings['connections'] ?? 'Подключения';
  String get overview => _strings['overview'] ?? 'Обзор';

  // Конфигурация
  String get port => _strings['port'] ?? 'Порт';
  String get portServer => _strings['port_server'] ?? 'Порт сервера';
  String get secretKeys => _strings['secret_keys'] ?? 'Секретные ключи';
  String get addKey => _strings['add_key'] ?? 'Добавить ключ';
  String get deleteKey => _strings['delete_key'] ?? 'Удалить ключ';
  String get generate => _strings['generate'] ?? 'Сгенерировать';
  String get paste => _strings['paste'] ?? 'Вставить';
  String get show => _strings['show'] ?? 'Показать';
  String get hide => _strings['hide'] ?? 'Скрыть';
  String get maxConnections => _strings['max_connections'] ?? 'Макс. подключений';
  String get ipv6 => _strings['ipv6'] ?? 'IPv6';
  String get enableStats => _strings['enable_stats'] ?? 'Статистика';
  String get save => _strings['save'] ?? 'Сохранить';
  String get saveConfig => _strings['save_config'] ?? 'Сохранить конфигурацию';
  String get configSaved => _strings['config_saved'] ?? 'Конфигурация сохранена';

  // Настройки
  String get darkTheme => _strings['dark_theme'] ?? 'Тёмная тема';
  String get notifications => _strings['notifications'] ?? 'Уведомления';
  String get autoStart => _strings['auto_start'] ?? 'Автозапуск';
  String get language => _strings['language'] ?? 'Язык';
  String get version => _strings['version'] ?? 'Версия';
  String get checkUpdates => _strings['check_updates'] ?? 'Проверить обновления';
  String get license => _strings['license'] ?? 'Лицензия';
  String get documentation => _strings['documentation'] ?? 'Документация';
  String get githubRepo => _strings['github_repo'] ?? 'GitHub репозиторий';
  String get resetSettings => _strings['reset_settings'] ?? 'Сбросить настройки';
  String get confirmReset => _strings['confirm_reset'] ?? 'Сброс настроек';
  String get confirmResetMessage => _strings['confirm_reset_message'] ?? 
      'Вы уверены? Все настройки будут сброшены к значениям по умолчанию.';
  String get reset => _strings['reset'] ?? 'Сбросить';
  String get settingsReset => _strings['settings_reset'] ?? 'Настройки сброшены';

  // Уведомления
  String get proxyStarted => _strings['proxy_started'] ?? 'Прокси запущен';
  String get proxyStartedMessage => _strings['proxy_started_message'] ?? 
      'MTProxy сервер успешно запущен и готов к работе';
  String get proxyStopped => _strings['proxy_stopped'] ?? 'Прокси остановлен';
  String get proxyStoppedMessage => _strings['proxy_stopped_message'] ?? 
      'MTProxy сервер был остановлен';
  String get proxyError => _strings['proxy_error'] ?? 'Ошибка прокси';

  // Валидация
  String get enterPort => _strings['enter_port'] ?? 'Введите порт';
  String get portMustBeNumber => _strings['port_must_be_number'] ?? 'Порт должен быть числом';
  String get portRange => _strings['port_range'] ?? 'Порт должен быть от 1 до 65535';
  String get enterSecret => _strings['enter_secret'] ?? 'Введите секрет';
  String get secretLength => _strings['secret_length'] ?? 
      'Секрет должен содержать 64 hex-символа (32 байта)';
  String get secretHexOnly => _strings['secret_hex_only'] ?? 
      'Секрет должен содержать только hex-символы (0-9, a-f)';

  // Разное
  String get about => _strings['about'] ?? 'О приложении';
  String get refresh => _strings['refresh'] ?? 'Обновить';
  String get delete => _strings['delete'] ?? 'Удалить';
  String get add => _strings['add'] ?? 'Добавить';
  String get edit => _strings['edit'] ?? 'Редактировать';
  String get close => _strings['close'] ?? 'Закрыть';
  String get ok => _strings['ok'] ?? 'OK';
  String get yes => _strings['yes'] ?? 'Да';
  String get no => _strings['no'] ?? 'Нет';
  String get loading => _strings['loading'] ?? 'Загрузка...';
  String get noData => _strings['no_data'] ?? 'Нет данных';
  String get availableOn => _strings['available_on'] ?? 'Доступно на:';

  /// Загрузка переводов
  Future<void> load() async {
    _strings = _localizedStringsForLocale(locale);
  }

  /// Переводы для локали
  static Map<String, String> _localizedStringsForLocale(Locale locale) {
    switch (locale.languageCode) {
      case 'en':
        return _enTranslations;
      case 'ru':
      default:
        return _ruTranslations;
    }
  }

  /// Русские переводы
  static const Map<String, String> _ruTranslations = {
    'app_title': 'MTProxy Manager',
    'home': 'Главная',
    'config': 'Конфигурация',
    'statistics': 'Статистика',
    'settings': 'Настройки',
    'proxy_status': 'Статус прокси',
    'running': 'Работает',
    'stopped': 'Остановлен',
    'initializing': 'Инициализация...',
    'error': 'Ошибка',
    'start_proxy': 'Запустить прокси',
    'stop_proxy': 'Остановить прокси',
    'confirm_stop': 'Остановить прокси?',
    'confirm_stop_message': 'Все активные подключения будут разорваны. Вы уверены?',
    'cancel': 'Отмена',
    'confirm': 'Подтвердить',
    'active_connections': 'Активные подключения',
    'total_connections': 'Всего подключений',
    'bytes_sent': 'Отправлено',
    'bytes_received': 'Получено',
    'uptime': 'Время работы',
    'cpu_usage': 'Использование CPU',
    'memory_usage': 'Использование памяти',
    'traffic': 'Трафик',
    'connections': 'Подключения',
    'overview': 'Обзор',
    'port': 'Порт',
    'port_server': 'Порт сервера',
    'secret_keys': 'Секретные ключи',
    'add_key': 'Добавить ключ',
    'delete_key': 'Удалить ключ',
    'generate': 'Сгенерировать',
    'paste': 'Вставить',
    'show': 'Показать',
    'hide': 'Скрыть',
    'max_connections': 'Макс. подключений',
    'ipv6': 'IPv6',
    'enable_stats': 'Статистика',
    'save': 'Сохранить',
    'save_config': 'Сохранить конфигурацию',
    'config_saved': 'Конфигурация сохранена',
    'dark_theme': 'Тёмная тема',
    'notifications': 'Уведомления',
    'auto_start': 'Автозапуск',
    'language': 'Язык',
    'version': 'Версия',
    'check_updates': 'Проверить обновления',
    'license': 'Лицензия',
    'documentation': 'Документация',
    'github_repo': 'GitHub репозиторий',
    'reset_settings': 'Сбросить настройки',
    'confirm_reset': 'Сброс настроек',
    'confirm_reset_message': 'Вы уверены? Все настройки будут сброшены к значениям по умолчанию.',
    'reset': 'Сбросить',
    'settings_reset': 'Настройки сброшены',
    'proxy_started': 'Прокси запущен',
    'proxy_started_message': 'MTProxy сервер успешно запущен и готов к работе',
    'proxy_stopped': 'Прокси остановлен',
    'proxy_stopped_message': 'MTProxy сервер был остановлен',
    'proxy_error': 'Ошибка прокси',
    'enter_port': 'Введите порт',
    'port_must_be_number': 'Порт должен быть числом',
    'port_range': 'Порт должен быть от 1 до 65535',
    'enter_secret': 'Введите секрет',
    'secret_length': 'Секрет должен содержать 64 hex-символа (32 байта)',
    'secret_hex_only': 'Секрет должен содержать только hex-символы (0-9, a-f)',
    'about': 'О приложении',
    'refresh': 'Обновить',
    'delete': 'Удалить',
    'add': 'Добавить',
    'edit': 'Редактировать',
    'close': 'Закрыть',
    'ok': 'OK',
    'yes': 'Да',
    'no': 'Нет',
    'loading': 'Загрузка...',
    'no_data': 'Нет данных',
    'available_on': 'Доступно на:',
  };

  /// Английские переводы
  static const Map<String, String> _enTranslations = {
    'app_title': 'MTProxy Manager',
    'home': 'Home',
    'config': 'Configuration',
    'statistics': 'Statistics',
    'settings': 'Settings',
    'proxy_status': 'Proxy Status',
    'running': 'Running',
    'stopped': 'Stopped',
    'initializing': 'Initializing...',
    'error': 'Error',
    'start_proxy': 'Start Proxy',
    'stop_proxy': 'Stop Proxy',
    'confirm_stop': 'Stop Proxy?',
    'confirm_stop_message': 'All active connections will be terminated. Are you sure?',
    'cancel': 'Cancel',
    'confirm': 'Confirm',
    'active_connections': 'Active Connections',
    'total_connections': 'Total Connections',
    'bytes_sent': 'Sent',
    'bytes_received': 'Received',
    'uptime': 'Uptime',
    'cpu_usage': 'CPU Usage',
    'memory_usage': 'Memory Usage',
    'traffic': 'Traffic',
    'connections': 'Connections',
    'overview': 'Overview',
    'port': 'Port',
    'port_server': 'Server Port',
    'secret_keys': 'Secret Keys',
    'add_key': 'Add Key',
    'delete_key': 'Delete Key',
    'generate': 'Generate',
    'paste': 'Paste',
    'show': 'Show',
    'hide': 'Hide',
    'max_connections': 'Max Connections',
    'ipv6': 'IPv6',
    'enable_stats': 'Statistics',
    'save': 'Save',
    'save_config': 'Save Configuration',
    'config_saved': 'Configuration saved',
    'dark_theme': 'Dark Theme',
    'notifications': 'Notifications',
    'auto_start': 'Auto Start',
    'language': 'Language',
    'version': 'Version',
    'check_updates': 'Check for Updates',
    'license': 'License',
    'documentation': 'Documentation',
    'github_repo': 'GitHub Repository',
    'reset_settings': 'Reset Settings',
    'confirm_reset': 'Reset Settings',
    'confirm_reset_message': 'Are you sure? All settings will be reset to default values.',
    'reset': 'Reset',
    'settings_reset': 'Settings reset',
    'proxy_started': 'Proxy Started',
    'proxy_started_message': 'MTProxy server successfully started and ready to work',
    'proxy_stopped': 'Proxy Stopped',
    'proxy_stopped_message': 'MTProxy server has been stopped',
    'proxy_error': 'Proxy Error',
    'enter_port': 'Enter port',
    'port_must_be_number': 'Port must be a number',
    'port_range': 'Port must be between 1 and 65535',
    'enter_secret': 'Enter secret',
    'secret_length': 'Secret must be 64 hex characters (32 bytes)',
    'secret_hex_only': 'Secret must contain only hex characters (0-9, a-f)',
    'about': 'About',
    'refresh': 'Refresh',
    'delete': 'Delete',
    'add': 'Add',
    'edit': 'Edit',
    'close': 'Close',
    'ok': 'OK',
    'yes': 'Yes',
    'no': 'No',
    'loading': 'Loading...',
    'no_data': 'No data',
    'available_on': 'Available on:',
  };
}

/// Делегат локализации
class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  bool isSupported(Locale locale) {
    return ['en', 'ru'].contains(locale.languageCode);
  }

  @override
  Future<AppLocalizations> load(Locale locale) async {
    final localizations = AppLocalizations(locale);
    await localizations.load();
    return localizations;
  }

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

/// Расширение для получения локали из кода
extension LocaleExtension on String {
  Locale get toLocale {
    if (contains('_')) {
      final parts = split('_');
      return Locale(parts[0], parts[1]);
    }
    return Locale(this);
  }
}
