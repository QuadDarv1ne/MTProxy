import 'dart:async';
import 'package:flutter/foundation.dart';

/// Уровень логирования
enum LogLevel {
  debug,
  info,
  warning,
  error,
}

/// Запись лога
class LogEntry {
  final DateTime timestamp;
  final LogLevel level;
  final String message;
  final String? source;
  final dynamic error;
  final StackTrace? stackTrace;

  LogEntry({
    required this.timestamp,
    required this.level,
    required this.message,
    this.source,
    this.error,
    this.stackTrace,
  });

  /// Форматированная строка времени
  String get timeString {
    return '${timestamp.hour.toString().padLeft(2, '0')}:'
        '${timestamp.minute.toString().padLeft(2, '0')}:'
        '${timestamp.second.toString().padLeft(2, '0')}.'
        '${timestamp.millisecond.toString().padLeft(3, '0')}';
  }

  /// Форматированный уровень
  String get levelString {
    switch (level) {
      case LogLevel.debug:
        return 'D';
      case LogLevel.info:
        return 'I';
      case LogLevel.warning:
        return 'W';
      case LogLevel.error:
        return 'E';
    }
  }

  /// Форматированная строка лога
  @override
  String toString() {
    final buffer = StringBuffer();
    buffer.write('[$timeString]');
    buffer.write(' [$levelString]');
    if (source != null) {
      buffer.write(' [$source]');
    }
    buffer.write(' $message');
    if (error != null) {
      buffer.write(': $error');
    }
    if (stackTrace != null) {
      buffer.write('\n$stackTrace');
    }
    return buffer.toString();
  }

  Map<String, dynamic> toJson() {
    return {
      'timestamp': timestamp.toIso8601String(),
      'level': level.name,
      'message': message,
      'source': source,
      'error': error?.toString(),
    };
  }
}

/// Сервис логирования
class LoggerService extends ChangeNotifier {
  static final LoggerService _instance = LoggerService._internal();
  factory LoggerService() => _instance;
  LoggerService._internal();

  final List<LogEntry> _logs = [];
  final StreamController<LogEntry> _logController =
      StreamController<LogEntry>.broadcast();

  // Настройки
  bool _enabled = true;
  LogLevel _minLevel = LogLevel.debug;
  int _maxEntries = 1000;
  bool _logToFile = false;

  // Подписка на логи
  Stream<LogEntry> get logStream => _logController.stream;

  // Все логи
  List<LogEntry> get logs => List.unmodifiable(_logs);

  // Фильтрованные логи
  List<LogEntry> getLogsByLevel(LogLevel level) {
    return _logs.where((log) => log.level.index >= level.index).toList();
  }

  List<LogEntry> getDebugLogs() => getLogsByLevel(LogLevel.debug);
  List<LogEntry> getInfoLogs() => getLogsByLevel(LogLevel.info);
  List<LogEntry> getWarningLogs() => getLogsByLevel(LogLevel.warning);
  List<LogEntry> getErrorLogs() => getLogsByLevel(LogLevel.error);

  /// Настройка минимального уровня логирования
  void setMinLevel(LogLevel level) {
    _minLevel = level;
    notifyListeners();
  }

  /// Включение/выключение логирования
  void setEnabled(bool enabled) {
    _enabled = enabled;
    notifyListeners();
  }

  /// Логирование debug сообщения
  void debug(String message, {String? source}) {
    _log(LogLevel.debug, message, source: source);
  }

  /// Логирование info сообщения
  void info(String message, {String? source}) {
    _log(LogLevel.info, message, source: source);
  }

  /// Логирование warning сообщения
  void warning(String message, {String? source}) {
    _log(LogLevel.warning, message, source: source);
  }

  /// Логирование error сообщения
  void error(
    String message, {
    String? source,
    dynamic error,
    StackTrace? stackTrace,
  }) {
    _log(
      LogLevel.error,
      message,
      source: source,
      error: error,
      stackTrace: stackTrace,
    );
  }

  void _log(
    LogLevel level,
    String message, {
    String? source,
    dynamic error,
    StackTrace? stackTrace,
  }) {
    if (!_enabled || level.index < _minLevel.index) return;

    final entry = LogEntry(
      timestamp: DateTime.now(),
      level: level,
      message: message,
      source: source,
      error: error,
      stackTrace: stackTrace,
    );

    _logs.add(entry);

    // Ограничение количества записей
    if (_logs.length > _maxEntries) {
      _logs.removeAt(0);
    }

    // Отправка в стрим
    _logController.add(entry);

    // Вывод в консоль
    if (kDebugMode) {
      print(entry.toString());
    }

    notifyListeners();
  }

  /// Очистка логов
  void clear() {
    _logs.clear();
    notifyListeners();
  }

  /// Удаление старых логов
  void pruneOlderThan(Duration duration) {
    final cutoff = DateTime.now().subtract(duration);
    _logs.removeWhere((log) => log.timestamp.isBefore(cutoff));
    notifyListeners();
  }

  /// Экспорт логов в JSON
  List<Map<String, dynamic>> exportToJson() {
    return _logs.map((log) => log.toJson()).toList();
  }

  /// Экспорт логов в текст
  String exportToText() {
    return _logs.map((log) => log.toString()).join('\n');
  }

  @override
  void dispose() {
    _logController.close();
    super.dispose();
  }
}

/// Расширения для удобного логирования
extension LoggerExtension on Object {
  /// Получить logger для объекта
  LoggerService get log => LoggerService();

  /// Логирование с указанием класса как источника
  void _logDebug(String message) {
    LoggerService().debug(message, source: runtimeType.toString());
  }

  void _logInfo(String message) {
    LoggerService().info(message, source: runtimeType.toString());
  }

  void _logWarning(String message) {
    LoggerService().warning(message, source: runtimeType.toString());
  }

  void _logError(String message, {dynamic error, StackTrace? stackTrace}) {
    LoggerService().error(
      message,
      source: runtimeType.toString(),
      error: error,
      stackTrace: stackTrace,
    );
  }
}

/// Виджет для автоматического логирования жизненного цикла виджетов
class LoggableStatefulWidget<T extends StatefulWidget> extends State<T> {
  final String _widgetName;

  LoggableStatefulWidget(this._widgetName);

  @override
  void initState() {
    super.initState();
    LoggerService().debug('initState', source: _widgetName);
  }

  @override
  void dispose() {
    LoggerService().debug('dispose', source: _widgetName);
    super.dispose();
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    LoggerService().debug('didChangeDependencies', source: _widgetName);
  }

  @override
  void didUpdateWidget(T oldWidget) {
    super.didUpdateWidget(oldWidget);
    LoggerService().debug('didUpdateWidget', source: _widgetName);
  }
}
