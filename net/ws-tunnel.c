/*
 * ws-tunnel.c - WebSocket туннель для Telegram
 * WSS туннелирование с TCP fallback
 */

#include "ws-tunnel.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

/* Внутренние функции */
static int ws_create_handshake(const char *host, const char *path, char *buffer, int buffer_size);
static int ws_parse_response(const char *response, char *key, int key_size);
static int tcp_connect_to_dc(const char *ip, int port, int timeout_ms);

/* Инициализация туннеля */
int ws_tunnel_init(ws_tunnel_ctx_t *ctx, ws_tunnel_config_t *config) {
    if (!ctx) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(ws_tunnel_ctx_t));
    
    if (config) {
        memcpy(&ctx->config, config, sizeof(ws_tunnel_config_t));
    } else {
        /* Конфигурация по умолчанию */
        ctx->config.use_websocket = 1;
        ctx->config.auto_fallback = 1;
        ctx->config.dc_auto_select = 1;
        ctx->config.connection_timeout = 5000;
        strcpy(ctx->config.ws_server_prefix, "kws");
        
        /* DC по умолчанию */
        telegram_dc_t default_dcs[] = {
            {1, "149.154.175.53", 443, 0, 0, 1},
            {2, "149.154.167.220", 443, 0, 0, 1},
            {3, "149.154.175.100", 443, 0, 0, 1},
            {4, "149.154.167.214", 443, 0, 0, 1},
            {5, "149.154.171.5", 443, 0, 0, 1}
        };
        memcpy(ctx->config.dcs, default_dcs, sizeof(default_dcs));
        ctx->config.dc_count = 5;
    }
    
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        return -1;
    }
    
    /* Инициализация OpenSSL */
    SSL_library_init();
    SSL_load_error_strings();
    ctx->ssl_ctx = SSL_CTX_new(TLS_client_method());
    
    if (!ctx->ssl_ctx) {
        pthread_mutex_destroy(&ctx->lock);
        return -1;
    }
    
    ctx->initialized = 1;
    ctx->connection_count = 0;
    
    return 0;
}

/* Освобождение ресурсов */
void ws_tunnel_destroy(ws_tunnel_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    
    /* Закрываем все подключения */
    for (int i = 0; i < WS_TUNNEL_MAX_CONNECTIONS; i++) {
        if (ctx->connections[i].connected) {
            ws_tunnel_close(ctx, i);
        }
    }
    
    if (ctx->ssl_ctx) {
        SSL_CTX_free(ctx->ssl_ctx);
    }
    
    pthread_mutex_destroy(&ctx->lock);
    ctx->initialized = 0;
}

/* Создание WebSocket handshake */
static int ws_create_handshake(const char *host, const char *path, char *buffer, int buffer_size) {
    /* Генерируем ключ */
    unsigned char key_bytes[16];
    for (int i = 0; i < 16; i++) {
        key_bytes[i] = rand() % 256;
    }
    
    /* Base64 кодирование (упрощённое) */
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char key[25];
    int j = 0;
    for (int i = 0; i < 16; i += 3) {
        key[j++] = base64[key_bytes[i] >> 2];
        key[j++] = base64[((key_bytes[i] & 0x03) << 4) | (key_bytes[i + 1] >> 4)];
        key[j++] = base64[((key_bytes[i + 1] & 0x0F) << 2) | (key_bytes[i + 2] >> 6)];
        key[j++] = base64[key_bytes[i + 2] & 0x3F];
    }
    key[24] = '\0';
    
    return snprintf(buffer, buffer_size,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: binary\r\n"
        "\r\n",
        path, host, key);
}

/* TCP подключение к DC */
static int tcp_connect_to_dc(const char *ip, int port, int timeout_ms) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    
    /* Устанавливаем таймаут */
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

