#!/usr/bin/env python3
"""
Tests for MTProxy Metrics Collector
Тесты для scripts/metrics_collector.py
"""

import unittest
import sys
import os
from unittest.mock import patch, MagicMock
from io import StringIO

# Add scripts directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))

from metrics_collector import MTProxyMetricsCollector


class TestMTProxyMetricsCollector(unittest.TestCase):
    """Тесты для MTProxyMetricsCollector"""

    def setUp(self):
        """Настройка тестов"""
        self.collector = MTProxyMetricsCollector(host="127.0.0.1", port=8888, timeout=5)

    def tearDown(self):
        """Очистка после тестов"""
        pass

    # ========================================================================
    # Initialization Tests
    # ========================================================================

    def test_init_default(self):
        """Тест инициализации с параметрами по умолчанию"""
        collector = MTProxyMetricsCollector()
        self.assertEqual(collector.host, "127.0.0.1")
        self.assertEqual(collector.port, 8888)
        self.assertEqual(collector.timeout, 5)
        self.assertEqual(collector.base_url, "http://127.0.0.1:8888")

    def test_init_custom(self):
        """Тест инициализации с кастомными параметрами"""
        collector = MTProxyMetricsCollector(host="192.168.1.1", port=9999, timeout=10)
        self.assertEqual(collector.host, "192.168.1.1")
        self.assertEqual(collector.port, 9999)
        self.assertEqual(collector.timeout, 10)
        self.assertEqual(collector.base_url, "http://192.168.1.1:9999")

    # ========================================================================
    # fetch_stats Tests
    # ========================================================================

    @patch('metrics_collector.urllib.request.urlopen')
    def test_fetch_stats_success(self, mock_urlopen):
        """Тест успешного получения статистики"""
        mock_response = MagicMock()
        mock_response.read.return_value = b'{"connections": 100, "uptime": 3600}'
        mock_urlopen.return_value.__enter__.return_value = mock_response

        stats = self.collector.fetch_stats()

        self.assertIsNotNone(stats)
        self.assertEqual(stats["connections"], 100)
        self.assertEqual(stats["uptime"], 3600)

    @patch('metrics_collector.urllib.request.urlopen')
    def test_fetch_stats_error(self, mock_urlopen):
        """Тест ошибки получения статистики"""
        import urllib.error
        mock_urlopen.side_effect = urllib.error.URLError("Connection refused")

        stats = self.collector.fetch_stats()

        self.assertIsNone(stats)

    @patch('metrics_collector.urllib.request.urlopen')
    def test_fetch_stats_timeout(self, mock_urlopen):
        """Тест таймаута получения статистики"""
        import socket
        mock_urlopen.side_effect = socket.timeout("Timeout")

        stats = self.collector.fetch_stats()

        self.assertIsNone(stats)

    # ========================================================================
    # fetch_metrics Tests
    # ========================================================================

    @patch('metrics_collector.urllib.request.urlopen')
    def test_fetch_metrics_success(self, mock_urlopen):
        """Тест успешного получения метрик"""
        mock_response = MagicMock()
        mock_response.read.return_value = b'mtproxy_connections 100\nmtproxy_uptime 3600'
        mock_urlopen.return_value.__enter__.return_value = mock_response

        metrics = self.collector.fetch_metrics()

        self.assertIsNotNone(metrics)
        self.assertEqual(metrics["mtproxy_connections"], 100.0)
        self.assertEqual(metrics["mtproxy_uptime"], 3600.0)

    @patch('metrics_collector.urllib.request.urlopen')
    def test_fetch_metrics_error(self, mock_urlopen):
        """Тест ошибки получения метрик"""
        import urllib.error
        mock_urlopen.side_effect = urllib.error.URLError("Connection refused")

        metrics = self.collector.fetch_metrics()

        self.assertIsNone(metrics)

    # ========================================================================
    # _parse_prometheus_format Tests
    # ========================================================================

    def test_parse_prometheus_format_basic(self):
        """Тест парсинга Prometheus формата (базовый)"""
        data = "mtproxy_connections 100\nmtproxy_uptime 3600"
        metrics = self.collector._parse_prometheus_format(data)

        self.assertEqual(metrics["mtproxy_connections"], 100.0)
        self.assertEqual(metrics["mtproxy_uptime"], 3600.0)

    def test_parse_prometheus_format_with_comments(self):
        """Тест парсинга Prometheus формата (с комментариями)"""
        data = "# HELP mtproxy_connections Total connections\n# TYPE mtproxy_connections gauge\nmtproxy_connections 100"
        metrics = self.collector._parse_prometheus_format(data)

        self.assertEqual(metrics["mtproxy_connections"], 100.0)

    def test_parse_prometheus_format_empty(self):
        """Тест парсинга Prometheus формата (пустые данные)"""
        data = ""
        metrics = self.collector._parse_prometheus_format(data)

        self.assertEqual(len(metrics), 0)

    def test_parse_prometheus_format_float(self):
        """Тест парсинга Prometheus формата (float значения)"""
        data = "mtproxy_cpu_usage 45.5\nmtproxy_memory_mb 1024.75"
        metrics = self.collector._parse_prometheus_format(data)

        self.assertEqual(metrics["mtproxy_cpu_usage"], 45.5)
        self.assertEqual(metrics["mtproxy_memory_mb"], 1024.75)

    # ========================================================================
    # check_health Tests
    # ========================================================================

    @patch('metrics_collector.urllib.request.urlopen')
    def test_check_health_success(self, mock_urlopen):
        """Тест успешной проверки здоровья"""
        mock_response = MagicMock()
        mock_response.status = 200
        mock_urlopen.return_value.__enter__.return_value = mock_response

        result = self.collector.check_health()

        self.assertTrue(result)

    @patch('metrics_collector.urllib.request.urlopen')
    def test_check_health_failure(self, mock_urlopen):
        """Тест неуспешной проверки здоровья"""
        import urllib.error
        mock_urlopen.side_effect = urllib.error.URLError("Connection refused")

        result = self.collector.check_health()

        self.assertFalse(result)

    # ========================================================================
    # get_connections Tests
    # ========================================================================

    @patch('metrics_collector.urllib.request.urlopen')
    def test_get_connections_success(self, mock_urlopen):
        """Тест успешного получения соединений"""
        mock_response = MagicMock()
        mock_response.read.return_value = b'{"active": 50, "total": 100}'
        mock_urlopen.return_value.__enter__.return_value = mock_response

        connections = self.collector.get_connections()

        self.assertIsNotNone(connections)
        self.assertEqual(connections["active"], 50)
        self.assertEqual(connections["total"], 100)

    @patch('metrics_collector.urllib.request.urlopen')
    def test_get_connections_error(self, mock_urlopen):
        """Тест ошибки получения соединений"""
        import urllib.error
        mock_urlopen.side_effect = urllib.error.URLError("Connection refused")

        connections = self.collector.get_connections()

        self.assertIsNone(connections)

    # ========================================================================
    # export_prometheus Tests
    # ========================================================================

    @patch.object(MTProxyMetricsCollector, 'fetch_metrics')
    def test_export_prometheus_success(self, mock_fetch):
        """Тест успешного экспорта Prometheus"""
        mock_fetch.return_value = {"mtproxy_connections": 100, "mtproxy_uptime": 3600}

        output = self.collector.export_prometheus()

        self.assertIn("mtproxy_up 1", output)
        self.assertIn("mtproxy_connections 100", output)
        self.assertIn("mtproxy_uptime 3600", output)

    @patch.object(MTProxyMetricsCollector, 'fetch_metrics')
    def test_export_prometheus_empty(self, mock_fetch):
        """Тест экспорта Prometheus (пустые данные)"""
        mock_fetch.return_value = None

        output = self.collector.export_prometheus()

        self.assertEqual(output, "# No metrics available\n")

    # ========================================================================
    # export_json Tests
    # ========================================================================

    @patch.object(MTProxyMetricsCollector, 'fetch_metrics')
    def test_export_json_success(self, mock_fetch):
        """Тест успешного экспорта JSON"""
        mock_fetch.return_value = {"mtproxy_connections": 100}

        output = self.collector.export_json()

        self.assertIn("mtproxy_connections", output)
        self.assertIn("100", output)
        self.assertIn("timestamp", output)

    # ========================================================================
    # Integration Tests
    # ========================================================================

    def test_collector_workflow(self):
        """Тест полного рабочего процесса коллектора"""
        # Проверка инициализации
        self.assertIsNotNone(self.collector.base_url)

        # Проверка доступности методов
        self.assertTrue(hasattr(self.collector, 'fetch_stats'))
        self.assertTrue(hasattr(self.collector, 'fetch_metrics'))
        self.assertTrue(hasattr(self.collector, 'check_health'))
        self.assertTrue(hasattr(self.collector, 'get_connections'))
        self.assertTrue(hasattr(self.collector, 'export_prometheus'))
        self.assertTrue(hasattr(self.collector, 'export_json'))


