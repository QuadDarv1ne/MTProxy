# MTProxy v1.0.29 Release Notes

**Дата выпуска:** 29 марта 2026 г.  
**Версия:** 1.0.29  
**Статус:** ✅ Released

---

## 🎉 Обзор релиза

v1.0.29 — крупный релиз, завершающий задачи Q3 2026. Добавлены **gRPC API**, **REST API**, расширенный **Grafana дашборд** и полная интеграция с **Prometheus**.

### Ключевые возможности

- 🚀 **gRPC API** — высокопроизводительный RPC интерфейс для управления
- 🌐 **REST API** — HTTP/JSON API для интеграции с внешними системами
- 📊 **Grafana Enhanced Dashboard** — 16 панелей мониторинга
- 🔔 **Prometheus Alerting** — 20+ правил алертинга
- 🧪 **Тесты** — +10 C тестов для gRPC

---

## ✨ Новые возможности

### 1. gRPC API

**Файлы:**
- `api/grpc-server.h` (320+ строк) — заголовочный файл API
- `api/grpc-server.c` (400+ строк) — реализация сервера
- `api/protobuf/mtproxy.proto` (250+ строк) — Protobuf IDL
- `api/docs/GRPC_API.md` — документация

**Функционал:**
- 18 RPC методов для управления сервером
- Поддержка streaming (StreamStatistics, StreamLogs)
- Интеграция через callback функции
- Мультиязычная генерация кода (Go, Python, Java, C++, Node.js)

**Пример использования (Python):**
```python
import grpc
import mtproxy_pb2
import mtproxy_pb2_grpc

channel = grpc.insecure_channel('localhost:50051')
stub = mtproxy_pb2_grpc.MTProxyServiceStub(channel)

# Получить статус
status = stub.GetServerStatus(mtproxy_pb2.GetServerStatusRequest())
print(f"Server state: {status.state}")

# Получить статистику
stats = stub.GetStatistics(mtproxy_pb2.GetStatisticsRequest())
print(f"Active connections: {stats.active_connections}")
```

### 2. REST API

**Файлы:**
- `net/rest-api.c/h` — реализация REST API
- `testing/test_rest_api.c` — 18 тестов

**Endpoints:**
| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/server/status` | Статус сервера |
| POST | `/api/v1/server/start` | Запуск сервера |
| GET | `/api/v1/statistics` | Статистика |
| GET | `/api/v1/config` | Конфигурация |
| PUT | `/api/v1/config` | Обновление конфигурации |
| GET | `/api/v1/secrets` | Список секретов |
| POST | `/api/v1/secrets` | Добавить секрет |

**Пример:**
```bash
curl -X GET http://localhost:8888/api/v1/server/status \
  -H "X-API-Key: your-api-key"
```

### 3. Grafana Enhanced Dashboard

**Файлы:**
- `grafana/mtproxy-enhanced-dashboard.json` — расширенный дашборд
- `grafana/README.md` — обновлённая документация

**Новые панели (8):**
1. **Cache Performance** — hit rate с цветными порогами
2. **Cache Statistics** — entries, evictions, memory usage
3. **Rate Limiting** — active rules, blocked IPs, whitelisted IPs
4. **gRPC API Performance** — requests/sec, errors/sec, active streams
5. **Secrets Management** — total secrets, secrets in use
6. **gRPC API Latency** — p50, p95, p99 percentiles
7. **Connections by Status** — active, idle, closed, rejected
8. **Total Connections History** — total + max limit

### 4. Prometheus Integration

**Файлы:**
- `scripts/prometheus_exporter.py` — Prometheus экспортёр
- `prometheus/mtproxy-alerts.yml` — 20+ правил алертинга

**Новые метрики:**
```promql
# gRPC API
mtproxy_grpc_requests_total
mtproxy_grpc_errors_total
mtproxy_grpc_active_streams
mtproxy_grpc_request_duration_seconds_bucket

# Secrets
mtproxy_secrets_count
mtproxy_secrets_in_use

