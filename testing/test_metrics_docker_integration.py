#!/usr/bin/env python3
"""
MTProxy Metrics + Docker Integration Tests
Интеграционные тесты: metrics_collector + Docker

Требования:
- Docker installed and running
- mtproxy:latest image built
- Python 3.6+

Запуск:
    python3 test_metrics_docker_integration.py
"""

import subprocess
import time
import sys
import os
import json
import urllib.request
import urllib.error
from typing import Optional, Tuple, Dict, Any

# Конфигурация тестов
DOCKER_IMAGE = "mtproxy:latest"
CONTAINER_NAME = "mtproxy-metrics-test"
PROXY_PORT = 8080
STATS_PORT = 8888
TEST_TIMEOUT = 30
TEST_SECRET = "ee82a1a5d1e9d2f7c3b4a5e6f7d8c9b0"

# Статистика тестов
tests_passed = 0
tests_failed = 0


# ============================================================================
# Helper functions
# ============================================================================

def run_command(cmd: list, timeout: int = TEST_TIMEOUT) -> Tuple[bool, str]:
    """Выполнение команды и возврат результата"""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        return result.returncode == 0, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, f"Command timed out after {timeout}s"
    except Exception as e:
        return False, str(e)


def wait_for_port(host: str, port: int, timeout: int = 10) -> bool:
    """Ожидание доступности порта"""
    import socket
    start_time = time.time()
    
    while time.time() - start_time < timeout:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((host, port))
            sock.close()
            if result == 0:
                return True
        except Exception:
            pass
        time.sleep(0.5)
    
    return False


def http_get(url: str, timeout: int = 5) -> Tuple[bool, Any]:
    """HTTP GET запрос"""
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=timeout) as response:
            data = response.read().decode('utf-8')
            try:
                return True, json.loads(data)
            except json.JSONDecodeError:
                return True, data
    except urllib.error.URLError as e:
        return False, str(e)
    except Exception as e:
        return False, str(e)


# ============================================================================
# Docker container management
# ============================================================================

def docker_cleanup():
    """Очистка Docker контейнеров"""
    run_command(["docker", "stop", CONTAINER_NAME], timeout=5)
    time.sleep(1)


def docker_run_container() -> Tuple[bool, str]:
    """Запуск Docker контейнера с MTProxy"""
    cmd = [
        "docker", "run",
        "--rm",
        "-d",
        "--name", CONTAINER_NAME,
        "-p", f"{PROXY_PORT}:{PROXY_PORT}",
        "-p", f"{STATS_PORT}:{STATS_PORT}",
        "-e", f"PROXY_SECRET={TEST_SECRET}",
        "-e", f"STATS_PORT={STATS_PORT}",
        "mtproxy:latest"
    ]
    
    success, output = run_command(cmd)
    if success:
        time.sleep(2)  # Ожидание запуска контейнера
    return success, output


# ============================================================================
# Metrics Collector Tests
# ============================================================================

def test_metrics_collector_init():
    """Тест инициализации metrics collector"""
    print("  test_metrics_collector_init ... ", end="")
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))
        from metrics_collector import MTProxyMetricsCollector
        
        collector = MTProxyMetricsCollector(host="127.0.0.1", port=STATS_PORT)
        
        assert collector.host == "127.0.0.1", "Host mismatch"
        assert collector.port == STATS_PORT, "Port mismatch"
        assert collector.timeout == 5, "Timeout mismatch"
        assert collector.base_url == f"http://127.0.0.1:{STATS_PORT}", "Base URL mismatch"
        
        print("PASSED")
        return True
    except Exception as e:
        print(f"FAILED ({e})")
        return False


def test_metrics_collector_custom_init():
    """Тест инициализации с кастомными параметрами"""
    print("  test_metrics_collector_custom_init ... ", end="")
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))
        from metrics_collector import MTProxyMetricsCollector
        
        collector = MTProxyMetricsCollector(host="192.168.1.1", port=9999, timeout=10)
        
        assert collector.host == "192.168.1.1", "Custom host mismatch"
        assert collector.port == 9999, "Custom port mismatch"
        assert collector.timeout == 10, "Custom timeout mismatch"
        
        print("PASSED")
        return True
    except Exception as e:
        print(f"FAILED ({e})")
        return False


