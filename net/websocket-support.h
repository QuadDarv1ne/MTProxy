/*
    Система поддержки WebSocket протокола для MTProxy
    Совместимость с MTProto через WebSocket туннелирование
*/

#ifndef WEBSOCKET_SUPPORT_H
#define WEBSOCKET_SUPPORT_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация WebSocket
#define MAX_WEBSOCKET_CONNECTIONS 16384
#define WEBSOCKET_BUFFER_SIZE 8192
#define WEBSOCKET_HANDSHAKE_TIMEOUT 30000
#define MAX_WEBSOCKET_FRAME_SIZE 65536

// Состояния WebSocket соединения
typedef enum {
    WS_STATE_CONNECTING = 0,
    WS_STATE_OPEN = 1,
    WS_STATE_CLOSING = 2,
    WS_STATE_CLOSED = 3,
    WS_STATE_ERROR = 4
} websocket_state_t;

// Типы WebSocket фреймов
typedef enum {
    WS_FRAME_CONTINUATION = 0x0,
    WS_FRAME_TEXT = 0x1,
    WS_FRAME_BINARY = 0x2,
    WS_FRAME_CLOSE = 0x8,
    WS_FRAME_PING = 0x9,
    WS_FRAME_PONG = 0xA
} websocket_frame_type_t;

// Роли WebSocket
typedef enum {
    WS_ROLE_CLIENT = 0,
    WS_ROLE_SERVER = 1
} websocket_role_t;

// Флаги WebSocket фрейма
typedef enum {
    WS_FLAG_FIN = 0x80,
    WS_FLAG_RSV1 = 0x40,
    WS_FLAG_RSV2 = 0x20,
    WS_FLAG_RSV3 = 0x10,
    WS_FLAG_MASK = 0x80
} websocket_flags_t;

// WebSocket фрейм
typedef struct websocket_frame {
    websocket_frame_type_t type;
    int fin;           // Final fragment
    int rsv1, rsv2, rsv3;
    int masked;
    uint64_t payload_length;
    unsigned char masking_key[4];
    unsigned char *payload;
    size_t payload_size;
} websocket_frame_t;

// WebSocket соединение
typedef struct websocket_connection {
    uint64_t connection_id;
    websocket_state_t state;
    websocket_role_t role;
    int fd;
    int is_secure;     // WSS (WebSocket Secure)
    
    // Буферы
    unsigned char *read_buffer;
    size_t read_buffer_size;
    size_t read_buffer_pos;
    
    unsigned char *write_buffer;
    size_t write_buffer_size;
    size_t write_buffer_pos;
    
    // WebSocket специфичные данные
    char sec_websocket_key[256];
    char sec_websocket_accept[256];
    char websocket_protocol[64];
    char websocket_version[16];
    
    // MTProto интеграция
    int mtproto_tunnel_enabled;
    uint64_t mtproto_session_id;
    unsigned char mtproto_key[32];
    
    // Статистика
    long long bytes_received;
    long long bytes_sent;
    long long frames_received;
    long long frames_sent;
    long long ping_count;
    long long pong_count;
    
    // Временные метки
    long long connect_time;
    long long last_activity;
    long long last_ping;
    
    // Ошибки
    int error_code;
    char error_message[256];
} websocket_connection_t;

// Конфигурация WebSocket сервера
typedef struct websocket_config {
    int enable_server;
    int enable_client;
    int enable_ssl;
    int max_connections;
    int buffer_size;
    int ping_interval_ms;
    int timeout_ms;
    
    // Поддерживаемые протоколы
    char supported_protocols[256];
    int enable_mtproto_tunnel;
    
    // SSL конфигурация
    char ssl_cert_file[256];
    char ssl_key_file[256];
    char ssl_ca_file[256];
} websocket_config_t;

// Статистика WebSocket
typedef struct websocket_stats {
    long long total_connections;
    long long active_connections;
    long long total_frames;
    long long total_bytes;
    long long handshake_success;
    long long handshake_failed;
    long long protocol_errors;
    double avg_handshake_time_ms;
    double avg_frame_processing_time_us;
} websocket_stats_t;

