#!/usr/bin/env python3
"""
MTProxy Prometheus Exporter
Экспорт метрик MTProxy для Prometheus

Использование:
    python prometheus_exporter.py --host localhost --port 8888 --api-key your-api-key

Метрики:
    mtproxy_active_connections - Активные подключения
    mtproxy_total_connections_total - Всего подключений (counter)
    mtproxy_bytes_sent_total - Отправлено байт (counter)
    mtproxy_bytes_received_total - Получено байт (counter)
    mtproxy_cpu_usage_percent - Использование CPU
    mtproxy_memory_usage_bytes - Использование памяти
    mtproxy_errors_total - Всего ошибок (counter)
    mtproxy_rate_limited_total - Rate limited запросов (counter)
    mtproxy_uptime_seconds - Время работы
    mtproxy_cache_* - Метрики кэширования
    mtproxy_ratelimit_* - Метрики ограничения скорости
    mtproxy_connections_by_status - Подключения по статусам
"""

import argparse
import logging
import time
import sys
from typing import Optional, Dict, Any

import requests
from prometheus_client import (
    Counter,
    Gauge,
    Info,
    CollectorRegistry,
    generate_latest,
    CONTENT_TYPE_LATEST,
    start_http_server,
)

# Логирование
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('mtproxy-exporter')


