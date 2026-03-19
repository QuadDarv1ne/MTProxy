import 'package:flutter/services.dart';

/// Утилиты для валидации данных
class ValidationUtils {
  /// Валидация порта
  static String? validatePort(String? value) {
    if (value == null || value.isEmpty) {
      return 'Введите порт';
    }
    
    final port = int.tryParse(value);
    if (port == null) {
      return 'Порт должен быть числом';
    }
    
    if (port < 1 || port > 65535) {
      return 'Порт должен быть от 1 до 65535';
    }
    
    if (port < 1024 && port != 443 && port != 80) {
      return 'Порты < 1024 требуют root-прав';
    }
    
    return null;
  }

  /// Валидация hex-секрета
  static String? validateHexSecret(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите секрет';
    }
    
    if (value.length != 64) {
      return 'Секрет должен содержать 64 hex-символа (32 байта)';
    }
    
    if (!RegExp(r'^[0-9a-fA-F]+$').hasMatch(value)) {
      return 'Секрет должен содержать только hex-символы (0-9, a-f)';
    }
    
    return null;
  }

  /// Валидация IP-адреса (IPv4)
  static String? validateIpv4(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите IP-адрес';
    }
    
    final ipPattern = RegExp(
      r'^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}'
      r'(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$',
    );
    
    if (!ipPattern.hasMatch(value)) {
      return 'Неверный формат IPv4-адреса';
    }
    
    return null;
  }

  /// Валидация IP-адреса (IPv6)
  static String? validateIpv6(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите IPv6-адрес';
    }
    
    final ipv6Pattern = RegExp(
      r'^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^([0-9a-fA-F]{1,4}:){1,7}:$|'
      r'^([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}$|'
      r'^([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}$|'
      r'^([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}$|'
      r'^([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}$|'
      r'^([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}$|'
      r'^[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})$|'
      r'^:((:[0-9a-fA-F]{1,4}){1,7}|:)$',
    );
    
    if (!ipv6Pattern.hasMatch(value)) {
      return 'Неверный формат IPv6-адреса';
    }
    
    return null;
  }

  /// Валидация числа (диапазон)
  static String? validateNumber(
    String? value, {
    required int min,
    required int max,
    String fieldName = 'Значение',
  }) {
    if (value == null || value.isEmpty) {
      return 'Введите $fieldName';
    }
    
    final number = int.tryParse(value);
    if (number == null) {
      return '$fieldName должно быть числом';
    }
    
    if (number < min || number > max) {
      return '$fieldName должно быть от $min до $max';
    }
    
    return null;
  }

  /// Валидация email
  static String? validateEmail(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите email';
    }
    
    final emailPattern = RegExp(
      r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$',
    );
    
    if (!emailPattern.hasMatch(value)) {
      return 'Неверный формат email';
    }
    
    return null;
  }

  /// Валидация URL
  static String? validateUrl(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите URL';
    }
    
    final urlPattern = RegExp(
      r'^https?://[^\s/$.?#].[^\s]*$',
    );
    
    if (!urlPattern.hasMatch(value)) {
      return 'Неверный формат URL';
    }
    
    return null;
  }

  /// Валидация пути к файлу
  static String? validateFilePath(String? value, {bool allowEmpty = false}) {
    if (value == null || value.isEmpty) {
      if (allowEmpty) return null;
      return 'Введите путь к файлу';
    }
    
    // Базовая проверка на недопустимые символы
    final invalidChars = RegExp(r'[<>:"|?*]');
    if (invalidChars.hasMatch(value)) {
      return 'Путь содержит недопустимые символы';
    }
    
    return null;
  }

  /// Валидация длины строки
  static String? validateStringLength(
    String? value, {
    required int minLength,
    required int maxLength,
    String fieldName = 'Значение',
  }) {
    if (value == null) {
      return '$fieldName не может быть пустым';
    }
    
    if (value.length < minLength) {
      return '$fieldName должно быть не менее $minLength символов';
    }
    
    if (value.length > maxLength) {
      return '$fieldName должно быть не более $maxLength символов';
    }
    
    return null;
  }

  /// Валидация списка (не пустой)
  static String? validateListNotEmpty<T>(List<T>? value, String fieldName) {
    if (value == null || value.isEmpty) {
      return 'Добавьте хотя бы один элемент в поле "$fieldName"';
    }
    return null;
  }

  /// Комбинированная валидация
  static String? validateAll(
    String? value,
    List<String? Function(String?)> validators,
  ) {
    for (final validator in validators) {
      final error = validator(value);
      if (error != null) {
        return error;
      }
    }
    return null;
  }

  /// Копирование в буфер обмена с обратной связью
  static Future<void> copyToClipboard(
    String text,
    String successMessage,
  ) async {
    await Clipboard.setData(ClipboardData(text: text));
    
    // Виброотклик (если поддерживается)
    try {
      await HapticFeedback.lightImpact();
    } catch (_) {}
  }
}

/// Расширения для текстовых полей
extension ValidationExtensions on String {
  bool get isValidPort => ValidationUtils.validatePort(this) == null;
  bool get isValidHexSecret => ValidationUtils.validateHexSecret(this) == null;
  bool get isValidIpv4 => ValidationUtils.validateIpv4(this, allowEmpty: false) == null;
  bool get isValidIpv6 => ValidationUtils.validateIpv6(this, allowEmpty: false) == null;
  bool get isValidEmail => ValidationUtils.validateEmail(this, allowEmpty: false) == null;
  bool get isValidUrl => ValidationUtils.validateUrl(this, allowEmpty: false) == null;
}
