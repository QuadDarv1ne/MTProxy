# Улучшения MTProxy — Март 2026

Этот документ описывает все улучшения, добавленные в проект MTProxy в рамках задач 4, 6, 7.

## 📋 Содержание

1. [Docker Multi-Arch образы](#1-docker-multi-arch-образы)
2. [CodeQL Security Analysis](#2-codeql-security-analysis)
3. [Audit Logging System](#3-audit-logging-system)
4. [REST API](#4-rest-api)
5. [Plugin System](#5-plugin-system)

---

## 1. Docker Multi-Arch образы

### Файлы
- `Dockerfile` — обновлен для multi-arch сборок
- `docker-compose.yml` — production-ready конфигурация

### Возможности

#### Multi-arch поддержка
- `linux/amd64` — x86_64 серверы
- `linux/arm64` — ARM серверы (Raspberry Pi, AWS Graviton)
- `linux/arm/v7` — ARM устройства

#### Multi-stage сборка
```dockerfile
# Stage 1: Builder
FROM alpine:3.19 AS builder
# Сборка с оптимизациями

# Stage 2: Runtime (minimal)
FROM alpine:3.19 AS runtime
# Только runtime зависимости

# Stage 3: Debug (optional)
FROM alpine:3.19 AS debug
# С инструментами отладки
```

#### Оптимизации
- LTO (Link-Time Optimization)
- Strip бинарников
- Минимальный runtime образ
- OCI метки

### Использование

#### Сборка образа
```bash
# Локальная сборка
docker build -t mtproxy:latest .

# Multi-arch сборка
docker buildx build \
  --platform linux/amd64,linux/arm64,linux/arm/v7 \
  -t mtproxy:latest \
  --push .
```

#### Запуск с docker-compose
```bash
# Базовый запуск
docker-compose up -d

# С мониторингом (Prometheus + Grafana)
docker-compose --profile monitoring up -d

# С admin интерфейсом
docker-compose --profile admin up -d
```

#### Переменные окружения
```bash
# .env файл
MT_PROXY_PORT=8080
MT_PROXY_SECRET=your_secret_here
MT_PROXY_WORKERS=4
MT_PROXY_MAX_CONNECTIONS=65535
MT_PROXY_RATE_LIMIT=1000

GRAFANA_PORT=3000
GRAFANA_ADMIN_USER=admin
GRAFANA_ADMIN_PASSWORD=secure_password

CPU_LIMIT=2.0
MEMORY_LIMIT=512M
```

### Production конфигурация

docker-compose.yml включает:
- **Security**: non-root пользователь, cap_drop, read-only fs
- **Monitoring**: Prometheus, Grafana, Alertmanager
- **Logging**: rotation логов, json формат
- **Health checks**: проверка здоровья сервисов
- **Resource limits**: CPU и память

---

## 2. CodeQL Security Analysis

### Файлы
- `.github/workflows/codeql-analysis.yml` — workflow для CodeQL

### Возможности

#### Автоматический анализ
- При каждом push/PR
- Еженедельный scheduled scan
- Анализ C/C++ и Python кода

#### Категории проверок
- Security-extended — расширенные проверки безопасности
- Safety-and-security — безопасность и надежность
- Custom queries — пользовательские запросы

#### Интеграция
- GitHub Security tab
- SARIF отчеты
- Artifact загрузка

### Результаты

Анализ находит:
- Buffer overflow
- Use after free
- Null pointer dereference
- Race conditions
- SQL injection (для Python)
- Path traversal

### Настройка

Для добавления пользовательских проверок:
```yaml
# .github/workflows/codeql-analysis.yml
queries: security-extended,safety-and-security
packs: codeql/c-queries,codeql/cpp-queries
```

---

## 3. Audit Logging System

### Файлы
- `common/audit-log.h` — заголовочный файл
- `common/audit-log.c` — реализация

### Возможности

#### Уровни аудита
| Уровень | Описание |
|---------|----------|
| `AUDIT_LEVEL_NONE` | Отключено |
| `AUDIT_LEVEL_CRITICAL` | Только критические события |
| `AUDIT_LEVEL_SECURITY` | События безопасности |
| `AUDIT_LEVEL_ACCESS` | Доступ + безопасность |
| `AUDIT_LEVEL_ALL` | Все события |

#### Категории событий
- Аутентификация и авторизация
- Контроль доступа
- Нарушения безопасности
- Rate limiting
- DDoS защита
- Изменения конфигурации
- Подключения/отключения
- Системные события

#### Форматы вывода
- **JSON** — для парсинга (SIEM системы)
- **Text** — человекочитаемый

#### Асинхронная запись
- Очередь событий (4096 по умолчанию)
- Отдельный поток записи
- Минимальное влияние на производительность

#### Rotation логов
- Автоматическая ротация по размеру
- Сжатие старых логов
- Хранение последних N файлов

### Использование

#### Инициализация
```c
audit_logger_t logger;
audit_config_t config = {0};

config.enabled = true;
config.level = AUDIT_LEVEL_SECURITY;
config.log_path = "/var/log/mtproxy/audit.log";
config.rotation_size_bytes = 100 * 1024 * 1024;  // 100MB
config.rotation_count = 10;
config.json_format = true;
config.async_mode = true;
config.queue_size = 4096;

audit_logger_init(&logger, &config);
audit_logger_start(&logger);
```

#### Логирование событий
```c
// Событие безопасности
audit_log_security(&logger, AUDIT_EVENT_SECURITY_VIOLATION,
                  "192.168.1.100", "user123",
                  "Invalid authentication token");

// Изменение конфигурации
audit_log_config_change(&logger, "max_connections",
                       "10000", "20000", "admin");

// Подключение клиента
audit_log_client_connect(&logger, client_fd,
                        "10.0.0.1", 443, "secret1");

// Форматированное сообщение
audit_log_event_fmt(&logger, AUDIT_EVENT_CONFIG_MODIFIED,
                   AUDIT_CATEGORY_CONFIG_CHANGE, AUDIT_LEVEL_INFO,
                   "Parameter '%s' changed from %s to %s",
                   param, old_val, new_val);
```

#### Получение статистики
```c
audit_stats_t stats;
audit_logger_get_stats(&logger, &stats);

printf("Total events: %lu\n", stats.total_events);
printf("Events logged: %lu\n", stats.events_logged);
printf("Events dropped: %lu\n", stats.events_dropped);
```

#### JSON вывод
```json
{
  "event_id": 12345,
  "event_type": "SECURITY_VIOLATION",
  "category": "SECURITY_VIOLATION",
  "level": "SECURITY",
  "timestamp": 1711324800000,
  "timestamp_iso": "2026-03-25T12:00:00.000Z",
  "hostname": "proxy-server-1",
  "process_name": "mtproto-proxy",
  "process_id": 1234,
  "thread_id": 9876,
  "message": "Invalid authentication token",
  "correlation_id": "abc123-def456",
  "user_id": "user123",
  "ip_address": "192.168.1.100",
  "port": 443,
  "session_id": "sess_789",
  "bytes_sent": 1024,
  "bytes_received": 2048,
  "duration_ms": 50,
  "result_code": 401
}
```

### Интеграция с SIEM

Audit логи можно отправлять в:
- **Splunk** — через syslog
- **ELK Stack** — через Filebeat
- **Graylog** — через GELF
- **QRadar** — через syslog

---

## 4. REST API

### Файлы
- `net/rest-api.h` — заголовочный файл
- `net/rest-api.c` — реализация

### Возможности

#### HTTP Endpoints

| Метод | Endpoint | Описание | Auth |
|-------|----------|----------|------|
| GET | `/api/v1/status` | Статус сервера | ❌ |
| GET | `/api/v1/stats` | Статистика | ✅ |
| GET | `/api/v1/config` | Конфигурация | ✅ |
| PUT | `/api/v1/config` | Обновление конфига | ✅ |
| GET | `/api/v1/connections` | Активные подключения | ✅ |
| POST | `/api/v1/connections/{id}` | Закрыть подключение | ✅ |
| GET | `/api/v1/secrets` | Список секретов | ✅ |
| POST | `/api/v1/secrets` | Добавить секрет | ✅ |
| DELETE | `/api/v1/secrets/{id}` | Удалить секрет | ✅ |
| GET | `/api/v1/metrics` | Prometheus метрики | ❌ |
| POST | `/api/v1/admin/reload` | Перезагрузка конфига | ✅ |
| POST | `/api/v1/admin/restart` | Перезапуск сервера | ✅ |
| POST | `/api/v1/admin/shutdown` | Остановка сервера | ✅ |

#### Аутентификация
- Bearer token аутентификация
- API ключи с правами доступа
- Rate limiting на запросы

#### Форматы
- JSON request/response
- Prometheus metrics
- CORS поддержка

### Использование

#### Запуск REST API
```c
rest_api_server_t server;
rest_api_config_t config = {0};

config.enabled = true;
config.port = 8080;
config.bind_address = "0.0.0.0";
config.enable_cors = true;
config.enable_auth = true;
config.auth_token = "your-secret-token";
config.enable_rate_limiting = true;
config.requests_per_minute = 60;
config.enable_logging = true;

rest_api_init(&server, &config);
rest_api_start(&server);
```

#### Регистрация маршрутов
```c
// Публичный endpoint
rest_api_register_route(&server, HTTP_METHOD_GET,
                       "/api/v1/status",
                       handle_api_status,
                       "Get server status");

// Endpoint с авторизацией
rest_api_register_route_auth(&server, HTTP_METHOD_GET,
                            "/api/v1/stats",
                            handle_api_stats,
                            "Get server statistics");
```

#### Примеры запросов

**GET /api/v1/status**
```bash
curl http://localhost:8080/api/v1/status
```

Ответ:
```json
{
  "status": "ok",
  "version": "1.0.0"
}
```

**GET /api/v1/stats**
```bash
curl -H "Authorization: Bearer your-token" \
     http://localhost:8080/api/v1/stats
```

**PUT /api/v1/config**
```bash
curl -X PUT \
  -H "Authorization: Bearer your-token" \
  -H "Content-Type: application/json" \
  -d '{"max_connections": 20000}' \
  http://localhost:8080/api/v1/config
```

**GET /api/v1/metrics (Prometheus)**
```bash
curl http://localhost:8080/api/v1/metrics
```

Ответ:
```
# HELP mtproxy_requests_total Total HTTP requests
# TYPE mtproxy_requests_total counter
mtproxy_requests_total 1234
mtproxy_connections_active 567
mtproxy_bytes_sent_total 12345678
mtproxy_bytes_received_total 87654321
```

### Интеграция с Prometheus

1. Настройте scrape config:
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/api/v1/metrics'
```

2. Импортируйте дашборд в Grafana

---

## 5. Plugin System

### Файлы
- `include/plugin-system.h` — API плагинов
- `plugins/example-logger.c` — пример плагина
- `docs/PLUGIN_SYSTEM.md` — документация

### Возможности

#### Динамическая загрузка
- Загрузка .so файлов
- Горячая перезагрузка
- Зависимости между плагинами

#### Точки расширения (20+)
- Подключения/отключения
- Получение/отправка данных
- MTProto handshake/encrypt/decrypt
- Проверки безопасности
- Rate limiting
- Конфигурация
- Статистика
- Логирование

#### Приоритеты
- Контроль порядка выполнения
- Высокий приоритет = раньше выполнение

#### Статистика
- Количество вызовов хуков
- Время выполнения
- Использование памяти

### Создание плагина

#### Минимальный плагин
```c
#include <plugin-system.h>

// Данные плагина
typedef struct {
    int counter;
} my_plugin_data_t;

// Хук
static plugin_result_t on_connect(plugin_hook_context_t *ctx, 
                                  void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    data->counter++;
    printf("Connection #%d from %s\n", data->counter, ctx->client_ip);
    return PLUGIN_OK;
}

// Информация
PLUGIN_DECLARE_INFO("my-plugin", "My plugin", "1.0.0", 
                    "Author", "MIT")

// Инициализация
int plugin_init(const plugin_config_t *config, void **plugin_data) {
    my_plugin_data_t *data = calloc(1, sizeof(my_plugin_data_t));
    *plugin_data = data;
    
    // Регистрация хука
    plugin_register_hook(HOOK_CONNECTION_ACCEPT, on_connect, 0, data);
    
    return PLUGIN_OK;
}

// Завершение
void plugin_shutdown(void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    printf("Total connections: %d\n", data->counter);
    free(data);
}
```

#### Компиляция
```bash
gcc -shared -fPIC -o my-plugin.so my-plugin.c
```

#### Установка
```bash
cp my-plugin.so /usr/lib/mtproxy/plugins/
```

#### Конфигурация
```ini
# /etc/mtproxy/plugins.conf
[plugins]
load = my-plugin.so

[my-plugin]
enabled = true
priority = 10
```

### Примеры плагинов

#### 1. Логирование подключений
См. `plugins/example-logger.c`

#### 2. Блокировка по IP
```c
static plugin_result_t on_security_check(plugin_hook_context_t *ctx, 
                                         void *plugin_data) {
    if (is_blocked_ip(ctx->client_ip)) {
        ctx->result_code = 403;
        return PLUGIN_REJECT;
    }
    return PLUGIN_OK;
}
```

#### 3. Rate Limiting
```c
static plugin_result_t on_rate_limit_check(plugin_hook_context_t *ctx, 
                                           void *plugin_data) {
    if (!rate_limiter_allow(ctx->client_ip)) {
        ctx->result_code = 429;
        return PLUGIN_REJECT;
    }
    return PLUGIN_OK;
}
```

#### 4. Модификация данных
```c
static plugin_result_t on_data_received(plugin_hook_context_t *ctx, 
                                        void *plugin_data) {
    // Добавить префикс
    unsigned char *new_data = malloc(ctx->data_size + 4);
    memcpy(new_data, "\x00\x00\x00\x00", 4);
    memcpy(new_data + 4, ctx->data, ctx->data_size);
    
    ctx->data = new_data;
    ctx->data_size += 4;
    
    return PLUGIN_OK;
}
```

### API Менеджера

```c
// Инициализация
plugin_manager_t manager;
plugin_manager_config_t config = {0};
strncpy(config.plugin_dir, "/usr/lib/mtproxy/plugins/", 511);
plugin_manager_init(&manager, &config);

// Загрузка всех плагинов
plugin_manager_load_all(&manager);

// Выполнение хука
plugin_hook_context_t ctx = {0};
ctx.hook_type = HOOK_CONNECTION_ACCEPT;
ctx.connection_fd = client_fd;
plugin_manager_execute_hook(&manager, HOOK_CONNECTION_ACCEPT, &ctx);

// Статистика
plugin_manager_stats_t stats;
plugin_manager_get_stats(&manager, &stats);
```

---

## 📊 Сводка изменений

| Компонент | Файлов | Строк кода | Описание |
|-----------|--------|------------|----------|
| Docker | 2 | ~400 | Multi-arch образы |
| CodeQL | 1 | ~80 | Security анализ |
| Audit Log | 2 | ~800 | Логирование событий |
| REST API | 2 | ~900 | HTTP API |
| Plugin System | 3 | ~700 | Система плагинов |
| **Итого** | **10** | **~2880** | |

---

## 🚀 Быстрый старт

### 1. Сборка Docker образа
```bash
docker build -t mtproxy:latest .
```

### 2. Запуск с мониторингом
```bash
docker-compose --profile monitoring up -d
```

### 3. Проверка REST API
```bash
curl http://localhost:8080/api/v1/status
curl http://localhost:8080/api/v1/metrics
```

### 4. Включение audit logging
```c
audit_config_t config = {0};
config.enabled = true;
config.level = AUDIT_LEVEL_SECURITY;
config.log_path = "/var/log/mtproxy/audit.log";
config.json_format = true;
```

### 5. Загрузка плагина
```bash
cp plugins/example-logger.so /usr/lib/mtproxy/plugins/
echo "load = example-logger.so" >> /etc/mtproxy/plugins.conf
```

---

## 📚 Дополнительная документация

- `docs/PLUGIN_SYSTEM.md` — Полная документация по плагинам
- `common/audit-log.h` — API audit logging
- `net/rest-api.h` — API REST API
- `include/plugin-system.h` — API Plugin System

---

*Документ создан: 25 марта 2026 г.*
*Версия: 1.0*
