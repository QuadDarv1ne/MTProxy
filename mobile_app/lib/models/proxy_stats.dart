/// Статистика MTProxy
class ProxyStats {
  final int activeConnections;
  final int totalConnections;
  final int bytesSent;
  final int bytesReceived;
  final DateTime startTime;
  final double cpuUsage;
  final int memoryUsage;

  const ProxyStats({
    this.activeConnections = 0,
    this.totalConnections = 0,
    this.bytesSent = 0,
    this.bytesReceived = 0,
    required this.startTime,
    this.cpuUsage = 0.0,
    this.memoryUsage = 0,
  });

  factory ProxyStats.empty() {
    return ProxyStats(
      startTime: DateTime.now(),
    );
  }

  /// Форматированный трафик (отправлено)
  String get formattedSent {
    return _formatBytes(bytesSent);
  }

  /// Форматированный трафик (получено)
  String get formattedReceived {
    return _formatBytes(bytesReceived);
  }

  /// Форматированное использование памяти
  String get formattedMemory {
    if (memoryUsage < 1024) {
      return '$memoryUsage KB';
    } else if (memoryUsage < 1024 * 1024) {
      return '${(memoryUsage / 1024).toStringAsFixed(1)} MB';
    } else {
      return '${(memoryUsage / (1024 * 1024)).toStringAsFixed(2)} GB';
    }
  }

  /// Время работы
  Duration get uptime {
    return DateTime.now().difference(startTime);
  }

  /// Форматированное время работы
  String get formattedUptime {
    final days = uptime.inDays;
    final hours = uptime.inHours % 24;
    final minutes = uptime.inMinutes % 60;
    final seconds = uptime.inSeconds % 60;

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

  static String _formatBytes(int bytes) {
    if (bytes < 1024) {
      return '$bytes B';
    } else if (bytes < 1024 * 1024) {
      return '${(bytes / 1024).toStringAsFixed(1)} KB';
    } else if (bytes < 1024 * 1024 * 1024) {
      return '${(bytes / (1024 * 1024)).toStringAsFixed(1)} MB';
    } else {
      return '${(bytes / (1024 * 1024 * 1024)).toStringAsFixed(2)} GB';
    }
  }

  ProxyStats copyWith({
    int? activeConnections,
    int? totalConnections,
    int? bytesSent,
    int? bytesReceived,
    DateTime? startTime,
    double? cpuUsage,
    int? memoryUsage,
  }) {
    return ProxyStats(
      activeConnections: activeConnections ?? this.activeConnections,
      totalConnections: totalConnections ?? this.totalConnections,
      bytesSent: bytesSent ?? this.bytesSent,
      bytesReceived: bytesReceived ?? this.bytesReceived,
      startTime: startTime ?? this.startTime,
      cpuUsage: cpuUsage ?? this.cpuUsage,
      memoryUsage: memoryUsage ?? this.memoryUsage,
    );
  }
}
