/*
 * gRPC Server for MTProxy
 * gRPC интерфейс управления MTProxy сервером
 *
 * Поддерживаемые методы:
 * - Управление сервером (start, stop, restart, status)
 * - Конфигурация (get, update, validate)
 * - Статистика и мониторинг
 * - Управление секретами
 * - Rate limiting
 * - Логирование
 */

#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Константы
 * ============================================================================ */

#define GRPC_SERVER_VERSION "1.0.0"
#define GRPC_SERVER_DEFAULT_PORT 50051
#define GRPC_SERVER_MAX_CONNECTIONS 100
#define GRPC_SERVER_MAX_SECRETS 64
#define GRPC_SERVER_MAX_LOG_ENTRIES 1000
#define GRPC_SERVER_MAX_CONNECTION_INFO 256
#define GRPC_SERVER_MAX_WHITELIST_IPS 128

/* ============================================================================
 * Типы данных gRPC
 * ============================================================================ */

/**
 * @brief Состояние сервера
 */
typedef enum {
    GRPC_SERVER_STATE_UNKNOWN = 0,
    GRPC_SERVER_STATE_STARTING = 1,
    GRPC_SERVER_STATE_RUNNING = 2,
    GRPC_SERVER_STATE_STOPPING = 3,
    GRPC_SERVER_STATE_STOPPED = 4,
    GRPC_SERVER_STATE_ERROR = 5
} grpc_server_state_t;

/**
 * @brief Информация о платформе
 */
typedef struct {
    char os[64];
    char arch[32];
    int32_t cpu_cores;
    int64_t total_memory_bytes;
} grpc_platform_info_t;

/**
 * @brief Статус сервера
 */
typedef struct {
    grpc_server_state_t state;
    char version[32];
    char commit_hash[64];
    int64_t uptime_seconds;
    int64_t start_time_unix;
    char error_message[512];
    grpc_platform_info_t platform;
} grpc_server_status_t;

/**
 * @brief TLS конфигурация
 */
typedef struct {
    bool enabled;
    char cert_file[512];
    char key_file[512];
    char min_version[32];  // TLS1.2, TLS1.3
} grpc_tls_config_t;

/**
 * @brief Rate limit конфигурация
 */
typedef struct {
    bool enabled;
    int32_t requests_per_minute;
    int32_t burst_size;
    bool whitelist_enabled;
    char whitelist_ips[GRPC_SERVER_MAX_WHITELIST_IPS][64];
    int32_t whitelist_count;
} grpc_rate_limit_config_t;

/**
 * @brief Конфигурация прокси
 */
typedef struct {
    int32_t port;
    bool ipv6_enabled;
    int32_t max_connections;
    int32_t backlog;
    char secrets[GRPC_SERVER_MAX_SECRETS][128];
    int32_t secrets_count;
    char log_file[512];
    char log_level[32];
    int32_t workers;
    bool use_aes_ni;
    grpc_rate_limit_config_t rate_limit;
    grpc_tls_config_t tls;
} grpc_proxy_config_t;

/**
 * @brief Результат валидации
 */
typedef struct {
    bool valid;
    char errors[16][512];
    int32_t error_count;
    char warnings[16][512];
    int32_t warning_count;
} grpc_validation_result_t;

/**
 * @brief Статистика прокси
 */
typedef struct {
    int32_t active_connections;
    int64_t total_connections;
    int64_t connections_per_second;
    int64_t bytes_sent;
    int64_t bytes_received;
    int64_t bytes_per_second_in;
    int64_t bytes_per_second_out;
    double cpu_usage_percent;
    int64_t memory_usage_bytes;
    double memory_usage_mb;
    int64_t total_errors;
    int64_t errors_per_second;
    int64_t rate_limited_requests;
    int64_t timestamp_unix;
    int64_t uptime_seconds;
} grpc_proxy_statistics_t;

/**
 * @brief Информация о подключении
 */
typedef struct {
    char id[64];
    char client_ip[64];
    int32_t client_port;
    int64_t connected_at_unix;
    int64_t duration_seconds;
    int64_t bytes_sent;
    int64_t bytes_received;
    char status[32];
    char protocol_version[32];
} grpc_connection_info_t;

/**
 * @brief Список подключений
 */
typedef struct {
    grpc_connection_info_t connections[GRPC_SERVER_MAX_CONNECTION_INFO];
    int32_t total_count;
    int32_t returned_count;
} grpc_connection_list_t;

/**
 * @brief Информация о секрете
 */
