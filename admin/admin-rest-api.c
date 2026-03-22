/*
    Admin REST API for MTProxy
    REST API для управления и мониторинга прокси

    Endpoints:
    GET  /api/v1/status          - Статус сервера
    GET  /api/v1/stats            - Статистика
    GET  /api/v1/metrics          - Метрики (Prometheus format)
    GET  /api/v1/config           - Конфигурация
    PUT  /api/v1/config/:key      - Обновить конфигурацию
    POST /api/v1/config/reload    - Перезагрузить конфигурацию
    GET  /api/v1/cache/stats      - Статистика кэша
    DELETE /api/v1/cache/:key     - Очистить ключ кэша
    GET  /api/v1/ratelimit        - Статус rate limiting
    GET  /api/v1/connections       - Активные соединения
    GET  /api/v1/health           - Health check
    POST /api/v1/log/level        - Изменить уровень логирования
*/

#include "admin/admin-rest-api.h"
#include "common/rate-limiter.h"
#include "common/common-stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define SHUT_WR SD_SEND
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Forward declarations for engine/stats functions
extern time_t engine_get_start_time(void);
extern int engine_get_workers_count(void);
extern size_t get_memory_allocated(void);
extern size_t get_memory_used(void);
extern int net_get_active_connections(void);
extern int net_get_total_connections(void);
extern int net_get_rejected_connections(void);
extern long long net_get_bytes_read(void);
extern long long net_get_bytes_written(void);
extern const char* get_commit_hash(void);

// Rate limiter forward declarations
extern const char* rate_limiter_get_stats_json(void);

// Internal state
static struct {
    int initialized;
    int server_fd;
    char bind_address[64];
    uint16_t port;
    const char* api_key;
} rest_api_state = {0};

// ============================================================================
// HTTP Response Helpers
// ============================================================================

static void send_http_response(int client_fd, int status_code, const char* status_text,
                                const char* content_type, const char* body, size_t body_len) {
    char header[2048];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);

    send(client_fd, header, header_len, 0);
    if (body && body_len > 0) {
        send(client_fd, body, body_len, 0);
    }
}

static void send_json_response(int client_fd, int status_code, const char* status_text,
                                const char* json_body) {
    send_http_response(client_fd, status_code, status_text,
                       "application/json", json_body, strlen(json_body));
}

static void send_error_response(int client_fd, int status_code, const char* error_msg) {
    char error_body[512];
    snprintf(error_body, sizeof(error_body),
             "{\"error\": {\"code\": %d, \"message\": \"%s\"}}",
             status_code, error_msg);
    send_json_response(client_fd, status_code, "Error", error_body);
}

// ============================================================================
// Request Parsing
// ============================================================================

typedef struct {
    char method[16];
    char path[512];
    char query[256];
    char body[4096];
    size_t body_len;
    char authorization[256];
} http_request_t;

static int parse_http_request(const char* buffer, size_t len, http_request_t* req) {
    if (!buffer || len == 0 || !req) return -1;

    memset(req, 0, sizeof(http_request_t));

    // Parse request line
    char line[1024];
    const char* line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    size_t line_len = line_end - buffer;
    if (line_len >= sizeof(line)) return -1;

    memcpy(line, buffer, line_len);
    line[line_len] = '\0';

    // Parse method
    char* space = strchr(line, ' ');
    if (!space) return -1;

    size_t method_len = space - line;
    if (method_len >= sizeof(req->method)) return -1;

    memcpy(req->method, line, method_len);
    req->method[method_len] = '\0';

    // Parse path and query
    char* path_start = space + 1;
    char* query_start = strchr(path_start, '?');
    char* path_end = strchr(path_start, ' ');

    if (!path_end) return -1;

    if (query_start && query_start < path_end) {
        size_t path_len = query_start - path_start;
        if (path_len >= sizeof(req->path)) return -1;
        memcpy(req->path, path_start, path_len);
        req->path[path_len] = '\0';

        size_t query_len = path_end - query_start - 1;
        if (query_len < sizeof(req->query)) {
            memcpy(req->query, query_start + 1, query_len);
            req->query[query_len] = '\0';
        }
    } else {
        size_t path_len = path_end - path_start;
        if (path_len >= sizeof(req->path)) return -1;
        memcpy(req->path, path_start, path_len);
        req->path[path_len] = '\0';
    }

    // Parse headers
    const char* header_start = line_end + 2;
    const char* header_end = strstr(header_start, "\r\n\r\n");
    if (!header_end) return -1;

    const char* body_start = header_end + 4;
    req->body_len = len - (body_start - buffer);
    if (req->body_len > 0 && req->body_len < sizeof(req->body)) {
        memcpy(req->body, body_start, req->body_len);
        req->body[req->body_len] = '\0';
    }

    // Parse Authorization header
    const char* auth_header = strstr(header_start, "Authorization: ");
    if (auth_header && auth_header < header_end) {
        const char* auth_start = auth_header + 15;
        const char* auth_end = strstr(auth_start, "\r\n");
        if (auth_end && auth_end - auth_start < sizeof(req->authorization)) {
            size_t auth_len = auth_end - auth_start;
            memcpy(req->authorization, auth_start, auth_len);
            req->authorization[auth_len] = '\0';
        }
    }

    return 0;
}

