# MTProxy Monitoring and API Guide

Полное руководство по мониторингу, веб-интерфейсу и API MTProxy.

---

# Часть 1: Web UI (Веб-интерфейс)

## Обзор

Web UI предоставляет удобный интерфейс для:
- 📊 Мониторинга статистики сервера в реальном времени
- ⚙️ Управления конфигурацией
- 🔑 Управления секретами
- 📝 Просмотра логов
- 🚨 Отслеживания алертов

## Быстрый старт

### Запуск

Web UI автоматически обслуживается встроенным HTTP сервером MTProxy:

```bash
# Запуск с REST API
./mtproto-proxy --rest-port 8080 -S <secret>
```

### Доступ

Откройте браузер:
```
http://localhost:8080
```

Или для удалённого доступа:
```
http://<server-ip>:8080
```

## Функции

### Dashboard

**Панели статистики:**
- Статус сервера (Running/Stopped/Error)
- Активные подключения
- Трафик (входящий/исходящий)
- Использование CPU и памяти

**Графики:**
- Подключения over time
- Трафик over time

**Автообновление:** каждые 5 секунд

### Конфигурация

**Управление настройками:**
- Порт сервера
- Максимальное количество подключений
- IPv6 (включить/выключить)
- Уровень логирования
- Количество workers

**Действия:**
- Сохранить конфигурацию
- Перезагрузить конфигурацию

### Секреты

**Управление:**
- Просмотр списка секретов
- Добавление нового секрета
- Удаление секрета
- Копирование в буфер обмена

### Логи

**Фильтрация:**
- По уровню (ERROR, WARNING, INFO, DEBUG)
- По времени

## Настройка

### API Key

Для доступа к API необходим ключ аутентификации.

**Способ 1: Через localStorage:**
```javascript
localStorage.setItem('mtproxy_api_key', 'your-api-key');
```

### REST API Endpoints

| Endpoint | Метод | Описание |
|----------|-------|----------|
| `/api/v1/server/status` | GET | Статус сервера |
| `/api/v1/statistics` | GET | Статистика |
| `/api/v1/config` | GET/PUT | Конфигурация |
| `/api/v1/secrets` | GET/POST/DELETE | Секреты |
| `/api/v1/logs` | GET | Логи |

## Безопасность

### Рекомендации

1. **Используйте HTTPS** в production:
   ```bash
   # Настройте reverse proxy с SSL
   nginx + Let's Encrypt
   ```

2. **Ограничьте доступ** по IP:
   ```nginx
   location / {
       allow 192.168.1.0/24;
       deny all;
   }
   ```

3. **Используйте API ключи** для аутентификации

4. **Не открывайте Web UI** в публичный интернет без защиты

## Troubleshooting

### Web UI не загружается

**Проверьте:**
1. Запущен ли MTProxy с REST API
2. Правильно ли указан порт
3. Доступен ли порт (firewall)

```bash
# Проверка REST API
curl http://localhost:8080/api/v1/server/status
```

### Ошибка аутентификации

**Решение:**
1. Проверьте API ключ
2. Убедитесь что ключ передан в заголовке `X-API-Key`

---

# Часть 2: Grafana Monitoring

## Обзор

MTProxy предоставляет Grafana дашборды для мониторинга:

**Доступные дашборды:**
- `mtproxy-dashboard.json` — базовый (8 панелей)
- `mtproxy-enhanced-dashboard.json` — расширенный (16 панелей)

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

scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:9090']
```

### 3. Импорт в Grafana

1. Откройте Grafana (http://localhost:3000)
2. Dashboards → Import
3. Загрузите `grafana/mtproxy-dashboard.json`
4. Выберите Prometheus datasource
5. Нажмите Import

## Метрики

### Server Status
- `mtproxy_info` — Информация о сервере
- `mtproxy_uptime_seconds` — Время работы

### Connections
- `mtproxy_active_connections` — Активные подключения
- `mtproxy_total_connections_total` — Всего подключений
- `mtproxy_connections_per_second` — Подключений в секунду

### Network Traffic
- `mtproxy_bytes_sent_total` — Отправлено байт
- `mtproxy_bytes_received_total` — Получено байт
- `mtproxy_bytes_per_second_in/out` — Трафик в секунду

### Resources
- `mtproxy_cpu_usage_percent` — Использование CPU
- `mtproxy_memory_usage_bytes` — Использование памяти

### Cache Metrics
- `mtproxy_cache_hits_total` — Попадания в кэш
- `mtproxy_cache_misses_total` — Промахи кэша
- `mtproxy_cache_hit_rate` — Процент попаданий

### Rate Limiting
- `mtproxy_ratelimit_active_rules` — Активных правил
- `mtproxy_ratelimit_blocked_ips` — Заблокированных IP

### gRPC API
- `mtproxy_grpc_requests_total` — Всего gRPC запросов
- `mtproxy_grpc_active_streams` — Активные стримы

## Алёртинг

### Критические алерты

| Алерт | Условие | Severity |
|-------|---------|----------|
| MTProxyServerDown | state="error" > 1m | Critical |
| MTProxyCriticalCPUUsage | CPU > 95% > 2m | Critical |
| MTProxyCriticalErrorRate | errors > 50/s > 1m | Critical |

### Предупреждения

| Алерт | Условие | Severity |
|-------|---------|----------|
| MTProxyHighCPUUsage | CPU > 80% > 5m | Warning |
| MTProxyHighErrorRate | errors > 10/s > 2m | Warning |
| MTProxyLowCacheHitRate | hit_rate < 50% > 10m | Warning |

### Примеры PromQL

```promql
# Server Down
mtproxy_info{state="error"} == 1

# High CPU Usage
mtproxy_cpu_usage_percent > 80

# High Error Rate
rate(mtproxy_errors_total[1m]) > 10

# Cache Hit Rate
rate(mtproxy_cache_hits_total[1m]) * 100 / 
  (rate(mtproxy_cache_hits_total[1m]) + rate(mtproxy_cache_misses_total[1m]))
```

## Docker Compose (полный стек)

```yaml
version: '3.8'

services:
  mtproxy:
    image: mtproxy:latest
    ports:
      - "443:443"
      - "8888:8888"

  prometheus-exporter:
    image: python:3.11-slim
    command: python prometheus_exporter.py --host mtproxy --port 8888
    ports:
      - "9090:9090"

  prometheus:
    image: prom/prometheus:latest
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9091:9090"

  grafana:
    image: grafana/grafana:latest
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    ports:
      - "3000:3000"
```

## Troubleshooting

### Нет данных в дашборде

1. Проверьте Prometheus exporter:
   ```bash
   curl http://localhost:9090/metrics
   ```

2. Проверьте скрапинг:
   ```bash
   curl http://localhost:9091/api/v1/targets
   ```

---

# Часть 3: gRPC API

## Обзор

MTProxy предоставляет **gRPC API** для высокопроизводительного удалённого управления:

- 🚀 Высокая производительность (HTTP/2 + Protobuf)
- 🔒 Типобезопасный контракт
- 🌍 Мультиязычность (10+ языков)
- 📡 Bidirectional streaming
- 🔐 Встроенная аутентификация

## Быстрый старт

### 1. Включение gRPC при сборке

```bash
mkdir build && cd build
cmake -DENABLE_GRPC=ON ..
make -j4
```

**Зависимости:**
```bash
# Debian/Ubuntu
apt install libprotobuf-dev protobuf-compiler grpc-tools libgrpc++-dev
```

### 2. Запуск с gRPC сервером

```bash
./bin/mtproto-proxy --grpc-port 50051 -S <secret>
```

## Конфигурация

### Параметры командной строки

| Параметр | Описание | По умолчанию |
|----------|----------|--------------|
| `--grpc-port` | Порт gRPC сервера | 50051 |
| `--grpc-bind` | Адрес для bind | 0.0.0.0 |
| `--grpc-tls` | Включить TLS | false |
| `--grpc-auth` | Токен аутентификации | - |

### Конфигурация в файле

```ini
[grpc]
enabled = true
port = 50051
bind_address = 127.0.0.1
enable_tls = false
auth_token = your-secret-token
```

## gRPC сервисы

### MTProxyService

#### Методы управления сервером

| Метод | Описание |
|-------|----------|
| `StartServer` | Запуск сервера |
| `StopServer` | Остановка сервера |
| `RestartServer` | Перезапуск сервера |
| `GetServerStatus` | Получить статус |

#### Методы конфигурации

| Метод | Описание |
|-------|----------|
| `GetConfig` | Получить конфигурацию |
| `UpdateConfig` | Обновить конфигурацию |
| `ValidateConfig` | Валидировать конфигурацию |

#### Методы статистики

| Метод | Описание |
|-------|----------|
| `GetStatistics` | Получить статистику |
| `GetMetrics` | Получить метрики |
| `ResetStatistics` | Сбросить статистику |

#### Методы секретов

| Метод | Описание |
|-------|----------|
| `ListSecrets` | Список секретов |
| `AddSecret` | Добавить секрет |
| `RemoveSecret` | Удалить секрет |

## Примеры использования

### Python клиент

```python
import grpc
import mtproxy_pb2
import mtproxy_pb2_grpc

