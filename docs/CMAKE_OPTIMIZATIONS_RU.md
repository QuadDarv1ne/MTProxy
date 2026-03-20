# Оптимизации CMake для MTProxy

Этот документ описывает оптимизации сборки, доступные в CMakeLists.txt для MTProxy.

## 🚀 Быстрый старт

### Стандартная сборка

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

### Сборка с максимальными оптимизациями

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON
cmake --build . --parallel
# Запустите binary для сбора профиля
./bin/mtproto-proxy [typical workload]
# Пересоберите с использованием профиля
cmake .. -DUSE_PGO_PROFILE=ON
cmake --build . --parallel
```

## 📋 Доступные опции

### Кэширование

| Опция | Описание | Значение по умолчанию |
|-------|----------|----------------------|
| `CMAKE_CACHE_MAX_SIZE` | Максимальный размер CMake кэша (MB) | 50 |
| `CMAKE_SUPPRESS_REGENERATION` | Подавить автоматическую регенерацию | ON |
| `ENABLE_CCACHE` | Включить ccache для ускорения пересборки | ON |

### Оптимизации компиляции

| Опция | Описание | Значение по умолчанию |
|-------|----------|----------------------|
| `ENABLE_PCH` | Precompiled headers для ускорения сборки | ON |
| `ENABLE_PGO` | Profile-Guided Optimization (генерация) | OFF |
| `USE_PGO_PROFILE` | Использовать существующий PGO профиль | OFF |
| `STATIC_LINKING` | Статическая линковка | OFF |

### Сборка

| Опция | Описание | Значение по умолчанию |
|-------|----------|----------------------|
| `BUILD_SHARED_LIB` | Сборка shared library для FFI | OFF |
| `CMAKE_BUILD_PARALLEL_LEVEL` | Уровень параллелизма сборки | Auto (CPU count) |

## 🔬 Profile-Guided Optimization (PGO)

PGO позволяет компилятору оптимизировать код на основе реального профиля использования.

### Этап 1: Генерация профиля

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON
cmake --build . --parallel
```

Запустите приложение с типичной рабочей нагрузкой для сбора данных профиля:

```bash
# Пример: запуск с типичными параметрами
./bin/mtproto-proxy -p 8888 -H 443 -S <secret> --aes-pwd proxy-secret proxy-multi.conf -M 1

# Или запустите тесты для покрытия различных путей выполнения
./bin/test-new-modules
```

Данные профиля сохраняются в `build/pgo-data/`.

### Этап 2: Сборка с использованием профиля

```bash
cmake .. -DUSE_PGO_PROFILE=ON
cmake --build . --parallel
```

Компилятор использует собранные данные для оптимизации:
- Размещение часто выполняемого кода в кэше инструкций
- Оптимизация предсказания ветвлений
- Улучшение инлайнинга функций

**Ожидаемый прирост производительности:** 10-20%

## ⚡ Ccache

Ccache кэширует результаты компиляции для ускорения повторных сборок.

### Включение ccache

```bash
cmake .. -DENABLE_CCACHE=ON
```

CMake автоматически найдет `ccache` в PATH и будет использовать его для компиляции.

### Установка ccache

**Linux:**
```bash
# Debian/Ubuntu
sudo apt install ccache

# CentOS/Fedora
sudo dnf install ccache
```

**macOS:**
```bash
brew install ccache
```

**Windows (MSYS2):**
```bash
pacman -S ccache
```

### Статистика ccache

```bash
ccache --show-stats
ccache --zero-stats  # Сброс статистики
ccache --cleanup     # Очистка кэша
```

**Ожидаемое ускорение повторной сборки:** 5-10x

## 📦 Precompiled Headers (PCH)

Precompiled headers уменьшают время компиляции за счет предварительной компиляции часто используемых заголовков.

### Включение PCH

```bash
cmake .. -DENABLE_PCH=ON
```

**Поддерживается:** GCC, Clang

**Ожидаемое ускорение сборки:** 20-30%

## 🏗️ Параллельная сборка

CMake автоматически определяет количество ядер CPU и устанавливает оптимальный уровень параллелизма.

### Ручная установка уровня параллелизма

```bash
cmake --build . --parallel 8  # Использовать 8 потоков
```

Или через переменную окружения:

```bash
export CMAKE_BUILD_PARALLEL_LEVEL=4
cmake --build .
```

## 🎯 Рекомендуемые конфигурации

### Разработка (Debug)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_CCACHE=ON \
  -DENABLE_PCH=ON
```

### Релиз (максимальная производительность)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_CCACHE=ON \
  -DENABLE_PCH=ON \
  -DENABLE_PGO=ON

# После сбора профиля
cmake .. -DUSE_PGO_PROFILE=ON
```

### Релиз с отладочной информацией (RelWithDebInfo)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_CCACHE=ON \
  -DENABLE_PCH=ON
```

### Минимальный размер (MinSizeRel)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DENABLE_CCACHE=ON
```

## 📊 Сравнение производительности

| Конфигурация | Время сборки | Размер binary | Производительность |
|--------------|--------------|---------------|-------------------|
| Debug | 1x | 5x | Базовая |
| Release | 1.2x | 1x | +50% |
| Release + PGO | 1.5x (2 этапа) | 1x | +60-70% |
| Release + LTO | 2x | 0.9x | +55% |

## 🔧 Troubleshooting

### PGO: профиль не найден

```
WARNING: USE_PGO_PROFILE=ON but no profile data found
```

**Решение:**
1. Сначала соберите с `ENABLE_PGO=ON`
2. Запустите binary для сбора данных профиля
3. Пересоберите с `USE_PGO_PROFILE=ON`

### Ccache не найден

```
-- Ccache not found, continuing without caching
```

**Решение:** Установите ccache или отключите опцию:
```bash
cmake .. -DENABLE_CCACHE=OFF
```

### PCH ошибки компиляции

Если возникают ошибки с precompiled headers:

```bash
cmake .. -DENABLE_PCH=OFF
```

## 📝 Примечания

- PGO требует двух этапов сборки, но дает наилучшую производительность
- Ccache наиболее эффективен при частых пересборках
- PCH поддерживает только GCC и Clang
- Для Windows (MSVC) используются аналогичные оптимизации через `/GL` и `/LTCG`

## 📖 Дополнительные ресурсы

- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [GCC PGO Guide](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [Ccache Manual](https://ccache.dev/manual.html)

---

*Последнее обновление: 20 марта 2026 г.*