typedef struct {
    char secret_hash[128];  // Хеш для безопасности
    char description[256];
    int64_t created_at_unix;
    int64_t last_used_unix;
    int64_t usage_count;
} grpc_secret_info_t;

/**
 * @brief Список секретов
 */
typedef struct {
    grpc_secret_info_t secrets[GRPC_SERVER_MAX_SECRETS];
    int32_t total_count;
    int32_t returned_count;
} grpc_secret_list_t;

/**
 * @brief Ответ на операцию с секретом
 */
typedef struct {
    bool success;
    char message[512];
    char secret[128];
} grpc_secret_response_t;

/**
 * @brief Запись лога
 */
typedef struct {
    int64_t timestamp_unix;
    char level[16];  // ERROR, WARN, INFO, DEBUG
    char message[1024];
    char source[128];
    // Fields map ограничен для C
    char field_keys[16][64];
    char field_values[16][256];
    int32_t field_count;
} grpc_log_entry_t;

/**
 * @brief Список записей лога
 */
typedef struct {
    grpc_log_entry_t entries[GRPC_SERVER_MAX_LOG_ENTRIES];
    int32_t total_count;
    int32_t returned_count;
} grpc_log_entries_t;

/* ============================================================================
 * Конфигурация gRPC сервера
 * ============================================================================ */

/**
 * @brief Конфигурация gRPC сервера
 */
typedef struct {
    bool enabled;
    int32_t port;
    char bind_address[64];
    bool enable_tls;
    char tls_cert_file[512];
    char tls_key_file[512];
    bool enable_auth;
    char auth_token[256];
    bool enable_rate_limiting;
    int32_t max_connections;
    int32_t keepalive_seconds;
    bool enable_logging;
    bool enable_reflection;  // gRPC reflection service
} grpc_server_config_t;

/**
 * @brief Статистика gRPC сервера
 */
typedef struct {
    int64_t total_calls;
    int64_t calls_by_method[16];  // По методам
    int64_t total_errors;
    int64_t active_streams;
    double avg_latency_ms;
    double max_latency_ms;
    int64_t total_bytes_sent;
    int64_t total_bytes_received;
} grpc_server_stats_t;

/* ============================================================================
 * Контекст gRPC сервера
 * ============================================================================ */

/**
 * @brief Основной контекст gRPC сервера
 */
typedef struct {
    grpc_server_config_t config;
    grpc_server_stats_t stats;
    grpc_server_status_t status;

    void *grpc_server;      // gRPC server instance (opaque pointer)
    void *completion_queue; // gRPC completion queue

    bool running;
    bool initialized;

    pthread_t server_thread;
    pthread_mutex_t mutex;

    // Callback функции для интеграции с MTProxy
    int (*get_status_callback)(grpc_server_status_t *status);
    int (*start_server_callback)(bool force);
    int (*stop_server_callback)(bool graceful, int32_t timeout_sec);
    int (*restart_server_callback)(bool graceful, int32_t timeout_sec);
    int (*get_config_callback)(grpc_proxy_config_t *config);
    int (*update_config_callback)(const grpc_proxy_config_t *config, bool reload);
    int (*validate_config_callback)(const grpc_proxy_config_t *config, grpc_validation_result_t *result);
    int (*get_statistics_callback)(grpc_proxy_statistics_t *stats);
    int (*get_connections_callback)(int32_t limit, int32_t offset, const char *filter_ip, grpc_connection_list_t *list);
    int (*add_secret_callback)(const char *secret, const char *description);
    int (*remove_secret_callback)(const char *secret);
    int (*list_secrets_callback)(grpc_secret_list_t *list);
    int (*get_rate_limits_callback)(grpc_rate_limit_config_t *config);
    int (*update_rate_limit_callback)(const grpc_rate_limit_config_t *config);
    int (*get_logs_callback)(int32_t limit, const char *level, int64_t start_time, int64_t end_time, grpc_log_entries_t *entries);

} grpc_server_context_t;

/* ============================================================================
 * API инициализации и управления
 * ============================================================================ */

/**
 * @brief Инициализация gRPC сервера
 * @param ctx Контекст сервера
 * @param config Конфигурация
 * @return 0 при успехе, код ошибки при неудаче
 */
int grpc_server_init(grpc_server_context_t *ctx, const grpc_server_config_t *config);

/**
 * @brief Запуск gRPC сервера
 * @param ctx Контекст сервера
 * @return 0 при успехе, код ошибки при неудаче
 */
