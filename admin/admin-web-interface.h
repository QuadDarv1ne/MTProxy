/*
    Web-интерфейс администратора для MTProxy
    REST API и веб-панель управления
*/

#ifndef ADMIN_WEB_INTERFACE_H
#define ADMIN_WEB_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация веб-интерфейса
#define MAX_ADMIN_USERS 32
#define ADMIN_SESSION_TIMEOUT 3600 // 1 час
#define MAX_API_KEYS 64
#define WEB_INTERFACE_PORT 8080
#define MAX_CONCURRENT_REQUESTS 256

// Уровни доступа
typedef enum {
    ADMIN_ACCESS_NONE = 0,
    ADMIN_ACCESS_READ = 1,
    ADMIN_ACCESS_WRITE = 2,
    ADMIN_ACCESS_ADMIN = 3
} admin_access_level_t;

// Статусы пользователей
typedef enum {
    ADMIN_STATUS_ACTIVE = 0,
    ADMIN_STATUS_DISABLED = 1,
    ADMIN_STATUS_LOCKED = 2,
    ADMIN_STATUS_EXPIRED = 3
} admin_user_status_t;

// Типы API ключей
typedef enum {
    API_KEY_TYPE_READ = 0,
    API_KEY_TYPE_WRITE = 1,
    API_KEY_TYPE_ADMIN = 2,
    API_KEY_TYPE_SERVICE = 3
} api_key_type_t;

// Пользователь администратора
typedef struct admin_user {
    uint64_t user_id;
    char username[64];
    char password_hash[128]; // SHA256 хэш
    char email[128];
    admin_access_level_t access_level;
    admin_user_status_t status;
    long long created_time;
    long long last_login;
    long long login_attempts;
    char last_ip[46]; // IPv6
    int session_active;
    char session_token[64];
    long long session_expiry;
} admin_user_t;

// API ключ
typedef struct api_key {
    uint64_t key_id;
    char key_hash[128]; // Хэш ключа
    char description[256];
    api_key_type_t key_type;
    uint64_t user_id; // Владелец ключа
    long long created_time;
    long long expires_time;
    long long last_used;
    long long use_count;
    int is_active;
} api_key_t;

// Сессия администратора
typedef struct admin_session {
    char session_id[64];
    uint64_t user_id;
    long long created_time;
    long long last_activity;
    char client_ip[46];
    char user_agent[256];
    int is_valid;
} admin_session_t;

// Конфигурация веб-сервера
typedef struct web_server_config {
    int enable_http;
    int enable_https;
    int http_port;
    int https_port;
    char ssl_cert_file[256];
    char ssl_key_file[256];
    char ssl_ca_file[256];
    int max_connections;
    int request_timeout_ms;
    int enable_cors;
    char allowed_origins[1024];
    int enable_rate_limiting;
    int requests_per_minute;
    int enable_logging;
    char log_file[256];
} web_server_config_t;

// Статистика веб-интерфейса
typedef struct web_interface_stats {
    long long total_requests;
    long long successful_requests;
    long long failed_requests;
    long long api_requests;
    long long web_requests;
    long long active_sessions;
    long long total_sessions;
    long long blocked_requests;
    double avg_response_time_ms;
    long long bytes_transferred;
    int current_connections;
    int peak_connections;
} web_interface_stats_t;

// Веб-интерфейс администратора
typedef struct admin_web_interface {
    // Конфигурация
    web_server_config_t config;
    
    // Пользователи и сессии
    admin_user_t *users;
    int user_count;
    int max_users;
    
    api_key_t *api_keys;
    int api_key_count;
    int max_api_keys;
    
    admin_session_t *sessions;
    int session_count;
    int max_sessions;
    
    // Статистика
    web_interface_stats_t stats;
    
    // Состояние
    int is_initialized;
    int is_running;
    int server_fd;
    long long start_time;
    
    // Callback функции
    void (*on_user_login)(admin_user_t *user);
    void (*on_user_logout)(admin_user_t *user);
    void (*on_api_request)(const char *endpoint, int method);
    void (*on_security_event)(const char *event, const char *details);
} admin_web_interface_t;

// HTTP методы
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST = 1,
    HTTP_METHOD_PUT = 2,
    HTTP_METHOD_DELETE = 3,
    HTTP_METHOD_PATCH = 4
} http_method_t;

// HTTP статусы
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
    HTTP_STATUS_REQUEST_TIMEOUT = 408,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_INTERNAL_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503
} http_status_t;

