/*
    Система надежности и мониторинга протоколов MTProxy
    Обнаружение ошибок, восстановление соединений, контроль качества
*/

#ifndef PROTOCOL_RELIABILITY_H
#define PROTOCOL_RELIABILITY_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация надежности
#define MAX_CONNECTION_TRACKING 65536
#define PROTOCOL_TIMEOUT_MS 30000
#define MAX_RECONNECT_ATTEMPTS 5
#define HEALTH_CHECK_INTERVAL_MS 5000
#define ERROR_WINDOW_SIZE 1000

// Состояния протокола
typedef enum {
    PROTOCOL_STATE_UNKNOWN = 0,
    PROTOCOL_STATE_CONNECTING = 1,
    PROTOCOL_STATE_HANDSHAKE = 2,
    PROTOCOL_STATE_ESTABLISHED = 3,
    PROTOCOL_STATE_DEGRADED = 4,
    PROTOCOL_STATE_ERROR = 5,
    PROTOCOL_STATE_CLOSED = 6
} protocol_state_t;

// Типы ошибок
typedef enum {
    PROTOCOL_ERROR_NONE = 0,
    PROTOCOL_ERROR_TIMEOUT = 1,
    PROTOCOL_ERROR_INVALID_HEADER = 2,
    PROTOCOL_ERROR_AUTH_FAILED = 3,
    PROTOCOL_ERROR_CRYPTO_ERROR = 4,
    PROTOCOL_ERROR_VERSION_MISMATCH = 5,
    PROTOCOL_ERROR_BUFFER_OVERFLOW = 6,
    PROTOCOL_ERROR_NETWORK = 7,
    PROTOCOL_ERROR_RESOURCE_LIMIT = 8
} protocol_error_t;

// Типы протоколов
typedef enum {
    PROTOCOL_TYPE_MTProto = 0,
    PROTOCOL_TYPE_SHADOWSOCKS = 1,
    PROTOCOL_TYPE_HTTP_PROXY = 2,
    PROTOCOL_TYPE_SOCKS5 = 3
} protocol_type_t;

// Информация о соединении
typedef struct connection_info {
    uint64_t connection_id;
    protocol_type_t protocol_type;
    protocol_state_t state;
    int fd;
    int remote_port;
    unsigned char remote_ip[16]; // IPv6
    long long connect_time;
    long long last_activity;
    long long bytes_sent;
    long long bytes_received;
    int error_count;
    protocol_error_t last_error;
    int reconnect_attempts;
    int is_encrypted;
    int is_authenticated;
} connection_info_t;

// Статистика ошибок
typedef struct error_stats {
    protocol_error_t error_type;
    long long occurrence_count;
    long long first_occurrence;
    long long last_occurrence;
    int affected_connections;
} error_stats_t;

// Состояние протокола
typedef struct protocol_reliability {
    // Отслеживание соединений
    connection_info_t *connections;
    int connection_count;
    int max_connections;
    
    // Статистика ошибок
    error_stats_t error_stats[16];
    int error_stats_count;
    
    // Метрики качества
    long long total_connections;
    long long successful_connections;
    long long failed_connections;
    long long timeout_connections;
    long long total_bytes_transferred;
    double avg_response_time_ms;
    double error_rate;
    
    // Параметры восстановления
    int auto_reconnect_enabled;
    int max_reconnect_attempts;
    int reconnect_delay_ms;
    int health_check_interval_ms;
    
    // Состояние
    int is_initialized;
    int is_monitoring_active;
    long long start_time;
    
    // Callback функции
    void (*error_callback)(connection_info_t *conn, protocol_error_t error);
    void (*reconnect_callback)(connection_info_t *conn);
    void (*health_callback)(connection_info_t *conn, int is_healthy);
} protocol_reliability_t;

// Инициализация
protocol_reliability_t* protocol_reliability_init(int max_connections);
int protocol_reliability_configure(protocol_reliability_t *reliability, 
                                 int auto_reconnect, int max_attempts, int reconnect_delay);
void protocol_reliability_cleanup(protocol_reliability_t *reliability);

// Управление соединениями
int protocol_reliability_track_connection(protocol_reliability_t *reliability, 
                                        int fd, protocol_type_t type, 
                                        const unsigned char *remote_ip, int remote_port);
int protocol_reliability_update_connection_state(protocol_reliability_t *reliability, 
                                               uint64_t conn_id, protocol_state_t state);
int protocol_reliability_record_activity(protocol_reliability_t *reliability, 
                                       uint64_t conn_id, long long bytes_sent, 
                                       long long bytes_received);
int protocol_reliability_close_connection(protocol_reliability_t *reliability, 
                                        uint64_t conn_id);

// Обработка ошибок
int protocol_reliability_handle_error(protocol_reliability_t *reliability, 
                                    uint64_t conn_id, protocol_error_t error);
int protocol_reliability_check_reconnect_needed(protocol_reliability_t *reliability, 
                                              uint64_t conn_id);
int protocol_reliability_initiate_reconnect(protocol_reliability_t *reliability, 
                                          uint64_t conn_id);

// Мониторинг
int protocol_reliability_start_monitoring(protocol_reliability_t *reliability);
int protocol_reliability_stop_monitoring(protocol_reliability_t *reliability);
int protocol_reliability_perform_health_check(protocol_reliability_t *reliability);
void protocol_reliability_check_timeouts(protocol_reliability_t *reliability);

// Статистика и отчеты
void protocol_reliability_get_stats(protocol_reliability_t *reliability, 
                                  char *buffer, size_t buffer_size);
void protocol_reliability_get_error_report(protocol_reliability_t *reliability, 
                                         char *buffer, size_t buffer_size);
void protocol_reliability_get_connection_report(protocol_reliability_t *reliability, 
                                              uint64_t conn_id, char *buffer, size_t buffer_size);
double protocol_reliability_get_success_rate(protocol_reliability_t *reliability);
double protocol_reliability_get_error_rate(protocol_reliability_t *reliability);

// Утилиты
const char* protocol_reliability_state_to_string(protocol_state_t state);
const char* protocol_reliability_error_to_string(protocol_error_t error);
const char* protocol_reliability_type_to_string(protocol_type_t type);
uint64_t protocol_reliability_generate_connection_id(void);
int protocol_reliability_is_connection_healthy(protocol_reliability_t *reliability, 
                                             uint64_t conn_id);

// Callback регистрации
void protocol_reliability_set_error_callback(protocol_reliability_t *reliability,
                                           void (*callback)(connection_info_t*, protocol_error_t));
void protocol_reliability_set_reconnect_callback(protocol_reliability_t *reliability,
                                               void (*callback)(connection_info_t*));
void protocol_reliability_set_health_callback(protocol_reliability_t *reliability,
                                            void (*callback)(connection_info_t*, int));

#endif // PROTOCOL_RELIABILITY_H