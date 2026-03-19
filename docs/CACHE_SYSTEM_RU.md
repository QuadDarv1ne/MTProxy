# Система кэширования MTProxy

## Обзор

MTProxy включает современную систему кэширования с поддержкой различных алгоритмов вытеснения, partitioning для многопоточности и расширенными возможностями мониторинга.

## Архитектура

```
┌─────────────────────────────────────────────────────────┐
│                  Cache Manager                          │
├─────────────────────────────────────────────────────────┤
│  Partition 0  │  Partition 1  │  ...  │  Partition N   │
│  ┌─────────┐  │  ┌─────────┐  │       │  ┌─────────┐  │
│  │ Buckets │  │  │ Buckets │  │       │  │ Buckets │  │
│  │ LRU List│  │  │ LRU List│  │       │  │ LRU List│  │
│  └─────────┘  │  └─────────┘  │       │  └─────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Алгоритмы вытеснения

### LRU (Least Recently Used)
Вытесняет давно не используемые записи. Оптимально для общего использования.

### LFU (Least Frequently Used)
Вытесняет редко используемые записи. Лучше для паттернов с "горячими" данными.

### FIFO (First In First Out)
Простая очередь. Подходит для кэширования последовательных данных.

### TTL (Time To Live)
Автоматическое удаление по истечении времени. Для временных данных.

### ARC (Adaptive Replacement Cache)
Адаптивный алгоритм, сочетающий LRU и LFU. Лучший выбор для смешанных нагрузок.

## Инициализация

```c
#include "common/cache-manager.h"

cache_config_t config = {
    .type = CACHE_TYPE_MEMORY,
    .policy = CACHE_LRU,
    .max_entries = 100000,
    .max_size_mb = 512,
    .default_ttl_sec = 3600,
    .enable_compression = 0,
    .enable_locking = 1,
    .enable_statistics = 1,
    .enable_partitioning = 1,
    .partition_count = 16
};

cache_manager_t *cache = cache_manager_init(&config);
if (!cache) {
    fprintf(stderr, "Failed to initialize cache\n");
    return -1;
}
```

## Основные операции

### Запись в кэш

```c
// Базовая запись
const char *key = "user:12345";
const char *data = "{\"name\":\"John\",\"age\":30}";
cache_status_t status = cache_put(cache, key, data, strlen(data) + 1);

if (status == CACHE_OK) {
    printf("Data cached successfully\n");
}
```

### Чтение из кэша

```c
void *cached_data = NULL;
size_t data_size = 0;

cache_status_t status = cache_get(cache, "user:12345", 
                                  &cached_data, &data_size);

switch (status) {
    case CACHE_OK:
        printf("Cache hit: %.*s\n", (int)data_size, (char*)cached_data);
        free(cached_data);
        break;
    case CACHE_MISS:
        printf("Cache miss\n");
        break;
    case CACHE_EXPIRED:
        printf("Cache entry expired\n");
        break;
    default:
        printf("Cache error: %d\n", status);
}
```

### Запись с TTL

```c
// Кэширование на 5 минут
time_t ttl = 300;
cache_put_with_ttl(cache, "session:abc123", session_data, 
                  session_size, ttl);
```

### Удаление

```c
cache_status_t status = cache_delete(cache, "user:12345");
if (status == CACHE_OK) {
    printf("Entry deleted\n");
}
```

### Проверка существования

```c
if (cache_exists(cache, "user:12345")) {
    printf("Entry exists in cache\n");
}
```

## Массовые операции

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

// Обработка результатов
for (int i = 0; i < 3; i++) {
    if (results[i]) {
        process_data(results[i], result_sizes[i]);
        free(results[i]);
    }
}
free(results);
free(result_sizes);

// Batch удаление
cache_delete_batch(cache, keys, 3);
```

## Статистика и мониторинг

