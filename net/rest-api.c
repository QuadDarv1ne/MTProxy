/*
 * REST API Implementation
 * Базовая реализация HTTP REST API для MTProxy
 */

#include "rest-api.h"
#include "common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// Windows compatibility
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <errno.h>

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static char *generate_request_id(char *buffer, size_t size) {
    uint64_t timestamp = get_timestamp_ms();
    pid_t pid = getpid();
    unsigned int random = (unsigned int)(timestamp ^ (uint64_t)pid);
    snprintf(buffer, size, "req-%016lx-%08x", (unsigned long)timestamp, random);
    return buffer;
}

const char *http_method_to_string(http_method_t method) {
    static const char *methods[] = {
        "NONE", "GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS", "HEAD"
    };
    if (method >= 0 && method <= 7) {
        return methods[method];
    }
    return "UNKNOWN";
}

http_method_t http_method_from_string(const char *str) {
    if (!str) return HTTP_METHOD_NONE;
    if (strcmp(str, "GET") == 0) return HTTP_METHOD_GET;
    if (strcmp(str, "POST") == 0) return HTTP_METHOD_POST;
    if (strcmp(str, "PUT") == 0) return HTTP_METHOD_PUT;
    if (strcmp(str, "DELETE") == 0) return HTTP_METHOD_DELETE;
    if (strcmp(str, "PATCH") == 0) return HTTP_METHOD_PATCH;
    if (strcmp(str, "OPTIONS") == 0) return HTTP_METHOD_OPTIONS;
    if (strcmp(str, "HEAD") == 0) return HTTP_METHOD_HEAD;
    return HTTP_METHOD_NONE;
}

const char *http_status_to_text(http_status_t status) {
    switch (status) {
        case HTTP_STATUS_OK: return "OK";
        case HTTP_STATUS_CREATED: return "Created";
        case HTTP_STATUS_ACCEPTED: return "Accepted";
        case HTTP_STATUS_NO_CONTENT: return "No Content";
        case HTTP_STATUS_BAD_REQUEST: return "Bad Request";
        case HTTP_STATUS_UNAUTHORIZED: return "Unauthorized";
        case HTTP_STATUS_FORBIDDEN: return "Forbidden";
        case HTTP_STATUS_NOT_FOUND: return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_STATUS_CONFLICT: return "Conflict";
        case HTTP_STATUS_UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
        case HTTP_STATUS_TOO_MANY_REQUESTS: return "Too Many Requests";
        case HTTP_STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTP_STATUS_NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HTTP_STATUS_GATEWAY_TIMEOUT: return "Gateway Timeout";
        default: return "Unknown";
    }
}

const char *content_type_to_string(content_type_t content_type) {
    switch (content_type) {
        case CONTENT_TYPE_JSON: return "application/json";
        case CONTENT_TYPE_XML: return "application/xml";
        case CONTENT_TYPE_HTML: return "text/html";
        case CONTENT_TYPE_TEXT: return "text/plain";
        case CONTENT_TYPE_FORM: return "application/x-www-form-urlencoded";
        case CONTENT_TYPE_MULTIPART: return "multipart/form-data";
        default: return "application/octet-stream";
    }
}

/* ============================================================================
 * HTTP Response функции
 * ============================================================================ */

void http_response_init(http_response_t *response,
                       http_status_t status,
                       const char *body,
                       content_type_t content_type) {
    memset(response, 0, sizeof(http_response_t));
    response->status = status;
    response->timestamp = get_timestamp_ms();
    strncpy(response->status_text, http_status_to_text(status), 
            sizeof(response->status_text) - 1);
    response->content_type = content_type;
    
    if (body) {
        size_t len = strlen(body);
        if (len < sizeof(response->body)) {
            utils_strcpy(response->body, body, sizeof(response->body));
            response->body_length = len;
        }
    }
    
    // Add default headers
    http_response_add_header(response, "Content-Type", 
                            content_type_to_string(content_type));
    http_response_add_header(response, "X-Request-ID", 
                            generate_request_id(response->body, 64));
}