class TestMetricsCollectorCLI(unittest.TestCase):
    """Тесты CLI интерфейса metrics_collector"""

    def setUp(self):
        """Настройка тестов"""
        self.held_output = StringIO()
        self.original_stdout = sys.stdout
        sys.stdout = self.held_output

    def tearDown(self):
        """Очистка после тестов"""
        sys.stdout = self.original_stdout
        self.held_output.close()

    @patch('metrics_collector.MTProxyMetricsCollector')
    def test_cli_metrics_command(self, mock_collector_class):
        """Тест CLI команды metrics"""
        mock_collector = MagicMock()
        mock_collector.fetch_metrics.return_value = {"test": 123}
        mock_collector_class.return_value = mock_collector

        # Импортируем main после мока
        from metrics_collector import main

        with patch('sys.argv', ['metrics_collector.py', 'metrics']):
            try:
                main()
            except SystemExit:
                pass

    @patch('metrics_collector.MTProxyMetricsCollector')
    def test_cli_health_command(self, mock_collector_class):
        """Тест CLI команды health"""
        mock_collector = MagicMock()
        mock_collector.check_health.return_value = True
        mock_collector_class.return_value = mock_collector

        from metrics_collector import main

        with patch('sys.argv', ['metrics_collector.py', 'health']):
            try:
                main()
            except SystemExit:
                pass

    @patch('metrics_collector.MTProxyMetricsCollector')
    def test_cli_export_command(self, mock_collector_class):
        """Тест CLI команды export"""
        mock_collector = MagicMock()
        mock_collector.export_prometheus.return_value = "# test metrics"
        mock_collector_class.return_value = mock_collector

        from metrics_collector import main

        with patch('sys.argv', ['metrics_collector.py', 'export', '--format', 'prometheus']):
            try:
                main()
            except SystemExit:
                pass


if __name__ == '__main__':
    print("=== MTProxy Metrics Collector Tests ===\n")

    # Запуск тестов
    unittest.main(verbosity=2, exit=False, buffer=True)

    # Summary
    print("\n=== Test Summary ===")
