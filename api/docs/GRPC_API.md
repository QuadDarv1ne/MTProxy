# gRPC API для MTProxy

## Обзор

MTProxy предоставляет **gRPC API** для высокопроизводительного удалённого управления сервером. gRPC обеспечивает:

- 🚀 Высокую производительность (HTTP/2 + Protobuf)
- 🔒 Типобезопасный контракт (protobufIDL)
- 🌍 Мультиязычность (генерация кода для 10+ языков)
- 📡 Bidirectional streaming
- 🔐 Встроенную аутентификацию

## Быстрый старт

### 1. Включение gRPC при сборке

```bash
# Сборка с поддержкой gRPC
mkdir build && cd build
cmake -DENABLE_GRPC=ON ..
make -j4
```

**Зависимости:**
```bash
# Debian/Ubuntu
apt install libprotobuf-dev protobuf-compiler grpc-tools libgrpc++-dev

# Fedora/RHEL
dnf install protobuf-devel protobuf-compiler grpc-devel grpc-tools

# macOS
brew install protobuf grpc
```

### 2. Генерация кода (автоматически в CMake)

```bash
# CMake автоматически генерирует код из .proto файла
# Исходный файл: api/protobuf/mtproxy.proto
# Генерируемые файлы: build/grpc_gen/mtproxy.pb.*, mtproxy.grpc.pb.*
```

### 3. Запуск с gRPC сервером

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
| `--grpc-no-reflection` | Отключить reflection | false |

### Конфигурация в файле

```ini
[grpc]
enabled = true
port = 50051
bind_address = 127.0.0.1
enable_tls = false
tls_cert_file = /path/to/cert.pem
tls_key_file = /path/to/key.pem
enable_auth = true
auth_token = your-secret-token
max_connections = 100
keepalive_seconds = 30
enable_reflection = true
```

## gRPC сервисы

### MTProxyService

Основной сервис управления прокси.

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
| `StreamStatistics` | Поток статистики (streaming) |
| `GetActiveConnections` | Активные подключения |

#### Методы управления секретами

| Метод | Описание |
|-------|----------|
| `AddSecret` | Добавить секрет |
| `RemoveSecret` | Удалить секрет |
| `ListSecrets` | Список секретов |

#### Методы rate limiting

| Метод | Описание |
|-------|----------|
| `GetRateLimits` | Получить настройки |
| `UpdateRateLimit` | Обновить настройки |

#### Методы логирования

| Метод | Описание |
|-------|----------|
| `GetLogs` | Получить логи |
| `StreamLogs` | Поток логов (streaming) |

## Примеры использования

### Python

**Установка:**
```bash
pip install grpcio grpcio-tools
python -m grpc_tools.protoc -Iapi/protobuf --python_out=. --grpc_python_out=. api/protobuf/mtproxy.proto
```

**Пример клиента:**
```python
import grpc
import mtproxy_pb2
import mtproxy_pb2_grpc

def main():
    # Подключение
    channel = grpc.insecure_channel('localhost:50051')
    stub = mtproxy_pb2_grpc.MTProxyServiceStub(channel)
    
    # Получить статус сервера
    status = stub.GetServerStatus(mtproxy_pb2.GetServerStatusRequest())
    print(f"Server state: {mtproxy_pb2.ServerStatus.State.Name(status.state)}")
    print(f"Version: {status.version}")
    print(f"Uptime: {status.uptime_seconds}s")
    
    # Получить статистику
    stats = stub.GetStatistics(mtproxy_pb2.GetStatisticsRequest())
    print(f"Active connections: {stats.active_connections}")
    print(f"CPU usage: {stats.cpu_usage_percent}%")
    print(f"Memory: {stats.memory_usage_mb:.2f} MB")
    
    # Запустить сервер
    response = stub.StartServer(mtproxy_pb2.StartServerRequest(force=False))
    print(f"Start result: {response.state}")
    
    # Streaming статистики
    for stat in stub.StreamStatistics(mtproxy_pb2.StreamStatisticsRequest(interval_seconds=5)):
        print(f"Stats update: {stat.active_connections} connections")

if __name__ == '__main__':
    main()
```

