# MTProxy TODO

**Версия:** v1.0.32-dev
**Ветка:** dev (синхронизирована с main)
**Последнее обновление:** 1 апреля 2026
**Следующая версия:** v1.0.33
**Коммитов:** 2

---

## 🎯 Текущий спринт (Q2 2026)

### ✅ Выполнено (v1.0.32)
- [x] **ML Systems** — Anomaly Detection (5 алгоритмов, 1800+ строк) ✅
- [x] **ML Systems** — Predictive Analytics (6 алгоритмов, 1600+ строк) ✅
- [x] **Тесты ML** — +87 C тестов (45 + 42) ✅
- [x] **CMakeLists.txt** — ML тесты добавлены ✅
- [x] **Benchmark ML** — benchmark-ml-systems.c добавлен ✅
- [x] **Документация** — docs/ML_SYSTEMS.md (500+ строк) ✅
- [x] **Документация** — docs/OBFUSCATION.md (1000+ строк) ✅
- [x] **ML Integration** — исправлены баги (zscore_threshold, cleanup, remove_monitor) ✅
- [x] **Версия** — v1.0.32 установлена ✅
- [x] Очистка документации (26 → 7 файлов) ✅
- [x] Исправление сборки на Windows ✅
- [x] Консолидация руководств (CLI, Debugging, Platform, Monitoring, OBFUSCATION) ✅
- [x] Исправления Windows compatibility (7 файлов) ✅
- [x] Исправления cluster-manager (конфликт имён) ✅

---

## 🔴 Критичные задачи (v1.0.33)

### Сборка и CI/CD — ПРИОРИТЕТ
- [ ] **Docker образы** — обновлены после очистки документации
- [ ] **Linux сборка** — проверить с io_uring и jemalloc
- [ ] **Windows сборка** — тестирование производительности
- [x] **CMakeLists.txt** — проверить наличие всех ML тестов ✅
- [x] **CHANGELOG.md** — v1.0.32 документирован ✅

### Тесты — КРИТИЧНО
- [ ] **ASan** — проверка утечек памяти (все модули)
- [ ] **MSan** — проверка ошибок памяти
- [ ] **TSan** — проверка гонок потоков
- [x] **test-alert-manager** — исправлен kprintf на Windows ✅
- [x] **test-distributed-tracing** — исправлена span_t структура ✅
- [x] **test-production-integration** — исправлен health_check вызов ✅
- [x] **test-anomaly-detection** — 45 тестов ✅
- [x] **test-predictive-analytics** — 42 теста ✅
- [x] **test-ml-integration** — исправлены баги ✅

### Известные проблемы (KNOWN ISSUES)
- [ ] **padding.c** — test_padding_fixed_add_remove: data corruption при добавлении length prefix ❌
  - *Проблема:* `padding_add_fixed()` добавляет length prefix (4 байта), затем сдвигает данные, но `padding_remove_fixed()` некорректно восстанавливает
  - *Статус:* тест отключён (`#if 0`), требуется исправление логики
- [ ] **fragmentation.c** — test_fragmentation_fixed: проблемы с размером фрагментов (TLS header) ❌
  - *Проблема:* тест ожидает `fragments[i].len == ctx.fragment_size`, но фрагменты содержат payload + TLS header
  - *Статус:* тест отключён (`#if 0`), требуется исправление теста или логики
- [ ] **fragmentation.c** — test_fragmentation_calculate_count: не учитывает TLS header ❌
  - *Проблема:* `fragmentation_calculate_count()` не учитывает overhead TLS header при расчёте количества фрагментов
  - *Статус:* тест отключён (`#if 0`), требуется исправление
- [x] **obfuscate.c** — test_obfuscate_xor_*: раздельный контекст для encrypt/decrypt ✅

---

## 🟡 Технические долги

### Код — ТРЕБУЕТ ВНИМАНИЯ
- [ ] **Оптимизация бинарника** — уменьшить размер (сейчас ~85MB)
- [ ] **Windows совместимость** — полный аудит модулей
- [ ] **io_uring** — интеграция для Linux
- [x] `cluster-manager.c` — исправлен доступ к структуре ✅
- [x] `load-balancer.c` — исправлен kprintf ✅
- [x] `rest-api.c` — исправлена Windows совместимость ✅
- [x] `benchmark_cache_performance.c` — обновлён API cache_manager ✅
- [x] **ML modules** — исправлены баги Windows совместимости ✅
- [x] **padding.c** — добавлена поддержка Windows (getpid) ✅

### Производительность
- [ ] **Benchmark для Windows** — тесты для ML систем
- [ ] **Benchmark для Linux** — io_uring + jemalloc
- [x] **Benchmark для ML систем** — benchmark-ml-systems.c ✅

---

## 🟢 Улучшения (в работе)

