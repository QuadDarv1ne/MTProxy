# MTProxy v1.0.30 Release Notes

**Дата выпуска:** 29 марта 2026 г.
**Версия:** 1.0.30
**Статус:** ✅ Released

---

## 🎉 Обзор релиза

v1.0.30 — крупный релиз, завершающий задачи **Q4 2026**. Добавлены системы для **кластеризации**, **балансировки нагрузки**, **мониторинга** и **управления** MTProxy серверами.

### Ключевые возможности

- 🖥️ **CLI утилита** — кроссплатформенное управление через командную строку
- 🚨 **Alert Manager** — уведомления через Telegram, Email, Slack
- 🏥 **Health Check** — проверка здоровья узлов (6 типов проверок)
- 🌐 **Cluster Manager** — управление кластером с leader election и failover
- ⚖️ **Load Balancer** — балансировка нагрузки (6 алгоритмов)

---

## ✨ Новые возможности

### 1. CLI утилита (mtproxy-cli)

**Файлы:**
- `cli/mtcli.c/h` — реализация CLI (600+ строк)
- `cli/mtcli.h` — заголовочный файл
- `docs/CLI_GUIDE.md` — руководство пользователя

**Команды:**
| Команда | Описание |
|---------|----------|
| `status` | Показать статус сервера |
| `stats` | Показать статистику |
| `config get/set` | Управление конфигурацией |
| `secrets list/add/remove` | Управление секретами |
| `logs` | Просмотр логов |
| `connections` | Управление подключениями |
| `ratelimit` | Управление rate limiting |
| `health` | Проверка здоровья |
| `metrics` | Экспорт метрик |
| `reload/restart/stop` | Управление сервером |

**Пример использования:**
```bash
# Проверка статуса
mtproxy-cli status

# Статистика в JSON
mtproxy-cli stats --json

# Управление секретами
mtproxy-cli secrets add --secret "dd1234567890abcdef" --desc "My proxy"

# Просмотр логов
mtproxy-cli logs --level error --tail 50
```

### 2. Alert Manager

**Файлы:**
- `system/monitoring/alert-manager.c/h` — реализация (800+ строк)
- `testing/test_alert_manager.c` — 25 тестов

**Каналы уведомлений:**
- **Telegram** — Bot API с emoji и Markdown
- **Email** — SMTP отправка
- **Slack** — Webhook с attachments
- **Webhook** — JSON payload
- **Custom** — callback функция

**Уровни алертов:**
- `DEBUG` — отладочные сообщения
- `INFO` — информационные
- `WARNING` — предупреждения
- `ERROR` — ошибки
- `CRITICAL` — критические

**Встроенные алерты:**
- `alert_server_down/up` — статус сервера
- `alert_high_cpu` — высокий CPU
- `alert_high_memory` — высокая память
- `alert_high_connections` — много подключений
- `alert_rate_limit_exceeded` — превышен rate limit
- `alert_security_event` — проблема безопасности

**Пример использования:**
```c
alert_manager_init();
alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");

alert_high_cpu(95.5, 90.0);  // Отправит алерт в Telegram
```

### 3. Health Check System

**Файлы:**
- `system/monitoring/health-check.c/h` — реализация (900+ строк)
- `testing/test_health_check.c` — 20 тестов

**Типы проверок:**
- **HTTP/HTTPS** — проверка HTTP endpoint
- **TCP** — проверка TCP порта
- **Process** — проверка процесса по PID
- **Memory** — использование памяти
- **Disk** — использование диска
- **Custom** — callback функция

**Статусы узлов:**
- `Unknown` — статус неизвестен
- `Healthy` — узел здоров
- `Unhealthy` — узел нездоров
- `Degraded` — узел работает с проблемами
- `Timeout` — таймаут проверки

**Пример использования:**
```c
health_check_init();
health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
health_check_add_tcp_node("db1", "localhost", 5432, 2000);
health_check_start(5000);  // Интервал 5 секунд
```