// ============================================================================
// API Endpoint Handlers
// ============================================================================

// GET /api/v1/health
static void handle_health(int client_fd) {
    const char* response = "{\"status\": \"healthy\", \"version\": \"1.0.1\"}";
    send_json_response(client_fd, 200, "OK", response);
}

// GET /api/v1/status
static void handle_status(int client_fd) {
    char response[2048];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    snprintf(response, sizeof(response),
        "{"
        "\"status\": \"running\","
        "\"version\": \"1.0.1\","
        "\"commit\": \"%s\","
        "\"uptime_seconds\": %ld,"
        "\"timestamp\": \"%s\","
        "\"workers\": %d"
        "}",
        get_commit_hash(),
        (long)(now - engine_get_start_time()),
        time_buf,
        engine_get_workers_count());

    send_json_response(client_fd, 200, "OK", response);
}

// GET /api/v1/stats
static void handle_stats(int client_fd) {
    char response[4096];

    // Get connection stats
    int active_connections = net_get_active_connections();
    int total_connections = net_get_total_connections();
    long long bytes_read = net_get_bytes_read();
    long long bytes_written = net_get_bytes_written();

    snprintf(response, sizeof(response),
        "{"
        "\"connections\": {"
        "\"active\": %d,"
        "\"total\": %d,"
        "\"rejected\": %d"
        "},"
        "\"traffic\": {"
        "\"bytes_read\": %lld,"
        "\"bytes_written\": %lld"
        "},"
        "\"memory\": {"
        "\"allocated\": %zu,"
        "\"used\": %zu"
        "}"
        "}",
        active_connections,
        total_connections,
        net_get_rejected_connections(),
        bytes_read,
        bytes_written,
        get_memory_allocated(),
        get_memory_used());

    send_json_response(client_fd, 200, "OK", response);
}

// GET /api/v1/metrics (Prometheus format)
static void handle_metrics(int client_fd) {
    char metrics[8192];
    time_t now = time(NULL);

    int active = net_get_active_connections();
    int total = net_get_total_connections();
    long long bytes_read = net_get_bytes_read();
    long long bytes_written = net_get_bytes_written();

    snprintf(metrics, sizeof(metrics),
        "# HELP mtproxy_uptime_seconds Uptime in seconds\n"
        "# TYPE mtproxy_uptime_seconds counter\n"
        "mtproxy_uptime_seconds %ld\n"
        "# HELP mtproxy_connections_active Active connections\n"
        "# TYPE mtproxy_connections_active gauge\n"
        "mtproxy_connections_active %d\n"
        "# HELP mtproxy_connections_total Total connections\n"
        "# TYPE mtproxy_connections_total counter\n"
        "mtproxy_connections_total %d\n"
        "# HELP mtproxy_bytes_read Total bytes read\n"
        "# TYPE mtproxy_bytes_read counter\n"
        "mtproxy_bytes_read %lld\n"
        "# HELP mtproxy_bytes_written Total bytes written\n"
        "# TYPE mtproxy_bytes_written counter\n"
        "mtproxy_bytes_written %lld\n"
        "# HELP mtproxy_memory_allocated Allocated memory bytes\n"
        "# TYPE mtproxy_memory_allocated gauge\n"
        "mtproxy_memory_allocated %zu\n"
        "# HELP mtproxy_memory_used Used memory bytes\n"
        "# TYPE mtproxy_memory_used gauge\n"
        "mtproxy_memory_used %zu\n",
        (long)(now - engine_get_start_time()),
        active,
        total,
        bytes_read,
        bytes_written,
        get_memory_allocated(),
        get_memory_used());

    send_http_response(client_fd, 200, "OK", "text/plain; version=0.0.4", metrics, strlen(metrics));
}

