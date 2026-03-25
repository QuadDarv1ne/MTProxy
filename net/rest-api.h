/*
 * REST API for MTProxy Management
 * HTTP REST API для управления MTProxy
 * 
 * Endpoints:
 * - GET  /api/v1/status          - Статус сервера
 * - GET  /api/v1/stats            - Статистика
 * - GET  /api/v1/config           - Конфигурация
 * - PUT  /api/v1/config           - Обновление конфигурации
 * - GET  /api/v1/connections      - Активные подключения
 * - POST /api/v1/connections/{id}  - Закрыть подключение
 * - GET  /api/v1/secrets          - Список секретов
 * - POST /api/v1/secrets          - Добавить секрет
 * - DELETE /api/v1/secrets/{id}   - Удалить секрет
 * - GET  /api/v1/metrics          - Prometheus метрики
 * - POST /api/v1/admin/reload     - Перезагрузка конфигурации
 * - POST /api/v1/admin/restart    - Перезапуск сервера
 * - POST /api/v1/admin/shutdown   - Остановка сервера
 */

#ifndef REST_API_H
#define REST_API_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Константы
 * ============================================================================ */

#define REST_API_VERSION "1.0.0"
#define REST_API_MAX_PATH_LEN 512
#define REST_API_MAX_BODY_LEN (1024 * 1024)  // 1MB max body
#define REST_API_MAX_HEADERS 32
#define REST_API_MAX_QUERY_PARAMS 16
#define REST_API_DEFAULT_PORT 8080
#define REST_API_MAX_CONNECTIONS 1024
#define REST_API_MAX_URL_LEN 2048

/* ============================================================================
 * HTTP методы
 * ============================================================================ */

typedef enum {
    HTTP_METHOD_NONE = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD
} http_method_t;

/* ============================================================================
 * HTTP статусы
 * ============================================================================ */

typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,
    HTTP_STATUS_TOO_MANY_REQUESTS = 429,
    
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504
} http_status_t;

/* ============================================================================
 * Типы контента
 * ============================================================================ */

typedef enum {
    CONTENT_TYPE_NONE = 0,
    CONTENT_TYPE_JSON,
    CONTENT_TYPE_XML,
    CONTENT_TYPE_HTML,
    CONTENT_TYPE_TEXT,
    CONTENT_TYPE_FORM,
    CONTENT_TYPE_MULTIPART
} content_type_t;

/* ============================================================================
 * Структуры данных
 * ============================================================================ */

/* HTTP заголовок */
typedef struct {
    char name[128];
    char value[512];
} http_header_t;

/* Query параметр */
typedef struct {
    char name[128];
    char value[512];
} query_param_t;

/* HTTP запрос */
typedef struct {
    http_method_t method;
    char path[REST_API_MAX_PATH_LEN];
    char query[REST_API_MAX_PATH_LEN];
    char url[REST_API_MAX_URL_LEN];
    char body[REST_API_MAX_BODY_LEN];
    size_t body_length;
    http_header_t headers[REST_API_MAX_HEADERS];
    int header_count;
    query_param_t query_params[REST_API_MAX_QUERY_PARAMS];
    int query_param_count;
    content_type_t content_type;
    char content_type_str[128];
    uint64_t content_length;
    char client_ip[64];
    uint16_t client_port;
    int client_fd;
    uint64_t timestamp;
} http_request_t;

/* HTTP ответ */
typedef struct {
    http_status_t status;
    char status_text[64];
    http_header_t headers[REST_API_MAX_HEADERS];
    int header_count;
    char body[REST_API_MAX_BODY_LEN];
    size_t body_length;
    content_type_t content_type;
    uint64_t timestamp;
    uint64_t response_time_ms;
} http_response_t;

/* Конфигурация REST API */
typedef struct {
    bool enabled;
    uint16_t port;
    char bind_address[64];
    bool enable_cors;
    bool enable_auth;
    char auth_token[256];
    bool enable_rate_limiting;
    int requests_per_minute;
    bool enable_logging;
    bool enable_compression;
    int max_connections;
    int timeout_seconds;
    char cors_origins[512];
    char cors_methods[128];
    char cors_headers[256];
} rest_api_config_t;

