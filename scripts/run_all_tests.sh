#!/bin/bash
# run_all_tests.sh - Скрипт для запуска всех тестов MTProxy

set -e

echo "========================================"
echo "  MTProxy - Запуск всех тестов"
echo "========================================"
echo ""

BUILD_DIR="${1:-build}"
BIN_DIR="$BUILD_DIR/bin"

# Проверка наличия директории
if [ ! -d "$BIN_DIR" ]; then
    echo "Ошибка: Директория $BIN_DIR не найдена"
    echo "Сначала выполните сборку: cmake --build $BUILD_DIR"
    exit 1
fi

cd "$BIN_DIR"

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Функция для запуска теста
run_test() {
    local test_name=$1
    local test_executable=$2

    echo "----------------------------------------"
    echo "Запуск: $test_name"
    echo "----------------------------------------"

    if [ -f "$test_executable" ]; then
        # Запуск теста с таймаутом
        if timeout 60 "$test_executable" 2>&1; then
            echo "✅ $test_name: PASSED"
            ((PASSED_TESTS++)) || true
        else
            echo "❌ $test_name: FAILED"
            ((FAILED_TESTS++)) || true
        fi
        ((TOTAL_TESTS++)) || true
    else
        echo "⚠️  $test_name: executable not found ($test_executable)"
    fi

    echo ""
}

# Функция для запуска Python теста
run_python_test() {
    local test_name=$1
    local test_script=$2

    echo "----------------------------------------"
    echo "Запуск: $test_name"
    echo "----------------------------------------"

    if [ -f "$test_script" ]; then
        if timeout 120 python3 "$test_script" 2>&1; then
            echo "✅ $test_name: PASSED"
            ((PASSED_TESTS++)) || true
        else
            echo "❌ $test_name: FAILED"
            ((FAILED_TESTS++)) || true
        fi
        ((TOTAL_TESTS++)) || true
    else
        echo "⚠️  $test_name: script not found ($test_script)"
    fi

    echo ""
}

# C тесты
run_test "Utils Security Tests" "./test-utils-security"
run_test "Cache Pool Tests" "./test-cache-pool"
run_test "Cache No-Copy Tests" "./test-cache-no-copy"
run_test "Admin CLI Tests" "./test-admin-cli"
run_test "Admin CLI Integration" "./test-admin-cli-integration"
run_test "IO_URING Tests" "./test-io-uring"
run_test "REST API Tests" "./test-rest-api"

# Бенчмарки
run_test "Memory Allocator Benchmark" "./benchmark-memory-allocator"
run_test "IO_URING Benchmark" "./benchmark-io-uring"
run_test "Highload Benchmark (100K+)" "./benchmark-highload"
run_test "Cache Performance Benchmark" "./benchmark-cache-performance"

# Интеграционные тесты
run_test "Integration Tests Simple" "./integration-tests-simple"
run_test "Cache Performance Simple" "./cache-performance-test-simple"
run_test "Rate Limiter Simple" "./rate-limiter-highload-test-simple"

# Python тесты
run_python_test "Metrics + Docker Integration" "../testing/test_metrics_docker_integration.py"
run_python_test "Metrics Collector" "../testing/test_metrics_collector.py"

# Вывод статистики
echo "========================================"
echo "  Статистика тестирования"
echo "========================================"
echo "Всего тестов: $TOTAL_TESTS"
echo "Пройдено: $PASSED_TESTS"
echo "Не пройдено: $FAILED_TESTS"
echo "========================================"

if [ $FAILED_TESTS -eq 0 ]; then
    echo "✅ Все тесты пройдены!"
    exit 0
else
    echo "❌ Некоторые тесты не пройдены"
    exit 1
fi
