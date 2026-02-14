/*
 * Адаптивный пул соединений для MTProxy
 * Автоматическое масштабирование и оптимизация соединений
 */

#ifndef _ADAPTIVE_CONNECTION_POOL_H_
#define _ADAPTIVE_CONNECTION_POOL_H_

#include <stdint.h>

// Состояния соединения
typedef enum {
    CONN_STATE_IDLE = 0,
    CONN_STATE_ACTIVE = 1,
    CONN_STATE_BUSY = 2,
    CONN_STATE_ERROR = 3,
    CONN_STATE_CLOSED = 4
} connection_state_t;

// Типы соединений
typedef enum {
    CONN_TYPE_CLIENT = 0,
    CONN_TYPE_SERVER = 1,
    CONN_TYPE_RELAY = 2
} connection_type_t;

// Статус пула соединений
typedef enum {
    CONN_POOL_STATUS_UNINITIALIZED = 0,
    CONN_POOL_STATUS_INITIALIZED = 1,
    CONN_POOL_STATUS_ACTIVE = 2,
    CONN_POOL_STATUS_SCALING_UP = 3,
    CONN_POOL_STATUS_SCALING_DOWN = 4,
    CONN_POOL_STATUS_ERROR = 5
} connection_pool_status_t;

// Статистика пула соединений
typedef struct {
    long long total_connections_created;
    long long total_connections_closed;
    long long active_connections;
    long long idle_connections;
    long long busy_connections;
    long long failed_connections;
    long long recycled_connections;
    long long scaling_events;
    long long performance_improvements;
    connection_pool_status_t current_status;
    int current_pool_size;
    int peak_pool_size;
    double utilization_percent;
} connection_pool_stats_t;

// Конфигурация пула соединений
typedef struct {
    int enable_adaptive_scaling;
    int min_pool_size;
    int max_pool_size;
    int initial_pool_size;
    int scale_up_threshold_percent;   // % нагрузки для увеличения
    int scale_down_threshold_percent; // % нагрузки для уменьшения
    int scale_step_size;             // Количество соединений при масштабировании
    int connection_timeout_ms;
    int idle_timeout_ms;
    int max_lifetime_ms;
    int enable_recycling;
    int enable_health_check;
    int health_check_interval_ms;
    int max_retries;
    int retry_delay_ms;
} connection_pool_config_t;

// Контекст пула соединений
typedef struct {
    connection_pool_config_t config;
    connection_pool_stats_t stats;
    connection_pool_status_t status;
    void **connection_handles;
    connection_state_t *connection_states;
    connection_type_t *connection_types;
    long long *last_used_times;
    long long *creation_times;
    int *connection_fds;
    int pool_size;
    int active_count;
    int idle_count;
    int busy_count;
    int initialized;
    long long last_scale_time;
    int last_scale_direction;  // 1 для увеличения, -1 для уменьшения
    double current_utilization;
} connection_pool_context_t;

// Структура для соединения
typedef struct {
    int fd;
    connection_state_t state;
    connection_type_t type;
    long long creation_time;
    long long last_used_time;
    int is_active;
    int retry_count;
    char remote_addr[46];  // IPv6 address
    int remote_port;
    void *user_data;
} connection_info_t;

// Функции инициализации
int conn_pool_init(connection_pool_context_t *ctx);
int conn_pool_init_with_config(connection_pool_context_t *ctx, 
                              const connection_pool_config_t *config);
void conn_pool_cleanup(connection_pool_context_t *ctx);

// Функции управления соединениями
int conn_pool_acquire_connection(connection_pool_context_t *ctx, 
                               connection_info_t *conn_info);
int conn_pool_release_connection(connection_pool_context_t *ctx, 
                               int conn_fd);
int conn_pool_create_connection(connection_pool_context_t *ctx);
int conn_pool_destroy_connection(connection_pool_context_t *ctx, 
                               int conn_fd);
int conn_pool_close_all_connections(connection_pool_context_t *ctx);

// Функции масштабирования
int conn_pool_scale_up(connection_pool_context_t *ctx);
int conn_pool_scale_down(connection_pool_context_t *ctx);
int conn_pool_check_scaling_requirements(connection_pool_context_t *ctx);
int conn_pool_get_required_size(connection_pool_context_t *ctx);

// Функции мониторинга и обслуживания
int conn_pool_perform_health_check(connection_pool_context_t *ctx);
int conn_pool_recycle_old_connections(connection_pool_context_t *ctx);
int conn_pool_cleanup_idle_connections(connection_pool_context_t *ctx);
int conn_pool_update_statistics(connection_pool_context_t *ctx);

// Функции получения информации
int conn_pool_get_connection_info(connection_pool_context_t *ctx, 
                                int conn_fd, 
                                connection_info_t *info);
int conn_pool_get_pool_size(connection_pool_context_t *ctx);
int conn_pool_get_active_count(connection_pool_context_t *ctx);
int conn_pool_get_idle_count(connection_pool_context_t *ctx);
int conn_pool_get_busy_count(connection_pool_context_t *ctx);

// Функции статистики
connection_pool_stats_t conn_pool_get_stats(connection_pool_context_t *ctx);
void conn_pool_reset_stats(connection_pool_context_t *ctx);

// Функции конфигурации
void conn_pool_get_config(connection_pool_context_t *ctx, 
                         connection_pool_config_t *config);
int conn_pool_update_config(connection_pool_context_t *ctx, 
                           const connection_pool_config_t *new_config);

// Вспомогательные функции
int conn_pool_is_available(void);
double conn_pool_get_utilization(connection_pool_context_t *ctx);
int conn_pool_estimate_load(connection_pool_context_t *ctx);
int conn_pool_get_peak_utilization(connection_pool_context_t *ctx);
int conn_pool_set_callback_functions(int (*connect_func)(const char*, int),
                                   int (*disconnect_func)(int),
                                   int (*send_func)(int, const void*, size_t),
                                   int (*recv_func)(int, void*, size_t));

#endif