/* Статистика REST API */
typedef struct {
    uint64_t total_requests;
    uint64_t requests_by_method[8];  // По методам
    uint64_t requests_by_status[6];  // По классам статусов
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t active_connections;
    uint64_t max_connections;
    uint64_t errors;
    uint64_t auth_failures;
    uint64_t rate_limited;
    double avg_response_time_ms;
    double max_response_time_ms;
    uint64_t requests_by_endpoint[32];  // По эндпоинтам
} rest_api_stats_t;

/* Контекст обработчика */
typedef struct {
    http_request_t *request;
    http_response_t *response;
    void *user_data;
    char path_params[16][128];  // Параметры пути {id}, {name}, etc.
    int path_param_count;
} handler_context_t;

/* Обработчик запросов */
typedef int (*request_handler_t)(handler_context_t *ctx);

/* Маршрут */
typedef struct {
    http_method_t method;
    char path[REST_API_MAX_PATH_LEN];
    char pattern[REST_API_MAX_PATH_LEN];  // С паттернами /api/{resource}/{id}
    request_handler_t handler;
    void *user_data;
    bool requires_auth;
    const char *description;
} api_route_t;

/* Основной контекст REST API */
typedef struct {
    rest_api_config_t config;
    rest_api_stats_t stats;
    
    int server_fd;
    bool running;
    bool initialized;
    
    pthread_t server_thread;
    pthread_mutex_t mutex;
    
    api_route_t *routes;
    int route_count;
    int route_capacity;

    // Connection tracking
    int *active_fds;
    int active_fd_count;
    int max_fd;
#ifndef _WIN32
    fd_set read_fds;
#endif

    // Rate limiting
    uint64_t *request_timestamps;
    int request_timestamp_count;
    
    // Auth tokens
    char **valid_tokens;
    int token_count;
    
    // Middleware
    request_handler_t pre_handlers[16];
    int pre_handler_count;
    request_handler_t post_handlers[16];
    int post_handler_count;
} rest_api_server_t;

/* ============================================================================
 * API инициализации и управления
 * ============================================================================ */

/**
 * @brief Инициализация REST API сервера
 */
int rest_api_init(rest_api_server_t *server, const rest_api_config_t *config);

/**
 * @brief Запуск REST API сервера
 */
int rest_api_start(rest_api_server_t *server);

/**
 * @brief Остановка REST API сервера
 */
void rest_api_stop(rest_api_server_t *server);

/**
 * @brief Очистка REST API сервера
 */
void rest_api_cleanup(rest_api_server_t *server);

/**
 * @brief Регистрация обработчика маршрута
 */
int rest_api_register_route(rest_api_server_t *server,
                           http_method_t method,
                           const char *path,
                           request_handler_t handler,
                           const char *description);

/**
 * @brief Регистрация маршрута с авторизацией
 */
int rest_api_register_route_auth(rest_api_server_t *server,
                                http_method_t method,
                                const char *path,
                                request_handler_t handler,
                                const char *description);

/**
 * @brief Удаление маршрута
 */
int rest_api_unregister_route(rest_api_server_t *server,
                             http_method_t method,
                             const char *path);

/**
 * @brief Регистрация pre-middleware
 */
int rest_api_register_pre_handler(rest_api_server_t *server,
                                 request_handler_t handler);

/**
 * @brief Регистрация post-middleware
 */
int rest_api_register_post_handler(rest_api_server_t *server,
                                  request_handler_t handler);

/**
 * @brief Добавление валидного auth токена
 */
int rest_api_add_auth_token(rest_api_server_t *server, const char *token);

/**
 * @brief Удаление auth токена
 */
int rest_api_remove_auth_token(rest_api_server_t *server, const char *token);

/* ============================================================================
 * Helper функции для работы с HTTP
 * ============================================================================ */

/**
 * @brief Создание HTTP ответа
 */
void http_response_init(http_response_t *response,
                       http_status_t status,
                       const char *body,
                       content_type_t content_type);

/**
 * @brief Создание JSON ответа
 */
void http_response_json(http_response_t *response,
                       http_status_t status,
                       const char *json_body);

