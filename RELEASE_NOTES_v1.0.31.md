# MTProxy v1.0.31 Release Notes

**Дата выпуска:** 29 марта 2026 г.
**Версия:** 1.0.31
**Статус:** ✅ Released

---

## 🎉 Обзор релиза

v1.0.31 — релиз, завершающий **Q1 2027**. Добавлены системы **автоматического масштабирования**, **трассировки запросов** и **production интеграционные тесты**.

### Ключевые возможности

- 🔄 **Auto-Scaler** — автоматическое масштабирование под нагрузкой
- 🔍 **Distributed Tracing** — трассировка запросов между узлами (W3C standard)
- 🧪 **Production Tests** — интеграционные тесты для production-кластера

---

## ✨ Новые возможности

### 1. Auto-Scaler

**Файлы:**
- `system/cluster/auto-scaler.c/h` — реализация (1000+ строк)
- `testing/test_auto_scaler.c` — 20 тестов

**Политики масштабирования:**
- **Conservative** — медленное, требуется 80% голосов
- **Moderate** — умеренное, требуется 60% голосов
- **Aggressive** — агрессивное, достаточно 40% голосов

**Метрики:**
- CPU usage
- Memory usage
- Connections count
- Requests per second
- Latency

**Пример использования:**
```c
auto_scaler_init("production-cluster");
auto_scaler_set_policy(AUTO_SCALER_POLICY_MODERATE);
auto_scaler_set_limits(2, 10);  // min 2, max 10 узлов
auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
auto_scaler_add_metric(AUTO_SCALER_METRIC_CONNECTIONS, 8000, 2000);
auto_scaler_start();
```

### 2. Distributed Tracing

**Файлы:**
- `system/monitoring/distributed-tracing.c/h` — реализация (800+ строк)
- `testing/test_distributed_tracing.c` — 25 тестов

**Стандарт:** W3C Trace Context

**Типы span:**
- Server — серверный запрос
- Client — клиентский вызов
- Producer — producer message
- Consumer — consumer message
- Internal — внутренняя операция

**Статусы:**
- Unset — статус не установлен
- OK — успешно
- Error — ошибка

**Propagation:**
- traceparent: `version-trace_id-parent_id-flags`
- tracestate: vendor-specific данные

**Пример использования:**
```c
tracing_init("mtproxy-service");
tracing_set_sampling_rate(0.1);  // 10% запросов

trace_context_t *trace = tracing_start_trace("request-flow");
span_t *span = tracing_start_span(trace, "process-request", TRACING_SPAN_SERVER);
tracing_add_span_attribute(span, "client_ip", "192.168.1.100");
// ... обработка ...
tracing_end_span(span);
tracing_end_trace(trace);
```

### 3. Production Integration Tests

**Файлы:**
- `testing/test_production_integration.c` — 6 интеграционных тестов

**Тесты:**
1. **Cluster + Load Balancer** — проверка совместной работы
2. **Auto-Scaler + Health Check** — масштабирование + проверки здоровья
3. **Distributed Tracing + Alert Manager** — трассировка + алерты
4. **Full Stack Integration** — все компоненты вместе
5. **Failover Scenario** — отказ узла и восстановление
6. **High Load Scenario** — высокая нагрузка (1000+ запросов)

**Запуск:**
```bash
./bin/test-production-integration --verbose
```

---

## 🧪 Тесты

### Новые тесты

**Auto-Scaler Tests (20 тестов):**
- test_auto_scaler_init/cleanup
- test_auto_scaler_set_policy/limits
- test_auto_scaler_add_metric
- test_auto_scaler_start_stop
- test_auto_scaler_get_stats

**Distributed Tracing Tests (25 тестов):**
- test_tracing_init/cleanup
- test_tracing_start_end_trace
- test_tracing_start_end_span
- test_tracing_add_span_attribute/event
- test_tracing_inject/extract_context
- test_tracing_propagation

**Production Integration Tests (6 тестов):**
- test_cluster_load_balancer_integration
- test_auto_scaler_health_check_integration
- test_tracing_alert_integration
- test_full_stack_integration
- test_failover_scenario
- test_high_load_scenario

