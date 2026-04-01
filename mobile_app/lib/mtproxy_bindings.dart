// FFI bindings для MTProxy C библиотеки
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// ============================================================================
// Типы функций из C библиотеки
// ============================================================================

// Инициализация
typedef MtProxyInit = Int32 Function();
typedef MtProxyInitDart = int Function();

// Запуск/остановка
typedef MtProxyStart = Int32 Function();
typedef MtProxyStartDart = int Function();

typedef MtProxyStop = Void Function();
typedef MtProxyStopDart = void Function();

// Проверка статуса
typedef MtProxyIsRunning = Uint8 Function();
typedef MtProxyIsRunningDart = bool Function();

// Конфигурация - порт
typedef MtProxySetPort = Int32 Function(Uint16);
typedef MtProxySetPortDart = int Function(int port);

// Конфигурация - секреты
typedef MtProxyAddSecret = Int32 Function(Pointer<Utf8>);
typedef MtProxyAddSecretDart = int Function(Pointer<Utf8> secret);

typedef MtProxyRemoveSecret = Int32 Function(Pointer<Utf8>);
typedef MtProxyRemoveSecretDart = int Function(Pointer<Utf8> secret);

typedef MtProxyClearSecrets = Void Function();
typedef MtProxyClearSecretsDart = void Function();

// Конфигурация - подключения
typedef MtProxySetMaxConnections = Int32 Function(Uint32);
typedef MtProxySetMaxConnectionsDart = int Function(int maxConnections);

// Конфигурация - IPv6
typedef MtProxySetIpv6 = Int32 Function(Uint8);
typedef MtProxySetIpv6Dart = int Function(int enable);

// Статистика
typedef MtProxyGetStats = Pointer<MtProxyStatsNative> Function();
typedef MtProxyGetStatsDart = Pointer<MtProxyStatsNative> Function();

typedef MtProxyGetActiveConnections = Uint32 Function();
typedef MtProxyGetActiveConnectionsDart = int Function();

typedef MtProxyGetTotalConnections = Uint32 Function();
typedef MtProxyGetTotalConnectionsDart = int Function();

typedef MtProxyGetBytesSent = Uint64 Function();
typedef MtProxyGetBytesSentDart = int Function();

typedef MtProxyGetBytesReceived = Uint64 Function();
typedef MtProxyGetBytesReceivedDart = int Function();

typedef MtProxyGetUptime = Uint64 Function();
typedef MtProxyGetUptimeDart = int Function();

// Утилиты
typedef MtProxyGenerateSecret = Int32 Function(Pointer<Utf8>, UintPtr);
typedef MtProxyGenerateSecretDart = int Function(Pointer<Utf8> buffer, int bufferSize);

typedef MtProxyValidateSecret = Uint8 Function(Pointer<Utf8>);
typedef MtProxyValidateSecretDart = bool Function(Pointer<Utf8> secret);

typedef MtProxyGetVersion = Pointer<Utf8> Function();
typedef MtProxyGetVersionDart = Pointer<Utf8> Function();

typedef MtProxyGetLastError = Pointer<Utf8> Function();
typedef MtProxyGetLastErrorDart = Pointer<Utf8> Function();

// ============================================================================
// C структуры
// ============================================================================

/// Структура статистики (должна соответствовать C mtproxy_stats_t)
class MtProxyStatsNative extends Struct {
  @Uint32()
  external int active_connections;
  
  @Uint32()
  external int total_connections;
  
  @Uint64()
  external int bytes_sent;
  
  @Uint64()
  external int bytes_received;
  
  @Uint64()
  external int start_time;
  
  @Double()
  external double cpu_usage;
  
  @Uint32()
  external int memory_usage;
}

// ============================================================================
/// Класс для управления MTProxy через FFI
// ============================================================================
class MtProxyLib {
  DynamicLibrary? _lib;
  bool _isInitialized = false;