/**
 * @brief Создание ответа об ошибке
 */
void http_response_error(http_response_t *response,
                        http_status_t status,
                        const char *message);

/**
 * @brief Добавление заголовка к ответу
 */
void http_response_add_header(http_response_t *response,
                             const char *name,
                             const char *value);

/**
 * @brief Добавление CORS заголовков
 */
void http_response_add_cors_headers(http_response_t *response,
                                   const char *origins,
                                   const char *methods,
                                   const char *headers);

/**
 * @brief Парсинг HTTP запроса
 */
int http_request_parse(http_request_t *request,
                      const char *raw_data,
                      size_t data_length);

/**
 * @brief Получение заголовка из запроса
 */
const char *http_request_get_header(const http_request_t *request,
                                   const char *name);

/**
 * @brief Получение query параметра
 */
const char *http_request_get_query_param(const http_request_t *request,
                                        const char *name);

/**
 * @brief Получение path параметра
 */
const char *http_request_get_path_param(const handler_context_t *ctx,
                                       const char *name);

/**
 * @brief Проверка метода запроса
 */
bool http_request_is_method(const http_request_t *request, http_method_t method);

/**
 * @brief Проверка Content-Type
 */
bool http_request_is_content_type(const http_request_t *request,
                                 content_type_t content_type);

/**
 * @brief Преобразование метода в строку
 */
const char *http_method_to_string(http_method_t method);

/**
 * @brief Преобразование строки в метод
 */
http_method_t http_method_from_string(const char *str);

/**
 * @brief Преобразование статуса в строку
 */
const char *http_status_to_text(http_status_t status);

/**
 * @brief Преобразование content type в строку
 */
const char *content_type_to_string(content_type_t content_type);

/* ============================================================================
 * Встроенные обработчики
 * ============================================================================ */

/**
 * @brief Обработчик GET /api/v1/status
 */
int handle_api_status(handler_context_t *ctx);

/**
 * @brief Обработчик GET /api/v1/stats
 */
int handle_api_stats(handler_context_t *ctx);

/**
 * @brief Обработчик GET /api/v1/metrics (Prometheus)
 */
int handle_api_metrics(handler_context_t *ctx);

/**
 * @brief Обработчик GET /api/v1/config
 */
int handle_api_config_get(handler_context_t *ctx);

/**
 * @brief Обработчик PUT /api/v1/config
 */
int handle_api_config_put(handler_context_t *ctx);

/**
 * @brief Обработчик GET /api/v1/connections
 */
int handle_api_connections_get(handler_context_t *ctx);

/**
 * @brief Обработчик POST /api/v1/connections/{id}
 */
int handle_api_connections_post(handler_context_t *ctx);

/**
 * @brief Обработчик GET /api/v1/secrets
 */
int handle_api_secrets_get(handler_context_t *ctx);

/**
 * @brief Обработчик POST /api/v1/secrets
 */
int handle_api_secrets_post(handler_context_t *ctx);

/**
 * @brief Обработчик DELETE /api/v1/secrets/{id}
 */
int handle_api_secrets_delete(handler_context_t *ctx);

/**
 * @brief Обработчик POST /api/v1/admin/reload
 */
int handle_api_admin_reload(handler_context_t *ctx);

/**
 * @brief Обработчик POST /api/v1/admin/restart
 */
int handle_api_admin_restart(handler_context_t *ctx);

/**
 * @brief Обработчик POST /api/v1/admin/shutdown
 */
int handle_api_admin_shutdown(handler_context_t *ctx);

/* ============================================================================
 * Утилиты
 * ============================================================================ */

/**
 * @brief Получение статистики сервера
 */
void rest_api_get_stats(rest_api_server_t *server, rest_api_stats_t *stats);

/**
 * @brief Сброс статистики
 */
void rest_api_reset_stats(rest_api_server_t *server);

/**
 * @brief Генерация correlation ID
 */
char *rest_generate_request_id(char *buffer, size_t size);

/**
 * @brief Логирование запроса
 */
void rest_api_log_request(const http_request_t *request,
                         const http_response_t *response,
                         uint64_t response_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* REST_API_H */