# Connection limits
mtproxy_max_connections_limit
mtproxy_connections_by_status
```

**Алерты:**
- MTProxyServerDown — сервер в ошибке
- MTProxyCriticalCPUUsage — CPU > 95%
- MTProxyPossibleAttack — rate limited > 500/s
- MTProxyLowCacheHitRate — hit rate < 50%

---

## 🧪 Тесты

### Новые тесты

**gRPC Server Tests (10 тестов):**
- `testing/test_grpc_server.c`
  - test_grpc_server_init
  - test_grpc_server_start_stop
  - test_grpc_server_status_callback
  - test_grpc_server_custom_callback
  - test_grpc_server_statistics
  - test_grpc_server_state_conversion
  - test_grpc_server_is_running
  - test_grpc_server_multiple_cycles
  - test_grpc_server_null_checks
  - test_grpc_server_platform_info

**REST API Tests (18 тестов):**
- `testing/test_rest_api.c`
  - Тесты для всех endpoints

### Запуск тестов

```bash
# Все тесты
ctest --output-on-failure

# gRPC тесты
ctest -R grpc --output-on-failure

# REST API тесты
ctest -R rest-api --output-on-failure
```

---

## 📦 Сборка

### С gRPC API

```bash
mkdir build && cd build
cmake -DENABLE_GRPC=ON ..
make -j4
```

### Зависимости для gRPC

```bash
# Debian/Ubuntu
apt install libprotobuf-dev protobuf-compiler grpc-tools libgrpc++-dev

# Fedora/RHEL
dnf install protobuf-devel protobuf-compiler grpc-devel grpc-tools
```

### Без gRPC

```bash
mkdir build && cd build
cmake ..
make -j4
```

---

## 📊 Статистика релиза

### Файлы
| Категория | v1.0.28 | v1.0.29 | Изменение |
|-----------|---------|---------|-----------|
| **C/H файлов** | 412+ | 415+ | +3 |
| **Тестов** | 130 | 140 | +10 |
| **Документов** | 29 | 30 | +1 |
| **Grafana дашбордов** | 1 | 2 | +1 |
| **Панелей** | 8 | 24 | +16 |

### Код
| Метрика | Значение |
|---------|----------|
| **Строк кода** | ~1000 (gRPC server + тесты) |
| **Строк тестов** | ~400 |
| **Строк документации** | ~500 |

---

## 🔧 Изменения в API

### gRPC Service Definition

```protobuf
service MTProxyService {
  // Server management
  rpc StartServer(StartServerRequest) returns (ServerStatus);
  rpc StopServer(StopServerRequest) returns (ServerStatus);
  rpc GetServerStatus(GetServerStatusRequest) returns (ServerStatus);

  // Configuration
  rpc GetConfig(GetConfigRequest) returns (ProxyConfig);
  rpc UpdateConfig(UpdateConfigRequest) returns (ProxyConfig);

  // Statistics
  rpc GetStatistics(GetStatisticsRequest) returns (ProxyStatistics);
  rpc StreamStatistics(StreamStatisticsRequest) returns (stream ProxyStatistics);

  // Secrets
  rpc AddSecret(AddSecretRequest) returns (SecretResponse);
  rpc ListSecrets(ListSecretsRequest) returns (SecretList);

  // Rate limiting
  rpc GetRateLimits(GetRateLimitsRequest) returns (RateLimitConfig);

  // Logs
  rpc GetLogs(GetLogsRequest) returns (LogEntries);
  rpc StreamLogs(StreamLogsRequest) returns (stream LogEntry);
}
```

---

## 🐛 Исправления

- Улучшена обработка ошибок в callback функциях
- Исправлена совместимость с Windows для gRPC заглушки
- Оптимизировано использование памяти в REST API

---

## ⚠️ Breaking Changes

Нет. Все изменения обратно совместимы.

---

## 📝 Migration Guide

### Обновление с v1.0.28

1. **Обновите код:**
   ```bash
   git pull origin dev
   ```

2. **Пересоберите с gRPC (опционально):**
   ```bash
   cd build
   cmake -DENABLE_GRPC=ON ..
   make -j4
   ```

3. **Импортируйте новый Grafana дашборд:**
   - Откройте Grafana
   - Dashboards → Import
   - Загрузите `grafana/mtproxy-enhanced-dashboard.json`

4. **Настройте Prometheus алерты:**
   - Добавьте `prometheus/mtproxy-alerts.yml` в `rule_files`

---

## 🙏 Благодарности

Спасибо всем контрибьюторам за вклад в релиз v1.0.29!

---

## 📞 Поддержка

- **GitHub Issues:** [Сообщить о проблеме](https://github.com/QuadDarv1ne/MTProxy/issues)
- **Discussions:** [Обсуждения](https://github.com/QuadDarv1ne/MTProxy/discussions)
- **Telegram:** @Maestro7IT

---

*Release Notes v1.0.29 — 29 марта 2026 г.*