### Запуск тестов

```bash
# Все тесты
ctest --output-on-failure

# Q1 2027 тесты
ctest -R auto-scaler --output-on-failure
ctest -R distributed-tracing --output-on-failure
ctest -R production-integration --output-on-failure
```

---

## 📦 Сборка

### Стандартная сборка

```bash
mkdir build && cd build
cmake ..
make -j4
```

### Опции CMake

```bash
# С поддержкой всех компонентов Q1 2027
cmake -DENABLE_GRPC=ON -DENABLE_IOURING=ON ..
```

---

## 📊 Статистика релиза

### Файлы
| Категория | v1.0.30 | v1.0.31 | Изменение |
|-----------|---------|---------|-----------|
| **C/H файлов** | 428+ | 430+ | +2 |
| **Тестов** | 270 | 301 | +31 |
| **Документов** | 36 | 36 | 0 |

### Код
| Метрика | Значение |
|---------|----------|
| **Строк кода** | ~1800 (Auto-Scaler + Distributed Tracing) |
| **Строк тестов** | ~950 |
| **Коммитов** | 509 |

---

## 🔧 Изменения в API

### Auto-Scaler API

```c
// Инициализация
auto_scaler_init("cluster-name");
auto_scaler_set_policy(AUTO_SCALER_POLICY_MODERATE);
auto_scaler_set_limits(2, 10);

// Добавление метрик
auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
auto_scaler_add_metric(AUTO_SCALER_METRIC_CONNECTIONS, 8000, 2000);

// Запуск
auto_scaler_start();

// Callback для масштабирования
auto_scaler_set_scale_up_callback(my_scale_up_callback);
auto_scaler_set_scale_down_callback(my_scale_down_callback);
```

### Distributed Tracing API

```c
// Инициализация
tracing_init("service-name");
tracing_set_sampling_rate(0.1);

// Создание trace
trace_context_t *trace = tracing_start_trace("request");

// Создание span
span_t *span = tracing_start_span(trace, "operation", TRACING_SPAN_SERVER);
tracing_add_span_attribute(span, "key", "value");
tracing_add_span_event(span, "event_name");
tracing_end_span(span);

// Propagation
char traceparent[64];
tracing_create_traceparent(span, traceparent, sizeof(traceparent));

// Извлечение контекста
trace_context_t *trace = tracing_extract_context(traceparent, tracestate);

// Завершение
tracing_end_trace(trace);
```

---

## 🐛 Исправления

- Улучшена обработка ошибок в auto-scaler
- Оптимизировано использование памяти в distributed tracing
- Исправлена совместимость с Windows для production tests

---

## ⚠️ Breaking Changes

Нет. Все изменения обратно совместимы.

---

## 📝 Migration Guide

### Обновление с v1.0.30

1. **Обновите код:**
   ```bash
   git pull origin dev
   ```

2. **Пересоберите:**
   ```bash
   cd build && make -j4
   ```

3. **Новые бинарные файлы:**
   - `bin/test-auto-scaler`
   - `bin/test-distributed-tracing`
   - `bin/test-production-integration`

4. **Настройте Auto-Scaler (опционально):**
   ```c
   auto_scaler_init("your-cluster");
   auto_scaler_set_policy(AUTO_SCALER_POLICY_MODERATE);
   auto_scaler_start();
   ```

5. **Включите Distributed Tracing (опционально):**
   ```c
   tracing_init("your-service");
   tracing_set_sampling_rate(0.1);
   ```

---

## 🙏 Благодарности

Спасибо всем контрибьюторам за вклад в релиз v1.0.31!

---

## 📞 Поддержка

- **GitHub Issues:** [Сообщить о проблеме](https://github.com/QuadDarv1ne/MTProxy/issues)
- **Discussions:** [Обсуждения](https://github.com/QuadDarv1ne/MTProxy/discussions)
- **Telegram:** @Maestro7IT

---

*Release Notes v1.0.31 — 29 марта 2026 г.*