### Документация
- [x] Консолидация завершена (7 файлов в docs/) ✅
- [x] Проверены все ссылки в README.md ✅
- [x] **docs/ML_SYSTEMS.md** — добавлено полное руководство (500+ строк) ✅
- [x] **docs/OBFUSCATION.md** — добавлено руководство по обфускации (1000+ строк) ✅
- [x] **CHANGELOG.md** — v1.0.32 полностью документирован ✅

### Код
- [x] Добавить обработку ошибок в CLI ✅
- [x] Улучшить сообщения об ошибках ✅
- [x] Добавить unit-тесты для новых модулей ✅
- [x] **ML интеграция** — Anomaly Detection + Predictive Analytics ✅
- [x] **Улучшен CLI** — JSON форматирование + справка ✅
- [x] **Удалены TODO** — реализована справка и quit/exit команды ✅
- [x] **ML интеграция в production** — тестирование на реальных данных ✅
- [x] **Исправлены тесты obfuscate** — раздельные контексты для encrypt/decrypt ✅

---

## 📊 Статистика проекта

| Метрика | Значение | Примечание |
|---------|----------|------------|
| C файлов | 249 | в git |
| H файлов | 215 | в git |
| Всего файлов | 464 | C + H в git |
| Тестов C | 30 | testing/test_*.c |
| Тестов всего | 388 | +87 ML тестов |
| Бенчмарков | 5 | ML benchmark включён |
| Коммитов | 2 | новый репозиторий |
| ML алгоритмов | 11 | 5 Anomaly + 6 Predictive |
| ML модулей | 3 | system/ml/ |
| Строк кода (ML) | ~3400 | Anomaly + Predictive |

---

## 📝 Правила проекта

1. **Качество важнее количества** — фокус на стабильность и тесты
2. **Workflow:** dev → проверка → main (через PR)
3. **Без документации без запроса** — только код и исправления
4. **Синхронизация:** все изменения в dev, затем merge в main
5. **Тесты обязательны** — для любого нового кода
6. **Проверка перед коммитом** — ASan/MSan/TSan для критичных модулей

---

## 📋 План на Q3 2026

### Приоритеты (v1.0.33)
1. [ ] **Linux сборка** — проверка с io_uring и jemalloc
2. [ ] **ASan тесты** — проверка утечек памяти (все модули)
3. [ ] **Оптимизация бинарника** — уменьшить размер (~85MB → ~50MB)
4. [ ] **Windows benchmark** — тесты производительности для ML систем
5. [ ] **Production интеграция** — развёртывание и мониторинг
6. [ ] **Тестовое покрытие** — расширить coverage для критичных модулей

### Исправления (технический долг)
- [x] **ML zscore_threshold** — инициализация по умолчанию ✅
- [x] **ML cleanup mutex** — исправлен unlock после memset ✅
- [x] **ML remove_monitor** — очистка детекторов при удалении ✅
- [x] **ML divide-by-zero** — защита в compute_zscore_anomaly ✅
- [x] **ML empty samples** — защита в compute_isolation_forest_score ✅

### Статус релиза
- [x] **v1.0.32** — ML integration завершена ✅
- [x] **Первый коммит** — инициализация репозитория ✅
- [ ] **dev → main** — синхронизация после тестов ✅
- [ ] **v1.0.33** — подготовка (Q3 2026)
- [ ] **upstream sync** — синхронизация с upstream/master

---

## 🔧 Ближайшие задачи (эта неделя)

### Код
- [ ] Проверить сборку на Linux (io_uring, jemalloc)
- [ ] Запустить ASan для ML модулей
- [ ] Оптимизировать размер бинарника
- [ ] Исправить **padding.c** — data corruption в test_padding_fixed_add_remove
  - [ ] Исправить `padding_add_fixed()` — корректно добавлять length prefix
  - [ ] Исправить `padding_remove_fixed()` — корректно удалять length prefix
  - [ ] Включить тест test_padding_fixed_add_remove
- [ ] Исправить **fragmentation.c** — учёт TLS header в тестах
  - [ ] Исправить test_fragmentation_fixed — проверка с учётом TLS header
  - [ ] Исправить fragmentation_calculate_count() — учёт overhead TLS header
  - [ ] Включить тесты test_fragmentation_*

### Тесты
- [ ] Прогнать все тесты на Linux
- [ ] Прогнать все тесты на Windows
- [ ] Проверить coverage для system/ml/
- [ ] Включить отключённые тесты (test_padding_fixed_add_remove, test_fragmentation_*)

### Документация
- [ ] Обновить README.md с информацией о ML системах
- [ ] Добавить примеры использования в docs/ML_SYSTEMS.md
- [ ] Документировать KNOWN ISSUES в соответствующих модулях

---

*Последнее обновление: 1 апреля 2026 — v1.0.32-dev (обновлено: KNOWN ISSUES, статистика, коммиты)
*Следующая проверка: 8 апреля 2026
*Статус: 2 коммита в dev/main, синхронизированы
