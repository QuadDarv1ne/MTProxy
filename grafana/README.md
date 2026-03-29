# MTProxy Grafana Dashboard

## Обзор

Этот каталог содержит Grafana дашборды для мониторинга MTProxy.

**Доступные дашборды:**
- `mtproxy-dashboard.json` — базовый дашборд (8 панелей)
- `mtproxy-enhanced-dashboard.json` — расширенный дашборд (16 панелей) ✅ NEW

## Быстрый старт

### 1. Запуск Prometheus Exporter

```bash
# Установка зависимостей
pip install prometheus-client requests

# Запуск экспортёра
python scripts/prometheus_exporter.py \
  --host localhost \
  --port 8888 \
  --api-key your-api-key \
  --exporter-port 9090
```

### 2. Настройка Prometheus

Добавьте в `prometheus.yml`:

```yaml
global:
  scrape_interval: 5s
  evaluation_interval: 5s

alerting:
  alertmanagers:
    - static_configs:
        - targets: ['localhost:9093']

rule_files:
  - "mtproxy-alerts.yml"

scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:9090']
    scrape_interval: 5s
    metrics_path: /metrics
```

### 3. Импорт дашборда в Grafana

1. Откройте Grafana (http://localhost:3000)
2. Перейдите в **Dashboards** → **Import**
3. Загрузите файл `grafana/mtproxy-dashboard.json`
4. Выберите Prometheus datasource
5. Нажмите **Import**

## Метрики

### Server Status
- **mtproxy_info** — Информация о сервере (version, state, platform)
- **mtproxy_uptime_seconds** — Время работы

### Connections
- **mtproxy_active_connections** — Активные подключения
- **mtproxy_total_connections_total** — Всего подключений
- **mtproxy_connections_per_second** — Подключений в секунду
- **mtproxy_connections_by_status** — Подключения по статусам (active, idle, closed, rejected)

### Network Traffic
- **mtproxy_bytes_sent_total** — Отправлено байт (counter)
- **mtproxy_bytes_received_total** — Получено байт (counter)
- **mtproxy_bytes_per_second_in** — Байт/сек входящий трафик
- **mtproxy_bytes_per_second_out** — Байт/сек исходящий трафик

### Resources
- **mtproxy_cpu_usage_percent** — Использование CPU %
- **mtproxy_memory_usage_bytes** — Использование памяти

### Cache Metrics (NEW)
- **mtproxy_cache_hits_total** — Всего попаданий в кэш
- **mtproxy_cache_misses_total** — Всего промахов кэша
- **mtproxy_cache_hit_rate** — Процент попаданий в кэш
- **mtproxy_cache_entries** — Количество записей в кэше
- **mtproxy_cache_memory_bytes** — Использование памяти кэшем
- **mtproxy_cache_evictions_total** — Всего вытеснений из кэша

### Rate Limiting Metrics (NEW)
- **mtproxy_ratelimit_active_rules** — Количество активных правил
- **mtproxy_ratelimit_blocked_ips** — Количество заблокированных IP
- **mtproxy_ratelimit_whitelisted_ips** — Количество IP в белом списке
- **mtproxy_rate_limited_total** — Всего rate limited запросов

### Errors
- **mtproxy_errors_total** — Всего ошибок
- **mtproxy_errors_per_second** — Ошибок в секунду

## Дашборд панели

### Базовый дашборд (8 панелей)

| Панель | Описание |
|--------|----------|
| Server Status | Текущий статус сервера (Running/Stopped/Error) |
| Uptime | Время работы сервера |
| Active Connections | Количество активных подключений |
| Memory Usage | Использование памяти |
| Connections | График подключений (active + per second) |
| Network Traffic | График сетевого трафика (in/out) |
| CPU Usage | График использования CPU |
| Errors & Rate Limiting | График ошибок и rate limiting |

### Расширенный дашборд (16 панелей) ✅ NEW

Дополнительно к базовым панелям:

| Панель | Описание |
|--------|----------|
| Cache Performance | Hit rate, hit/miss ratio с порогами |
| Cache Statistics | Entries, evictions/sec, memory usage |
| Rate Limiting | Active rules, blocked IPs, whitelisted IPs |
| gRPC API Performance | Requests/sec, errors/sec, active streams |
| Secrets Management | Total secrets, secrets in use |
| gRPC API Latency | p50, p95, p99 latency percentiles |
| Connections by Status | Active, idle, closed, rejected connections |
| Total Connections History | Total connections + max limit |

## Новые метрики для расширенного дашборда

### gRPC API Metrics (NEW)
- **mtproxy_grpc_requests_total** — Всего gRPC запросов
- **mtproxy_grpc_errors_total** — Всего gRPC ошибок
- **mtproxy_grpc_active_streams** — Активные gRPC стримы
- **mtproxy_grpc_request_duration_seconds** — Duration histogram для latency

### Secrets Metrics (NEW)
- **mtproxy_secrets_count** — Общее количество секретов
- **mtproxy_secrets_in_use** — Количество используемых секретов

### Connection Limits (NEW)
- **mtproxy_max_connections_limit** — Максимальный лимит подключений

## Скриншот

![MTProxy Dashboard](dashboard-screenshot.png)

## Алёртинг

Полный список правил алертинга находится в `prometheus/mtproxy-alerts.yml`.

### Критические алерты

| Алерт | Условие | Severity |
|-------|---------|----------|
| MTProxyServerDown | state="error" > 1m | Critical |
| MTProxyCriticalCPUUsage | CPU > 95% > 2m | Critical |
| MTProxyCriticalErrorRate | errors > 50/s > 1m | Critical |
| MTProxyPossibleAttack | rate_limited > 500/s > 2m | Critical |

### Предупреждения

| Алерт | Условие | Severity |
|-------|---------|----------|
| MTProxyHighCPUUsage | CPU > 80% > 5m | Warning |
| MTProxyHighConnectionCount | connections > 10000 > 5m | Warning |
| MTProxyHighErrorRate | errors > 10/s > 2m | Warning |
| MTProxyLowCacheHitRate | hit_rate < 50% > 10m | Warning |
| MTProxyHighRateLimiting | rate_limited > 100/s > 5m | Warning |

### Примеры PromQL запросов

```promql
# Server Down
mtproxy_info{state="error"} == 1

# High CPU Usage
mtproxy_cpu_usage_percent > 80

# High Error Rate
rate(mtproxy_errors_total[1m]) > 10

# Too Many Connections
mtproxy_active_connections > 10000

# Low Cache Hit Rate
mtproxy_cache_hit_rate < 50

# Possible DDoS Attack
rate(mtproxy_rate_limited_total[1m]) > 500

# gRPC API Performance (NEW)
rate(mtproxy_grpc_requests_total[1m])
rate(mtproxy_grpc_errors_total[1m])
mtproxy_grpc_active_streams

# gRPC API Latency (NEW)
histogram_quantile(0.50, rate(mtproxy_grpc_request_duration_seconds_bucket[5m]))
histogram_quantile(0.95, rate(mtproxy_grpc_request_duration_seconds_bucket[5m]))
histogram_quantile(0.99, rate(mtproxy_grpc_request_duration_seconds_bucket[5m]))

# Cache Hit Rate calculation
rate(mtproxy_cache_hits_total[1m]) * 100 / (rate(mtproxy_cache_hits_total[1m]) + rate(mtproxy_cache_misses_total[1m]))

# Secrets usage
mtproxy_secrets_count
mtproxy_secrets_in_use

# Connection limit percentage
mtproxy_active_connections / mtproxy_max_connections_limit * 100

# Connections by status
mtproxy_connections_by_status{status="active"}
mtproxy_connections_by_status{status="idle"}
mtproxy_connections_by_status{status="closed"}
mtproxy_connections_by_status{status="rejected"}
```

## Docker Compose (полный стек)

```yaml
version: '3.8'

services:
  mtproxy:
    image: mtproxy:latest
    ports:
      - "443:443"
      - "8888:8888"  # API port
    volumes:
      - ./config:/etc/mtproxy

  prometheus-exporter:
    image: python:3.11-slim
    working_dir: /app
    volumes:
      - ./scripts:/app
    command: >
      python prometheus_exporter.py
      --host mtproxy
      --port 8888
      --api-key your-api-key
      --exporter-port 9090
    ports:
      - "9090:9090"

  prometheus:
    image: prom/prometheus:latest
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    ports:
      - "9091:9090"

  grafana:
    image: grafana/grafana:latest
    volumes:
      - grafana_data:/var/lib/grafana
      - ./grafana:/etc/grafana/provisioning/dashboards
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    ports:
      - "3000:3000"

volumes:
  prometheus_data:
  grafana_data:
```

## Troubleshooting

### Нет данных в дашборде

1. Проверьте что Prometheus exporter запущен:
   ```bash
   curl http://localhost:9090/metrics
   ```

2. Проверьте что Prometheus скрапит метрики:
   ```bash
   curl http://localhost:9091/api/v1/targets
   ```

3. Проверьте API ключ:
   ```bash
   curl -H "X-API-Key: your-api-key" http://localhost:8888/api/v1/server/status
   ```

### Ошибки подключения

- Убедитесь что MTProxy API доступен на порту 8888
- Проверьте firewall правила
- Проверьте что API ключ правильный

---

*Версия: 2.0.0*
*Последнее обновление: 29 марта 2026 г. — Enhanced Dashboard (16 панелей)*
