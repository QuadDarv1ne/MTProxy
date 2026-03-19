/// Конфигурация MTProxy
class ProxyConfig {
  final int port;
  final List<String> secrets;
  final int maxConnections;
  final bool enableIpv6;
  final bool enableStats;

  const ProxyConfig({
    this.port = 443,
    this.secrets = const [],
    this.maxConnections = 10000,
    this.enableIpv6 = false,
    this.enableStats = true,
  });

  factory ProxyConfig.defaultConfig() {
    return const ProxyConfig(
      port: 443,
      secrets: [],
      maxConnections = 10000,
      enableIpv6: false,
      enableStats: true,
    );
  }

  ProxyConfig copyWith({
    int? port,
    List<String>? secrets,
    int? maxConnections,
    bool? enableIpv6,
    bool? enableStats,
  }) {
    return ProxyConfig(
      port: port ?? this.port,
      secrets: secrets ?? this.secrets,
      maxConnections: maxConnections ?? this.maxConnections,
      enableIpv6: enableIpv6 ?? this.enableIpv6,
      enableStats: enableStats ?? this.enableStats,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'port': port,
      'secrets': secrets,
      'max_connections': maxConnections,
      'enable_ipv6': enableIpv6,
      'enable_stats': enableStats,
    };
  }

  factory ProxyConfig.fromJson(Map<String, dynamic> json) {
    return ProxyConfig(
      port: json['port'] ?? 443,
      secrets: List<String>.from(json['secrets'] ?? []),
      maxConnections: json['max_connections'] ?? 10000,
      enableIpv6: json['enable_ipv6'] ?? false,
      enableStats: json['enable_stats'] ?? true,
    );
  }
}
