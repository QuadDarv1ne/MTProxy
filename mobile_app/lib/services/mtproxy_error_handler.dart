/// Типы ошибок MTProxy
enum MtProxyErrorType {
  /// Ошибка инициализации
  initialization,
  
  /// Ошибка конфигурации
  configuration,
  
  /// Ошибка запуска
  startup,
  
  /// Ошибка выполнения
  runtime,
  
  /// Ошибка FFI (интеграция с C)
  ffi,
  
  /// Ошибка платформы
  platform,
  
  /// Неизвестная ошибка
  unknown,
}

/// Исключение MTProxy
class MtProxyException implements Exception {
  final String message;
  final MtProxyErrorType type;
  final int? errorCode;
  final Exception? innerException;
  final String? source;

  const MtProxyException(
    this.message, {
    this.type = MtProxyErrorType.unknown,
    this.errorCode,
    this.innerException,
    this.source,
  });

  @override
  String toString() {
    final buffer = StringBuffer('MtProxyException: $message');
    if (source != null) {
      buffer.write(' (источник: $source)');
    }
    if (errorCode != null) {
      buffer.write(' [код ошибки: $errorCode]');
    }
    if (innerException != null) {
      buffer.write(' Причина: $innerException');
    }
    return buffer.toString();
  }

  /// Создание исключения инициализации
  factory MtProxyException.initialization(String message, {int? code}) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.initialization,
      errorCode: code,
    );
  }

  /// Создание исключения конфигурации
  factory MtProxyException.configuration(String message, {int? code}) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.configuration,
      errorCode: code,
    );
  }

  /// Создание исключения запуска
  factory MtProxyException.startup(String message, {int? code}) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.startup,
      errorCode: code,
    );
  }

  /// Создание исключения выполнения
  factory MtProxyException.runtime(String message, {int? code}) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.runtime,
      errorCode: code,
    );
  }

  /// Создание исключения FFI
  factory MtProxyException.ffi(String message, {Exception? inner}) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.ffi,
      innerException: inner,
    );
  }

  /// Создание исключения платформы
  factory MtProxyException.platform(String message) {
    return MtProxyException(
      message,
      type: MtProxyErrorType.platform,
    );
  }
}

/// Результат операции MTProxy
class MtProxyResult {
  final bool isSuccess;
  final String? errorMessage;
  final MtProxyErrorType? errorType;
  final int? errorCode;

  const MtProxyResult._({
    required this.isSuccess,
    this.errorMessage,
    this.errorType,
    this.errorCode,
  });

  /// Успешный результат
  factory MtProxyResult.success() {
    return const MtProxyResult._(isSuccess: true);
  }

  /// Результат с ошибкой
  factory MtProxyResult.failure(
    String message, {
    MtProxyErrorType? type,
    int? code,
  }) {
    return MtProxyResult._(
      isSuccess: false,
      errorMessage: message,
      errorType: type,
      errorCode: code,
    );
  }

  /// Бросить исключение если ошибка
  void throwIfError() {
    if (!isSuccess) {
      throw MtProxyException(
        errorMessage!,
        type: errorType ?? MtProxyErrorType.unknown,
        errorCode: errorCode,
      );
    }
  }

  /// Выполнить действие если успех
  void onSuccess(void Function() action) {
    if (isSuccess) {
      action();
    }
  }

  /// Выполнить действие если ошибка
  void onFailure(void Function(String message) action) {
    if (!isSuccess) {
      action(errorMessage!);
    }
  }
}