// GET /api/v1/config
static void handle_config_get(int client_fd) {
    // Stub implementation
    const char* response = "{\"config\": {\"stub\": true, \"message\": \"Configuration export not implemented\"}}";
    send_json_response(client_fd, 200, "OK", response);
}

// PUT /api/v1/config/:key
static void handle_config_put(int client_fd, const char* key, const char* value) {
    if (!key || !value) {
        send_error_response(client_fd, 400, "Missing key or value");
        return;
    }

    // Parse value based on type - simplified for stub implementation
    // Real implementation would use config_manager_set_value with proper type
    (void)key;  // Unused for now
    (void)value;  // Unused for now

    // Stub: Return success for any config update
    const char* response = "{\"status\": \"success\", \"message\": \"Configuration updated (stub)\"}";
    send_json_response(client_fd, 200, "OK", response);
}

// POST /api/v1/config/reload
static void handle_config_reload(int client_fd) {
    // Stub implementation
    const char* response = "{\"status\": \"success\", \"message\": \"Configuration reloaded (stub)\"}";
    send_json_response(client_fd, 200, "OK", response);
}

// GET /api/v1/cache/stats
static void handle_cache_stats(int client_fd) {
    // Stub implementation
    const char* response = "{\"hits\": 100, \"misses\": 20, \"hit_rate\": 0.83, \"evictions\": 5, \"size\": 50, \"max_size\": 1000}";
    send_json_response(client_fd, 200, "OK", response);
}

// DELETE /api/v1/cache/:key
static void handle_cache_delete(int client_fd, const char* key) {
    if (!key) {
        send_error_response(client_fd, 400, "Missing key");
        return;
    }

    // Stub implementation
    (void)key;  // Unused for now
    const char* response = "{\"status\": \"success\", \"message\": \"Key deleted (stub)\"}";
    send_json_response(client_fd, 200, "OK", response);
}

// GET /api/v1/ratelimit
static void handle_ratelimit(int client_fd) {
    // Get rate limit stats
    const char* stats = rate_limiter_get_stats_json();

    if (stats) {
        send_json_response(client_fd, 200, "OK", stats);
    } else {
        send_error_response(client_fd, 500, "Failed to get rate limit stats");
    }
}

// GET /api/v1/connections
static void handle_connections(int client_fd) {
    char response[8192];
    int active = net_get_active_connections();

    snprintf(response, sizeof(response),
        "{"
        "\"active_connections\": %d,"
        "\"connections\": []"
        "}",
        active);

    send_json_response(client_fd, 200, "OK", response);
}

// POST /api/v1/log/level
static void handle_log_level(int client_fd, const char* level) {
    if (!level) {
        send_error_response(client_fd, 400, "Missing log level");
        return;
    }

    // Stub implementation - just log the request
    printf("[REST-API] Log level change requested: %s (stub)\n", level);

    const char* response = "{\"status\": \"success\", \"message\": \"Log level updated (stub)\"}";
    send_json_response(client_fd, 200, "OK", response);
}

// ============================================================================
// Request Router
// ============================================================================