void http_response_json(http_response_t *response,
                       http_status_t status,
                       const char *json_body) {
    http_response_init(response, status, json_body, CONTENT_TYPE_JSON);
}

void http_response_error(http_response_t *response,
                        http_status_t status,
                        const char *message) {
    char error_body[1024];
    snprintf(error_body, sizeof(error_body),
            "{\"error\":{\"code\":%d,\"message\":\"%s\"}}",
            (int)status, message ? message : "Unknown error");
    http_response_json(response, status, error_body);
}

void http_response_add_header(http_response_t *response,
                             const char *name,
                             const char *value) {
    if (!response || !name || !value) return;
    if (response->header_count >= REST_API_MAX_HEADERS) return;
    
    strncpy(response->headers[response->header_count].name, name, 127);
    strncpy(response->headers[response->header_count].value, value, 511);
    response->header_count++;
}

void http_response_add_cors_headers(http_response_t *response,
                                   const char *origins,
                                   const char *methods,
                                   const char *headers) {
    if (origins) {
        http_response_add_header(response, "Access-Control-Allow-Origin", origins);
    }
    if (methods) {
        http_response_add_header(response, "Access-Control-Allow-Methods", methods);
    }
    if (headers) {
        http_response_add_header(response, "Access-Control-Allow-Headers", headers);
    }
    http_response_add_header(response, "Access-Control-Allow-Credentials", "true");
}

/* ============================================================================
 * HTTP Request парсинг
 * ============================================================================ */

static void parse_query_string(const char *query, query_param_t *params, int *count) {
    if (!query || !params || !count) return;

    *count = 0;
    char *query_copy = strdup(query);
    if (!query_copy) return;
    
    char *saveptr = NULL;
    char *pair = strtok_r(query_copy, "&", &saveptr);

    while (pair && *count < REST_API_MAX_QUERY_PARAMS) {
        char *eq = strchr(pair, '=');
        if (eq) {
            *eq = '\0';
            strncpy(params[*count].name, pair, 127);
            strncpy(params[*count].value, eq + 1, 511);
            (*count)++;
        }
        pair = strtok_r(NULL, "&", &saveptr);
    }

    free(query_copy);
}

static content_type_t parse_content_type(const char *ct) {
    if (!ct) return CONTENT_TYPE_NONE;
    if (strstr(ct, "application/json")) return CONTENT_TYPE_JSON;
    if (strstr(ct, "application/xml")) return CONTENT_TYPE_XML;
    if (strstr(ct, "text/html")) return CONTENT_TYPE_HTML;
    if (strstr(ct, "text/plain")) return CONTENT_TYPE_TEXT;
    if (strstr(ct, "application/x-www-form-urlencoded")) return CONTENT_TYPE_FORM;
    if (strstr(ct, "multipart/form-data")) return CONTENT_TYPE_MULTIPART;
    return CONTENT_TYPE_NONE;
}

