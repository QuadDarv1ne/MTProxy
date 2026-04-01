import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';
import 'mtproxy_bindings.dart';
import 'mtproxy_error_handler.dart';
import '../models/proxy_config.dart';
import '../models/proxy_stats.dart';

/// Сервис для управления MTProxy через FFI
class MTProxyService extends ChangeNotifier {
  MTProxyService() {
    _init();
  }

  final _proxyLib = MtProxyLib();
  final _errorManager = MtProxyErrorManager(maxHistorySize: 50);
  
  bool _isRunning = false;
  bool _isInitialized = false;
  ProxyConfig _config = ProxyConfig.defaultConfig();
  ProxyStats _stats = ProxyStats.empty();
  String? _error;
  Timer? _statsTimer;

  // Getters
  bool get isRunning => _isRunning;
  bool get isInitialized => _isInitialized;
  ProxyConfig get config => _config;
  ProxyStats get stats => _stats;
  String? get error => _error;
  bool get hasError => _error != null;
  
  // Ошибки
  List<MtProxyException> get errorHistory => _errorManager.errorHistory;
  MtProxyException? get lastException => _errorManager.lastError;
  int get errorCount => _errorManager.errorCount;

  /// Инициализация сервиса
  Future<void> _init() async {
    try {
      String? libPath = await _findLibraryPath();
      
      if (libPath == null) {
        final exception = MtProxyException.platform(
          'Нативная библиотека не найдена',
        );
        _errorManager.addError(exception);
        _error = exception.message;
        _isInitialized = false;
        notifyListeners();
        return;
      }

      debugPrint('Загрузка библиотеки: $libPath');
      
      await _proxyLib.loadLibrary(libPath);
      
      final result = _proxyLib.init();
      if (result != 0) {
        _errorManager.recordNativeError(result, source: 'init');
        _error = MtProxyErrorInterpreter.interpret(result);
        _isInitialized = false;
        notifyListeners();
        return;
      }
      
      _isInitialized = true;
      _error = null;
      
      debugPrint('MTProxy успешно инициализирован');
      notifyListeners();
    } catch (e) {
      final exception = MtProxyException.ffi(
        'Ошибка инициализации: $e',
        inner: e is Exception ? e : Exception(e),
      );
      _errorManager.addError(exception);
      _error = exception.message;
      _isInitialized = false;
      debugPrint('Ошибка инициализации MTProxy: $e');
      notifyListeners();
    }
  }

  /// Поиск пути к нативной библиотеке
  Future<String?> _findLibraryPath() async {
    String libName;
    List<String> searchPaths = [];

    if (Platform.isWindows) {
      libName = 'mtproxy.dll';
      
      // Поиск в директории приложения
      final appDir = await getApplicationDocumentsDirectory();
      searchPaths.add('${appDir.path}/$libName');
      
      // Поиск в текущей директории
      searchPaths.add(libName);
      
      // Поиск в системных путях
      final exeDir = File(Platform.resolvedExecutable).parent;
      searchPaths.add('${exeDir.path}/$libName');
      
    } else if (Platform.isMacOS) {
      libName = 'libmtproxy.dylib';
      searchPaths.add(libName);
      searchPaths.add('/usr/local/lib/$libName');
      searchPaths.add('/opt/homebrew/lib/$libName');
      
    } else if (Platform.isLinux) {
      libName = 'libmtproxy.so';
      searchPaths.add(libName);
      searchPaths.add('/usr/lib/$libName');
      searchPaths.add('/usr/local/lib/$libName');
      searchPaths.add('/lib/$libName');
      
    } else if (Platform.isAndroid) {
      libName = 'libmtproxy.so';
      // На Android библиотека загружается по имени из assets
      return null;
      
    } else if (Platform.isIOS) {
      libName = 'libmtproxy.a';
      // На iOS библиотека линкуется статически
      return null;
      
    } else {
      throw UnsupportedError('Платформа ${Platform.operatingSystem} не поддерживается');
    }

    // Проверка путей
    for (final path in searchPaths) {
      if (await File(path).exists()) {
        return path;
      }
    }

    return null;
  }

  /// Запуск прокси
  Future<bool> start() async {
    if (!_isInitialized) {
      final exception = MtProxyException.startup(
        'MTProxy не инициализирован',
        code: -1,
      );
      _errorManager.addError(exception);
      _error = exception.message;
      notifyListeners();
      return false;
    }

    if (_isRunning) {
      final exception = MtProxyException.startup(
        'MTProxy уже запущен',
        code: -2,
      );
      _errorManager.addError(exception);
      _error = exception.message;
      notifyListeners();
      return false;
    }

    try {
      // Применение конфигурации перед запуском
      await _applyConfigToNative();

      final result = _proxyLib.start();

      if (result == 0) {
        _isRunning = true;
        _error = null;
        
        // Запуск опроса статистики
        _startStatsPolling();
        
        notifyListeners();
        debugPrint('MTProxy запущен успешно');
        return true;
      } else {
        _errorManager.recordNativeError(result, source: 'start');
        _error = MtProxyErrorInterpreter.interpret(result);
        notifyListeners();
        return false;
      }
    } catch (e) {
      final exception = MtProxyException.runtime(
        'Ошибка запуска: $e',
        source: 'start',
      );
      _errorManager.addError(exception);
      _error = exception.message;
      notifyListeners();
      return false;
    }
  }