int grpc_server_start(grpc_server_context_t *ctx);

/**
 * @brief Остановка gRPC сервера
 * @param ctx Контекст сервера
 */
void grpc_server_stop(grpc_server_context_t *ctx);

/**
 * @brief Очистка gRPC сервера
 * @param ctx Контекст сервера
 */
void grpc_server_cleanup(grpc_server_context_t *ctx);

/**
 * @brief Регистрация callback функции для получения статуса
 */
void grpc_server_set_status_callback(grpc_server_context_t *ctx,
                                    int (*callback)(grpc_server_status_t *status));

/**
 * @brief Регистрация callback функции для запуска сервера
 */
void grpc_server_set_start_callback(grpc_server_context_t *ctx,
                                   int (*callback)(bool force));

/**
 * @brief Регистрация callback функции для остановки сервера
 */
void grpc_server_set_stop_callback(grpc_server_context_t *ctx,
                                  int (*callback)(bool graceful, int32_t timeout_sec));

/**
 * @brief Регистрация callback функции для перезапуска сервера
 */
void grpc_server_set_restart_callback(grpc_server_context_t *ctx,
                                     int (*callback)(bool graceful, int32_t timeout_sec));

/**
 * @brief Регистрация callback функции для получения конфигурации
 */
void grpc_server_set_get_config_callback(grpc_server_context_t *ctx,
                                        int (*callback)(grpc_proxy_config_t *config));

/**
 * @brief Регистрация callback функции для обновления конфигурации
 */
void grpc_server_set_update_config_callback(grpc_server_context_t *ctx,
                                           int (*callback)(const grpc_proxy_config_t *config, bool reload));

/**
 * @brief Регистрация callback функции для валидации конфигурации
 */
void grpc_server_set_validate_config_callback(grpc_server_context_t *ctx,
                                             int (*callback)(const grpc_proxy_config_t *config, grpc_validation_result_t *result));

/**
 * @brief Регистрация callback функции для получения статистики
 */
void grpc_server_set_statistics_callback(grpc_server_context_t *ctx,
                                        int (*callback)(grpc_proxy_statistics_t *stats));

/**
 * @brief Регистрация callback функции для получения подключений
 */
void grpc_server_set_connections_callback(grpc_server_context_t *ctx,
                                         int (*callback)(int32_t limit, int32_t offset, const char *filter_ip, grpc_connection_list_t *list));

/**
 * @brief Регистрация callback функции для добавления секрета
 */
void grpc_server_set_add_secret_callback(grpc_server_context_t *ctx,
                                        int (*callback)(const char *secret, const char *description));

/**
 * @brief Регистрация callback функции для удаления секрета
 */
void grpc_server_set_remove_secret_callback(grpc_server_context_t *ctx,
                                           int (*callback)(const char *secret));

/**
 * @brief Регистрация callback функции для списка секретов
 */
void grpc_server_set_list_secrets_callback(grpc_server_context_t *ctx,
                                          int (*callback)(grpc_secret_list_t *list));

/**
 * @brief Регистрация callback функции для получения rate limits
 */
void grpc_server_set_rate_limits_callback(grpc_server_context_t *ctx,
                                         int (*callback)(grpc_rate_limit_config_t *config));

/**
 * @brief Регистрация callback функции для обновления rate limits
 */
void grpc_server_set_update_rate_limit_callback(grpc_server_context_t *ctx,
                                               int (*callback)(const grpc_rate_limit_config_t *config));

/**
 * @brief Регистрация callback функции для получения логов
 */
void grpc_server_set_logs_callback(grpc_server_context_t *ctx,
                                  int (*callback)(int32_t limit, const char *level, int64_t start_time, int64_t end_time, grpc_log_entries_t *entries));

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

/**
 * @brief Получение статистики gRPC сервера
 */
void grpc_server_get_stats(grpc_server_context_t *ctx, grpc_server_stats_t *stats);

/**
 * @brief Сброс статистики
 */
void grpc_server_reset_stats(grpc_server_context_t *ctx);

/**
 * @brief Проверка, запущен ли сервер
 */
bool grpc_server_is_running(grpc_server_context_t *ctx);

/**
 * @brief Преобразование состояния сервера в строку
 */
const char *grpc_server_state_to_string(grpc_server_state_t state);

/**
 * @brief Преобразование строки в состояние сервера
 */
grpc_server_state_t grpc_server_state_from_string(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* GRPC_SERVER_H */