int http_request_parse(http_request_t *request,
                      const char *raw_data,
                      size_t data_length) {
    if (!request || !raw_data || data_length == 0) {
        return -1;
    }
    
    memset(request, 0, sizeof(http_request_t));
    request->timestamp = get_timestamp_ms();
    
    // Parse request line
    const char *line_end = strstr(raw_data, "\r\n");
    if (!line_end) return -1;
    
    char request_line[REST_API_MAX_URL_LEN];
    size_t line_len = line_end - raw_data;
    if (line_len >= sizeof(request_line)) line_len = sizeof(request_line) - 1;
    strncpy(request_line, raw_data, line_len);
    request_line[line_len] = '\0';
    
    // Parse method, path, query
    char method_str[32], path_str[REST_API_MAX_PATH_LEN];
    int parsed = sscanf(request_line, "%31s %2047s", method_str, path_str);
    if (parsed < 2) return -1;
    
    request->method = http_method_from_string(method_str);
    
    // Split path and query
    char *query_start = strchr(path_str, '?');
    if (query_start) {
        *query_start = '\0';
        strncpy(request->query, query_start + 1, sizeof(request->query) - 1);
        parse_query_string(request->query, request->query_params, 
                          &request->query_param_count);
    }
    
    strncpy(request->path, path_str, sizeof(request->path) - 1);
    snprintf(request->url, sizeof(request->url), "%s%s%s", 
             path_str, query_start ? "?" : "", request->query);
    
    // Parse headers
    const char *ptr = line_end + 2;
    const char *body_start = NULL;
    
    while (*ptr && *ptr != '\r') {
        line_end = strstr(ptr, "\r\n");
        if (!line_end) break;
        
        if (line_end == ptr) {
            body_start = ptr + 2;
            break;
        }
        
        if (request->header_count < REST_API_MAX_HEADERS) {
            char header_line[1024];
            size_t hlen = line_end - ptr;
            if (hlen < sizeof(header_line)) {
                strncpy(header_line, ptr, hlen);
                header_line[hlen] = '\0';
                
                char *colon = strchr(header_line, ':');
                if (colon) {
                    *colon = '\0';
                    char *value = colon + 1;
                    while (*value == ' ') value++;
                    
                    strncpy(request->headers[request->header_count].name, 
                           header_line, 127);
                    strncpy(request->headers[request->header_count].value, 
                           value, 511);
                    request->header_count++;
                    
                    // Check for Content-Type
                    if (strcasecmp(header_line, "Content-Type") == 0) {
                        strncpy(request->content_type_str, value, 
                               sizeof(request->content_type_str) - 1);
                        request->content_type = parse_content_type(value);
                    }
                    
                    // Check for Content-Length
                    if (strcasecmp(header_line, "Content-Length") == 0) {
                        request->content_length = strtoull(value, NULL, 10);
                    }
                }
            }
        }
        
        ptr = line_end + 2;
    }
    
    // Parse body
    if (body_start) {
        request->body_length = data_length - (body_start - raw_data);
        if (request->body_length > 0 && 
            request->body_length < sizeof(request->body)) {
            memcpy(request->body, body_start, request->body_length);
            request->body[request->body_length] = '\0';
        }
    }
    
    return 0;
}

const char *http_request_get_header(const http_request_t *request,
                                   const char *name) {
    if (!request || !name) return NULL;
    
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

const char *http_request_get_query_param(const http_request_t *request,
                                        const char *name) {
    if (!request || !name) return NULL;
    
    for (int i = 0; i < request->query_param_count; i++) {
        if (strcmp(request->query_params[i].name, name) == 0) {
            return request->query_params[i].value;
        }
    }
    return NULL;
}

const char *http_request_get_path_param(const handler_context_t *ctx,
                                       const char *name) {
    if (!ctx || !name) return NULL;
    // Implementation would extract from pattern-matched path
    return NULL;
}

bool http_request_is_method(const http_request_t *request, http_method_t method) {
    return request && request->method == method;
}

bool http_request_is_content_type(const http_request_t *request,
                                 content_type_t content_type) {
    return request && request->content_type == content_type;
}

/* ============================================================================
 * REST API Server
 * ============================================================================ */

static int send_response(int client_fd, const http_response_t *response) {
    char buffer[8192];
    int offset = 0;
    
    // Status line
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "HTTP/1.1 %d %s\r\n",
                      (int)response->status, response->status_text);
    
    // Headers
    for (int i = 0; i < response->header_count; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "%s: %s\r\n",
                          response->headers[i].name,
                          response->headers[i].value);
    }
    
    // Content-Length
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "Content-Length: %zu\r\n", response->body_length);
    
    // End headers
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");
    
    // Send headers
    if (send(client_fd, buffer, offset, 0) < 0) {
        return -1;
    }
    
    // Send body
    if (response->body_length > 0) {
        if (send(client_fd, response->body, response->body_length, 0) < 0) {
            return -1;
        }
    }
    
    return 0;
}