### Go

**Установка:**
```bash
go get google.golang.org/grpc
go get google.golang.org/protobuf
protoc --go_out=. --go_opt=paths=source_relative \
       --go-grpc_out=. --go-grpc_opt=paths=source_relative \
       api/protobuf/mtproxy.proto
```

**Пример клиента:**
```go
package main

import (
    "context"
    "fmt"
    "log"
    "time"

    "google.golang.org/grpc"
    pb "github.com/QuadDarv1ne/MTProxy/api/protobuf"
)

func main() {
    conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
    if err != nil {
        log.Fatal(err)
    }
    defer conn.Close()

    client := pb.NewMTProxyServiceClient(conn)
    ctx := context.Background()

    // Получить статус
    status, err := client.GetServerStatus(ctx, &pb.GetServerStatusRequest{})
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Server state: %s\n", status.State.String())
    fmt.Printf("Version: %s\n", status.Version)
    fmt.Printf("Uptime: %ds\n", status.UptimeSeconds)

    // Получить статистику
    stats, err := client.GetStatistics(ctx, &pb.GetStatisticsRequest{})
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Active connections: %d\n", stats.ActiveConnections)
    fmt.Printf("CPU usage: %.2f%%\n", stats.CpuUsagePercent)

    // Streaming статистики
    stream, err := client.StreamStatistics(ctx, &pb.StreamStatisticsRequest{IntervalSeconds: 5})
    if err != nil {
        log.Fatal(err)
    }
    for {
        stat, err := stream.Recv()
        if err != nil {
            log.Fatal(err)
        }
        fmt.Printf("Stats: %d connections, %.2f MB memory\n", 
                   stat.ActiveConnections, stat.MemoryUsageMb)
        time.Sleep(5 * time.Second)
    }
}
```

### Node.js (TypeScript)

**Установка:**
```bash
npm install @grpc/grpc-js @grpc/proto-loader
npx protoc --ts_out=. --proto_path=api/protobuf api/protobuf/mtproxy.proto
```

**Пример клиента:**
```typescript
import * as grpc from '@grpc/grpc-js';
import * as protoLoader from '@grpc/proto-loader';
import { MTProxyServiceClient } from './mtproxy';

const packageDefinition = protoLoader.loadSync('api/protobuf/mtproxy.proto', {});
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
const client = new MTProxyServiceClient('localhost:50051', grpc.credentials.createInsecure());

// Получить статус
client.getServerStatus({}, (err, status) => {
    if (err) {
        console.error(err);
        return;
    }
    console.log(`Server state: ${status.state}`);
    console.log(`Version: ${status.version}`);
});

// Получить статистику
client.getStatistics({}, (err, stats) => {
    if (err) {
        console.error(err);
        return;
    }
    console.log(`Active connections: ${stats.activeConnections}`);
    console.log(`CPU usage: ${stats.cpuUsagePercent}%`);
});
```

### Java

**Установка:**
```bash
# Maven зависимости
# io.grpc:grpc-netty-shaded
# io.grpc:grpc-protobuf
# io.grpc:grpc-stub
```

**Пример клиента:**
```java
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import com.mtproxy.api.v1.MTProxyServiceGrpc;
import com.mtproxy.api.v1.MtProxyProto.*;

public class MTProxyClient {
    public static void main(String[] args) throws Exception {
        ManagedChannel channel = ManagedChannelBuilder
            .forAddress("localhost", 50051)
            .usePlaintext()
            .build();

        MTProxyServiceGrpc.MTProxyServiceBlockingStub stub = 
            MTProxyServiceGrpc.newBlockingStub(channel);

        // Получить статус
        ServerStatus status = stub.getServerStatus(GetServerStatusRequest.getDefaultInstance());
        System.out.println("Server state: " + status.getState());
        System.out.println("Version: " + status.getVersion());

        // Получить статистику
        ProxyStatistics stats = stub.getStatistics(GetStatisticsRequest.getDefaultInstance());
        System.out.println("Active connections: " + stats.getActiveConnections());
        System.out.println("CPU usage: " + stats.getCpuUsagePercent() + "%");

        channel.shutdown();
    }
}
```