### 4. Cluster Manager

**Файлы:**
- `system/cluster/cluster-manager.c/h` — реализация (1100+ строк)
- `testing/test_cluster_manager.c` — 30 тестов

**Роли узлов:**
- `Leader` — лидер кластера
- `Follower` — ведомый узел
- `Candidate` — кандидат на выборах
- `Standalone` — автономный режим

**Функционал:**
- **Leader Election** — выборы лидера
- **Auto Failover** — автоматическое восстановление
- **Config Sync** — синхронизация конфигурации
- **Heartbeat** — проверка доступности
- **Load Balancing** — балансировка нагрузки

**Типы сообщений:**
- `Heartbeat` — проверка доступности
- `ElectionRequest/Response` — выборы
- `ConfigSync/Ack` — синхронизация
- `LoadBalance` — балансировка
- `Failover` — обработка отказов

**Пример использования:**
```c
cluster_init("cluster-main", "node1");
cluster_add_node("node2", "192.168.1.102", 8888, 9000);
cluster_add_node("node3", "192.168.1.103", 8888, 9000);
cluster_start(9000);
cluster_become_leader();
```

### 5. Load Balancer

**Файлы:**
- `system/cluster/load-balancer.c/h` — реализация (700+ строк)
- `testing/test_load_balancer.c` — 25 тестов

**Алгоритмы балансировки:**
- **Round Robin** — циклический перебор
- **Least Connections** — наименьшее количество подключений
- **Least Load** — наименьшая нагрузка
- **Weighted** — с учётом веса
- **IP Hash** — хеш от IP клиента
- **Random** — случайный выбор

**Функционал:**
- Добавление/удаление backend узлов
- Health check узлов
- Автоматическое перераспределение
- Уведомления о подключениях
- Статистика и мониторинг

**Пример использования:**
```c
load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);
load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
load_balancer_add_backend("node2", "192.168.1.102", 8888, 2.0);
load_balancer_start_health_checks(5000);

const char *backend = load_balancer_select_backend("192.168.1.100");
```

---

## 🧪 Тесты

### Новые тесты

**CLI Tests (10 тестов):**
- `test_mtcli_config_init`
- `test_mtcli_parse_command`
- `test_mtcli_parse_args_*`
- `test_mtcli_format_*`
- `test_mtcli_result_free`

**Alert Manager Tests (25 тестов):**
- `test_alert_manager_init/cleanup`
- `test_alert_manager_add_channel`
- `test_alert_manager_send_alert`
- `test_alert_server_down/up`
- `test_alert_high_cpu/memory/connections`

**Health Check Tests (20 тестов):**
- `test_health_check_init/cleanup`
- `test_health_check_add_*_node`
- `test_health_check_get_node_status`
- `test_health_check_get_stats`

**Cluster Manager Tests (30 тестов):**
- `test_cluster_init/cleanup`
- `test_cluster_add/remove_node`
- `test_cluster_become_leader`
- `test_cluster_start_election`
- `test_cluster_handle_node_failure`

**Load Balancer Tests (25 тестов):**
- `test_load_balancer_init/cleanup`
- `test_load_balancer_add/remove_backend`
- `test_load_balancer_select_backend`
- `test_load_balancer_rebalance`

### Запуск тестов

```bash
# Все тесты
ctest --output-on-failure

# Конкретные тесты
ctest -R cli --output-on-failure
ctest -R alert --output-on-failure
ctest -R health --output-on-failure
ctest -R cluster --output-on-failure
ctest -R load --output-on-failure
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
# С поддержкой gRPC
cmake -DENABLE_GRPC=ON ..

# С jemalloc
cmake -DENABLE_JEMALLOC=ON ..

# С io_uring (Linux)
cmake -DENABLE_IOURING=ON ..
```

### Зависимости

```bash
# Debian/Ubuntu
apt install git curl build-essential libssl-dev zlib1g-dev cmake

# Для gRPC
apt install libprotobuf-dev protobuf-compiler grpc-tools libgrpc++-dev
```