static void *server_thread_func(void *arg) {
    rest_api_server_t *server = (rest_api_server_t *)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    int client_fd;
    char buffer[REST_API_MAX_BODY_LEN];
    int bytes_read;
    
    while (server->running) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(server->server_fd, &readfds);
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ret = select(server->server_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret <= 0) continue;
        
        client_len = sizeof(client_addr);
        client_fd = accept(server->server_fd, 
                          (struct sockaddr *)&client_addr, 
                          &client_len);
        
        if (client_fd < 0) continue;

        // Check connection limit
        if (server->active_fd_count >= server->config.max_connections) {
            http_response_t response;
            http_response_error(&response, HTTP_STATUS_TOO_MANY_REQUESTS,
                              "Too many connections");
            send_response(client_fd, &response);
            close(client_fd);
            continue;
        }

        // Устанавливаем таймауты для предотвращения зависания
        struct timeval rcv_timeout, snd_timeout;
        rcv_timeout.tv_sec = 30;  // 30 секунд на получение
        rcv_timeout.tv_usec = 0;
        snd_timeout.tv_sec = 30;  // 30 секунд на отправку
        snd_timeout.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_timeout, sizeof(rcv_timeout));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&snd_timeout, sizeof(snd_timeout));

        // Read request
        bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        
        // Parse request
        http_request_t request;
        if (http_request_parse(&request, buffer, bytes_read) != 0) {
            http_response_t response;
            http_response_error(&response, HTTP_STATUS_BAD_REQUEST,
                              "Invalid request");
            send_response(client_fd, &response);
            close(client_fd);
            continue;
        }
        
        // Set client info
        strncpy(request.client_ip, inet_ntoa(client_addr.sin_addr), 63);
        request.client_port = ntohs(client_addr.sin_port);
        request.client_fd = client_fd;
        
        // Find matching route
        api_route_t *matched_route = NULL;
        for (int i = 0; i < server->route_count; i++) {
            if (server->routes[i].method == request.method &&
                strcmp(server->routes[i].path, request.path) == 0) {
                matched_route = &server->routes[i];
                break;
            }
        }
        
        // Handle request
        http_response_t response;
        uint64_t start_time = get_timestamp_ms();
        
        if (matched_route) {
            // Execute pre-handlers
            bool continue_processing = true;
            for (int i = 0; i < server->pre_handler_count && continue_processing; i++) {
                handler_context_t ctx = {&request, &response, NULL, {{0}}, 0};
                if (server->pre_handlers[i](&ctx) != 0) {
                    continue_processing = false;
                }
            }
            
            if (continue_processing) {
                // Execute handler
                handler_context_t ctx = {&request, &response, 
                                        matched_route->user_data, {{0}}, 0};
                matched_route->handler(&ctx);
            }
            
            // Execute post-handlers
            for (int i = 0; i < server->post_handler_count; i++) {
                handler_context_t ctx = {&request, &response, NULL, {{0}}, 0};
                server->post_handlers[i](&ctx);
            }
        } else {
            http_response_error(&response, HTTP_STATUS_NOT_FOUND,
                              "Endpoint not found");
        }
        
        // Send response
        send_response(client_fd, &response);
        
        // Update statistics
        server->stats.total_requests++;
        server->stats.requests_by_method[request.method]++;
        
        int status_class = response.status / 100;
        if (status_class >= 2 && status_class <= 5) {
            server->stats.requests_by_status[status_class - 2]++;
        }
        
        uint64_t response_time = get_timestamp_ms() - start_time;
        response.response_time_ms = response_time;
        
        // Update average response time
        static uint64_t total_response_time = 0;
        static uint64_t response_count = 0;
        total_response_time += response_time;
        response_count++;
        server->stats.avg_response_time_ms = 
            (double)total_response_time / response_count;
        
        if (response_time > server->stats.max_response_time_ms) {
            server->stats.max_response_time_ms = response_time;
        }
        
        server->stats.total_bytes_sent += response.body_length;
        server->stats.total_bytes_received += request.body_length;
        
        // Log request
        if (server->config.enable_logging) {
            printf("[%s] %s %s -> %d (%lu ms)\n",
                   request.client_ip,
                   http_method_to_string(request.method),
                   request.path,
                   (int)response.status,
                   (unsigned long)response_time);
        }
        
        close(client_fd);
    }
    
    return NULL;
}

