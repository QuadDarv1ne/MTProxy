# Scripts Directory

Скрипты для автоматизации работы с MTProxy.

## Доступные скрипты

### Тестирование

#### run_all_tests.bat (Windows)
```bash
scripts\run_all_tests.bat [build_dir]
```

**Пример:**
```bash
scripts\run_all_tests.bat build
```

#### run_all_tests.sh (Linux/macOS)
```bash
chmod +x scripts/run_all_tests.sh
./scripts/run_all_tests.sh [build_dir]
```

**Пример:**
```bash
./scripts/run_all_tests.sh build
```

### Сборка

#### build.bat (Windows)
```bash
scripts\build.bat [Release^|Debug]
```

#### build.sh (Linux/macOS)
```bash
chmod +x scripts/build.sh
./scripts/build.sh [Release^|Debug]
```

---

## Переменные окружения

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `MTProxy_BUILD_DIR` | Директория сборки | `build` |
| `MTProxy_BIN_DIR` | Директория бинарников | `build/bin` |
| `MTProxy_TESTS` | Запуск тестов | `1` |

---

## Примеры использования

### Быстрая сборка и тестирование

**Windows:**
```bash
# Сборка
scripts\build.bat Release

# Тесты
scripts\run_all_tests.bat
```

**Linux/macOS:**
```bash
# Сборка
./scripts/build.sh Release

# Тесты
./scripts/run_all_tests.sh
```

### Debug сборка с ASan

**Windows:**
```bash
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=deps\ninja.exe -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -B build
cmake --build build
```

**Linux:**
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -B build
cmake --build build
```

---

## Выходные данные тестов

После запуска тестов вы увидите:

```
========================================
  MTProxy - Запуск всех тестов
========================================

----------------------------------------
Запуск: New Modules Tests
----------------------------------------
===========================================
  MTProxy New Modules Test Suite
===========================================
...
===========================================
  Test Summary
===========================================
  Total:  45
  Passed: 45
  Failed: 0
===========================================
✅ New Modules Tests: PASSED

...

========================================
  Статистика тестирования
========================================
Всего тестов: 6
Пройдено: 6
Не пройдено: 0
========================================
✅ Все тесты пройдены!
```

---

## Интеграция с CI/CD

### GitHub Actions

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: |
          sudo apt-get install -y libssl-dev zlib1g-dev  # Ubuntu
          brew install openssl zlib                      # macOS
      
      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
      
      - name: Test
        run: |
          ./scripts/run_all_tests.sh build
```

---

## Поддерживаемые тесты

| Тест | Файл | Описание |
|------|------|----------|
| **New Modules** | test-new-modules.exe | Тесты новых модулей (cache, rate-limiter, error-handler) |
| **Utils** | test-utils.exe | Тесты утилит (строки, память, хэши) |
| **Traffic Stats** | test-traffic-stats.exe | Тесты статистики трафика |
| **Integration** | integration-tests-simple.exe | Интеграционные тесты |
| **Cache Performance** | cache-performance-test-simple.exe | Тесты производительности кэша |
| **Rate Limiter** | rate-limiter-highload-test-simple.exe | Тесты нагрузки rate-limiter |

---

## Покрытие кода

Для измерения покрытия кода используйте gcov/lcov:

**Linux:**
```bash
# Сборка с покрытием
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" -B build
cmake --build build

# Запуск тестов
./scripts/run_all_tests.sh build

# Генерация отчёта
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

**Просмотр отчёта:**
```bash
xdg-open coverage_report/index.html  # Linux
open coverage_report/index.html      # macOS
start coverage_report\index.html     # Windows
```

---

## Дополнительные ресурсы

- [DEBUGGING.md](../docs/DEBUGGING.md) — Руководство по отладке
- [TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md) — Диагностика проблем
- [PERFORMANCE_TUNING.md](../docs/PERFORMANCE_TUNING.md) — Оптимизация производительности
