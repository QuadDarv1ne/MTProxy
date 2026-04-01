# API Reference для MTProxy

## 📋 Содержание

1. [Обзор](#обзор)
2. [Публичный API (mtproxy.h)](#публичный-api-mtproxyh)
3. [Config Manager API](#config-manager-api)
4. [Cache Manager API](#cache-manager-api)
5. [Rate Limiter API](#rate-limiter-api)
6. [Error Handler API](#error-handler-api)
7. [Admin CLI API](#admin-cli-api)
8. [Примеры использования](#примеры-использования)

---

## Обзор

MTProxy предоставляет модульный API для интеграции с внешними системами.

### Компоненты API

| Компонент | Описание | Заголовок |
|-----------|----------|-----------|
| **MTProxy Core** | Базовое управление прокси | `include/mtproxy.h` |
| **Config Manager** | Управление конфигурацией | `common/config-manager.h` |
| **Cache Manager** | Кэширование данных | `common/cache-manager.h` |
| **Rate Limiter** | Ограничение скорости | `common/rate-limiter.h` |
| **Error Handler** | Обработка ошибок | `common/error-handler.h` |
| **Admin CLI** | Администрирование | `admin/admin-cli.h` |

---

## Публичный API (mtproxy.h)

### Инициализация и управление

```c
int mtproxy_init(void);
int mtproxy_start(void);
void mtproxy_stop(void);
bool mtproxy_is_running(void);
```

### Конфигурация

```c
int mtproxy_set_port(uint16_t port);
int mtproxy_add_secret(const char* secret);
int mtproxy_remove_secret(const char* secret);
void mtproxy_clear_secrets(void);
int mtproxy_set_max_connections(uint32_t max_connections);
int mtproxy_set_ipv6(bool enable);
int mtproxy_apply_config(const mtproxy_config_t* config);
```

### Статистика

```c
mtproxy_stats_t* mtproxy_get_stats(void);
uint32_t mtproxy_get_active_connections(void);
uint64_t mtproxy_get_total_connections(void);
uint64_t mtproxy_get_bytes_sent(void);
uint64_t mtproxy_get_bytes_received(void);
uint64_t mtproxy_get_start_time(void);
```

### Структуры данных

```c
typedef struct {
    uint32_t active_connections;
    uint32_t total_connections;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t start_time;
    double cpu_usage;
    uint32_t memory_usage;
} mtproxy_stats_t;

typedef struct {
    uint16_t port;
    const char* secret;
    uint32_t max_connections;
    bool enable_ipv6;
    bool enable_stats;
} mtproxy_config_t;
```

---

## Config Manager API

### Инициализация

```c
config_manager_t* config_manager_init(void);
void config_manager_free(config_manager_t *manager);
```

### Операции с параметрами

```c
config_status_t config_set(config_manager_t *manager, 
                           const char *key, 
                           const config_value_t *value);

config_value_t* config_get(config_manager_t *manager, 
                           const char *key);

config_status_t config_delete(config_manager_t *manager, 
                              const char *key);
```

### История и экспорт

```c
config_history_t* config_get_history(config_manager_t *manager);
char* config_to_json(config_manager_t *manager);
config_status_t config_from_json(config_manager_t *manager, 
                                  const char *json);
config_status_t config_reload(config_manager_t *manager);
```

### Типы значений

```c
typedef enum {
    CONFIG_TYPE_INT,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_DOUBLE,
    CONFIG_TYPE_ARRAY,
    CONFIG_TYPE_OBJECT
} config_value_type_t;

typedef struct {
    config_value_type_t type;
    union {
        int int_val;
        char* string_val;
        bool bool_val;
        double double_val;
    } value;
} config_value_t;
```

---

## Cache Manager API

### Инициализация

```c
cache_manager_t* cache_manager_init(const cache_config_t *config);
void cache_manager_free(cache_manager_t *cache);
```

### Базовые операции

```c
cache_status_t cache_put(cache_manager_t *cache, 
                         const char *key, 
                         const void *value, 
                         size_t value_size);

cache_status_t cache_get(cache_manager_t *cache, 
                         const char *key, 
                         void **value, 
                         size_t *value_size);

cache_status_t cache_delete(cache_manager_t *cache, 
                            const char *key);
```

### Операции с TTL

```c
cache_status_t cache_put_ttl(cache_manager_t *cache, 
                             const char *key, 
                             const void *value, 
                             size_t value_size, 
                             uint32_t ttl_sec);
```

### Массовые операции

```c
cache_status_t cache_batch_put(cache_manager_t *cache, 
                               const char **keys, 
                               const void **values, 
                               size_t *sizes, 
                               int count);

cache_status_t cache_batch_get(cache_manager_t *cache, 
                               const char **keys, 
                               void **values, 
                               size_t *sizes, 
                               int count);

cache_status_t cache_batch_delete(cache_manager_t *cache, 
                                  const char **keys, 
                                  int count);
```

### Статистика

```c
cache_stats_t cache_get_stats(cache_manager_t *cache);
void cache_print_stats(cache_manager_t *cache);
```

### Алгоритмы вытеснения

```c
typedef enum {
    CACHE_LRU,      // Least Recently Used
    CACHE_LFU,      // Least Frequently Used
    CACHE_FIFO,     // First In First Out
    CACHE_TTL,      // Time To Live
    CACHE_ARC       // Adaptive Replacement Cache
} cache_policy_t;
```

---

## Rate Limiter API

### Инициализация

```c
rate_limiter_t* rate_limiter_init(const rate_limit_config_t *config);
void rate_limiter_cleanup(rate_limiter_t *limiter);
```

### Проверка лимитов

```c
rate_limit_status_t rate_limit_check(rate_limiter_t *limiter, 
                                     const char *key);

rate_limit_status_t rate_limit_acquire(rate_limiter_t *limiter, 
                                       const char *key);

int rate_limit_is_allowed(rate_limiter_t *limiter, 
                          const char *key);
```

### Whitelist/Blacklist

```c
int rate_limit_add_to_whitelist(rate_limiter_t *limiter, 
                                const char *key);

int rate_limit_remove_from_whitelist(rate_limiter_t *limiter, 
                                     const char *key);

int rate_limit_add_to_blacklist(rate_limiter_t *limiter, 
                                const char *key);

int rate_limit_remove_from_blacklist(rate_limiter_t *limiter, 
                                     const char *key);
```

### Retry-After и Reset-Time

```c
uint64_t rate_limit_calculate_retry_after(rate_limiter_t *limiter, 
                                          const char *key);

time_t rate_limit_calculate_reset_time(rate_limiter_t *limiter, 
                                       const char *key);
```

### Алгоритмы

```c
typedef enum {
    RATE_LIMIT_TOKEN_BUCKET,
    RATE_LIMIT_SLIDING_WINDOW,
    RATE_LIMIT_FIXED_WINDOW,
    RATE_LIMIT_LEAKY_BUCKET,
    RATE_LIMIT_ADAPTIVE
} rate_limit_algorithm_t;
```

---

## Error Handler API

### Инициализация

```c
error_handler_t* error_handler_init(void);
void error_handler_free(error_handler_t *handler);
```

### Регистрация ошибок

```c
error_t* error_create(error_category_t category, 
                      error_code_t code, 
                      const char *message, 
                      const char *context);

void error_register(error_handler_t *handler, 
                    error_t *error);

error_t* error_get_last(error_handler_t *handler);
```

### Статистика

```c
void error_handler_get_stats(error_handler_t *handler, 
                             error_stats_t *stats);

void error_handler_clear(error_handler_t *handler);
```

### Circuit Breaker

```c
bool error_handler_is_circuit_breaker_enabled(error_handler_t *handler);
bool error_handler_is_circuit_breaker_open(error_handler_t *handler);
void error_handler_reset_circuit_breaker(error_handler_t *handler);
```

### Категории ошибок

```c
typedef enum {
    ERROR_CATEGORY_CONFIG,
    ERROR_CATEGORY_NETWORK,
    ERROR_CATEGORY_CRYPTO,
    ERROR_CATEGORY_MEMORY,
    ERROR_CATEGORY_IO,
    ERROR_CATEGORY_RATE_LIMIT,
    ERROR_CATEGORY_AUTH,
    ERROR_CATEGORY_INTERNAL,
    ERROR_CATEGORY_CLIENT,
    ERROR_CATEGORY_SERVER,
    ERROR_CATEGORY_TIMEOUT,
    ERROR_CATEGORY_UNKNOWN
} error_category_t;
```

---

## Admin CLI API

### Парсинг команд

```c
char** admin_cli_tokenize(const char *command, int *argc);
void admin_cli_free_tokens(char **argv, int argc);
admin_command_t admin_cli_parse_command(const char *name);
```

### Выполнение команд

```c
int admin_cli_execute(admin_command_t command, 
                      int argc, 
                      char **argv, 
                      char **output);

void admin_cli_free_output(char *output);
```

### Доступные команды

| Команда | Описание |
|---------|----------|
| `status` | Проверка статуса прокси |
| `stats` | Показать статистику |
| `config show [section]` | Показать конфигурацию |
| `config reload` | Перезагрузить конфигурацию |
| `cache-get <key>` | Получить значение из кэша |
| `cache-put <key> <value>` | Положить значение в кэш |
| `cache-delete <key>` | Удалить значение из кэша |
| `cache stats` | Статистика кэша |
| `rate-limit show <client>` | Показать лимиты клиента |
| `rate-limit reset <client>` | Сбросить лимиты клиента |
| `help` | Показать справку |

---

## Примеры использования

### Пример 1: Базовая инициализация

```c
#include "include/mtproxy.h"

int main() {
    // Инициализация
    if (mtproxy_init() != 0) {
        fprintf(stderr, "Failed to initialize MTProxy\n");
        return 1;
    }

    // Настройка
    mtproxy_set_port(8080);
    mtproxy_add_secret("0123456789abcdef0123456789abcdef");
    mtproxy_set_max_connections(10000);

    // Запуск
    if (mtproxy_start() != 0) {
        fprintf(stderr, "Failed to start MTProxy\n");
        return 1;
    }

    // Работа...

    // Остановка
    mtproxy_stop();
    return 0;
}
```

### Пример 2: Работа с кэшем

```c
#include "common/cache-manager.h"

void cache_example() {
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,
        .default_ttl_sec = 300
    };

    cache_manager_t *cache = cache_manager_init(&config);

    // Запись
    cache_put(cache, "user:123", "{\"name\":\"John\"}", 17);

    // Чтение
    void *result = NULL;
    size_t size = 0;
    cache_get(cache, "user:123", &result, &size);
    
    printf("Cached: %s\n", (char*)result);
    free(result);

    // Статистика
    cache_stats_t stats = cache_get_stats(cache);
    printf("Hit rate: %.2f%%\n", 
           (stats.hit_count * 100.0) / stats.total_accesses);

    cache_manager_free(cache);
}
```

### Пример 3: Rate Limiting

```c
#include "common/rate-limiter.h"

void rate_limit_example() {
    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .requests_per_second = 100,
        .burst_size = 150
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);

    const char *client = "client_001";

    // Проверка лимита
    rate_limit_status_t status = rate_limit_check(limiter, client);

    if (status == RATE_LIMIT_STATUS_OK) {
        // Запрос разрешён
        process_request(client);
    } else if (status == RATE_LIMIT_STATUS_EXCEEDED) {
        // Лимит превышен
        uint64_t retry_after = rate_limit_calculate_retry_after(
            limiter, client);
        printf("Retry after %lu ms\n", retry_after);
    }

    rate_limiter_cleanup(limiter);
}
```

### Пример 4: Обработка ошибок

```c
#include "common/error-handler.h"

void error_handling_example() {
    error_handler_t *handler = error_handler_init();

    // Создание и регистрация ошибки
    error_t *error = error_create(
        ERROR_CATEGORY_NETWORK,
        ERROR_NETWORK_TIMEOUT,
        "Connection timeout",
        "server:8080"
    );
    error_register(handler, error);

    // Проверка circuit breaker
    if (error_handler_is_circuit_breaker_open(handler)) {
        printf("Circuit breaker is open - requests blocked\n");
    }

    // Статистика
    error_stats_t stats;
    error_handler_get_stats(handler, &stats);
    printf("Total errors: %d\n", stats.total_errors);

    error_handler_free(handler);
}
```

### Пример 5: Admin CLI интеграция

```c
#include "admin/admin-cli.h"

void cli_example() {
    // Парсинг команды
    int argc = 0;
    char **argv = admin_cli_tokenize("cache-get user:123", &argc);

    // Выполнение
    char *output = NULL;
    admin_command_t cmd = admin_cli_parse_command("cache-get");
    admin_cli_execute(cmd, argc, argv, &output);

    printf("Result: %s\n", output);

    // Очистка
    admin_cli_free_tokens(argv, argc);
    admin_cli_free_output(output);
}
```

---

## Коды ошибок

### MTProxy Core

| Код | Описание |
|-----|----------|
| `MTPROXY_OK` | Успех |
| `MTPROXY_ERR_INIT` | Ошибка инициализации |
| `MTPROXY_ERR_CONFIG` | Ошибка конфигурации |
| `MTPROXY_ERR_START` | Ошибка запуска |
| `MTPROXY_ERR_STOP` | Ошибка остановки |
| `MTPROXY_ERR_SECRET` | Ошибка секретного ключа |

### Config Manager

| Код | Описание |
|-----|----------|
| `CONFIG_OK` | Успех |
| `CONFIG_ERR_NOT_FOUND` | Ключ не найден |
| `CONFIG_ERR_INVALID_KEY` | Неверный ключ |
| `CONFIG_ERR_INVALID_VALUE` | Неверное значение |
| `CONFIG_ERR_TYPE_MISMATCH` | Несовпадение типов |

### Cache Manager

| Код | Описание |
|-----|----------|
| `CACHE_OK` | Успех |
| `CACHE_ERR_NOT_FOUND` | Ключ не найден |
| `CACHE_ERR_EXPIRED` | Истёк TTL |
| `CACHE_ERR_FULL` | Кэш заполнен |
| `CACHE_ERR_INVALID_KEY` | Неверный ключ |

### Rate Limiter

| Код | Описание |
|-----|----------|
| `RATE_LIMIT_STATUS_OK` | Успех |
| `RATE_LIMIT_STATUS_ALLOWED` | Разрешено |
| `RATE_LIMIT_STATUS_EXCEEDED` | Лимит превышен |
| `RATE_LIMIT_STATUS_BLACKLISTED` | В чёрном списке |

---

*Последнее обновление: 20 марта 2026 г.*