```c
cache_stats_t stats;
cache_get_stats(cache, &stats);

printf("Cache Statistics:\n");
printf("  Entries: %zu / %zu\n", stats.current_entries, stats.max_entries);
printf("  Size: %zu / %zu MB\n", 
       stats.current_size_bytes / (1024*1024), 
       stats.max_size_bytes / (1024*1024));
printf("  Hits: %lld\n", stats.hits);
printf("  Misses: %lld\n", stats.misses);
printf("  Hit Rate: %.2f%%\n", stats.hit_rate);
printf("  Evictions: %lld\n", stats.evictions);
printf("  Expirations: %lld\n", stats.expirations);

// Быстрая проверка hit rate
double hit_rate = cache_get_hit_rate(cache);
printf("Current hit rate: %.2f%%\n", hit_rate);

// Использование памяти
size_t mem_usage = cache_get_memory_usage(cache);
printf("Memory usage: %zu bytes\n", mem_usage);
```

## Callback функции

```c
// Callback при вытеснении
void on_eviction(const char *key, const void *data) {
    printf("Evicted: %s\n", key);
    // Можно сохранить на диск или отправить в лог
}

// Callback при истечении TTL
void on_expiration(const char *key, const void *data) {
    printf("Expired: %s\n", key);
}

// Регистрация callback'ов
cache_set_eviction_callback(cache, on_eviction);
cache_set_expiration_callback(cache, on_expiration);
```

## Персистентность

```c
// Сохранение на диск
cache_save_to_disk(cache, "/var/cache/mtproxy.dat");

// Загрузка с диска
cache_load_from_disk(cache, "/var/cache/mtproxy.dat");

// Автоматическая персистентность
cache_start_persistence_thread(cache);

// ... позже ...
cache_stop_persistence_thread(cache);
```

## Предвыборка и прогрев

```c
// Предвыборка популярных ключей
const char *popular_keys[] = {"key1", "key2", "key3"};
cache_prefetch(cache, popular_keys, 3);

// Прогрев кэша при старте
const char *warm_keys[] = {"config", "users", "sessions"};
const void *warm_values[] = {config_data, users_data, sessions_data};
const size_t warm_sizes[] = {config_size, users_size, sessions_size};

cache_warm_up(cache, warm_keys, warm_values, warm_sizes, 3);
```

## Управление памятью

```c
// Принудительная eviction
cache_evict(cache, 100);  // Вытеснить 100 записей

// Удаление истекших записей
cache_evict_expired(cache);

// LRU eviction
cache_evict_lru(cache, 50);

// LFU eviction
cache_evict_lfu(cache, 50);

// Полная очистка
cache_clear(cache);

// Изменение размера
cache_resize(cache, 200000, 1024);  // 200K записей, 1GB
```

## Атомарные операции

```c
// Счетчики в кэше
long long count = cache_increment(cache, "page:views", 1);
printf("Page views: %lld\n", count);

// Декремент
count = cache_decrement(cache, "inventory:item123", 1);
```

## Утилиты

```c
// Генерация ключа
char *key = cache_generate_key("user", user_data, user_data_size);
// key = "user:A1B2C3D4"

// Валидация ключа
if (cache_validate_key("valid_key")) {
    printf("Key is valid\n");
}

// Хэш ключа
uint64_t hash = cache_hash_key("my_key");
```

## Расширенные функции

### Приоритеты

```c
// Установка приоритета
cache_set_priority(cache, "important_data", 10);  // 0-10

// Получение приоритета
int priority = cache_get_priority(cache, "important_data");
```

### Touch операция

```c
// Обновление времени доступа без получения данных
cache_touch(cache, "frequently_accessed_key");
```

### Replace операция

```c
// Замена данных существующего ключа
cache_replace(cache, "key", new_data, new_size);
```

### Проверка целостности

```c
int health = cache_get_health(cache);
if (health) {
    printf("Cache is healthy\n");
}

int integrity = cache_verify_integrity(cache);
if (integrity) {
    printf("Cache integrity OK\n");
}

// Дефрагментация
cache_defragment(cache);
```

## Примеры использования

### Кэширование конфигурации

