/// Утилиты для форматирования данных
class FormatUtils {
  /// Форматирование байтов в человекочитаемый вид
  static String formatBytes(int bytes) {
    if (bytes < 0) return '0 B';
    
    const suffixes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
    var i = 0;
    double size = bytes.toDouble();
    
    while (size >= 1024 && i < suffixes.length - 1) {
      size /= 1024;
      i++;
    }
    
    if (i == 0) {
      return '${size.toInt()} ${suffixes[i]}';
    } else {
      return '${size.toStringAsFixed(size < 10 ? 2 : 1)} ${suffixes[i]}';
    }
  }

  /// Форматирование длительности
  static String formatDuration(Duration duration) {
    final days = duration.inDays;
    final hours = duration.inHours.remainder(24);
    final minutes = duration.inMinutes.remainder(60);
    final seconds = duration.inSeconds.remainder(60);

    if (days > 0) {
      return '${days}д ${hours}ч ${minutes}м';
    } else if (hours > 0) {
      return '${hours}ч ${minutes}м ${seconds}с';
    } else if (minutes > 0) {
      return '${minutes}м ${seconds}с';
    } else {
      return '${seconds}с';
    }
  }

  /// Форматирование даты и времени
  static String formatDateTime(DateTime dateTime, {bool showSeconds = false}) {
    final buffer = StringBuffer();
    buffer.write('${dateTime.day.toString().padLeft(2, '0')}.');
    buffer.write('${dateTime.month.toString().padLeft(2, '0')}.');
    buffer.write('${dateTime.year} ');
    buffer.write('${dateTime.hour.toString().padLeft(2, '0')}:');
    buffer.write('${dateTime.minute.toString().padLeft(2, '0')}');
    if (showSeconds) {
      buffer.write(':${dateTime.second.toString().padLeft(2, '0')}');
    }
    return buffer.toString();
  }

  /// Форматирование процента
  static String formatPercent(double percent, {int decimals = 1}) {
    return '${percent.toStringAsFixed(decimals)}%';
  }

  /// Форматирование числа с разделителями тысяч
  static String formatNumber(int number) {
    return number.toString().replaceAllMapped(
      RegExp(r'(\d{1,3})(?=(\d{3})+(?!\d))'),
      (Match m) => '${m[1]} ',
    );
  }

  /// Форматирование скорости (байт/сек)
  static String formatSpeed(int bytesPerSecond) {
    return '${formatBytes(bytesPerSecond)}/с';
  }

  /// Форматирование hex-строки (с сокращением)
  static String formatHexSecret(String secret, {int visibleChars = 8}) {
    if (secret.length <= visibleChars * 2) {
      return secret;
    }
    return '${secret.substring(0, visibleChars)}...${secret.substring(secret.length - visibleChars)}';
  }

  /// Маскировка чувствительных данных
  static String maskSecret(String secret, {int visibleChars = 4}) {
    if (secret.length <= visibleChars * 2) {
      return '*' * secret.length;
    }
    return '${secret.substring(0, visibleChars)}${'*' * (secret.length - visibleChars * 2)}${secret.substring(secret.length - visibleChars)}';
  }

  /// Форматирование IP-адреса
  static String formatIpAddress(String? address) {
    if (address == null || address.isEmpty) {
      return '—';
    }
    return address;
  }

  /// Форматирование статуса
  static String formatStatus(String status, {Map<String, String>? customNames}) {
    if (customNames != null && customNames.containsKey(status)) {
      return customNames[status]!;
    }
    
    // Стандартные статусы
    const statusNames = {
      'running': 'Работает',
      'stopped': 'Остановлен',
      'starting': 'Запуск...',
      'stopping': 'Остановка...',
      'error': 'Ошибка',
      'initializing': 'Инициализация...',
    };
    
    return statusNames[status.toLowerCase()] ?? status;
  }

  /// Получение цвета для статуса
  static int getStatusColor(String status) {
    const colors = {
      'running': 0xFF4CAF50,      // Green
      'stopped': 0xFF9E9E9E,      // Grey
      'starting': 0xFFFFC107,     // Amber
      'stopping': 0xFFFFC107,     // Amber
      'error': 0xFFF44336,        // Red
      'initializing': 0xFF2196F3, // Blue
    };
    
    return colors[status.toLowerCase()] ?? 0xFF9E9E9E;
  }
}
