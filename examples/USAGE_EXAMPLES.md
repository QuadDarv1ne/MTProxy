# Примеры использования MTProxy

## 📚 Обзор

Эта директория содержит примеры использования различных функций MTProxy.

## 🔧 Примеры

### 1. Конфигурация (config-manager)

#### Базовое использование

```c
#include "common/config-manager.h"

int main() {
    // Инициализация
    config_manager_init("/etc/mtproxy.conf");
    
    // Регистрация параметра
    int max_connections = 1000;
    CONFIG_REGISTER_INT("network", "max_connections", 
                       &max_connections, 1, "1000",
                       "Maximum connections");
    
    // Загрузка из файла
    config_manager_load_from_file("/etc/mtproxy.conf");
    
    // Получение значения
    int value;
    config_manager_get_parameter("network", "max_connections", 
                                &value, sizeof(int));
    
    // Установка значения
    int new_value = 2000;
    config_manager_set_parameter("network", "max_connections",
                                &new_value, sizeof(int));
    
    // Горячая перезагрузка
    config_manager_hot_reload("/etc/mtproxy_new.conf");
    
    // Очистка
    config_manager_cleanup();
    
    return 0;
}
```

#### Callback для изменений

```c
void on_config_change(const char *section, const char *param,
                     enum config_change_event event, void *userdata) {
    printf("Config changed: %s.%s, event: %d\n", section, param, event);
}

// Регистрация callback
config_manager_register_callback("network", "max_connections",
                                on_config_change, NULL);
```

#### JSON экспорт/импорт

```c
// Экспорт в JSON
config_manager_export_to_json("/etc/mtproxy/config.json", 0);

// Импорт из JSON
config_manager_import_from_json("/etc/mtproxy/config.json");
```

---

### 2. Кэширование (cache-manager)

#### Базовое использование

```c
#include "common/cache-manager.h"

int main() {
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 10000,
        .max_size_mb = 512,
        .default_ttl_sec = 3600,
        .enable_locking = 1,
        .enable_partitioning = 1,
        .partition_count = 16
    };
    
    cache_manager_t *cache = cache_manager_init(&config);
    
    // Запись в кэш
    const char *key = "user:12345";
    const char *value = "{\"name\":\"John\"}";
    cache_put(cache, key, value, strlen(value) + 1);
    
    // Чтение из кэша
    void *result = NULL;
    size_t size = 0;
    cache_status_t status = cache_get(cache, key, &result, &size);
    
    if (status == CACHE_OK) {
        printf("Cached: %s\n", (char*)result);
        free(result);
    }
    
    // Проверка существования
    if (cache_exists(cache, key)) {
        printf("Key exists in cache\n");
    }
    
    // Удаление
    cache_delete(cache, key);
    
    cache_manager_cleanup(cache);
    return 0;
}
```

#### Кэширование с TTL

```c
// Кэширование на 5 минут
time_t ttl = 300;
cache_put_with_ttl(cache, "session:abc", session_data, 
                  session_size, ttl);
```

#### Массовые операции

```c
// Batch запись
const char *keys[] = {"key1", "key2", "key3"};
const void *values[] = {data1, data2, data3};
const size_t sizes[] = {size1, size2, size3};

cache_put_batch(cache, keys, values, sizes, 3);

// Batch чтение
void **results = NULL;
size_t *result_sizes = NULL;
cache_get_batch(cache, keys, &results, &result_sizes, 3);

// Batch удаление
cache_delete_batch(cache, keys, 3);
```

#### Статистика кэша

```c
cache_stats_t stats;
cache_get_stats(cache, &stats);

printf("Hit rate: %.2f%%\n", stats.hit_rate);
printf("Entries: %zu / %zu\n", stats.current_entries, stats.max_entries);
printf("Memory: %zu / %zu bytes\n", stats.current_size_bytes, stats.max_size_bytes);
```

---

### 3. Rate Limiting (rate-limiter)

#### Базовое использование

