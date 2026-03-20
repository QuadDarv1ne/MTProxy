# MTProxy API Документация

## Обзор

MTProxy предоставляет два типа API для управления сервером:

1. **gRPC API** — высокопроизводительный RPC интерфейс
2. **REST API** — HTTP/JSON интерфейс для управления

## Быстрый старт

### REST API Примеры

**Получить статус сервера:**
```bash
curl -X GET http://localhost:8888/api/v1/server/status \
  -H "X-API-Key: your-api-key"
```

**Запустить сервер:**
```bash
curl -X POST http://localhost:8888/api/v1/server/start \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{"force": false}'
```

**Получить статистику:**
```bash
curl -X GET http://localhost:8888/api/v1/statistics \
  -H "X-API-Key: your-api-key"
```

**Добавить секрет:**
```bash
curl -X POST http://localhost:8888/api/v1/secrets \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{"secret": "0123456789abcdef...", "description": "My secret"}'
```

### gRPC Примеры (Go)

```go
package main

import (
    "context"
    "google.golang.org/grpc"
    pb "github.com/QuadDarv1ne/MTProxy/api/protobuf"
)

func main() {
    conn, _ := grpc.Dial("localhost:50051", grpc.WithInsecure())
    defer conn.Close()
    
    client := pb.NewMTProxyServiceClient(conn)
    
    // Получить статус
    status, _ := client.GetServerStatus(context.Background(), &pb.GetServerStatusRequest{})
    println("Server state:", status.State.String())
    
    // Получить статистику
    stats, _ := client.GetStatistics(context.Background(), &pb.GetStatisticsRequest{})
    println("Active connections:", stats.ActiveConnections)
}
```

## Аутентификация

Все API запросы требуют API ключ:

**REST API:**
```
X-API-Key: your-api-key
```

**gRPC:**
```go
// Добавить metadata с API ключом
md := metadata.Pairs("x-api-key", "your-api-key")
ctx := metadata.NewOutgoingContext(context.Background(), md)
```

## Endpoints

### Server Management

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/v1/server/start` | Запуск сервера |
| POST | `/api/v1/server/stop` | Остановка сервера |
| POST | `/api/v1/server/restart` | Перезапуск сервера |
| GET | `/api/v1/server/status` | Получить статус |

### Configuration

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/config` | Получить конфигурацию |
| PUT | `/api/v1/config` | Обновить конфигурацию |
| POST | `/api/v1/config/validate` | Валидировать конфигурацию |

### Statistics

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/statistics` | Получить статистику |
| GET | `/api/v1/statistics/stream` | Stream статистики (SSE) |

### Connections

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/connections` | Активные подключения |

### Secrets

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/secrets` | Список секретов |
| POST | `/api/v1/secrets` | Добавить секрет |
| DELETE | `/api/v1/secrets/{secret}` | Удалить секрет |

### Rate Limiting

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/ratelimit` | Получить настройки |
| PUT | `/api/v1/ratelimit` | Обновить настройки |

### Logs

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/logs` | Получить логи |
| GET | `/api/v1/logs/stream` | Stream логов (SSE) |

## Структуры данных

### ServerStatus

```json
{
  "state": "running",
  "version": "1.0.1",
  "commit_hash": "de8d674",
  "uptime_seconds": 3600,
  "start_time_unix": 1710936000,
  "error_message": "",
  "platform": {
    "os": "linux",
    "arch": "x64",
    "cpu_cores": 4,
    "total_memory_bytes": 8589934592
  }
}
```

### ProxyStatistics

```json
{
  "active_connections": 150,
  "total_connections": 10000,
  "connections_per_second": 50,
  "bytes_sent": 1000000000,
  "bytes_received": 500000000,
  "bytes_per_second_in": 1000000,
  "bytes_per_second_out": 2000000,
  "cpu_usage_percent": 25.5,
  "memory_usage_mb": 128.5,
  "total_errors": 10,
  "errors_per_second": 0,
  "rate_limited_requests": 5,
  "timestamp_unix": 1710939600,
  "uptime_seconds": 3600
}
```

## Server States

| State | Описание |
|-------|----------|
| `unknown` | Неизвестное состояние |
| `starting` | Сервер запускается |
| `running` | Сервер работает |
| `stopping` | Сервер останавливается |
| `stopped` | Сервер остановлен |
| `error` | Ошибка |

## Логирование уровней

| Level | Описание |
|-------|----------|
| `ERROR` | Критические ошибки |
| `WARN` | Предупреждения |
| `INFO` | Информационные сообщения |
| `DEBUG` | Отладочная информация |

## Генерация кода

### gRPC (Go)

```bash
protoc --go_out=. --go_opt=paths=source_relative \
       --go-grpc_out=. --go-grpc_opt=paths=source_relative \
       api/protobuf/mtproxy.proto
```

### gRPC (Python)

```bash
python -m grpc_tools.protoc -Iapi/protobuf \
  --python_out=. --grpc_python_out=. \
  api/protobuf/mtproxy.proto
```

### OpenAPI Client

```bash
# TypeScript
openapi-generator generate -i api/openapi/openapi.yaml -g typescript-axios -o ./client

# Python
openapi-generator generate -i api/openapi/openapi.yaml -g python -o ./client

# Go
openapi-generator generate -i api/openapi/openapi.yaml -g go -o ./client
```

## Rate Limiting API

API сам имеет rate limiting:
- 100 запросов в минуту
- 20 запросов в burst

Превышение лимита возвращает `429 Too Many Requests`.

## Ошибки

| Код | Описание |
|-----|----------|
| 400 | Неверный запрос |
| 401 | Неавторизован |
| 404 | Не найдено |
| 429 | Too Many Requests |
| 500 | Внутренняя ошибка |

## Примеры использования

### Мониторинг с Prometheus

```python
import requests
import time
from prometheus_client import Counter, Gauge, start_http_server

# Метрики
active_connections = Gauge('mtproxy_active_connections', 'Active connections')
bytes_sent = Counter('mtproxy_bytes_sent_total', 'Bytes sent')
bytes_received = Counter('mtproxy_bytes_received_total', 'Bytes received')
cpu_usage = Gauge('mtproxy_cpu_usage_percent', 'CPU usage %')

def collect_metrics():
    resp = requests.get('http://localhost:8888/api/v1/statistics',
                        headers={'X-API-Key': 'api-key'})
    stats = resp.json()
    
    active_connections.set(stats['active_connections'])
    bytes_sent.inc(stats['bytes_per_second_out'])
    bytes_received.inc(stats['bytes_per_second_in'])
    cpu_usage.set(stats['cpu_usage_percent'])

# Запуск сбора метрик
start_http_server(8000)
while True:
    collect_metrics()
    time.sleep(5)
```

### Автоматический рестарт при ошибке

```python
import requests
import time

API_KEY = 'your-api-key'
BASE_URL = 'http://localhost:8888/api/v1'

def check_and_restart():
    resp = requests.get(f'{BASE_URL}/server/status',
                        headers={'X-API-Key': API_KEY})
    status = resp.json()
    
    if status['state'] == 'error':
        print(f"Server error: {status['error_message']}")
        requests.post(f'{BASE_URL}/server/restart',
                      headers={'X-API-Key': API_KEY})
        print("Server restarted")

while True:
    check_and_restart()
    time.sleep(60)
```

---

*Версия API: 1.0.0*  
*Последнее обновление: 20 марта 2026 г.*