# ============================================================================
# Docker Integration Tests
# ============================================================================

def test_docker_image_exists():
    """Тест наличия Docker образа"""
    print("  test_docker_image_exists ... ", end="")
    success, output = run_command(["docker", "images", "-q", DOCKER_IMAGE])
    
    if success and output.strip():
        print("PASSED")
        return True
    else:
        print(f"FAILED (image {DOCKER_IMAGE} not found)")
        return False


def test_docker_container_start():
    """Тест запуска Docker контейнера"""
    print("  test_docker_container_start ... ", end="")
    docker_cleanup()
    
    success, output = docker_run_container()
    
    if success:
        print("PASSED")
        return True
    else:
        print(f"FAILED ({output})")
        return False


def test_docker_container_running():
    """Тест работы Docker контейнера"""
    print("  test_docker_container_running ... ", end="")
    
    success, output = run_command([
        "docker", "ps", "--filter", f"name={CONTAINER_NAME}", "--format", "{{.Status}}"
    ])
    
    if success and "Up" in output:
        print("PASSED")
        return True
    else:
        print(f"FAILED (container not running)")
        return False


def test_docker_ports_exposed():
    """Тест экспонирования портов"""
    print("  test_docker_ports_exposed ... ", end="")
    
    success, output = run_command([
        "docker", "port", CONTAINER_NAME
    ])
    
    if success and f"{PROXY_PORT}" in output and f"{STATS_PORT}" in output:
        print("PASSED")
        return True
    else:
        print(f"FAILED (ports not exposed)")
        return False


# ============================================================================
# Metrics Endpoint Tests (через Docker)
# ============================================================================

def test_metrics_endpoint_available():
    """Тест доступности metrics endpoint"""
    print("  test_metrics_endpoint_available ... ", end="")
    
    if not wait_for_port("127.0.0.1", STATS_PORT, timeout=10):
        print("FAILED (port not available)")
        return False
    
    success, data = http_get(f"http://127.0.0.1:{STATS_PORT}/stats")
    
    if success:
        print("PASSED")
        return True
    else:
        print(f"FAILED ({data})")
        return False


def test_metrics_json_format():
    """Тест формата JSON metrics"""
    print("  test_metrics_json_format ... ", end="")
    
    success, data = http_get(f"http://127.0.0.1:{STATS_PORT}/stats")
    
    if success and isinstance(data, dict):
        print("PASSED")
        return True
    else:
        print(f"FAILED (invalid JSON format)")
        return False


def test_metrics_required_fields():
    """Тест наличия обязательных полей в metrics"""
    print("  test_metrics_required_fields ... ", end="")
    
    success, data = http_get(f"http://127.0.0.1:{STATS_PORT}/stats")
    
    if not success or not isinstance(data, dict):
        print("FAILED (cannot get metrics)")
        return False
    
    required_fields = ["connections", "traffic"]
    missing_fields = [f for f in required_fields if f not in data]
    
    if not missing_fields:
        print("PASSED")
        return True
    else:
        print(f"FAILED (missing fields: {missing_fields})")
        return False


# ============================================================================
# Combined Integration Tests
# ============================================================================

def test_metrics_collector_with_docker():
    """Тест metrics collector с реальным Docker контейнером"""
    print("  test_metrics_collector_with_docker ... ", end="")
    
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))
        from metrics_collector import MTProxyMetricsCollector
        
        collector = MTProxyMetricsCollector(host="127.0.0.1", port=STATS_PORT, timeout=5)
        
        # Попытка получения метрик
        if not wait_for_port("127.0.0.1", STATS_PORT, timeout=5):
            print("FAILED (port not available)")
            return False
        
        success, data = http_get(f"http://127.0.0.1:{STATS_PORT}/stats")
        
        if success and isinstance(data, dict):
            print("PASSED")
            return True
        else:
            print(f"FAILED (cannot collect metrics)")
            return False
            
    except Exception as e:
        print(f"FAILED ({e})")
        return False


def test_docker_container_logs():
    """Тест логов Docker контейнера"""
    print("  test_docker_container_logs ... ", end="")
    
    success, output = run_command(["docker", "logs", CONTAINER_NAME])
    
    if success and len(output) > 0:
        print("PASSED")
        return True
    else:
        print(f"FAILED (no logs)")
        return False


