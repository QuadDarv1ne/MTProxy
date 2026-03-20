# MTProxy Grafana Dashboard

## Обзор

Этот каталог содержит Grafana дашборд для мониторинга MTProxy.

## Установка

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
scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:9090']
    scrape_interval: 5s
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

### Network Traffic
- **mtproxy_bytes_sent_total** — Отправлено байт (counter)
- **mtproxy_bytes_received_total** — Получено байт (counter)
- **mtproxy_bytes_per_second_in** — Байт/сек входящий трафик
- **mtproxy_bytes_per_second_out** — Байт/сек исходящий трафик

### Resources
- **mtproxy_cpu_usage_percent** — Использование CPU %
- **mtproxy_memory_usage_bytes** — Использование памяти

### Errors
- **mtproxy_errors_total** — Всего ошибок
- **mtproxy_errors_per_second** — Ошибок в секунду
- **mtproxy_rate_limited_total** — Rate limited запросов

## Дашборд панели

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

## Скриншот

![MTProxy Dashboard](dashboard-screenshot.png)

## Алёртинг

Примеры алертов для настройки:

### Server Down
```promql
mtproxy_info{state="error"} == 1
```
**For:** 1m  
**Severity:** Critical

### High CPU Usage
```promql
mtproxy_cpu_usage_percent > 80
```
**For:** 5m  
**Severity:** Warning

### High Error Rate
```promql
rate(mtproxy_errors_total[1m]) > 10
```
**For:** 2m  
**Severity:** Warning

### Too Many Connections
```promql
mtproxy_active_connections > 10000
```
**For:** 5m  
**Severity:** Warning

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

*Версия: 1.0.0*  
*Последнее обновление: 20 марта 2026 г.*
