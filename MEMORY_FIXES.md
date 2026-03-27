# Исправления проблем с памятью (OOM Fix)

## Проблема
При запуске тестов и приложения происходило переполнение оперативной памяти, что приводило к выключению ПК.

## Причины
1. **Отсутствие лимитов памяти** в модулях cache-manager и memory-pool
2. **Тяжёлые модули оптимизации** (NUMA, DPDK, ML-предикторы) потребляли много памяти
3. **Unix-пути в тестах** вызывали проблемы на Windows
4. **Отсутствовала оптимизация** для систем с ограниченной памятью

## Внесённые исправления

### 1. Исправление тестов (`testing/test_new_modules.c`)
- Добавлена функция `get_temp_path()` для кроссплатформенных временных путей
- Заменены хардкоженные пути `/tmp/` на платформо-зависимые

### 2. Ограничения памяти в cache-manager (`common/cache-manager.c`)
- Максимальный размер кэша: **256 MB** (было: без лимита)
- Максимальное количество бакетов: **4096** на раздел
- Добавлены предупреждения при попытке выделения слишком большой памяти

### 3. Исправление memory-pool (`system/memory-pool.c`)
- Максимальный размер пула: **64 MB**
- Минимальный размер пула: **4 KB**
- Добавлена защита от:
  - Двойного освобождения (double free)
  - Выделения за границы буфера (invalid free)
  - Слишком больших выделений (>50% буфера)

### 4. Оптимизация Makefile
- Добавлены флаги памяти: `-DMAX_CACHE_SIZE_MB=128 -DMAX_POOL_SIZE_MB=64`
- Автоматическое определение платформы (Windows/Unix)
- Исключены тяжёлые модули для Windows:
  - `system/numa-allocator.o`
  - `system/io-uring-interface.o`
  - `system/dpdk-interface.o`
  - `system/advanced-optimizer.o`
- Добавлена опция `MEMORY_OPT=1` для дополнительной оптимизации

### 5. Оптимизация CMakeLists.txt
- Добавлены опции:
  - `ENABLE_LOW_MEMORY` - режим низкого потребления памяти
  - `EXCLUDE_HEAVY_MODULES` - исключение тяжёлых модулей
- Автоматическое исключение тяжёлых модулей для Windows:
  - `system/numa-allocator.*`
  - `system/advanced-optimizer.*`
  - `system/advanced-predictive-analytics.*`
  - `system/ml-performance-predictor.*`
  - `system/proactive-allocator.*`
  - `system/advanced-memory-management.*`

## Использование

### Сборка с помощью Make (Windows/Unix)

```bash
# Обычная сборка
make

# Сборка с оптимизацией памяти
make MEMORY_OPT=1

# Очистка
make clean
```

### Сборка с помощью CMake

```bash
# Обычная сборка
cmake -B build -S .
cmake --build build

# Сборка с низким потреблением памяти
cmake -B build -S . -DENABLE_LOW_MEMORY=ON
cmake --build build

# Принудительное исключение тяжёлых модулей
cmake -B build -S . -DEXCLUDE_HEAVY_MODULES=ON
cmake --build build
```

### Запуск тестов

```bash
# Make
make test

# CMake
ctest --test-dir build
```

## Рекомендуемые настройки

### Для систем с 4-8 GB RAM
```bash
cmake -B build -S . -DENABLE_LOW_MEMORY=ON
```

### Для систем с <4 GB RAM
```bash
cmake -B build -S . -DENABLE_LOW_MEMORY=ON -DEXCLUDE_HEAVY_MODULES=ON
```

### Для Windows (автоматически)
```bash
# Тяжёлые модули исключаются автоматически
cmake -B build -S .
```

## Мониторинг памяти

После запуска приложения следите за потреблением памяти:
- **Нормальное потребление**: 50-200 MB
- **Предупреждение**: 200-500 MB
- **Критично**: >500 MB (используйте `ENABLE_LOW_MEMORY=ON`)

## Дополнительные рекомендации

1. **Закройте лишние приложения** перед сборкой и тестами
2. **Используйте 64-битную сборку** для лучшего управления памятью
3. **Отключите отладочные символы** в релизной сборке:
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

## Статус исправлений

- ✅ Кроссплатформенные пути в тестах
- ✅ Лимиты памяти в cache-manager
- ✅ Защита от утечек в memory-pool
- ✅ Исключение тяжёлых модулей для Windows
- ✅ Опции оптимизации памяти в CMake

## Если проблемы продолжаются

1. Попробуйте режим минимальной памяти:
   ```bash
   cmake -B build -S . -DENABLE_LOW_MEMORY=ON -DEXCLUDE_HEAVY_MODULES=ON -DCMAKE_BUILD_TYPE=MinSizeRel
   ```

2. Отключите дополнительные модули в CMakeLists.txt:
   - `perf_monitor/*`
   - `ml/*`
   - `system/diagnostic/*`

3. Увеличьте файл подкачки Windows до 8-16 GB

## Контакты

При возникновении проблем создайте issue с:
- Версией ОС и объёмом RAM
- Логами сборки
- Уровнем потребления памяти