  // Функции
  MtProxyInitDart? _init;
  MtProxyStartDart? _start;
  MtProxyStopDart? _stop;
  MtProxyIsRunningDart? _isRunning;
  MtProxySetPortDart? _setPort;
  MtProxyAddSecretDart? _addSecret;
  MtProxyRemoveSecretDart? _removeSecret;
  MtProxyClearSecretsDart? _clearSecrets;
  MtProxySetMaxConnectionsDart? _setMaxConnections;
  MtProxySetIpv6Dart? _setIpv6;
  MtProxyGetStatsDart? _getStats;
  MtProxyGetActiveConnectionsDart? _getActiveConnections;
  MtProxyGetTotalConnectionsDart? _getTotalConnections;
  MtProxyGetBytesSentDart? _getBytesSent;
  MtProxyGetBytesReceivedDart? _getBytesReceived;
  MtProxyGetUptimeDart? _getUptime;
  MtProxyGenerateSecretDart? _generateSecret;
  MtProxyValidateSecretDart? _validateSecret;
  MtProxyGetVersionDart? _getVersion;
  MtProxyGetLastErrorDart? _getLastError;

  MtProxyLib();

  /// Инициализация библиотеки
  Future<void> loadLibrary(String? path) async {
    if (path != null && path.isNotEmpty) {
      try {
        _lib = DynamicLibrary.open(path);
      } catch (e) {
        throw Exception('Не удалось загрузить библиотеку по пути: $path. Ошибка: $e');
      }
    } else {
      // Авто-определение платформы
      if (Platform.isWindows) {
        _lib = DynamicLibrary.open('mtproxy.dll');
      } else if (Platform.isMacOS) {
        _lib = DynamicLibrary.open('libmtproxy.dylib');
      } else if (Platform.isLinux) {
        _lib = DynamicLibrary.open('libmtproxy.so');
      } else if (Platform.isAndroid) {
        _lib = DynamicLibrary.open('libmtproxy.so');
      } else if (Platform.isIOS) {
        _lib = DynamicLibrary.process();
      } else {
        throw UnsupportedError(
          'Платформа ${Platform.operatingSystem} не поддерживается',
        );
      }
    }

    // Загрузка функций
    try {
      _init = _lib!.lookupFunction<MtProxyInit, MtProxyInitDart>('mtproxy_init');
      _start = _lib!.lookupFunction<MtProxyStart, MtProxyStartDart>('mtproxy_start');
      _stop = _lib!.lookupFunction<MtProxyStop, MtProxyStopDart>('mtproxy_stop');
      _isRunning = _lib!.lookupFunction<MtProxyIsRunning, MtProxyIsRunningDart>('mtproxy_is_running');
      _setPort = _lib!.lookupFunction<MtProxySetPort, MtProxySetPortDart>('mtproxy_set_port');
      _addSecret = _lib!.lookupFunction<MtProxyAddSecret, MtProxyAddSecretDart>('mtproxy_add_secret');
      _removeSecret = _lib!.lookupFunction<MtProxyRemoveSecret, MtProxyRemoveSecretDart>('mtproxy_remove_secret');
      _clearSecrets = _lib!.lookupFunction<MtProxyClearSecrets, MtProxyClearSecretsDart>('mtproxy_clear_secrets');
      _setMaxConnections = _lib!.lookupFunction<MtProxySetMaxConnections, MtProxySetMaxConnectionsDart>('mtproxy_set_max_connections');
      _setIpv6 = _lib!.lookupFunction<MtProxySetIpv6, MtProxySetIpv6Dart>('mtproxy_set_ipv6');
      _getStats = _lib!.lookupFunction<MtProxyGetStats, MtProxyGetStatsDart>('mtproxy_get_stats');
      _getActiveConnections = _lib!.lookupFunction<MtProxyGetActiveConnections, MtProxyGetActiveConnectionsDart>('mtproxy_get_active_connections');
      _getTotalConnections = _lib!.lookupFunction<MtProxyGetTotalConnections, MtProxyGetTotalConnectionsDart>('mtproxy_get_total_connections');
      _getBytesSent = _lib!.lookupFunction<MtProxyGetBytesSent, MtProxyGetBytesSentDart>('mtproxy_get_bytes_sent');
      _getBytesReceived = _lib!.lookupFunction<MtProxyGetBytesReceived, MtProxyGetBytesReceivedDart>('mtproxy_get_bytes_received');
      _getUptime = _lib!.lookupFunction<MtProxyGetUptime, MtProxyGetUptimeDart>('mtproxy_get_uptime');
      _generateSecret = _lib!.lookupFunction<MtProxyGenerateSecret, MtProxyGenerateSecretDart>('mtproxy_generate_secret');
      _validateSecret = _lib!.lookupFunction<MtProxyValidateSecret, MtProxyValidateSecretDart>('mtproxy_validate_secret');
      _getVersion = _lib!.lookupFunction<MtProxyGetVersion, MtProxyGetVersionDart>('mtproxy_get_version');
      _getLastError = _lib!.lookupFunction<MtProxyGetLastError, MtProxyGetLastErrorDart>('mtproxy_get_last_error');
      
      _isInitialized = true;
    } catch (e) {
      throw Exception('Не удалось загрузить функции из библиотеки: $e');
    }
  }