/* Создание WebSocket подключения */
int ws_tunnel_connect(ws_tunnel_ctx_t *ctx, int dc_id) {
    if (!ctx || !ctx->initialized || dc_id < 1 || dc_id > WS_TUNNEL_MAX_DC) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    /* Находим свободный слот */
    int slot = -1;
    for (int i = 0; i < WS_TUNNEL_MAX_CONNECTIONS; i++) {
        if (!ctx->connections[i].connected) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        pthread_mutex_unlock(&ctx->lock);
        ctx->stats.failed_connections++;
        return -1;
    }
    
    /* Находим DC */
    telegram_dc_t *dc = NULL;
    for (int i = 0; i < ctx->config.dc_count; i++) {
        if (ctx->config.dcs[i].dc_id == dc_id && ctx->config.dcs[i].is_available) {
            dc = &ctx->config.dcs[i];
            break;
        }
    }
    
    if (!dc) {
        pthread_mutex_unlock(&ctx->lock);
        ctx->stats.failed_connections++;
        return -1;
    }
    
    ws_connection_t *conn = &ctx->connections[slot];
    memset(conn, 0, sizeof(ws_connection_t));
    conn->id = slot;
    conn->dc_id = dc_id;
    conn->created = time(NULL);
    conn->last_activity = conn->created;
    
    /* Пытаемся WebSocket */
    if (ctx->config.use_websocket) {
        char ws_url[256];
        snprintf(ws_url, sizeof(ws_url), "%s%d.web.telegram.org", 
                 ctx->config.ws_server_prefix, dc_id);
        
        strncpy(conn->ws_url, ws_url, sizeof(conn->ws_url) - 1);
        
        /* Создаём SSL подключение */
        conn->ssl = SSL_new(ctx->ssl_ctx);
        if (!conn->ssl) {
            pthread_mutex_unlock(&ctx->lock);
            ctx->stats.tcp_fallbacks++;
            /* Fallback к TCP */
            return ws_tunnel_fallback_to_tcp(ctx, slot);
        }
        
        /* Подключаемся */
        int tcp_fd = tcp_connect_to_dc(dc->ip, dc->port, ctx->config.connection_timeout);
        if (tcp_fd < 0) {
            SSL_free(conn->ssl);
            conn->ssl = NULL;
            pthread_mutex_unlock(&ctx->lock);
            ctx->stats.tcp_fallbacks++;
            return ws_tunnel_fallback_to_tcp(ctx, slot);
        }
        
        SSL_set_fd(conn->ssl, tcp_fd);
        SSL_set_tlsext_host_name(conn->ssl, ws_url);
        
        if (SSL_connect(conn->ssl) <= 0) {
            close(tcp_fd);
            SSL_free(conn->ssl);
            conn->ssl = NULL;
            pthread_mutex_unlock(&ctx->lock);
            ctx->stats.tcp_fallbacks++;
            return ws_tunnel_fallback_to_tcp(ctx, slot);
        }
        
        /* Отправляем WebSocket handshake */
        char handshake[512];
        char path[128];
        snprintf(path, sizeof(path), "/apiws");
        int hs_len = ws_create_handshake(ws_url, path, handshake, sizeof(handshake));
        
        if (SSL_write(conn->ssl, handshake, hs_len) <= 0) {
            SSL_shutdown(conn->ssl);
            close(tcp_fd);
            SSL_free(conn->ssl);
            conn->ssl = NULL;
            pthread_mutex_unlock(&ctx->lock);
            ctx->stats.tcp_fallbacks++;
            return ws_tunnel_fallback_to_tcp(ctx, slot);
        }
        
        /* Читаем ответ (упрощённо) */
        char response[1024];
        int resp_len = SSL_read(conn->ssl, response, sizeof(response) - 1);
        if (resp_len <= 0 || strstr(response, "101 Switching Protocols") == NULL) {
            SSL_shutdown(conn->ssl);
            close(tcp_fd);
            SSL_free(conn->ssl);
            conn->ssl = NULL;
            pthread_mutex_unlock(&ctx->lock);
            ctx->stats.tcp_fallbacks++;
            return ws_tunnel_fallback_to_tcp(ctx, slot);
        }
        
        conn->connected = 1;
        ctx->stats.ws_connections++;
        ctx->stats.total_connections++;
        ctx->stats.active_connections++;
        
        pthread_mutex_unlock(&ctx->lock);
        return slot;
    }
    
    /* Fallback к TCP */
    pthread_mutex_unlock(&ctx->lock);
    ctx->stats.tcp_fallbacks++;
    return ws_tunnel_fallback_to_tcp(ctx, slot);
}