  /// Остановка прокси
  Future<void> stop() async {
    if (!_isRunning) {
      return;
    }

    try {
      _proxyLib.stop();
      _isRunning = false;
      _stopStatsPolling();
      notifyListeners();
      debugPrint('MTProxy остановлен');
    } catch (e) {
      final exception = MtProxyException.runtime(
        'Ошибка остановки: $e',
        source: 'stop',
      );
      _errorManager.addError(exception);
      _error = exception.message;
      notifyListeners();
    }
  }

  /// Применение конфигурации к нативной библиотеке
  Future<void> _applyConfigToNative() async {
    try {
      // Установка порта
      final portResult = _proxyLib.setPort(_config.port);
      if (portResult != 0) {
        _errorManager.recordNativeError(portResult, source: 'setPort');
        throw MtProxyException.configuration(
          'Ошибка установки порта: ${MtProxyErrorInterpreter.interpret(portResult)}',
          code: portResult,
        );
      }

      // Добавление секретов
      for (final secret in _config.secrets) {
        final secretResult = _proxyLib.addSecret(secret);
        if (secretResult != 0) {
          _errorManager.recordNativeError(secretResult, source: 'addSecret');
          debugPrint('Предупреждение: ошибка добавления секрета ($secretResult)');
        }
      }

      // Установка максимального количества подключений
      final maxConnResult = _proxyLib.setMaxConnections(_config.maxConnections);
      if (maxConnResult != 0) {
        _errorManager.recordNativeError(maxConnResult, source: 'setMaxConnections');
        debugPrint('Предупреждение: ошибка установки max_connections ($maxConnResult)');
      }

      // Настройка IPv6
      final ipv6Result = _proxyLib.setIpv6(_config.enableIpv6);
      if (ipv6Result != 0) {
        _errorManager.recordNativeError(ipv6Result, source: 'setIpv6');
        debugPrint('Предупреждение: ошибка настройки IPv6 ($ipv6Result)');
      }

      debugPrint('Конфигурация применена успешно');
    } catch (e) {
      debugPrint('Ошибка применения конфигурации: $e');
      rethrow;
    }
  }

  /// Применение конфигурации (публичный метод)
  Future<void> applyConfig(ProxyConfig config) async {
    _config = config;
    
    if (_isRunning) {
      // Если прокси запущен, применяем изменения
      await _applyConfigToNative();
    }
    
    notifyListeners();
  }

  /// Обновление отдельного параметра конфигурации
  Future<void> updateConfig({
    int? port,
    List<String>? secrets,
    int? maxConnections,
    bool? enableIpv6,
    bool? enableStats,
  }) async {
    final newConfig = ProxyConfig(
      port: port ?? _config.port,
      secrets: secrets ?? _config.secrets,
      maxConnections: maxConnections ?? _config.maxConnections,
      enableIpv6: enableIpv6 ?? _config.enableIpv6,
      enableStats: enableStats ?? _config.enableStats,
    );
    
    await applyConfig(newConfig);
  }

  /// Очистить историю ошибок
  void clearErrorHistory() {
    _errorManager.clear();
    _error = null;
    notifyListeners();
  }

  /// Опрос статистики
  void _startStatsPolling() {
    _statsTimer?.cancel();
    _statsTimer = Timer.periodic(const Duration(seconds: 2), (_) => _updateStats());
    _updateStats();
  }

  void _stopStatsPolling() {
    _statsTimer?.cancel();
    _statsTimer = null;
  }

  void _updateStats() {
    if (!_isRunning) return;

    try {
      final nativeStats = _proxyLib.getStats();
      
      _stats = ProxyStats(
        activeConnections: nativeStats.activeConnections,
        totalConnections: nativeStats.totalConnections,
        bytesSent: nativeStats.bytesSent,
        bytesReceived: nativeStats.bytesReceived,
        startTime: DateTime.fromMillisecondsSinceEpoch(
          nativeStats.startTime.toInt() * 1000,
        ),
        cpuUsage: nativeStats.cpuUsage,
        memoryUsage: nativeStats.memoryUsage,
      );
      
      notifyListeners();
    } catch (e) {
      debugPrint('Ошибка обновления статистики: $e');
    }
  }

  /// Получение версии библиотеки
  String getVersion() {
    try {
      return _proxyLib.getVersion();
    } catch (e) {
      return 'неизвестно';
    }
  }

  /// Проверка доступности библиотеки
  bool isLibraryAvailable() {
    return _isInitialized;
  }

  @override
  void dispose() {
    _stopStatsPolling();
    if (_isRunning) {
      stop();
    }
    super.dispose();
  }
}
