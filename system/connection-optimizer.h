/*
    Заголовочный файл для оптимизации соединений MTProxy
    Пул соединений и оптимизация производительности
*/

#ifndef CONNECTION_OPTIMIZER_H
#define CONNECTION_OPTIMIZER_H

#include <stdint.h>
#include <stddef.h>

// Состояния соединений
typedef enum {
    CONN_STATE_IDLE = 0,
    CONN_STATE_ACTIVE = 1,
    CONN_STATE_CLOSED = 2,
    CONN_STATE_ERROR = 3
} conn_state_t;

// Типы соединений
typedef enum {
    CONN_TYPE_TCP = 0,
    CONN_TYPE_UDP = 1,
    CONN_TYPE_TLS = 2,
    CONN_TYPE_WEBSOCKET = 3
} conn_type_t;

// Запись соединения
typedef struct connection_entry {
    int id;
    conn_state_t state;
    conn_type_t type;
    long long last_used;
    long long created_time;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    int reused_count;
    int is_keepalive;
    void *user_data;
} connection_entry_t;

// Конфигурация оптимизатора соединений
typedef struct conn_opt_config {
    int max_connections;
    int min_idle_connections;
    int max_idle_connections;
    int connection_timeout_sec;
    int enable_keepalive;
    int keepalive_interval_sec;
    size_t memory_pool_size;
    int enable_compression;
    int compression_threshold;
    int enable_multiplexing;
} conn_opt_config_t;

// Пул соединений
typedef struct connection_pool {
    connection_entry_t *connections;
    int *free_list;
    int total_capacity;
    int active_count;
    int idle_count;
    int free_count;
    int created_count;
    int reused_count;
} connection_pool_t;

// Пул памяти
typedef struct memory_pool {
    void *base_ptr;
    size_t size;
    size_t used;
    size_t alignment;
} memory_pool_t;

// Статистика оптимизатора
typedef struct conn_opt_stats {
    long long acquired_connections;
    long long released_connections;
    long long acquire_failures;
    long long closed_connections;
    long long malloc_fallbacks;
    long long allocated_bytes;
    long long freed_bytes;
    double reuse_ratio;
    double pool_utilization;
    int idle_connections;
    int active_connections;
    size_t memory_pool_usage;
    size_t memory_pool_total;
    long long init_time;
} conn_opt_stats_t;

// Настройки производительности
typedef struct conn_performance_tuning {
    int max_connections;
    int min_idle_connections;
    int max_idle_connections;
    int timeout_seconds;
    int keepalive_interval;
    int enable_keepalive;
    int enable_compression;
    int enable_multiplexing;
} conn_performance_tuning_t;

// Основная структура оптимизатора
typedef struct connection_optimizer {
    // Конфигурация
    conn_opt_config_t config;
    
    // Пулы
    connection_pool_t pool;
    memory_pool_t memory_pool;
    
    // Статистика
    conn_opt_stats_t stats;
    
    // Состояние
    int is_initialized;
    int thread_safety_enabled;
    
} connection_optimizer_t;

// Инициализация
connection_optimizer_t* conn_opt_init(const conn_opt_config_t *config);
void conn_opt_cleanup(connection_optimizer_t *optimizer);

// Управление соединениями
connection_entry_t* conn_opt_acquire_connection(connection_optimizer_t *optimizer);
int conn_opt_release_connection(connection_optimizer_t *optimizer, connection_entry_t *conn);

// Управление памятью
void* conn_opt_alloc(connection_optimizer_t *optimizer, size_t size);
void conn_opt_free(connection_optimizer_t *optimizer, void *ptr, size_t size);

// Многопоточная безопасность
int conn_opt_enable_thread_safety(connection_optimizer_t *optimizer);
int conn_opt_disable_thread_safety(connection_optimizer_t *optimizer);

// Статистика
void conn_opt_get_stats(connection_optimizer_t *optimizer, conn_opt_stats_t *stats);
void conn_opt_print_stats(connection_optimizer_t *optimizer);

// Утилиты
double conn_opt_get_efficiency_score(connection_optimizer_t *optimizer);
int conn_opt_resize_pool(connection_optimizer_t *optimizer, int new_size);

// Оптимизация производительности
int conn_opt_apply_performance_tuning(connection_optimizer_t *optimizer, 
                                     conn_performance_tuning_t *tuning);
int conn_opt_adjust_for_load(connection_optimizer_t *optimizer, int current_load);
void conn_opt_reset_stats(connection_optimizer_t *optimizer);

#endif // CONNECTION_OPTIMIZER_H