## Аутентификация

### Metadata аутентификация

```python
import grpc

# Создание metadata с токеном
metadata = [('x-api-key', 'your-secret-token')]

# Использование в запросе
response = stub.GetServerStatus(
    mtproxy_pb2.GetServerStatusRequest(),
    metadata=metadata
)
```

### TLS аутентификация

```python
import grpc

# Чтение сертификатов
with open('ca.crt', 'rb') as f:
    ca_cert = f.read()

credentials = grpc.ssl_channel_credentials(root_certificates=ca_cert)
channel = grpc.secure_channel('localhost:50051', credentials)
```

## Структуры данных

### ServerStatus

```protobuf
message ServerStatus {
  State state = 1;           // unknown, starting, running, stopping, stopped, error
  string version = 2;        // Версия сервера
  string commit_hash = 3;    // Git commit hash
  int64 uptime_seconds = 4;  // Время работы
  int64 start_time_unix = 5; // Время запуска (unix timestamp)
  string error_message = 6;  // Сообщение об ошибке
  PlatformInfo platform = 7; // Информация о платформе
}
```

### ProxyStatistics

```protobuf
message ProxyStatistics {
  int32 active_connections = 1;     // Активные подключения
  int64 total_connections = 2;      // Всего подключений
  int64 connections_per_second = 3; // Подключений в секунду
  int64 bytes_sent = 4;             // Отправлено байт
  int64 bytes_received = 5;         // Получено байт
  int64 bytes_per_second_in = 6;    // Байт/сек вход
  int64 bytes_per_second_out = 7;   // Байт/сек выход
  double cpu_usage_percent = 8;     // Использование CPU %
  int64 memory_usage_bytes = 9;     // Использование памяти (байты)
  double memory_usage_mb = 10;      // Использование памяти (MB)
  int64 total_errors = 11;          // Всего ошибок
  int64 errors_per_second = 12;     // Ошибок в секунду
  int64 rate_limited_requests = 13; // Rate limited запросов
  int64 timestamp_unix = 14;        // Timestamp
  int64 uptime_seconds = 15;        // Uptime
}
```

### ProxyConfig

```protobuf
message ProxyConfig {
  int32 port = 1;                // Порт
  bool ipv6_enabled = 2;         // IPv6
  int32 max_connections = 3;     // Макс. подключений
  int32 backlog = 4;             // Backlog
  repeated string secrets = 5;   // Секреты
  string log_file = 6;           // Файл логов
  string log_level = 7;          // Уровень логов
  int32 workers = 8;             // Количество workers
  bool use_aes_ni = 9;           // AES-NI
  RateLimitConfig rate_limit = 10;
  TLSConfig tls = 11;
}
```

## Обработка ошибок

### Коды ошибок gRPC

| Код | Описание |
|-----|----------|
| `OK` (0) | Успех |
| `INVALID_ARGUMENT` (3) | Неверный аргумент |
| `NOT_FOUND` (5) | Ресурс не найден |
| `ALREADY_EXISTS` (6) | Ресурс уже существует |
| `PERMISSION_DENIED` (7) | Нет прав доступа |
| `UNAUTHENTICATED` (16) | Не аутентифицирован |
| `INTERNAL` (13) | Внутренняя ошибка |
| `UNAVAILABLE` (14) | Сервис недоступен |

### Пример обработки ошибок

```python
import grpc

try:
    response = stub.StartServer(mtproxy_pb2.StartServerRequest())
except grpc.RpcError as e:
    if e.code() == grpc.StatusCode.UNAUTHENTICATED:
        print("Authentication failed")
    elif e.code() == grpc.StatusCode.UNAVAILABLE:
        print("Server unavailable")
    elif e.code() == grpc.StatusCode.INVALID_ARGUMENT:
        print("Invalid arguments")
    else:
        print(f"Error: {e.code()} - {e.details()}")
```

## Мониторинг с Prometheus