int rest_api_init(rest_api_server_t *server, const rest_api_config_t *config) {
    if (!server || !config) {
        return -1;
    }
    
    memset(server, 0, sizeof(rest_api_server_t));
    server->config = *config;
    
    // Initialize routes array
    server->route_capacity = 64;
    server->routes = calloc(server->route_capacity, sizeof(api_route_t));
    if (!server->routes) {
        return -1;
    }
    
    // Initialize connection tracking
    server->active_fds = calloc(server->config.max_connections, sizeof(int));
    if (!server->active_fds) {
        free(server->routes);
        return -1;
    }
    
    // Initialize mutex
    pthread_mutex_init(&server->mutex, NULL);
    
    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    server->initialized = true;
    return 0;
}

int rest_api_start(rest_api_server_t *server) {
    if (!server || !server->initialized || server->running) {
        return -1;
    }
    
    // Create socket
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);
    
    if (strcmp(server->config.bind_address, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, server->config.bind_address, &addr.sin_addr);
    }
    
    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server->server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server->server_fd, 128) < 0) {
        close(server->server_fd);
        return -1;
    }
    
    // Start server thread
    server->running = true;
    if (pthread_create(&server->server_thread, NULL, 
                      server_thread_func, server) != 0) {
        server->running = false;
        close(server->server_fd);
        return -1;
    }
    
    printf("REST API server started on %s:%d\n",
           server->config.bind_address, server->config.port);
    
    return 0;
}

void rest_api_stop(rest_api_server_t *server) {
    if (!server || !server->running) {
        return;
    }
    
    server->running = false;
    
    // Close server socket
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    // Wait for server thread
    pthread_join(server->server_thread, NULL);
    
    // Close active connections
    for (int i = 0; i < server->active_fd_count; i++) {
        if (server->active_fds[i] >= 0) {
            close(server->active_fds[i]);
        }
    }
}

void rest_api_cleanup(rest_api_server_t *server) {
    if (!server) {
        return;
    }
    
    if (server->running) {
        rest_api_stop(server);
    }
    
    if (server->routes) {
        free(server->routes);
        server->routes = NULL;
    }
    
    if (server->active_fds) {
        free(server->active_fds);
        server->active_fds = NULL;
    }
    
    pthread_mutex_destroy(&server->mutex);
    server->initialized = false;
}

int rest_api_register_route(rest_api_server_t *server,
                           http_method_t method,
                           const char *path,
                           request_handler_t handler,
                           const char *description) {
    if (!server || !path || !handler) {
        return -1;
    }
    
    if (server->route_count >= server->route_capacity) {
        // Resize routes array
        server->route_capacity *= 2;
        api_route_t *new_routes = realloc(server->routes, 
                                         server->route_capacity * sizeof(api_route_t));
        if (!new_routes) {
            return -1;
        }
        server->routes = new_routes;
    }
    
    api_route_t *route = &server->routes[server->route_count];
    route->method = method;
    strncpy(route->path, path, sizeof(route->path) - 1);
    strncpy(route->pattern, path, sizeof(route->pattern) - 1);
    route->handler = handler;
    route->requires_auth = false;
    route->description = description;
    
    server->route_count++;
    return 0;
}

int rest_api_register_route_auth(rest_api_server_t *server,
                                http_method_t method,
                                const char *path,
                                request_handler_t handler,
                                const char *description) {
    int ret = rest_api_register_route(server, method, path, handler, description);
    if (ret == 0 && server->route_count > 0) {
        server->routes[server->route_count - 1].requires_auth = true;
    }
    return ret;
}