/* TCP fallback */
int ws_tunnel_fallback_to_tcp(ws_tunnel_ctx_t *ctx, int conn_id) {
    if (!ctx || conn_id < 0 || conn_id >= WS_TUNNEL_MAX_CONNECTIONS) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    ws_connection_t *conn = &ctx->connections[conn_id];
    
    /* Находим DC */
    telegram_dc_t *dc = NULL;
    for (int i = 0; i < ctx->config.dc_count; i++) {
        if (ctx->config.dcs[i].dc_id == conn->dc_id && ctx->config.dcs[i].is_available) {
            dc = &ctx->config.dcs[i];
            break;
        }
    }
    
    if (!dc) {
        pthread_mutex_unlock(&ctx->lock);
        return -1;
    }
    
    /* TCP подключение */
    int fd = tcp_connect_to_dc(dc->ip, dc->port, ctx->config.connection_timeout);
    if (fd < 0) {
        pthread_mutex_unlock(&ctx->lock);
        return -1;
    }
    
    conn->tcp_fd = fd;
    conn->connected = 1;
    conn->last_activity = time(NULL);
    
    ctx->stats.total_connections++;
    ctx->stats.active_connections++;
    
    pthread_mutex_unlock(&ctx->lock);
    return conn_id;
}

/* Отправка данных */
int ws_tunnel_send(ws_tunnel_ctx_t *ctx, int conn_id, const void *data, int len) {
    if (!ctx || conn_id < 0 || conn_id >= WS_TUNNEL_MAX_CONNECTIONS || !data || len <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    ws_connection_t *conn = &ctx->connections[conn_id];
    if (!conn->connected) {
        pthread_mutex_unlock(&ctx->lock);
        return -1;
    }
    
    int sent = 0;
    
    if (conn->ssl) {
        /* WebSocket frame */
        unsigned char frame[WS_TUNNEL_BUFFER_SIZE + 14];
        int frame_len = 0;
        
        frame[frame_len++] = 0x82; /* Binary frame, FIN */
        
        if (len < 126) {
            frame[frame_len++] = len;
        } else if (len < 65536) {
            frame[frame_len++] = 126;
            frame[frame_len++] = (len >> 8) & 0xFF;
            frame[frame_len++] = len & 0xFF;
        } else {
            frame[frame_len++] = 127;
            for (int i = 7; i >= 0; i--) {
                frame[frame_len++] = (len >> (i * 8)) & 0xFF;
            }
        }
        
        memcpy(frame + frame_len, data, len);
        frame_len += len;
        
        sent = SSL_write(conn->ssl, frame, frame_len);
    } else {
        /* TCP */
        sent = write(conn->tcp_fd, data, len);
    }
    
    if (sent > 0) {
        conn->bytes_sent += sent;
        ctx->stats.total_bytes_sent += sent;
        conn->last_activity = time(NULL);
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return sent;
}

/* Получение данных */
int ws_tunnel_recv(ws_tunnel_ctx_t *ctx, int conn_id, void *buffer, int max_len) {
    if (!ctx || conn_id < 0 || conn_id >= WS_TUNNEL_MAX_CONNECTIONS || !buffer || max_len <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    ws_connection_t *conn = &ctx->connections[conn_id];
    if (!conn->connected) {
        pthread_mutex_unlock(&ctx->lock);
        return -1;
    }
    
    int received = 0;
    
    if (conn->ssl) {
        unsigned char frame[WS_TUNNEL_BUFFER_SIZE + 14];
        int frame_len = SSL_read(conn->ssl, frame, sizeof(frame));
        
        if (frame_len <= 0) {
            pthread_mutex_unlock(&ctx->lock);
            return -1;
        }
        
        /* Парсим WebSocket frame */
        if ((frame[0] & 0x8F) != 0x82) {
            pthread_mutex_unlock(&ctx->lock);
            return -1;
        }
        
        int payload_start = 2;
        int payload_len = frame[1] & 0x7F;
        
        if (payload_len == 126) {
            payload_start = 4;
            payload_len = (frame[2] << 8) | frame[3];
        } else if (payload_len == 127) {
            payload_start = 10;
            payload_len = 0;
            for (int i = 0; i < 8; i++) {
                payload_len = (payload_len << 8) | frame[2 + i];
            }
        }
        
        received = (payload_len < max_len) ? payload_len : max_len;
        memcpy(buffer, frame + payload_start, received);
    } else {
        /* TCP */
        received = read(conn->tcp_fd, buffer, max_len);
    }
    
    if (received > 0) {
        conn->bytes_received += received;
        ctx->stats.total_bytes_received += received;
        conn->last_activity = time(NULL);
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return received;
}

/* Закрытие подключения */
int ws_tunnel_close(ws_tunnel_ctx_t *ctx, int conn_id) {
    if (!ctx || conn_id < 0 || conn_id >= WS_TUNNEL_MAX_CONNECTIONS) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    ws_connection_t *conn = &ctx->connections[conn_id];
    
    if (!conn->connected) {
        pthread_mutex_unlock(&ctx->lock);
        return -1;
    }
    
    if (conn->ssl) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
        conn->ssl = NULL;
    }
    
    if (conn->tcp_fd >= 0) {
        close(conn->tcp_fd);
        conn->tcp_fd = -1;
    }
    
    conn->connected = 0;
    
    if (ctx->stats.active_connections > 0) {
        ctx->stats.active_connections--;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

/* Замер задержки */
uint64_t ws_tunnel_measure_latency(ws_tunnel_ctx_t *ctx, int dc_id) {
    if (!ctx || dc_id < 1 || dc_id > WS_TUNNEL_MAX_DC) {
        return 0;
    }
    
    telegram_dc_t *dc = NULL;
    for (int i = 0; i < ctx->config.dc_count; i++) {
        if (ctx->config.dcs[i].dc_id == dc_id) {
            dc = &ctx->config.dcs[i];
            break;
        }
    }
    
    if (!dc) {
        return 0;
    }
    
    struct timeval start, end;
    
    gettimeofday(&start, NULL);
    
    int fd = tcp_connect_to_dc(dc->ip, dc->port, 1000);
    if (fd >= 0) {
        close(fd);
    }
    
    gettimeofday(&end, NULL);
    
    uint64_t latency = (end.tv_sec - start.tv_sec) * 1000 + 
                       (end.tv_usec - start.tv_usec) / 1000;
    
    dc->latency = latency;
    
    return latency;
}

/* Выбор лучшего DC */
int ws_tunnel_select_best_dc(ws_tunnel_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    int best_dc = -1;
    uint64_t min_latency = UINT64_MAX;
    
    for (int i = 0; i < ctx->config.dc_count; i++) {
        if (!ctx->config.dcs[i].is_available) {
            continue;
        }
        
        /* Замеряем задержку */
        uint64_t latency = ws_tunnel_measure_latency(ctx, ctx->config.dcs[i].dc_id);
        
        if (latency > 0 && latency < min_latency) {
            min_latency = latency;
            best_dc = ctx->config.dcs[i].dc_id;
        }
    }
    
    if (best_dc > 0) {
        ctx->stats.dc_switches++;
    }
    
    return best_dc;
}

/* Получение статистики */
void ws_tunnel_get_stats(ws_tunnel_ctx_t *ctx, ws_tunnel_stats_t *stats) {
    if (!ctx || !stats) {
        return;
    }
    
    pthread_mutex_lock(&ctx->lock);
    memcpy(stats, &ctx->stats, sizeof(ws_tunnel_stats_t));
    pthread_mutex_unlock(&ctx->lock);
}

/* Экспорт статистики в JSON */
int ws_tunnel_export_json(ws_tunnel_ctx_t *ctx, char *buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size < 512) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"stats\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_connections\": %llu,\n", 
                       (unsigned long long)ctx->stats.total_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"active_connections\": %llu,\n", 
                       (unsigned long long)ctx->stats.active_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_bytes_sent\": %llu,\n", 
                       (unsigned long long)ctx->stats.total_bytes_sent);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_bytes_received\": %llu,\n", 
                       (unsigned long long)ctx->stats.total_bytes_received);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"ws_connections\": %llu,\n", 
                       (unsigned long long)ctx->stats.ws_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"tcp_fallbacks\": %llu,\n", 
                       (unsigned long long)ctx->stats.tcp_fallbacks);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"failed_connections\": %llu,\n", 
                       (unsigned long long)ctx->stats.failed_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"dc_switches\": %llu\n", 
                       (unsigned long long)ctx->stats.dc_switches);
    offset += snprintf(buffer + offset, buffer_size - offset, "  },\n");
    
    /* DC информация */
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"dcs\": [\n");
    for (int i = 0; i < ctx->config.dc_count; i++) {
        telegram_dc_t *dc = &ctx->config.dcs[i];
        offset += snprintf(buffer + offset, buffer_size - offset, 
                           "    {\"id\": %d, \"ip\": \"%s\", \"port\": %d, \"latency\": %llu, \"available\": %s}%s\n",
                           dc->dc_id, dc->ip, dc->port, 
                           (unsigned long long)dc->latency,
                           dc->is_available ? "true" : "false",
                           (i < ctx->config.dc_count - 1) ? "," : "");
    }
    offset += snprintf(buffer + offset, buffer_size - offset, "  ]\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    pthread_mutex_unlock(&ctx->lock);
    
    return offset;
}