# Подключение
channel = grpc.insecure_channel('localhost:50051')
stub = mtproxy_pb2_grpc.MTProxyServiceStub(channel)

# Получение статуса
response = stub.GetServerStatus(mtproxy_pb2.Empty())
print(f"Status: {response.status}")
print(f"Uptime: {response.uptime_seconds}s")
print(f"Connections: {response.active_connections}")

# Обновление конфигурации
config = mtproxy_pb2.ConfigUpdate(
    key="network.port",
    value="8889"
)
response = stub.UpdateConfig(config)
print(f"Config updated: {response.success}")
```

### Go клиент

```go
package main

import (
    "context"
    "google.golang.org/grpc"
    pb "github.com/mtproxy/grpc_gen"
)

func main() {
    conn, _ := grpc.Dial("localhost:50051", grpc.WithInsecure())
    client := pb.NewMTProxyServiceClient(conn)

    // Статус
    status, _ := client.GetServerStatus(context.Background(), &pb.Empty{})
    println("Status:", status.Status)
    println("Connections:", status.ActiveConnections)
}
```

### Java клиент

```java
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import com.mtproxy.grpc.MTProxyServiceGrpc;
import com.mtproxy.grpc.MTProxyOuterClass.*;

public class MTProxyClient {
    public static void main(String[] args) {
        ManagedChannel channel = ManagedChannelBuilder
            .forTarget("localhost:50051")
            .usePlaintext()
            .build();

        MTProxyServiceGrpc.MTProxyServiceBlockingStub stub =
            MTProxyServiceGrpc.newBlockingStub(channel);

        ServerStatus status = stub.getServerStatus(Empty.newBuilder().build());
        System.out.println("Status: " + status.getStatus());
        System.out.println("Connections: " + status.getActiveConnections());
    }
}
```

## Безопасность

### TLS шифрование

```bash
# Генерация сертификатов
openssl req -x509 -nodes -newkey rsa:4096 \
  -keyout server.key -out server.crt -days 365

# Запуск с TLS
./bin/mtproto-proxy --grpc-port 50051 \
  --grpc-tls \
  --grpc-tls-cert server.crt \
  --grpc-tls-key server.key
```

### Аутентификация

```bash
# Запуск с токеном
./bin/mtproto-proxy --grpc-auth your-secret-token
```

**Python клиент с аутентификацией:**
```python
metadata = [('authorization', 'Bearer your-secret-token')]
response = stub.GetServerStatus(mtproxy_pb2.Empty(), metadata=metadata)
```

## Troubleshooting

### Ошибка подключения

**Проверьте:**
1. Запущен ли gRPC сервер
2. Правильный ли порт
3. Firewall правила

```bash
# Проверка порта
netstat -tlnp | grep 50051

# Проверка с grpcurl
grpcurl -plaintext localhost:50051 list
```

### Ошибка аутентификации

**Решение:**
1. Проверьте токен
2. Убедитесь что токен передан в metadata

---

# Дополнительные ресурсы

- [API_REFERENCE.md](../API_REFERENCE.md) — REST API справочник
- [DEPLOYMENT.md](../DEPLOYMENT.md) — Развёртывание в production
- [SECURITY.md](../SECURITY.md) — Безопасность
- [CLI_GUIDES.md](CLI_GUIDES.md) — Руководство по CLI