```c
#include "common/rate-limiter.h"

int main() {
    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 100,
        .window_seconds = 60,
        .bucket_capacity = 100,
        .refill_rate = 10,  // 10 tokens per second
        .enable_locking = 1
    };
    
    rate_limiter_t *limiter = rate_limiter_init(&config);
    
    const char *client_ip = "192.168.1.100";
    
    // Проверка rate limit
    rate_limit_status_t status = rate_limit_check(limiter, client_ip);
    
    if (status == RATE_LIMIT_OK) {
        printf("Request allowed\n");
        // Обработать запрос
    } else if (status == RATE_LIMIT_EXCEEDED) {
        printf("Rate limit exceeded\n");
        // Отклонить запрос
    }
    
    rate_limiter_cleanup(limiter);
    return 0;
}
```

#### Whitelist/Blacklist

```c
// Добавление в whitelist
rate_limit_add_to_whitelist(limiter, "10.0.0.1");
rate_limit_add_to_whitelist(limiter, "vip_client");

// Добавление в blacklist
rate_limit_add_to_blacklist(limiter, "bad_client");
rate_limit_add_to_blacklist(limiter, "attacker_ip");

// Проверка
if (rate_limit_is_whitelisted(limiter, client_ip)) {
    printf("Whitelisted - always allowed\n");
}

if (rate_limit_is_blacklisted(limiter, client_ip)) {
    printf("Blacklisted - always blocked\n");
}
```

#### Получение информации о клиенте

```c
rate_limit_info_t info;
if (rate_limiter_get_info(limiter, client_ip, &info) == 0) {
    printf("Remaining requests: %lu\n", info.remaining_requests);
    printf("Limit: %lu\n", info.limit);
    printf("Reset time: %ld\n", info.reset_time);
    printf("Retry after: %ld\n", info.retry_after);
}
```

---

### 4. Обработка ошибок (error-handler)

#### Базовое использование

```c
#include "common/error-handler.h"

int main() {
    recovery_config_t config = {
        .strategy = RECOVERY_RETRY,
        .max_retries = 3,
        .retry_delay_ms = 1000,
        .backoff_multiplier = 2.0,
        .enable_circuit_breaker = 1,
        .circuit_breaker_threshold = 5
    };
    
    error_handler_context_t ctx;
    error_handler_init(&ctx, &config);
    
    // Создание и обработка ошибки
    error_info_t *error = ERROR_CREATE(MTERR_NETWORK_CONNECT,
                                       ERROR_LEVEL_ERROR,
                                       ERROR_CATEGORY_NETWORK,
                                       "Connection failed");
    
    error_handle(&ctx, error);
    
    // Получение последней ошибки
    error_info_t last_error;
    if (error_get_last(&ctx, &last_error) == 0) {
        printf("Last error: %s\n", last_error.message);
    }
    
    error_free(error);
    error_handler_cleanup(&ctx);
    return 0;
}
```

#### Circuit Breaker

```c
// Проверка возможности выполнения
if (error_circuit_breaker_can_execute(&ctx)) {
    // Выполнить операцию
    int result = some_operation();
    
    if (result == 0) {
        error_circuit_breaker_record_success(&ctx);
    } else {
        error_circuit_breaker_record_failure(&ctx);
    }
} else {
    printf("Circuit breaker is open - operation skipped\n");
}
```

#### Retry с exponential backoff

```c
int my_operation(void *user_data) {
    // Операция которая может упасть
    return perform_network_request();
}

// Выполнение с retry
int result = error_execute_with_retry(&ctx, my_operation, NULL, 3);

if (result == 0) {
    printf("Operation succeeded\n");
} else {
    printf("Operation failed after retries\n");
}
```

#### Макросы для удобной обработки

```c
// Проверка результата с автоматической обработкой ошибки
ERROR_CHECK(&ctx, network_connect(), MTERR_NETWORK_CONNECT,
           ERROR_LEVEL_ERROR, ERROR_CATEGORY_NETWORK,
           "Failed to connect");

// Создание ошибки с деталями
error_info_t *err = ERROR_CREATE_DETAIL(
    MTERR_CONFIG_PARSE,
    ERROR_LEVEL_ERROR,
    ERROR_CATEGORY_CONFIG,
    "Invalid configuration",
    "Expected integer value"
);
```

---

### 5. Admin CLI

#### Использование утилиты

```bash
# Показать статус
./mtproxy-admin status

# Показать статистику
./mtproxy-admin stats

# Интерактивный режим
./mtproxy-admin -i

# Конкретная команда
./mtproxy-admin cache-stats

# С подключением к удаленному серверу
./mtproxy-admin --host 192.168.1.100 --port 8888 status
```