int rest_api_add_auth_token(rest_api_server_t *server, const char *token) {
    if (!server || !token) return -1;
    
    // Simple implementation - in production use dynamic array
    if (server->token_count >= 100) return -1;
    
    if (!server->valid_tokens) {
        server->valid_tokens = calloc(100, sizeof(char *));
    }
    
    server->valid_tokens[server->token_count] = strdup(token);
    server->token_count++;
    return 0;
}

void rest_api_get_stats(rest_api_server_t *server, rest_api_stats_t *stats) {
    if (!server || !stats) return;
    
    pthread_mutex_lock(&server->mutex);
    *stats = server->stats;
    stats->active_connections = server->active_fd_count;
    pthread_mutex_unlock(&server->mutex);
}

void rest_api_reset_stats(rest_api_server_t *server) {
    if (!server) return;
    
    pthread_mutex_lock(&server->mutex);
    memset(&server->stats, 0, sizeof(rest_api_stats_t));
    pthread_mutex_unlock(&server->mutex);
}

char *rest_generate_request_id(char *buffer, size_t size) {
    return generate_request_id(buffer, size);
}

void rest_api_log_request(const http_request_t *request,
                         const http_response_t *response,
                         uint64_t response_time_ms) {
    printf("[%s] %s %s %s -> %d %s (%lu ms)\n",
           request->client_ip,
           http_method_to_string(request->method),
           request->path,
           request->query,
           (int)response->status,
           response->status_text,
           (unsigned long)response_time_ms);
}

/* ============================================================================
 * Встроенные обработчики (заглушки)
 * ============================================================================ */

int handle_api_status(handler_context_t *ctx) {
    const char *json = "{\"status\":\"ok\",\"version\":\"" REST_API_VERSION "\"}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_stats(handler_context_t *ctx) {
    // Placeholder - would return actual stats
    const char *json = "{\"requests\":0,\"connections\":0}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_metrics(handler_context_t *ctx) {
    // Prometheus format
    const char *metrics = 
        "# HELP mtproxy_requests_total Total HTTP requests\n"
        "# TYPE mtproxy_requests_total counter\n"
        "mtproxy_requests_total 0\n";
    
    http_response_init(ctx->response, HTTP_STATUS_OK, metrics, CONTENT_TYPE_TEXT);
    http_response_add_header(ctx->response, "Content-Type", 
                            "text/plain; version=0.0.4");
    return 0;
}

int handle_api_config_get(handler_context_t *ctx) {
    const char *json = "{\"config\":{}}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_config_put(handler_context_t *ctx) {
    const char *json = "{\"status\":\"updated\"}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_connections_get(handler_context_t *ctx) {
    const char *json = "{\"connections\":[]}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_connections_post(handler_context_t *ctx) {
    const char *json = "{\"status\":\"closed\"}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_secrets_get(handler_context_t *ctx) {
    const char *json = "{\"secrets\":[]}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_secrets_post(handler_context_t *ctx) {
    const char *json = "{\"status\":\"created\",\"id\":\"1\"}";
    http_response_json(ctx->response, HTTP_STATUS_CREATED, json);
    return 0;
}

int handle_api_secrets_delete(handler_context_t *ctx) {
    const char *json = "{\"status\":\"deleted\"}";
    http_response_json(ctx->response, HTTP_STATUS_OK, json);
    return 0;
}

int handle_api_admin_reload(handler_context_t *ctx) {
    const char *json = "{\"status\":\"reloading\"}";
    http_response_json(ctx->response, HTTP_STATUS_ACCEPTED, json);
    return 0;
}

int handle_api_admin_restart(handler_context_t *ctx) {
    const char *json = "{\"status\":\"restarting\"}";
    http_response_json(ctx->response, HTTP_STATUS_ACCEPTED, json);
    return 0;
}

int handle_api_admin_shutdown(handler_context_t *ctx) {
    const char *json = "{\"status\":\"shutting_down\"}";
    http_response_json(ctx->response, HTTP_STATUS_ACCEPTED, json);
    return 0;
}
