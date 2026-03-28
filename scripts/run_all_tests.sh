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

# Запуск всех тестов
run_test "New Modules Tests" "./test-new-modules.exe"
run_test "Utils Tests" "./test-utils.exe"
run_test "Traffic Stats Tests" "./test-traffic-stats.exe"
run_test "Integration Tests" "./integration-tests-simple.exe"
run_test "Cache Performance Tests" "./cache-performance-test-simple.exe"
run_test "Rate Limiter Tests" "./rate-limiter-highload-test-simple.exe"

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