---

## 📊 Статистика релиза

### Файлы
| Категория | v1.0.29 | v1.0.30 | Изменение |
|-----------|---------|---------|-----------|
| **C/H файлов** | 415+ | 426+ | +11 |
| **Тестов** | 140 | 250 | +110 |
| **Документов** | 32 | 35 | +3 |

### Код
| Метрика | Значение |
|---------|----------|
| **Строк кода** | ~4000 (CLI + Alert + Health + Cluster + LB) |
| **Строк тестов** | ~1500 |
| **Строк документации** | ~500 |

---

## 🔧 Изменения в API

### CLI Commands

```bash
mtproxy-cli <command> [options]

Commands:
  status              Показать статус сервера
  stats               Показать статистику
  config get/set      Управление конфигурацией
  secrets list/add/remove  Управление секретами
  logs                Просмотр логов
  connections         Управление подключениями
  ratelimit           Управление rate limiting
  health              Проверка здоровья
  metrics             Экспорт метрик
  reload/restart/stop Управление сервером
```

### Alert Manager API

```c
// Инициализация
alert_manager_init();

// Добавление канала
alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");

// Отправка алерта
alert_manager_send_alert(ALERT_LEVEL_CRITICAL, ALERT_TYPE_SERVER_DOWN, 
                         "Server Down", "MTProxy server is down!");

// Встроенные функции
alert_server_down("server1");
alert_high_cpu(95.5, 90.0);
alert_high_memory(7000000000, 8000000000, 80.0);
```

### Health Check API

```c
// Инициализация
health_check_init();

// Добавление узлов
health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
health_check_add_tcp_node("db1", "localhost", 5432, 2000);

// Запуск
health_check_start(5000);

// Проверка статуса
health_status_t status = health_check_get_node_status("web1");
```

### Cluster Manager API

```c
// Инициализация
cluster_init("cluster-main", "node1");

// Добавление узлов
cluster_add_node("node2", "192.168.1.102", 8888, 9000);

// Запуск
cluster_start(9000);

// Выборы лидера
cluster_become_leader();

// Синхронизация конфигурации
cluster_sync_config("{\"key\":\"value\"}");
```

### Load Balancer API

```c
// Инициализация
load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);

// Добавление backend
load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
load_balancer_add_backend("node2", "192.168.1.102", 8888, 2.0);

// Выбор backend
const char *backend = load_balancer_select_backend("192.168.1.100");

// Уведомления
load_balancer_notify_new_connection(backend);
load_balancer_notify_connection_closed(backend);
```

---

## 🐛 Исправления

- Улучшена обработка ошибок в network модулях
- Оптимизировано использование памяти в health check
- Исправлена совместимость с Windows для cluster manager

---

## ⚠️ Breaking Changes

Нет. Все изменения обратно совместимы.

---

## 📝 Migration Guide

### Обновление с v1.0.29

1. **Обновите код:**
   ```bash
   git pull origin dev
   ```

2. **Пересоберите:**
   ```bash
   cd build
   make -j4
   ```

3. **Новые бинарные файлы:**
   - `bin/mtproxy-cli` — CLI утилита
   - `bin/test-alert-manager` — тесты alert manager
   - `bin/test-health-check` — тесты health check
   - `bin/test-cluster-manager` — тесты cluster manager
   - `bin/test-load-balancer` — тесты load balancer

---

## 🙏 Благодарности

Спасибо всем контрибьюторам за вклад в релиз v1.0.30!

---

## 📞 Поддержка

- **GitHub Issues:** [Сообщить о проблеме](https://github.com/QuadDarv1ne/MTProxy/issues)
- **Discussions:** [Обсуждения](https://github.com/QuadDarv1ne/MTProxy/discussions)
- **Telegram:** @Maestro7IT

---

*Release Notes v1.0.30 — 29 марта 2026 г.*