```c
typedef struct {
    cache_manager_t *config_cache;
    time_t config_ttl;
} config_cache_context_t;

int init_config_cache(config_cache_context_t *ctx) {
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 100,
        .max_size_mb = 10,
        .default_ttl_sec = 300,  // 5 минут
        .enable_locking = 1
    };
    
    ctx->config_cache = cache_manager_init(&config);
    ctx->config_ttl = 300;
    
    return ctx->config_cache ? 0 : -1;
}

int get_config(cache_manager_t *cache, const char *section,
               char **config_json) {
    char key[64];
    snprintf(key, sizeof(key), "config:%s", section);
    
    cache_status_t status = cache_get(cache, key, 
                                      (void**)config_json, NULL);
    
    if (status == CACHE_MISS || status == CACHE_EXPIRED) {
        // Загрузка из файла
        *config_json = load_config_from_file(section);
        if (*config_json) {
            cache_put_with_ttl(cache, key, *config_json,
                             strlen(*config_json) + 1, 300);
        }
        return 1;  // Загружено из файла
    }
    
    return 0;  // Получено из кэша
}
```

### Кэширование сессий

```c
typedef struct {
    char user_id[64];
    char token[128];
    time_t created;
    time_t expires;
} session_data_t;

int create_session(cache_manager_t *cache, const char *user_id,
                  session_data_t *out_session) {
    session_data_t session = {0};
    strncpy(session.user_id, user_id, sizeof(session.user_id) - 1);
    
    // Генерация токена
    generate_random_token(session.token, sizeof(session.token));
    
    session.created = time(NULL);
    session.expires = session.created + 86400;  // 24 часа
    
    // Ключ сессии
    char key[128];
    snprintf(key, sizeof(key), "session:%s", session.token);
    
    // Сохранение в кэш
    cache_status_t status = cache_put_with_ttl(cache, key,
                                              &session, sizeof(session),
                                              86400);
    
    if (status == CACHE_OK) {
        *out_session = session;
        return 0;
    }
    
    return -1;
}

int validate_session(cache_manager_t *cache, const char *token,
                    session_data_t *out_session) {
    char key[128];
    snprintf(key, sizeof(key), "session:%s", token);
    
    void *data = NULL;
    size_t size = 0;
    
    cache_status_t status = cache_get(cache, key, &data, &size);
    
    if (status == CACHE_OK && data && size == sizeof(session_data_t)) {
        *out_session = *(session_data_t*)data;
        free(data);
        return 0;
    }
    
    return -1;  // Сессия не найдена или истекла
}
```

### Многопоточный доступ

```c
// Partitioned кэш для многопоточности
cache_config_t config = {
    .enable_partitioning = 1,
    .partition_count = 32,  // Один раздел на ~2 потока
    .enable_locking = 1
};

cache_manager_t *cache = cache_manager_init(&config);

// Теперь потоки могут работать параллельно с разными разделами
void* worker_thread(void *arg) {
    int thread_id = *(int*)arg;
    
    // Каждый поток работает со своим разделом
    char key[64];
    snprintf(key, sizeof(key), "data:%d", thread_id);
    
    cache_put(cache, key, data, size);
    cache_get(cache, key, &result, &result_size);
    
    return NULL;
}
```

## Лучшие практики

1. **Выбор алгоритма**: LRU для общего использования, ARC для смешанных нагрузок
2. **Размер кэша**: 20-30% от доступной памяти
3. **TTL**: Всегда устанавливайте разумный TTL
4. **Partitioning**: 1 раздел на 2-4 потока
5. **Мониторинг**: Следите за hit rate > 80%
6. **Персистентность**: Для критичных данных
7. **Callback'и**: Для логирования eviction/expiration

## Производительность

- O(1) для get/put операций
- Минимальные блокировки благодаря partitioning
- Масштабирование до 64+ потоков
- Hit rate > 90% для типичных нагрузок

## См. также

- [CONFIGURATION_ENHANCEMENTS_RU.md](CONFIGURATION_ENHANCEMENTS_RU.md) - Система конфигурации
- [ADVANCED_LOGGING_RU.md](ADVANCED_LOGGING_RU.md) - Система логирования
- [PERFORMANCE_OPTIMIZATIONS_RU.md](PERFORMANCE_OPTIMIZATIONS_RU.md) - Оптимизация производительности