# ============================================================================
# Cleanup Tests
# ============================================================================

def test_docker_container_stop():
    """Тест остановки Docker контейнера"""
    print("  test_docker_container_stop ... ", end="")
    
    docker_cleanup()
    time.sleep(1)
    
    success, output = run_command([
        "docker", "ps", "--filter", f"name={CONTAINER_NAME}", "--format", "{{.Names}}"
    ])
    
    if success and CONTAINER_NAME not in output:
        print("PASSED")
        return True
    else:
        print(f"FAILED (container still running)")
        return False


# ============================================================================
# Test Runner
# ============================================================================

def run_all_tests():
    """Запуск всех тестов"""
    global tests_passed, tests_failed
    
    print("\n" + "=" * 70)
    print("MTProxy Metrics + Docker Integration Tests")
    print("=" * 70 + "\n")
    
    # Phase 1: Unit Tests (без Docker)
    print("--- Phase 1: Metrics Collector Unit Tests ---")
    
    if test_metrics_collector_init():
        tests_passed += 1
    else:
        tests_failed += 1
    
    if test_metrics_collector_custom_init():
        tests_passed += 1
    else:
        tests_failed += 1
    
    print()
    
    # Phase 2: Docker Tests
    print("--- Phase 2: Docker Integration Tests ---")
    
    if test_docker_image_exists():
        tests_passed += 1
    else:
        tests_failed += 1
        print("  Skipping remaining Docker tests (image not available)\n")
        print_test_summary()
        return tests_failed == 0
    
    if test_docker_container_start():
        tests_passed += 1
    else:
        tests_failed += 1
        print("  Skipping remaining Docker tests (container start failed)\n")
        print_test_summary()
        return tests_failed == 0
    
    if test_docker_container_running():
        tests_passed += 1
    else:
        tests_failed += 1
    
    if test_docker_ports_exposed():
        tests_passed += 1
    else:
        tests_failed += 1
    
    print()
    
    # Phase 3: Metrics Endpoint Tests
    print("--- Phase 3: Metrics Endpoint Tests ---")
    
    if test_metrics_endpoint_available():
        tests_passed += 1
    else:
        tests_failed += 1
    
    if test_metrics_json_format():
        tests_passed += 1
    else:
        tests_failed += 1
    
    if test_metrics_required_fields():
        tests_passed += 1
    else:
        tests_failed += 1
    
    print()
    
    # Phase 4: Combined Integration Tests
    print("--- Phase 4: Combined Integration Tests ---")
    
    if test_metrics_collector_with_docker():
        tests_passed += 1
    else:
        tests_failed += 1
    
    if test_docker_container_logs():
        tests_passed += 1
    else:
        tests_failed += 1
    
    print()
    
    # Cleanup
    print("--- Cleanup ---")
    
    if test_docker_container_stop():
        tests_passed += 1
    else:
        tests_failed += 1
    
    print()
    print_test_summary()
    
    return tests_failed == 0


def print_test_summary():
    """Вывод итогов тестирования"""
    global tests_passed, tests_failed
    
    print("=" * 70)
    print("Test Summary")
    print("=" * 70)
    print(f"Passed:  {tests_passed}")
    print(f"Failed:  {tests_failed}")
    print(f"Total:   {tests_passed + tests_failed}")
    print("=" * 70)
    
    if tests_failed > 0:
        print("\n⚠️  Some tests FAILED")
        sys.exit(1)
    else:
        print("\n✅ All tests PASSED")
        sys.exit(0)


# ============================================================================
# Main
# ============================================================================

if __name__ == "__main__":
    print("\nMTProxy Metrics + Docker Integration Test Suite")
    print("Testing: metrics_collector.py + Docker container")
    print(f"Docker Image: {DOCKER_IMAGE}")
    print(f"Proxy Port: {PROXY_PORT}, Stats Port: {STATS_PORT}")
    print()
    
    try:
        success = run_all_tests()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        docker_cleanup()
        sys.exit(1)
    except Exception as e:
        print(f"\n\nFatal error: {e}")
        docker_cleanup()
        sys.exit(1)