// Основная структура WebSocket поддержки
typedef struct websocket_support {
    // Конфигурация
    websocket_config_t config;
    
    // Соединения
    websocket_connection_t *connections;
    int connection_count;
    int max_connections;
    
    // Статистика
    websocket_stats_t stats;
    
    // Состояние
    int is_initialized;
    int is_running;
    long long start_time;
    
    // Callback функции
    void (*on_open)(websocket_connection_t *conn);
    void (*on_message)(websocket_connection_t *conn, websocket_frame_t *frame);
    void (*on_close)(websocket_connection_t *conn, int code, const char *reason);
    void (*on_error)(websocket_connection_t *conn, int error_code, const char *message);
} websocket_support_t;

// Инициализация
websocket_support_t* websocket_init(const websocket_config_t *config);
int websocket_configure(websocket_support_t *ws, const websocket_config_t *config);
void websocket_cleanup(websocket_support_t *ws);

// Управление соединениями
int websocket_accept_connection(websocket_support_t *ws, int fd);
int websocket_connect(websocket_support_t *ws, const char *url);
int websocket_close_connection(websocket_support_t *ws, uint64_t conn_id, int code, const char *reason);
websocket_connection_t* websocket_get_connection(websocket_support_t *ws, uint64_t conn_id);

// Обработка данных
int websocket_handle_data(websocket_support_t *ws, uint64_t conn_id);
int websocket_send_frame(websocket_support_t *ws, uint64_t conn_id, 
                        websocket_frame_type_t type, const void *data, size_t length);
int websocket_send_text(websocket_support_t *ws, uint64_t conn_id, const char *text);
int websocket_send_binary(websocket_support_t *ws, uint64_t conn_id, const void *data, size_t length);
int websocket_send_ping(websocket_support_t *ws, uint64_t conn_id);
int websocket_send_pong(websocket_support_t *ws, uint64_t conn_id);

// MTProto интеграция
int websocket_enable_mtproto_tunnel(websocket_support_t *ws, uint64_t conn_id, 
                                  const unsigned char *key, uint64_t session_id);
int websocket_mtproto_send_data(websocket_support_t *ws, uint64_t conn_id, 
                               const void *data, size_t length);
int websocket_mtproto_receive_data(websocket_support_t *ws, uint64_t conn_id, 
                                 void *buffer, size_t buffer_size);

// Обработка фреймов
int websocket_parse_frame(websocket_support_t *ws, uint64_t conn_id, 
                         websocket_frame_t *frame);
int websocket_build_frame(websocket_frame_t *frame, websocket_frame_type_t type,
                         int fin, const void *payload, size_t payload_length);
void websocket_free_frame(websocket_frame_t *frame);

// WebSocket handshake
int websocket_perform_server_handshake(websocket_support_t *ws, uint64_t conn_id);
int websocket_perform_client_handshake(websocket_support_t *ws, uint64_t conn_id, 
                                     const char *host, const char *path);

// Утилиты
const char* websocket_state_to_string(websocket_state_t state);
const char* websocket_frame_type_to_string(websocket_frame_type_t type);
uint64_t websocket_generate_connection_id(void);
int websocket_validate_handshake(websocket_support_t *ws, uint64_t conn_id);
char* websocket_generate_accept_key(const char *websocket_key);

// Статистика и мониторинг
void websocket_get_stats(websocket_support_t *ws, websocket_stats_t *stats);
void websocket_get_connection_stats(websocket_support_t *ws, uint64_t conn_id, 
                                  char *buffer, size_t buffer_size);
void websocket_reset_stats(websocket_support_t *ws);

// Callback регистрации
void websocket_set_open_callback(websocket_support_t *ws, 
                               void (*callback)(websocket_connection_t*));
void websocket_set_message_callback(websocket_support_t *ws,
                                  void (*callback)(websocket_connection_t*, websocket_frame_t*));
void websocket_set_close_callback(websocket_support_t *ws,
                                void (*callback)(websocket_connection_t*, int, const char*));
void websocket_set_error_callback(websocket_support_t *ws,
                                void (*callback)(websocket_connection_t*, int, const char*));

#endif // WEBSOCKET_SUPPORT_H