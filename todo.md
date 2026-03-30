# MTProxy TODO

**Версия:** v1.0.32-dev
**Ветка:** dev → master ✅ synced
**Последнее обновление:** 30 марта 2026

---

## 🎯 Текущий спринт (Q2 2027)

### ✅ Выполнено
- [x] Очистка документации (26 → 6 файлов)
- [x] Исправление сборки на Windows
- [x] Консолидация руководств (CLI, Debugging, Platform, Monitoring)
- [x] Исправления Windows compatibility (7 файлов)
- [x] Исправления cluster-manager (конфликт имён)
- [x] **ML Systems** — Anomaly Detection (5 алгоритмов, 1800+ строк) ✅
- [x] **ML Systems** — Predictive Analytics (6 алгоритмов, 1600+ строк) ✅
- [x] **Тесты ML** — +87 C тестов (45 + 42) ✅
- [x] **CMakeLists.txt** — ML тесты добавлены ✅
- [x] **Benchmark ML** — benchmark-ml-systems.c добавлен ✅

---

## 🔴 Критичные задачи

### Сборка и CI/CD
- [x] **Docker образы** — обновлены после очистки документации ✅
- [ ] **Linux сборка** — проверить с io_uring и jemalloc
- [x] **CMakeLists.txt** — проверить наличие всех ML тестов ✅

### Тесты
- [x] **test-alert-manager** — исправлен kprintf на Windows ✅
- [x] **test-distributed-tracing** — исправлена span_t структура ✅
- [x] **test-production-integration** — исправлен health_check вызов ✅
- [x] **test-anomaly-detection** — 45 тестов ✅
- [x] **test-predictive-analytics** — 42 теста ✅

---

## 🟡 Технические долги

### Код
- [x] `cluster-manager.c` — исправлен доступ к структуре ✅
- [x] `load-balancer.c` — исправлен kprintf ✅
- [x] `rest-api.c` — исправлена Windows совместимость ✅
- [x] `benchmark_cache_performance.c` — обновлён API cache_manager ✅

### Производительность
- [ ] Проверить утечки памяти через ASan
- [ ] Оптимизировать размер бинарника (сейчас 85MB)
- [ ] Benchmark для Windows сборки
- [x] Benchmark для ML систем — benchmark-ml-systems.c ✅

---

## 🟢 Улучшения

### Документация
- [x] Консолидация завершена (6 файлов в docs/)
- [x] Проверены все ссылки в README.md
- [x] **docs/ML_SYSTEMS.md** — добавлено полное руководство (500+ строк) ✅

### Код
- [x] Добавить обработку ошибок в CLI ✅
- [x] Улучшить сообщения об ошибках ✅
- [x] Добавить unit-тесты для новых модулей ✅
- [x] **ML интеграция** — Anomaly Detection + Predictive Analytics ✅

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| C файлов | 246 | +3 (Q2 2027) |
| MD файлов | 6 (было 26) | -77% |
| Строк кода | ~3400 (ML) | +3400 (Q2 2027) |
| Тестов C | 27 | +2 (ML тесты) |
| Бенчмарков | 5 | +1 (ML benchmark) |
| Коммитов | 542 | +1 (сегодня) |
| ML алгоритмов | 11 (5 Anomaly + 6 Predictive) | +11 (Q2 2027) |
| ML модулей | 3 (system/ml/) | +3 (Q2 2027) |

---

## 📝 Правила

1. **Качество важнее количества** — фокус на стабильность
2. **Workflow:** dev → проверка → master ✅
3. **Без документации без запроса** — только код и исправления
4. **Синхронизация:** все изменения в dev, затем merge в master ✅

---

*Последнее обновление: 30 марта 2026 — v1.0.32-dev*