#### Примеры команд

```bash
# Конфигурация
mtproxy-admin config show
mtproxy-admin config-set network.max_connections 20000

# Кэш
mtproxy-admin cache-stats
mtproxy-admin cache-clear

# Rate limiting
mtproxy-admin ratelimit status
mtproxy-admin whitelist add 10.0.0.1

# Логи
mtproxy-admin log-level debug
mtproxy-admin log-flush

# Здоровье
mtproxy-admin health
mtproxy-admin metrics
```

---

### 6. Мониторинг

#### Использование monitor.sh

```bash
# Показать статус
./scripts/monitor.sh status

# Непрерывный мониторинг
./scripts/monitor.sh monitor

# С кастомными параметрами
MTProxy_HOST=192.168.1.100 CHECK_INTERVAL=30 ./scripts/monitor.sh monitor
```

#### Использование metrics_collector.py

```bash
# Показать метрики
python3 scripts/metrics_collector.py metrics

# Экспорт в Prometheus формат
python3 scripts/metrics_collector.py export --format prometheus

# Непрерывное наблюдение
python3 scripts/metrics_collector.py watch --interval 5
```

---

## 📊 Интеграция

### Пример: Полный цикл

```c
#include "common/config-manager.h"
#include "common/cache-manager.h"
#include "common/rate-limiter.h"
#include "common/error-handler.h"

typedef struct {
    cache_manager_t *cache;
    rate_limiter_t *limiter;
    error_handler_context_t errors;
} app_context_t;

int init_app(app_context_t *ctx) {
    // Инициализация конфигурации
    config_manager_init("/etc/app.conf");
    
    // Инициализация кэша
    cache_config_t cache_config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 100000,
        .max_size_mb = 512
    };
    ctx->cache = cache_manager_init(&cache_config);
    
    // Инициализация rate limiter
    rate_limit_config_t rl_config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 1000,
        .window_seconds = 60
    };
    ctx->limiter = rate_limiter_init(&rl_config);
    
    // Инициализация error handler
    recovery_config_t err_config = {
        .strategy = RECOVERY_RETRY,
        .max_retries = 3,
        .enable_circuit_breaker = 1
    };
    error_handler_init(&ctx->errors, &err_config);
    
    return 0;
}

int handle_request(app_context_t *ctx, const char *client_id, 
                  const char *key, char *response, size_t response_size) {
    // Проверка rate limit
    rate_limit_status_t rl_status = rate_limit_check(ctx->limiter, client_id);
    
    if (rl_status == RATE_LIMIT_EXCEEDED) {
        snprintf(response, response_size, "Rate limit exceeded");
        return -1;
    }
    
    // Проверка кэша
    void *cached = NULL;
    size_t cached_size = 0;
    cache_status_t cache_status = cache_get(ctx->cache, key, &cached, &cached_size);
    
    if (cache_status == CACHE_OK) {
        snprintf(response, response_size, "%s", (char*)cached);
        if (cached) free(cached);
        return 0;
    }
    
    // Кэш-мисс - выполнение операции
    char *result = perform_expensive_operation(key);
    
    if (!result) {
        error_info_t *error = ERROR_CREATE(MTERR_RESOURCE_UNAVAILABLE,
                                          ERROR_LEVEL_ERROR,
                                          ERROR_CATEGORY_RESOURCE,
                                          "Operation failed");
        error_handle(&ctx->errors, error);
        error_free(error);
        
        snprintf(response, response_size, "Error");
        return -1;
    }
    
    // Сохранение в кэш
    cache_put(ctx->cache, key, result, strlen(result) + 1);
    snprintf(response, response_size, "%s", result);
    
    free(result);
    return 0;
}

void cleanup_app(app_context_t *ctx) {
    if (ctx->cache) cache_manager_cleanup(ctx->cache);
    if (ctx->limiter) rate_limiter_cleanup(ctx->limiter);
    error_handler_cleanup(&ctx->errors);
    config_manager_cleanup();
}
```

---

## 📝 Лицензия

Примеры распространяются под той же лицензией, что и MTProxy.

---

*Последнее обновление: Март 2026*