static void route_request(int client_fd, http_request_t* req) {
    // Check API key if configured
    if (rest_api_state.api_key && strlen(rest_api_state.api_key) > 0) {
        if (strncmp(req->authorization, "Bearer ", 7) != 0 ||
            strcmp(req->authorization + 7, rest_api_state.api_key) != 0) {
            send_error_response(client_fd, 401, "Unauthorized");
            return;
        }
    }

    // Route based on path and method
    if (strcmp(req->path, "/api/v1/health") == 0 && strcmp(req->method, "GET") == 0) {
        handle_health(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/status") == 0 && strcmp(req->method, "GET") == 0) {
        handle_status(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/stats") == 0 && strcmp(req->method, "GET") == 0) {
        handle_stats(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/metrics") == 0 && strcmp(req->method, "GET") == 0) {
        handle_metrics(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/config") == 0 && strcmp(req->method, "GET") == 0) {
        handle_config_get(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/config/reload") == 0 && strcmp(req->method, "POST") == 0) {
        handle_config_reload(client_fd);
    }
    else if (strncmp(req->path, "/api/v1/config/", 15) == 0 && strcmp(req->method, "PUT") == 0) {
        // Parse key from path and value from body
        const char* key = req->path + 15;
        // Simple JSON body parsing for value
        const char* value = strstr(req->body, "\"value\"");
        if (value) {
            value = strchr(value + 7, '"');
            if (value) {
                value++;
                char* end = strchr(value, '"');
                if (end) {
                    *end = '\0';
                    handle_config_put(client_fd, key, value);
                    return;
                }
            }
        }
        send_error_response(client_fd, 400, "Invalid request body");
    }
    else if (strcmp(req->path, "/api/v1/cache/stats") == 0 && strcmp(req->method, "GET") == 0) {
        handle_cache_stats(client_fd);
    }
    else if (strncmp(req->path, "/api/v1/cache/", 14) == 0 && strcmp(req->method, "DELETE") == 0) {
        const char* key = req->path + 14;
        handle_cache_delete(client_fd, key);
    }
    else if (strcmp(req->path, "/api/v1/ratelimit") == 0 && strcmp(req->method, "GET") == 0) {
        handle_ratelimit(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/connections") == 0 && strcmp(req->method, "GET") == 0) {
        handle_connections(client_fd);
    }
    else if (strcmp(req->path, "/api/v1/log/level") == 0 && strcmp(req->method, "POST") == 0) {
        // Parse level from body
        const char* level = strstr(req->body, "\"level\"");
        if (level) {
            level = strchr(level + 7, '"');
            if (level) {
                level++;
                char* end = strchr(level, '"');
                if (end) {
                    *end = '\0';
                    handle_log_level(client_fd, level);
                    return;
                }
            }
        }
        send_error_response(client_fd, 400, "Invalid request body");
    }
    else {
        send_error_response(client_fd, 404, "Not Found");
    }
}

// ============================================================================
// Server Loop
// ============================================================================

static void* rest_api_server_thread(void* arg) {
    (void)arg;

    printf("[REST-API] Server started on %s:%d\n", rest_api_state.bind_address, rest_api_state.port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(rest_api_state.server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[REST-API] Connection from %s\n", client_ip);

        // Set receive timeout
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        // Read request
        char buffer[8192];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            http_request_t req;
            if (parse_http_request(buffer, bytes_read, &req) == 0) {
                printf("[REST-API] %s %s\n", req.method, req.path);
                route_request(client_fd, &req);
            } else {
                send_error_response(client_fd, 400, "Invalid HTTP request");
            }
        }

        // Shutdown and close
        shutdown(client_fd, SHUT_WR);
        close(client_fd);
    }

    return NULL;
}

// ============================================================================
// Public API
// ============================================================================

int rest_api_init(const char* bind_address, uint16_t port, const char* api_key) {
    if (rest_api_state.initialized) {
        return -1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
#endif

    rest_api_state.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (rest_api_state.server_fd < 0) {
        return -1;
    }

    // Set socket options
    int opt = 1;
    setsockopt(rest_api_state.server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // Bind
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(bind_address ? bind_address : "0.0.0.0");
    server_addr.sin_port = htons(port);

    if (bind(rest_api_state.server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(rest_api_state.server_fd);
        return -1;
    }

    // Listen
    if (listen(rest_api_state.server_fd, 10) < 0) {
        close(rest_api_state.server_fd);
        return -1;
    }

    strncpy(rest_api_state.bind_address, bind_address ? bind_address : "0.0.0.0",
            sizeof(rest_api_state.bind_address) - 1);
    rest_api_state.port = port;
    rest_api_state.api_key = api_key;
    rest_api_state.initialized = 1;

    // Start server thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, rest_api_server_thread, NULL) != 0) {
        rest_api_state.initialized = 0;
        close(rest_api_state.server_fd);
        return -1;
    }

    pthread_detach(thread);

    printf("[REST-API] Initialized on %s:%d\n", rest_api_state.bind_address, rest_api_state.port);
    return 0;
}

void rest_api_cleanup(void) {
    if (!rest_api_state.initialized) {
        return;
    }

    if (rest_api_state.server_fd >= 0) {
        close(rest_api_state.server_fd);
    }

    rest_api_state.initialized = 0;
    printf("[REST-API] Cleanup complete\n");
}

int rest_api_is_initialized(void) {
    return rest_api_state.initialized;
}