// API эндпоинты
typedef enum {
    API_ENDPOINT_STATUS = 0,
    API_ENDPOINT_STATS = 1,
    API_ENDPOINT_CONNECTIONS = 2,
    API_ENDPOINT_USERS = 3,
    API_ENDPOINT_CONFIG = 4,
    API_ENDPOINT_LOGS = 5,
    API_ENDPOINT_SECURITY = 6,
    API_ENDPOINT_PERFORMANCE = 7
} api_endpoint_t;

// Инициализация
admin_web_interface_t* admin_web_init(const web_server_config_t *config);
int admin_web_configure(admin_web_interface_t *web, const web_server_config_t *config);
void admin_web_cleanup(admin_web_interface_t *web);

// Управление пользователями
int admin_web_create_user(admin_web_interface_t *web, const char *username, 
                         const char *password, const char *email, 
                         admin_access_level_t access_level);
int admin_web_authenticate_user(admin_web_interface_t *web, const char *username, 
                               const char *password, const char *client_ip);
int admin_web_update_user(admin_web_interface_t *web, uint64_t user_id, 
                         const char *email, admin_access_level_t access_level);
int admin_web_disable_user(admin_web_interface_t *web, uint64_t user_id);
int admin_web_delete_user(admin_web_interface_t *web, uint64_t user_id);

// Управление API ключами
int admin_web_create_api_key(admin_web_interface_t *web, uint64_t user_id,
                            const char *description, api_key_type_t key_type,
                            long long expires_in_seconds, char *generated_key);
int admin_web_revoke_api_key(admin_web_interface_t *web, uint64_t key_id);
int admin_web_validate_api_key(admin_web_interface_t *web, const char *key, 
                              api_key_type_t *key_type, uint64_t *user_id);

// Управление сессиями
int admin_web_create_session(admin_web_interface_t *web, uint64_t user_id, 
                            const char *client_ip, const char *user_agent,
                            char *session_token);
int admin_web_validate_session(admin_web_interface_t *web, const char *session_token, 
                              uint64_t *user_id);
int admin_web_destroy_session(admin_web_interface_t *web, const char *session_token);
void admin_web_cleanup_expired_sessions(admin_web_interface_t *web);

// Веб-сервер
int admin_web_start_server(admin_web_interface_t *web);
int admin_web_stop_server(admin_web_interface_t *web);
int admin_web_handle_request(admin_web_interface_t *web, int client_fd);

// API обработчики
int admin_web_handle_api_request(admin_web_interface_t *web, http_method_t method,
                                api_endpoint_t endpoint, const char *params,
                                const char *auth_token, char *response, size_t response_size);
int admin_web_handle_status_api(admin_web_interface_t *web, char *response, size_t response_size);
int admin_web_handle_stats_api(admin_web_interface_t *web, char *response, size_t response_size);
int admin_web_handle_connections_api(admin_web_interface_t *web, char *response, size_t response_size);
int admin_web_handle_users_api(admin_web_interface_t *web, http_method_t method,
                              const char *params, char *response, size_t response_size);

// Утилиты
const char* admin_web_access_level_to_string(admin_access_level_t level);
const char* admin_web_user_status_to_string(admin_user_status_t status);
const char* admin_web_api_key_type_to_string(api_key_type_t type);
const char* admin_web_http_status_to_string(http_status_t status);
uint64_t admin_web_generate_user_id(void);
char* admin_web_hash_password(const char *password);
int admin_web_verify_password(const char *password, const char *hash);
char* admin_web_generate_api_key(void);
char* admin_web_generate_session_token(void);

// Статистика и мониторинг
void admin_web_get_stats(admin_web_interface_t *web, web_interface_stats_t *stats);
void admin_web_get_user_stats(admin_web_interface_t *web, uint64_t user_id, 
                             char *buffer, size_t buffer_size);
void admin_web_reset_stats(admin_web_interface_t *web);

// Callback регистрации
void admin_web_set_login_callback(admin_web_interface_t *web,
                                 void (*callback)(admin_user_t*));
void admin_web_set_logout_callback(admin_web_interface_t *web,
                                  void (*callback)(admin_user_t*));
void admin_web_set_api_request_callback(admin_web_interface_t *web,
                                       void (*callback)(const char*, int));
void admin_web_set_security_callback(admin_web_interface_t *web,
                                    void (*callback)(const char*, const char*));

#endif // ADMIN_WEB_INTERFACE_H