```python
from prometheus_client import Counter, Gauge, start_http_server
import grpc
import time

# Метрики
active_connections = Gauge('mtproxy_active_connections', 'Active connections')
bytes_sent = Counter('mtproxy_bytes_sent_total', 'Bytes sent')
bytes_received = Counter('mtproxy_bytes_received_total', 'Bytes received')
cpu_usage = Gauge('mtproxy_cpu_usage_percent', 'CPU usage %')

def collect_metrics():
    channel = grpc.insecure_channel('localhost:50051')
    stub = mtproxy_pb2_grpc.MTProxyServiceStub(channel)
    
    stats = stub.GetStatistics(mtproxy_pb2.GetStatisticsRequest())
    
    active_connections.set(stats.active_connections)
    bytes_sent.inc(stats.bytes_per_second_out)
    bytes_received.inc(stats.bytes_per_second_in)
    cpu_usage.set(stats.cpu_usage_percent)

start_http_server(8000)
while True:
    collect_metrics()
    time.sleep(5)
```

## Best Practices

### 1. Connection pooling

```python
# Создайте один канал и переиспользуйте его
channel = grpc.insecure_channel('localhost:50051')
stub = mtproxy_pb2_grpc.MTProxyServiceStub(channel)

# Не создавайте канал для каждого запроса!
```

### 2. Retry logic

```python
import grpc
import time

def retry_request(stub, request, max_retries=3):
    for attempt in range(max_retries):
        try:
            return stub.GetStatistics(request)
        except grpc.RpcError as e:
            if e.code() == grpc.StatusCode.UNAVAILABLE:
                time.sleep(2 ** attempt)  # Exponential backoff
            else:
                raise
    raise Exception("Max retries exceeded")
```

### 3. Timeouts

```python
from grpc import Timeout

# Установка timeout
response = stub.GetStatistics(
    mtproxy_pb2.GetStatisticsRequest(),
    timeout=5.0  # 5 секунд
)
```

### 4. Streaming

```python
# Streaming для реального времени
def watch_statistics():
    for stat in stub.StreamStatistics(
        mtproxy_pb2.StreamStatisticsRequest(interval_seconds=1)
    ):
        yield {
            'connections': stat.active_connections,
            'cpu': stat.cpu_usage_percent,
            'memory': stat.memory_usage_mb
        }
```

## Интеграция с существующими системами

### Интеграция с REST API

gRPC может работать параллельно с REST API:

```bash
# Запуск с обоими API
./mtproto-proxy \
  --grpc-port 50051 \
  --rest-port 8080 \
  -S <secret>
```

### Интеграция с admin-cli

```bash
# Admin CLI через gRPC
mtproxy-admin grpc://localhost:50051 status
mtproxy-admin grpc://localhost:50051 stats
```

## Тестирование

### Unit тесты

```bash
# Запуск тестов gRPC
ctest -R grpc --output-on-failure
```

### Интеграционные тесты

```bash
# Python интеграционные тесты
python testing/test_grpc_integration.py
```

### Load тесты

```bash
# Benchmark gRPC
./bin/benchmark-grpc --target localhost:50051 --requests 10000
```

## Troubleshooting

### Проблема: gRPC сервер не запускается

**Решение:**
```bash
# Проверьте, что порт свободен
netstat -tlnp | grep 50051

# Проверьте логи
tail -f /var/log/mtproxy.log
```

### Проблема: ошибка генерации кода

**Решение:**
```bash
# Переустановите protobuf и grpc
apt install --reinstall libprotobuf-dev protobuf-compiler grpc-tools

# Очистите build директорию
rm -rf build && mkdir build
```

### Проблема: ошибка аутентификации

**Решение:**
```bash
# Проверьте токен
echo $MTPOXY_GRPC_TOKEN

# Используйте metadata
metadata = [('x-api-key', 'your-token')]
```

## Ссылки

- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers Documentation](https://developers.google.com/protocol-buffers)
- [gRPC by Language](https://grpc.io/docs/languages/)
- [MTProxy API Reference](../API_REFERENCE.md)

---

*Версия gRPC API: 1.0.0*
*Последнее обновление: 29 марта 2026 г.*
