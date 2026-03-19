#!/usr/bin/env python3
"""
MTProxy Metrics Collector
Скрипт для сбора и экспорта метрик MTProxy
"""

import argparse
import json
import sys
import time
import urllib.request
import urllib.error
from datetime import datetime
from typing import Optional, Dict, Any


class MTProxyMetricsCollector:
    """Коллектор метрик MTProxy"""
    
    def __init__(self, host: str = "127.0.0.1", port: int = 8888, timeout: int = 5):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.base_url = f"http://{host}:{port}"
    
    def fetch_stats(self) -> Optional[Dict[str, Any]]:
        """Получение статистики из MTProxy"""
        try:
            url = f"{self.base_url}/stats"
            with urllib.request.urlopen(url, timeout=self.timeout) as response:
                data = response.read().decode('utf-8')
                return json.loads(data) if data.startswith('{') else {"raw": data}
        except urllib.error.URLError as e:
            print(f"Error fetching stats: {e}", file=sys.stderr)
            return None
        except json.JSONDecodeError:
            return {"raw": data}
    
    def fetch_metrics(self) -> Optional[Dict[str, Any]]:
        """Получение метрик"""
        try:
            url = f"{self.base_url}/metrics"
            with urllib.request.urlopen(url, timeout=self.timeout) as response:
                data = response.read().decode('utf-8')
                return self._parse_prometheus_format(data)
        except urllib.error.URLError as e:
            print(f"Error fetching metrics: {e}", file=sys.stderr)
            return None
    
    def _parse_prometheus_format(self, data: str) -> Dict[str, Any]:
        """Парсинг метрик в формате Prometheus"""
        metrics = {}
        for line in data.strip().split('\n'):
            if line and not line.startswith('#'):
                parts = line.split()
                if len(parts) >= 2:
                    metrics[parts[0]] = float(parts[1])
        return metrics
    
    def check_health(self) -> bool:
        """Проверка здоровья сервера"""
        try:
            url = f"{self.base_url}/health"
            with urllib.request.urlopen(url, timeout=self.timeout) as response:
                return response.status == 200
        except urllib.error.URLError:
            return False
    
    def get_connections(self) -> Optional[Dict[str, Any]]:
        """Получение информации о соединениях"""
        try:
            url = f"{self.base_url}/connections"
            with urllib.request.urlopen(url, timeout=self.timeout) as response:
                return json.loads(response.read().decode('utf-8'))
        except urllib.error.URLError as e:
            print(f"Error fetching connections: {e}", file=sys.stderr)
            return None
    
    def export_prometheus(self) -> str:
        """Экспорт метрик в формате Prometheus"""
        metrics = self.fetch_metrics()
        if not metrics:
            return "# No metrics available\n"
        
        output = []
        output.append("# HELP mtproxy_up MTProxy server status")
        output.append("# TYPE mtproxy_up gauge")
        output.append(f"mtproxy_up 1")
        output.append("")
        
        for name, value in metrics.items():
            output.append(f"# TYPE {name} gauge")
            output.append(f"{name} {value}")
        
        return '\n'.join(output)
    
    def export_json(self) -> str:
        """Экспорт метрик в формате JSON"""
        data = {
            "timestamp": datetime.utcnow().isoformat(),
            "host": self.host,
            "port": self.port,
            "metrics": self.fetch_metrics() or {},
            "stats": self.fetch_stats() or {}
        }
        return json.dumps(data, indent=2)


def cmd_status(args):
    """Показать статус сервера"""
    collector = MTProxyMetricsCollector(args.host, args.port)
    
    if collector.check_health():
        print("✓ Server is healthy")
        
        stats = collector.fetch_stats()
        if stats:
            print(f"\nStatistics:")
            for key, value in stats.items():
                print(f"  {key}: {value}")
    else:
        print("✗ Server is not responding")
        sys.exit(1)


def cmd_metrics(args):
    """Показать метрики"""
    collector = MTProxyMetricsCollector(args.host, args.port)
    
    if args.format == 'prometheus':
        print(collector.export_prometheus())
    elif args.format == 'json':
        print(collector.export_json())
    else:
        metrics = collector.fetch_metrics()
        if metrics:
            print("Metrics:")
            for name, value in metrics.items():
                print(f"  {name}: {value}")


def cmd_health(args):
    """Проверка здоровья"""
    collector = MTProxyMetricsCollector(args.host, args.port, timeout=args.timeout)
    
    if collector.check_health():
        print("OK")
        sys.exit(0)
    else:
        print("CRITICAL")
        sys.exit(2)


def cmd_watch(args):
    """Непрерывное наблюдение за метриками"""
    collector = MTProxyMetricsCollector(args.host, args.port)
    
    print(f"Watching metrics (interval: {args.interval}s)...")
    print("Press Ctrl+C to stop\n")
    
    try:
        while True:
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            if args.format == 'json':
                data = {
                    "timestamp": timestamp,
                    "metrics": collector.fetch_metrics() or {}
                }
                print(json.dumps(data))
            else:
                metrics = collector.fetch_metrics()
                if metrics:
                    print(f"[{timestamp}]")
                    for name, value in metrics.items():
                        print(f"  {name}: {value}")
            
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopped")


def cmd_export(args):
    """Экспорт метрик"""
    collector = MTProxyMetricsCollector(args.host, args.port)
    
    if args.format == 'prometheus':
        output = collector.export_prometheus()
    elif args.format == 'json':
        output = collector.export_json()
    else:
        output = collector.export_json()
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output)
        print(f"Metrics exported to {args.output}")
    else:
        print(output)


def main():
    parser = argparse.ArgumentParser(
        description='MTProxy Metrics Collector',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('--host', default='127.0.0.1',
                       help='MTProxy host (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=8888,
                       help='MTProxy stats port (default: 8888)')
    parser.add_argument('--timeout', type=int, default=5,
                       help='Request timeout in seconds (default: 5)')
    
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Command: status
    status_parser = subparsers.add_parser('status', help='Show server status')
    status_parser.set_defaults(func=cmd_status)
    
    # Command: metrics
    metrics_parser = subparsers.add_parser('metrics', help='Show metrics')
    metrics_parser.add_argument('--format', choices=['text', 'json', 'prometheus'],
                               default='text', help='Output format')
    metrics_parser.set_defaults(func=cmd_metrics)
    
    # Command: health
    health_parser = subparsers.add_parser('health', help='Health check')
    health_parser.set_defaults(func=cmd_health)
    
    # Command: watch
    watch_parser = subparsers.add_parser('watch', help='Watch metrics')
    watch_parser.add_argument('--interval', type=int, default=5,
                             help='Update interval in seconds')
    watch_parser.add_argument('--format', choices=['text', 'json'],
                             default='text', help='Output format')
    watch_parser.set_defaults(func=cmd_watch)
    
    # Command: export
    export_parser = subparsers.add_parser('export', help='Export metrics')
    export_parser.add_argument('--format', choices=['json', 'prometheus'],
                              default='json', help='Export format')
    export_parser.add_argument('--output', '-o', help='Output file')
    export_parser.set_defaults(func=cmd_export)
    
    args = parser.parse_args()
    
    if args.command:
        args.func(args)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == '__main__':
    main()