class MTProxyMetricsCollector:
    """Сборщик метрик MTProxy"""

    def __init__(self, host: str, port: int, api_key: str, https: bool = False):
        self.host = host
        self.port = port
        self.api_key = api_key
        self.protocol = 'https' if https else 'http'
        self.base_url = f"{self.protocol}://{host}:{port}/api/v1"

        self.headers = {'X-API-Key': self.api_key}
        self.session = requests.Session()
        self.session.headers.update(self.headers)

        # Регистрация метрик Prometheus
        self.registry = CollectorRegistry()

        # Info
        self.info = Info('mtproxy', 'MTProxy Server Info', registry=self.registry)

        # Gauges
        self.active_connections = Gauge(
            'mtproxy_active_connections',
            'Number of active connections',
            registry=self.registry
        )
        self.cpu_usage = Gauge(
            'mtproxy_cpu_usage_percent',
            'CPU usage percentage',
            registry=self.registry
        )
        self.memory_usage = Gauge(
            'mtproxy_memory_usage_bytes',
            'Memory usage in bytes',
            registry=self.registry
        )
        self.uptime = Gauge(
            'mtproxy_uptime_seconds',
            'Server uptime in seconds',
            registry=self.registry
        )

        # Counters
        self.total_connections = Counter(
            'mtproxy_total_connections_total',
            'Total number of connections',
            registry=self.registry
        )
        self.bytes_sent = Counter(
            'mtproxy_bytes_sent_total',
            'Total bytes sent',
            registry=self.registry
        )
        self.bytes_received = Counter(
            'mtproxy_bytes_received_total',
            'Total bytes received',
            registry=self.registry
        )
        self.errors_total = Counter(
            'mtproxy_errors_total',
            'Total number of errors',
            registry=self.registry
        )
        self.rate_limited = Counter(
            'mtproxy_rate_limited_total',
            'Total rate limited requests',
            registry=self.registry
        )

        # Speed gauges
        self.connections_per_second = Gauge(
            'mtproxy_connections_per_second',
            'Connections per second',
            registry=self.registry
        )
        self.bytes_per_second_in = Gauge(
            'mtproxy_bytes_per_second_in',
            'Bytes received per second',
            registry=self.registry
        )
        self.bytes_per_second_out = Gauge(
            'mtproxy_bytes_per_second_out',
            'Bytes sent per second',
            registry=self.registry
        )
        self.errors_per_second = Gauge(
            'mtproxy_errors_per_second',
            'Errors per second',
            registry=self.registry
        )

        # Cache metrics
        self.cache_hits = Gauge(
            'mtproxy_cache_hits_total',
            'Total cache hits',
            registry=self.registry
        )
        self.cache_misses = Gauge(
            'mtproxy_cache_misses_total',
            'Total cache misses',
            registry=self.registry
        )
        self.cache_hit_rate = Gauge(
            'mtproxy_cache_hit_rate',
            'Cache hit rate percentage',
            registry=self.registry
        )
        self.cache_entries = Gauge(
            'mtproxy_cache_entries',
            'Number of entries in cache',
            registry=self.registry
        )
        self.cache_memory_bytes = Gauge(
            'mtproxy_cache_memory_bytes',
            'Cache memory usage in bytes',
            registry=self.registry
        )
        self.cache_evictions = Gauge(
            'mtproxy_cache_evictions_total',
            'Total cache evictions',
            registry=self.registry
        )

        # Rate limiting metrics
        self.ratelimit_active_rules = Gauge(
            'mtproxy_ratelimit_active_rules',
            'Number of active rate limit rules',
            registry=self.registry
        )
        self.ratelimit_blocked_ips = Gauge(
            'mtproxy_ratelimit_blocked_ips',
            'Number of blocked IPs',
            registry=self.registry
        )
        self.ratelimit_whitelisted_ips = Gauge(
            'mtproxy_ratelimit_whitelisted_ips',
            'Number of whitelisted IPs',
            registry=self.registry
        )

        # Connection status metrics
        self.connections_by_status = Gauge(
            'mtproxy_connections_by_status',
            'Connections by status',
            ['status'],
            registry=self.registry
        )

    def get_statistics(self) -> Optional[dict]:
        """Получить статистику от MTProxy API"""
        try:
            response = self.session.get(
                f"{self.base_url}/statistics",
                timeout=5
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            logger.error(f"Failed to get statistics: {e}")
            return None

    def get_server_status(self) -> Optional[dict]:
        """Получить статус сервера"""
        try:
            response = self.session.get(
                f"{self.base_url}/server/status",
                timeout=5
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            logger.error(f"Failed to get server status: {e}")
            return None

    def collect(self):
        """Собрать и обновить метрики"""
        # Статистика
        stats = self.get_statistics()
        if stats:
            self.active_connections.set(stats.get('active_connections', 0))
            self.cpu_usage.set(stats.get('cpu_usage_percent', 0))
            self.memory_usage.set(stats.get('memory_usage_bytes', 0))
            self.uptime.set(stats.get('uptime_seconds', 0))

            # Counters (используем _total для Prometheus)
            self.total_connections._value.set(stats.get('total_connections', 0))
            self.bytes_sent._value.set(stats.get('bytes_sent', 0))
            self.bytes_received._value.set(stats.get('bytes_received', 0))
            self.errors_total._value.set(stats.get('total_errors', 0))
            self.rate_limited._value.set(stats.get('rate_limited_requests', 0))

            # Speed metrics
            self.connections_per_second.set(stats.get('connections_per_second', 0))
            self.bytes_per_second_in.set(stats.get('bytes_per_second_in', 0))
            self.bytes_per_second_out.set(stats.get('bytes_per_second_out', 0))
            self.errors_per_second.set(stats.get('errors_per_second', 0))

            # Cache metrics
            cache_stats = stats.get('cache', {})
            if cache_stats:
                self.cache_hits.set(cache_stats.get('hits', 0))
                self.cache_misses.set(cache_stats.get('misses', 0))
                self.cache_hit_rate.set(cache_stats.get('hit_rate', 0) * 100)
                self.cache_entries.set(cache_stats.get('entries', 0))
                self.cache_memory_bytes.set(cache_stats.get('memory_bytes', 0))
                self.cache_evictions.set(cache_stats.get('evictions', 0))

            # Rate limiting metrics
            ratelimit_stats = stats.get('ratelimit', {})
            if ratelimit_stats:
                self.ratelimit_active_rules.set(ratelimit_stats.get('active_rules', 0))
                self.ratelimit_blocked_ips.set(ratelimit_stats.get('blocked_ips', 0))
                self.ratelimit_whitelisted_ips.set(ratelimit_stats.get('whitelisted_ips', 0))

            # Connection status metrics
            conn_by_status = stats.get('connections_by_status', {})
            for status, count in conn_by_status.items():
                self.connections_by_status.labels(status=status).set(count)

        # Статус сервера
        status = self.get_server_status()
        if status:
            self.info.info({
                'version': status.get('version', 'unknown'),
                'commit_hash': status.get('commit_hash', 'unknown'),
                'state': status.get('state', 'unknown'),
                'platform_os': status.get('platform', {}).get('os', 'unknown'),
                'platform_arch': status.get('platform', {}).get('arch', 'unknown'),
            })


def main():
    parser = argparse.ArgumentParser(description='MTProxy Prometheus Exporter')
    parser.add_argument('--host', default='localhost', help='MTProxy API host')
    parser.add_argument('--port', type=int, default=8888, help='MTProxy API port')
    parser.add_argument('--api-key', required=True, help='MTProxy API key')
    parser.add_argument('--https', action='store_true', help='Use HTTPS')
    parser.add_argument('--exporter-port', type=int, default=9090,
                        help='Prometheus exporter port')
    parser.add_argument('--interval', type=int, default=5,
                        help='Collection interval in seconds')
    parser.add_argument('--log-level', default='info',
                        choices=['debug', 'info', 'warning', 'error'],
                        help='Log level')

    args = parser.parse_args()

    # Настройка логирования
    log_level = getattr(logging, args.log_level.upper())
    logging.getLogger().setLevel(log_level)

    logger.info(f"Starting MTProxy Prometheus Exporter")
    logger.info(f"MTProxy API: {args.host}:{args.port}")
    logger.info(f"Exporter port: {args.exporter_port}")
    logger.info(f"Collection interval: {args.interval}s")

    # Создание сборщика
    collector = MTProxyMetricsCollector(
        host=args.host,
        port=args.port,
        api_key=args.api_key,
        https=args.https
    )

    # Запуск HTTP сервера для Prometheus
    start_http_server(args.exporter_port, registry=collector.registry)
    logger.info(f"Exporter listening on port {args.exporter_port}")

    # Сбор метрик
    try:
        while True:
            collector.collect()
            time.sleep(args.interval)
    except KeyboardInterrupt:
        logger.info("Shutting down exporter")


if __name__ == '__main__':
    main()