/* Обновление DC */
int ws_tunnel_update_dc(ws_tunnel_ctx_t *ctx, int dc_id, const char *ip, int port) {
    if (!ctx || !ip) {
        return -1;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    for (int i = 0; i < ctx->config.dc_count; i++) {
        if (ctx->config.dcs[i].dc_id == dc_id) {
            strncpy(ctx->config.dcs[i].ip, ip, sizeof(ctx->config.dcs[i].ip) - 1);
            ctx->config.dcs[i].port = port;
            ctx->config.dcs[i].is_available = 1;
            pthread_mutex_unlock(&ctx->lock);
            return 0;
        }
    }
    
    /* Добавляем новый DC */
    if (ctx->config.dc_count < WS_TUNNEL_MAX_DC) {
        telegram_dc_t *dc = &ctx->config.dcs[ctx->config.dc_count];
        dc->dc_id = dc_id;
        strncpy(dc->ip, ip, sizeof(dc->ip) - 1);
        dc->port = port;
        dc->is_available = 1;
        dc->latency = 0;
        dc->is_premium = 0;
        ctx->config.dc_count++;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

/* Извлечение DC ID из MTProto пакета */
int ws_tunnel_extract_dc_id(const void *packet, int packet_len) {
    if (!packet || packet_len < 8) {
        return -1;
    }
    
    const unsigned char *data = (const unsigned char *)packet;
    
    /* MTProto auth key id находится на позиции 0-8 */
    /* DC ID можно извлечь из auth key id */
    uint64_t auth_key_id = 0;
    for (int i = 0; i < 8; i++) {
        auth_key_id |= ((uint64_t)data[i]) << (i * 8);
    }
    
    /* DC ID в старших битах */
    int dc_id = (auth_key_id >> 56) & 0xFF;
    
    if (dc_id >= 1 && dc_id <= 10) {
        return dc_id;
    }
    
    return -1;
}