  bool get isInitialized => _isInitialized;

  // ============================================================================
  // Основные функции
  // ============================================================================

  int init() {
    _checkInitialized();
    return _init!();
  }

  int start() {
    _checkInitialized();
    return _start!();
  }

  void stop() {
    _checkInitialized();
    _stop!();
  }

  bool isRunning() {
    _checkInitialized();
    return _isRunning!();
  }

  // ============================================================================
  // Функции конфигурации
  // ============================================================================

  int setPort(int port) {
    _checkInitialized();
    if (port < 1 || port > 65535) {
      throw ArgumentError('Порт должен быть от 1 до 65535');
    }
    return _setPort!(port);
  }

  int addSecret(String secret) {
    _checkInitialized();
    final cSecret = secret.toNativeUtf8();
    try {
      return _addSecret!(cSecret);
    } finally {
      calloc.free(cSecret);
    }
  }

  int removeSecret(String secret) {
    _checkInitialized();
    final cSecret = secret.toNativeUtf8();
    try {
      return _removeSecret!(cSecret);
    } finally {
      calloc.free(cSecret);
    }
  }

  void clearSecrets() {
    _checkInitialized();
    _clearSecrets!();
  }

  int setMaxConnections(int maxConnections) {
    _checkInitialized();
    if (maxConnections < 1) {
      throw ArgumentError('maxConnections должен быть больше 0');
    }
    return _setMaxConnections!(maxConnections);
  }

  int setIpv6(bool enable) {
    _checkInitialized();
    return _setIpv6!(enable ? 1 : 0);
  }

  // ============================================================================
  // Функции статистики
  // ============================================================================

  MtProxyStatsNative getStats() {
    _checkInitialized();
    final ptr = _getStats!();
    return ptr.ref;
  }

  int getActiveConnections() {
    _checkInitialized();
    return _getActiveConnections!();
  }

  int getTotalConnections() {
    _checkInitialized();
    return _getTotalConnections!();
  }

  int getBytesSent() {
    _checkInitialized();
    return _getBytesSent!();
  }

  int getBytesReceived() {
    _checkInitialized();
    return _getBytesReceived!();
  }

  int getUptime() {
    _checkInitialized();
    return _getUptime!();
  }

  // ============================================================================
  // Утилиты
  // ============================================================================

  String generateSecret() {
    _checkInitialized();
    final buffer = calloc<Utf8>(65);
    try {
      final result = _generateSecret!(buffer, 65);
      if (result != 0) {
        throw Exception('Ошибка генерации секрета: $result');
      }
      return buffer.toDartString();
    } finally {
      calloc.free(buffer);
    }
  }

  bool validateSecret(String secret) {
    _checkInitialized();
    final cSecret = secret.toNativeUtf8();
    try {
      return _validateSecret!(cSecret);
    } finally {
      calloc.free(cSecret);
    }
  }

  String getVersion() {
    _checkInitialized();
    final ptr = _getVersion!();
    return ptr.toDartString();
  }

  String getLastError() {
    _checkInitialized();
    final ptr = _getLastError!();
    return ptr.toDartString();
  }

  // ============================================================================
  // Внутренние методы
  // ============================================================================

  void _checkInitialized() {
    if (!_isInitialized) {
      throw StateError('Библиотека не инициализирована. Вызовите loadLibrary().');
    }
  }
}

// ============================================================================
// Глобальный экземпляр
// ============================================================================
final mtProxyLib = MtProxyLib();