/// Интерпретатор кодов ошибок из C-библиотеки
class MtProxyErrorInterpreter {
  /// Интерпретация кода ошибки в сообщение
  static String interpret(int code) {
    if (code == 0) {
      return 'Операция выполнена успешно';
    }

    switch (code) {
      case -1:
        return 'MTProxy не инициализирован';
      case -2:
        return 'MTProxy уже запущен';
      case -3:
        return 'Необходимо добавить хотя бы один секретный ключ';
      case -4:
        return 'Ошибка выделения памяти';
      case -5:
        return 'Недопустимый параметр';
      case -6:
        return 'Ошибка сети';
      case -7:
        return 'Ошибка криптографии';
      case -8:
        return 'Превышен лимит подключений';
      case -9:
        return 'Порт уже используется';
      case -10:
        return 'Недостаточно прав';
      default:
        if (code > 0) {
          return 'Системная ошибка: $code';
        }
        return 'Неизвестная ошибка: $code';
    }
  }

  /// Интерпретация кода ошибки в тип
  static MtProxyErrorType interpretType(int code) {
    if (code == 0) {
      return MtProxyErrorType.unknown;
    }

    switch (code) {
      case -1:
      case -4:
        return MtProxyErrorType.initialization;
      case -3:
      case -5:
        return MtProxyErrorType.configuration;
      case -2:
      case -8:
      case -9:
        return MtProxyErrorType.startup;
      case -6:
      case -7:
        return MtProxyErrorType.runtime;
      case -10:
        return MtProxyErrorType.platform;
      default:
        return MtProxyErrorType.unknown;
    }
  }

  /// Проверка является ли ошибка критической
  static bool isCritical(int code) {
    // Критические ошибки: -1, -4, -6, -7
    return [-1, -4, -6, -7].contains(code);
  }

  /// Проверка можно ли восстановить после ошибки
  static bool isRecoverable(int code) {
    // Восстановимые ошибки: -2, -3, -5, -8, -9, -10
    return [-2, -3, -5, -8, -9, -10].contains(code);
  }
}

/// Менеджер ошибок MTProxy
class MtProxyErrorManager {
  final List<MtProxyException> _errorHistory = [];
  final int _maxHistorySize;

  MtProxyErrorManager({int maxHistorySize = 100})
      : _maxHistorySize = maxHistorySize;

  /// История ошибок
  List<MtProxyException> get errorHistory =>
      List.unmodifiable(_errorHistory);

  /// Последняя ошибка
  MtProxyException? get lastError =>
      _errorHistory.isNotEmpty ? _errorHistory.last : null;

  /// Количество ошибок
  int get errorCount => _errorHistory.length;

  /// Добавить ошибку в историю
  void addError(MtProxyException error) {
    _errorHistory.add(error);
    
    // Ограничение размера истории
    if (_errorHistory.length > _maxHistorySize) {
      _errorHistory.removeAt(0);
    }
  }

  /// Записать ошибку из C-кода
  void recordNativeError(int code, {String? source}) {
    final error = MtProxyException(
      MtProxyErrorInterpreter.interpret(code),
      type: MtProxyErrorInterpreter.interpretType(code),
      errorCode: code,
      source: source,
    );
    addError(error);
  }

  /// Записать ошибку с сообщением
  void recordError(
    String message, {
    MtProxyErrorType type = MtProxyErrorType.unknown,
    int? code,
    String? source,
  }) {
    final error = MtProxyException(
      message,
      type: type,
      errorCode: code,
      source: source,
    );
    addError(error);
  }

  /// Очистить историю
  void clear() {
    _errorHistory.clear();
  }

  /// Получить ошибки по типу
  List<MtProxyException> getErrorsByType(MtProxyErrorType type) {
    return _errorHistory.where((e) => e.type == type).toList();
  }

  /// Получить критические ошибки
  List<MtProxyException> getCriticalErrors() {
    return _errorHistory
        .where((e) => e.errorCode != null && MtProxyErrorInterpreter.isCritical(e.errorCode!))
        .toList();
  }

  /// Получить статистику ошибок
  Map<String, int> getErrorStatistics() {
    final stats = <String, int>{};
    for (final error in _errorHistory) {
      final key = error.type.name;
      stats[key] = (stats[key] ?? 0) + 1;
    }
    return stats;
  }
}
