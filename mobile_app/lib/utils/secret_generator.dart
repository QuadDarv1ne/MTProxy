import 'dart:math';
import 'dart:typed_data';

/// Генератор безопасных случайных значений
class SecretGenerator {
  static final Random _random = Random.secure();

  /// Генерация случайного hex-секрета (32 байта = 64 hex символа)
  static String generateHexSecret() {
    final bytes = Uint8List(32);
    for (var i = 0; i < bytes.length; i++) {
      bytes[i] = _random.nextInt(256);
    }
    return _bytesToHex(bytes);
  }

  /// Генерация нескольких секретов
  static List<String> generateSecrets(int count) {
    return List.generate(count, (_) => generateHexSecret());
  }

  /// Генерация секрета с заданной длиной (в байтах)
  static String generateSecretWithLength(int byteLength) {
    final bytes = Uint8List(byteLength);
    for (var i = 0; i < bytes.length; i++) {
      bytes[i] = _random.nextInt(256);
    }
    return _bytesToHex(bytes);
  }

  /// Преобразование байтов в hex-строку
  static String _bytesToHex(Uint8List bytes) {
    const hexChars = '0123456789abcdef';
    final buffer = StringBuffer();
    for (final byte in bytes) {
      buffer.write(hexChars[byte >> 4]);
      buffer.write(hexChars[byte & 0x0F]);
    }
    return buffer.toString();
  }

  /// Генерация случайного числа в диапазоне
  static int randomInt(int min, int max) {
    if (min > max) {
      throw ArgumentError('min должно быть меньше или равно max');
    }
    return min + _random.nextInt(max - min + 1);
  }

  /// Генерация случайного порта (исключая привилегированные)
  static int randomPort() {
    // Выбираем из популярных безопасных портов
    final safePorts = [
      443, 8443, 8080, 8000, 9000, 9443, 10000, 10443,
      11000, 11443, 12000, 12443, 13000, 13443, 14000,
    ];
    return safePorts[_random.nextInt(safePorts.length)];
  }

  /// Генерация случайного ID
  static String generateId({int length = 16}) {
    const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
    return String.fromCharCodes(
      Iterable.generate(
        length,
        (_) => chars.codeUnitAt(_random.nextInt(chars.length)),
      ),
    );
  }

  /// Генерация безопасного пароля
  static String generatePassword({
    int length = 32,
    bool includeUppercase = true,
    bool includeLowercase = true,
    bool includeNumbers = true,
    bool includeSymbols = true,
  }) {
    var chars = '';
    if (includeLowercase) chars += 'abcdefghijklmnopqrstuvwxyz';
    if (includeUppercase) chars += 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    if (includeNumbers) chars += '0123456789';
    if (includeSymbols) chars += '!@#\$%^&*()_+-=[]{}|;:,.<>?';

    if (chars.isEmpty) {
      throw ArgumentError('Должен быть выбран хотя бы один тип символов');
    }

    return String.fromCharCodes(
      Iterable.generate(
        length,
        (_) => chars.codeUnitAt(_random.nextInt(chars.length)),
      ),
    );
  }

  /// Перемешивание списка
  static List<T> shuffleList<T>(List<T> list) {
    final shuffled = List<T>.from(list);
    for (var i = shuffled.length - 1; i > 0; i--) {
      final j = _random.nextInt(i + 1);
      final temp = shuffled[i];
      shuffled[i] = shuffled[j];
      shuffled[j] = temp;
    }
    return shuffled;
  }

  /// Случайный элемент из списка
  static T randomElement<T>(List<T> list) {
    if (list.isEmpty) {
      throw StateError('Список не может быть пустым');
    }
    return list[_random.nextInt(list.length)];
  }

  /// Случайный boolean с заданной вероятностью
  static bool randomBool({double probability = 0.5}) {
    if (probability < 0 || probability > 1) {
      throw ArgumentError('Вероятность должна быть от 0 до 1');
    }
    return _random.nextDouble() < probability;
  }
}
