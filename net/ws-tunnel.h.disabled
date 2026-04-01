/*
 * ws-tunnel.h - WebSocket туннель для Telegram
 * WSS туннелирование с TCP fallback
 */

#ifndef __WS_TUNNEL_H__
#define __WS_TUNNEL_H__

#include <stdint.h>
#include <pthread.h>
#include <openssl/ssl.h>

#define WS_TUNNEL_MAX_CONNECTIONS 256
#define WS_TUNNEL_BUFFER_SIZE 65536
#define WS_TUNNEL_MAX_DC 10

/* Telegram DC информация */
typedef struct {
    int dc_id;
    char ip[64];
    int port;
    int is_premium;
    uint64_t latency;
    int is_available;
} telegram_dc_t;

/* WebSocket подключение */
typedef struct {
    int id;
    SSL *ssl;
    int tcp_fd;
    int connected;
    int dc_id;
    char ws_url[256];
    uint64_t bytes_sent;
    uint64_t bytes_received;
    time_t created;
    time_t last_activity;
} ws_connection_t;

/* Статистика туннеля */
typedef struct {
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t ws_connections;
    uint64_t tcp_fallbacks;
    uint64_t failed_connections;
    uint64_t dc_switches;
} ws_tunnel_stats_t;

/* Конфигурация туннеля */
typedef struct {
    int use_websocket;
    int auto_fallback;
    int dc_auto_select;
    int connection_timeout;
    char ws_server_prefix[128];
    telegram_dc_t dcs[WS_TUNNEL_MAX_DC];
    int dc_count;
} ws_tunnel_config_t;

/* Контекст туннеля */
typedef struct {
    ws_tunnel_config_t config;
    ws_connection_t connections[WS_TUNNEL_MAX_CONNECTIONS];
    int connection_count;
    ws_tunnel_stats_t stats;
    pthread_mutex_t lock;
    SSL_CTX *ssl_ctx;
    int initialized;
} ws_tunnel_ctx_t;

/* Инициализация туннеля */
int ws_tunnel_init(ws_tunnel_ctx_t *ctx, ws_tunnel_config_t *config);

/* Освобождение ресурсов */
void ws_tunnel_destroy(ws_tunnel_ctx_t *ctx);

/* Создание WebSocket подключения */
int ws_tunnel_connect(ws_tunnel_ctx_t *ctx, int dc_id);

/* Отправка данных через туннель */
int ws_tunnel_send(ws_tunnel_ctx_t *ctx, int conn_id, const void *data, int len);

/* Получение данных из туннеля */
int ws_tunnel_recv(ws_tunnel_ctx_t *ctx, int conn_id, void *buffer, int max_len);

/* Закрытие подключения */
int ws_tunnel_close(ws_tunnel_ctx_t *ctx, int conn_id);

/* Переключение на TCP fallback */
int ws_tunnel_fallback_to_tcp(ws_tunnel_ctx_t *ctx, int conn_id);

/* Замер задержки до DC */
uint64_t ws_tunnel_measure_latency(ws_tunnel_ctx_t *ctx, int dc_id);

/* Выбор лучшего DC */
int ws_tunnel_select_best_dc(ws_tunnel_ctx_t *ctx);

/* Получение статистики */
void ws_tunnel_get_stats(ws_tunnel_ctx_t *ctx, ws_tunnel_stats_t *stats);

/* Экспорт статистики в JSON */
int ws_tunnel_export_json(ws_tunnel_ctx_t *ctx, char *buffer, size_t buffer_size);

/* Обновление списка DC */
int ws_tunnel_update_dc(ws_tunnel_ctx_t *ctx, int dc_id, const char *ip, int port);

/* Извлечение DC ID из MTProto пакета */
int ws_tunnel_extract_dc_id(const void *packet, int packet_len);

#endif /* __WS_TUNNEL_H__ */
