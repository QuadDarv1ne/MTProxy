@echo off
REM run_all_tests.bat - Скрипт для запуска всех тестов MTProxy (Windows)

echo ========================================
echo   MTProxy - Запуск всех тестов
echo ========================================
echo.

set BUILD_DIR=%~1
if "%BUILD_DIR%"=="" set BUILD_DIR=build
set BIN_DIR=%BUILD_DIR%\bin

REM Проверка наличия директории
if not exist "%BIN_DIR%" (
    echo Ошибка: Директория %BIN_DIR% не найдена
    echo Сначала выполните сборку: cmake --build %BUILD_DIR%
    exit /b 1
)

cd /d "%BIN_DIR%"

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM Функция для запуска теста
:run_test
set test_name=%~1
set test_executable=%~2

echo ----------------------------------------
echo Запуск: %test_name%
echo ----------------------------------------

if exist "%test_executable%" (
    REM Запуск теста
    call "%test_executable%" > test_output.tmp 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo ✅ %test_name%: PASSED
        set /a PASSED_TESTS+=1
    ) else (
        echo ❌ %test_name%: FAILED
        set /a FAILED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    REM Вывод лога теста
    type test_output.tmp
    del test_output.tmp
) else (
    echo ⚠️  %test_name%: executable not found (%test_executable%)
)

echo.
goto :eof

REM Запуск всех тестов
call :run_test "New Modules Tests" "test-new-modules.exe"
call :run_test "Utils Tests" "test-utils.exe"
call :run_test "Traffic Stats Tests" "test-traffic-stats.exe"
call :run_test "Integration Tests" "integration-tests-simple.exe"
call :run_test "Cache Performance Tests" "cache-performance-test-simple.exe"
call :run_test "Rate Limiter Tests" "rate-limiter-highload-test-simple.exe"

REM Вывод статистики
echo ========================================
echo   Статистика тестирования
echo ========================================
echo Всего тестов: %TOTAL_TESTS%
echo Пройдено: %PASSED_TESTS%
echo Не пройдено: %FAILED_TESTS%
echo ========================================

if %FAILED_TESTS% EQU 0 (
    echo ✅ Все тесты пройдены!
    exit /b 0
) else (
    echo ❌ Некоторые тесты не пройдены
    exit /b 1
)
