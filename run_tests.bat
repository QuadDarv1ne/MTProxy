@echo off
echo Запуск тестирования MTProxy...

REM Проверяем наличие необходимых инструментов
where cl >nul 2>nul
if %errorlevel% == 0 (
    echo Найден Microsoft Visual C++ Compiler
    goto :build_with_vc
)

where gcc >nul 2>nul
if %errorlevel% == 0 (
    echo Найден GCC
    goto :build_with_gcc
)

echo Не найден подходящий компилятор C
echo Установите Microsoft Visual Studio или MinGW для компиляции проекта
goto :end

:build_with_vc
echo Компиляция с использованием Visual Studio...
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo Ошибка при генерации проекта
    goto :end
)
cmake --build .
if %errorlevel% neq 0 (
    echo Ошибка при сборке проекта
    goto :end
)
echo Сборка завершена успешно
goto :run_tests

:build_with_gcc
echo Компиляция с использованием GCC...
if not exist build mkdir build
cd build
cmake .. -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo Ошибка при генерации проекта
    goto :end
)
mingw32-make
if %errorlevel% neq 0 (
    echo Ошибка при сборке проекта
    goto :end
)
echo Сборка завершена успешно
goto :run_tests

:run_tests
echo Запуск тестов...
cd ..
gcc -o test_runner.exe test_runner.c testing/automated-testing.c -I. -I./common -I./testing
if %errorlevel% == 0 (
    echo Запуск тестового раннера...
    test_runner.exe
) else (
    echo Ошибка компиляции тестового раннера
)

:end
echo Завершение тестирования
pause