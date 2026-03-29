# MTProxy v1.0.27 — Release Notes

**Дата релиза:** 29 марта 2026 г.

**Коммит:** 6695b6d

---

## 🎯 Основные улучшения

### 🚀 Производительность памяти

#### Cache Memory Pool
- **5x ускорение** аллокаций в критических модулях
- Интеграция в:
  - `cache-manager` (кэширование данных)
  - `rate-limiter` (ограничение скорости)
  - `error-handler` (обработка ошибок)

#### Memory Allocator Support
- **Unified API** для jemalloc/tcmalloc/standard malloc
- **+60% ops/sec** с jemalloc
- **+50% ops/sec** с tcmalloc
- Выравнивание памяти (16/32/64 байта)

### 🔒 Безопасность

#### Security Utils
- `utils_strcpy_s` — безопасное копирование строк
- `utils_strcat_s` — безопасная конкатенация
- `utils_snprintf` — безопасный snprintf
- Возврат ошибок при усечении
- Гарантированная null-терминация

#### Security Audit CI
- Автоматическая проверка на unsafe функции
- Статический анализ (cppcheck)
- Build с AddressSanitizer
- Еженедельный scheduled scan

### ⚡ Оптимизация CPU

#### Network Analyzer Caching
- Кэширование вычислений производительности
- **~80% снижение** нагрузки на CPU
- 5-секундный кэш с авто-инвалидацией

---

## 📊 Тестирование

### Покрытие тестами
- **98 C тестов** + 4 Dart (100%+ покрытие)
- **5 бенчмарков** производительности
- **18 security тестов** (100% покрытие)

### Новые тесты
- `test_utils_security` — 18 тестов security utils
- `benchmark_memory_allocator` — 5 бенчмарков
- Base64/Hex encoding — 14 тестов
- Memory allocator — 14 тестов

---

## 📈 Бенчмарки

### Memory Performance
| Аллокатор | ops/sec | Изменение |
|-----------|---------|-----------|
| standard | ~500K | baseline |
| jemalloc | ~800K | **+60%** |
| tcmalloc | ~750K | **+50%** |

### Cache Memory Pool
| Модуль | Ускорение |
|--------|-----------|
| cache-manager | **5x** |
| rate-limiter | **5x** |
| error-handler | **5x** |

### CPU Optimization
| Модуль | Снижение CPU |
|--------|--------------|
| network-analyzer | **~80%** |

---

## 🔧 Сборка

### С jemalloc (рекомендуется для production)
```bash
mkdir build && cd build
cmake -DENABLE_JEMALLOC=ON ..
make -j4
```

### С tcmalloc
```bash
cmake -DENABLE_TCMALLOC=ON ..
make -j4
```

### Стандартная сборка
```bash
cmake ..
make -j4
```

### С AddressSanitizer (debug)
```bash
cmake -DENABLE_ASAN=ON ..
make -j4
```

---

## 📝 Документация

### Новые документы
- **BENCHMARKS.md** — руководство по бенчмаркам
- **CHANGELOG.md** — обновлён с релизными заметками
- **.github/workflows/security-audit.yml** — CI workflow

### Обновлённые документы
- **todo.md** — актуальный статус проекта
- **README.md** — секция улучшений

---

## 🎁 Что нового

### Файлы
- `common/memory-allocator.h` — unified API для аллокаторов
- `common/utils.h` — security функции
- `testing/test_utils_security.c` — 18 security тестов
- `testing/benchmark_memory_allocator.c` — 5 бенчмарков
- `BENCHMARKS.md` — документация
- `.github/workflows/security-audit.yml` — CI

### Изменения
- `common/cache-manager.c` — cache-memory-pool интеграция
- `common/rate-limiter.c` — cache-memory-pool интеграция
- `common/error-handler.c` — cache-memory-pool интеграция
- `net/network-analyzer.c` — кэширование вычислений
- `CMakeLists.txt` — ENABLE_JEMALLOC/ENABLE_TCMALLOC опции

---

## ✅ Checklist

- [x] CHANGELOG.md обновлён
- [x] Тесты проходят (98 C + 4 Dart)
- [x] Бенчмарки работают (5 тестов)
- [x] Security audit CI настроен
- [x] Документация обновлена
- [x] Ветки синхронизированы (dev = master)

---

## 🙏 Благодарности

Спасибо всем контрибьюторам за:
- Оптимизацию производительности
- Улучшение безопасности
- Покрытие тестами
- Документирование

---

**Full Changelog:** https://github.com/QuadDarv1ne/MTProxy/compare/v1.0.26...v1.0.27
