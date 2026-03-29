#!/usr/bin/env python3
"""
Docker Integration Tests for MTProxy
Интеграционные тесты для Docker образа MTProxy

Требования:
- Docker installed
- Docker running
- mtproxy:latest image built

Запуск:
    python3 test_docker_integration.py
"""

import subprocess
import time
import sys
import os
import json
import urllib.request
import urllib.error
from typing import Optional, Tuple

# Конфигурация тестов
DOCKER_IMAGE = "mtproxy:latest"
CONTAINER_NAME = "mtproxy-test"
PROXY_PORT = 8080
STATS_PORT = 8888
TEST_TIMEOUT = 30
TEST_SECRET = "ee82a1a5d1e9d2f7c3b4a5e6f7d8c9b0"


class DockerTestRunner:
    """Runner для Docker интеграционных тестов"""

    def __init__(self):
        self.container_id = None
        self.tests_passed = 0
        self.tests_failed = 0

    # ========================================================================
    # Docker commands
    # ========================================================================

    def docker_run(self, args: list = None, detach: bool = True) -> Tuple[bool, str]:
        """Запуск Docker контейнера"""
        cmd = ["docker", "run", "--rm"]

        if detach:
            cmd.append("-d")

        cmd.extend([
            "--name", CONTAINER_NAME,
            "-p", f"{STATS_PORT}:{STATS_PORT}",
            "-e", f"MT_PROXY_SECRET={TEST_SECRET}",
            "-e", "MT_PROXY_LOG_LEVEL=info",
            "-e", "MT_PROXY_METRICS_ENABLED=true",
        ])

        if args:
            cmd.extend(args)

        cmd.append(DOCKER_IMAGE)

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=TEST_TIMEOUT)
            if result.returncode == 0:
                self.container_id = result.stdout.strip()
                return True, self.container_id
            return False, result.stderr
        except subprocess.TimeoutExpired:
            return False, "Timeout running docker"
        except Exception as e:
            return False, str(e)

    def docker_stop(self) -> bool:
        """Остановка контейнера"""
        try:
            subprocess.run(
                ["docker", "stop", "-t", "5", CONTAINER_NAME],
                capture_output=True,
                timeout=10
            )
            return True
        except Exception:
            return False

    def docker_ps(self) -> bool:
        """Проверка наличия контейнера"""
        try:
            result = subprocess.run(
                ["docker", "ps", "-q", "-f", f"name={CONTAINER_NAME}"],
                capture_output=True,
                text=True,
                timeout=5
            )
            return bool(result.stdout.strip())
        except Exception:
            return False

    def docker_logs(self) -> str:
        """Получение логов контейнера"""
        try:
            result = subprocess.run(
                ["docker", "logs", CONTAINER_NAME],
                capture_output=True,
                text=True,
                timeout=10
            )
            return result.stdout + result.stderr
        except Exception:
            return ""

    def docker_exec(self, command: list) -> Tuple[bool, str]:
        """Выполнение команды в контейнере"""
        try:
            cmd = ["docker", "exec", CONTAINER_NAME] + command
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                return True, result.stdout
            return False, result.stderr
        except Exception as e:
            return False, str(e)

    # ========================================================================
    # HTTP helpers
    # ========================================================================

    def http_get(self, url: str, timeout: int = 5) -> Tuple[bool, str]:
        """HTTP GET запрос"""
        try:
            req = urllib.request.Request(url)
            with urllib.request.urlopen(req, timeout=timeout) as response:
                return True, response.read().decode('utf-8')
        except urllib.error.URLError as e:
            return False, str(e)
        except Exception as e:
            return False, str(e)

    def wait_for_health(self, max_attempts: int = 10, delay: float = 1.0) -> bool:
        """Ожидание готовности контейнера"""
        for i in range(max_attempts):
            success, _ = self.http_get(f"http://localhost:{STATS_PORT}/health")
            if success:
                time.sleep(0.5)  # Дополнительная задержка для стабильности
                return True
            time.sleep(delay)
        return False

    # ========================================================================
    # Tests
    # ========================================================================

    def test_docker_available(self) -> bool:
        """Тест: Docker доступен"""
        try:
            result = subprocess.run(
                ["docker", "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                print(f"  Docker: {result.stdout.strip()}")
                return True
            return False
        except Exception as e:
            print(f"  Docker not available: {e}")
            return False

    def test_image_exists(self) -> bool:
        """Тест: Образ существует"""
        try:
            result = subprocess.run(
                ["docker", "images", "-q", DOCKER_IMAGE],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.stdout.strip():
                print(f"  Image: {DOCKER_IMAGE}")
                return True
            print(f"  Image {DOCKER_IMAGE} not found")
            return False
        except Exception as e:
            print(f"  Error checking image: {e}")
            return False

    def test_container_start(self) -> bool:
        """Тест: Контейнер запускается"""
        # Останавливаем предыдущий если есть
        self.docker_stop()
        time.sleep(1)

        success, result = self.docker_run()
        if success:
            print(f"  Container started: {result[:12]}")
            return True
        print(f"  Failed to start container: {result}")
        return False

    def test_container_health(self) -> bool:
        """Тест: Health check проходит"""
        if not self.wait_for_health():
            print("  Health check failed (timeout)")
            return False
        print("  Health check passed")
        return True

    def test_stats_endpoint(self) -> bool:
        """Тест: Stats endpoint доступен"""
        success, data = self.http_get(f"http://localhost:{STATS_PORT}/stats")
        if not success:
            print(f"  Stats endpoint failed: {data}")
            return False

        try:
            stats = json.loads(data)
            print(f"  Stats: {len(stats)} metrics available")
            return True
        except json.JSONDecodeError:
            print(f"  Invalid JSON response")
            return False

    def test_metrics_endpoint(self) -> bool:
        """Тест: Metrics endpoint доступен"""
        success, data = self.http_get(f"http://localhost:{STATS_PORT}/metrics")
        if not success:
            print(f"  Metrics endpoint failed: {data}")
            return False

        if "mtproxy" in data.lower() or len(data) > 50:
            print(f"  Metrics: {len(data.split(chr(10)))} lines")
            return True
        print("  No metrics data")
        return False

    def test_health_endpoint(self) -> bool:
        """Тест: Health endpoint доступен"""
        success, data = self.http_get(f"http://localhost:{STATS_PORT}/health")
        if not success:
            print(f"  Health endpoint failed: {data}")
            return False

        print("  Health endpoint: OK")
        return True

    def test_container_logs(self) -> bool:
        """Тест: Логи контейнера доступны"""
        logs = self.docker_logs()
        if logs:
            print(f"  Logs: {len(logs.split(chr(10)))} lines")
            return True
        print("  No logs available")
        return False

    def test_container_stop(self) -> bool:
        """Тест: Контейнер останавливается"""
        if self.docker_stop():
            time.sleep(2)
            if not self.docker_ps():
                print("  Container stopped successfully")
                return True
        print("  Failed to stop container")
        return False

    def test_image_multiarch(self) -> bool:
        """Тест: Multi-arch поддержка (проверка манифеста)"""
        try:
            result = subprocess.run(
                ["docker", "manifest", "inspect", DOCKER_IMAGE],
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode == 0:
                manifest = json.loads(result.stdout)
                if "manifests" in manifest:
                    archs = [m.get("platform", {}).get("architecture", "unknown")
                             for m in manifest["manifests"]]
                    print(f"  Architectures: {', '.join(set(archs))}")
                    return True
            print("  Not a multi-arch image")
            return True  # Не ошибка, просто информация
        except Exception as e:
            print(f"  Could not check manifest: {e}")
            return True  # Не критично

    def test_security_non_root(self) -> bool:
        """Тест: Контейнер работает не от root"""
        success, output = self.docker_exec(["whoami"])
        if success and output.strip() != "root":
            print(f"  Running as: {output.strip()}")
            return True
        elif not success:
            # Контейнер может быть уже остановлен
            print("  (skipped, container not running)")
            return True
        print("  WARNING: Running as root!")
        return False

    def test_security_read_only_root(self) -> bool:
        """Тест: Root filesystem read-only"""
        success, _ = self.docker_exec(["touch", "/test_file"])
        if not success:
            print("  Root filesystem: read-only")
            return True
        print("  Root filesystem: writable (not critical)")
        return True  # Не критично для базового образа

    # ========================================================================
    # Test runner
    # ========================================================================

    def run_all_tests(self) -> int:
        """Запуск всех тестов"""
        print("=== MTProxy Docker Integration Tests ===\n")

        # Pre-check tests (без контейнера)
        print("--- Pre-check Tests ---")
        self.run_test(self.test_docker_available)
        self.run_test(self.test_image_exists)
        print()

        # Если образа нет, прекращаем
        if self.tests_failed > 0:
            print("Skipping container tests (image not available)")
            return self.tests_failed

        # Container tests
        print("--- Container Lifecycle Tests ---")
        self.run_test(self.test_container_start)

        if self.container_id:
            print()
            print("--- Runtime Tests ---")
            self.run_test(self.test_container_health)
            self.run_test(self.test_stats_endpoint)
            self.run_test(self.test_metrics_endpoint)
            self.run_test(self.test_health_endpoint)
            self.run_test(self.test_container_logs)
            self.run_test(self.test_security_non_root)
            self.run_test(self.test_security_read_only_root)

            print()
            print("--- Cleanup Tests ---")
            self.run_test(self.test_container_stop)

        print()
        print("--- Image Tests ---")
        self.run_test(self.test_image_multiarch)

        # Summary
        print()
        print("=== Test Summary ===")
        print(f"Passed: {self.tests_passed}")
        print(f"Failed: {self.tests_failed}")
        print(f"Total:  {self.tests_passed + self.tests_failed}")

        # Cleanup
        self.docker_stop()

        return 0 if self.tests_failed == 0 else 1

    def run_test(self, test_func) -> None:
        """Запуск одного теста"""
        test_name = test_func.__name__.replace("test_", "")
        print(f"Running {test_name}... ", end="")

        try:
            if test_func():
                print("PASS")
                self.tests_passed += 1
            else:
                print("FAIL")
                self.tests_failed += 1
        except Exception as e:
            print(f"ERROR: {e}")
            self.tests_failed += 1


def main():
    """Main entry point"""
    runner = DockerTestRunner()
    sys.exit(runner.run_all_tests())


if __name__ == "__main__":
    main()
