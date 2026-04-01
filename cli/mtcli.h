/*
    MTProxy CLI - Cross-platform Command Line Interface
    Утилита для управления и мониторинга MTProxy сервера
    
    Поддерживает:
    - REST API (HTTP/JSON)
    - gRPC API (Protocol Buffers)
    - Прямое подключение к серверу
    
    Примеры использования:
    mtproxy-cli status
    mtproxy-cli stats --json
    mtproxy-cli config get
    mtproxy-cli secrets add --secret "dd1234567890abcdef"
    mtproxy-cli logs --level error --tail 50
*/

#ifndef MTCLI_H
#define MTCLI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия CLI
#define MTCLI_VERSION "1.0.0"
#define MTCLI_VERSION_MAJOR 1
#define MTCLI_VERSION_MINOR 0
#define MTCLI_VERSION_PATCH 0

// Максимальные размеры
#define MTCLI_MAX_HOSTNAME 256
#define MTCLI_MAX_PATH 512
#define MTCLI_MAX_TOKEN 512
#define MTCLI_MAX_RESPONSE (1024 * 1024)  // 1MB
#define MTCLI_MAX_COMMAND 4096

// Режимы подключения
typedef enum {
    MTCLI_MODE_REST,      // REST API (HTTP/JSON)
    MTCLI_MODE_GRPC,      // gRPC API
    MTCLI_MODE_DIRECT     // Прямое подключение к серверу
} mtcli_mode_t;

// Типы команд
typedef enum {
    MTCLI_CMD_STATUS,
    MTCLI_CMD_STATS,
    MTCLI_CMD_CONFIG,
    MTCLI_CMD_SECRETS,
    MTCLI_CMD_LOGS,
    MTCLI_CMD_CONNECTIONS,
    MTCLI_CMD_RATELIMIT,
    MTCLI_CMD_HEALTH,
    MTCLI_CMD_METRICS,
    MTCLI_CMD_RELOAD,
    MTCLI_CMD_RESTART,
    MTCLI_CMD_STOP,
    MTCLI_CMD_EXIT,
    MTCLI_CMD_QUIT,
    MTCLI_CMD_HELP,
    MTCLI_CMD_VERSION,
    MTCLI_CMD_UNKNOWN
} mtcli_command_t;

// Конфигурация CLI
typedef struct {
    char host[MTCLI_MAX_HOSTNAME];
    int rest_port;          // REST API порт (по умолчанию 8080)
    int grpc_port;          // gRPC порт (по умолчанию 50051)
    int server_port;        // Порт сервера для прямого подключения
    char api_key[MTCLI_MAX_TOKEN];
    mtcli_mode_t mode;      // Режим работы
    bool verbose;           // Подробный вывод
    bool json_output;       // JSON вывод
    bool insecure;          // Не проверять SSL (для тестов)
    int timeout_ms;         // Таймаут подключения (мс)
} mtcli_config_t;

// Статус сервера
typedef struct {
    bool running;
    char state[64];
    uint64_t uptime_ms;
    char version[32];
    char platform[64];
    int64_t timestamp;
} mtcli_server_status_t;

// Статистика сервера
typedef struct {
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t requests_total;
    uint64_t requests_per_second;
    double cpu_usage;
    uint64_t memory_usage;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_rate;
    uint64_t rate_limited_requests;
    uint64_t errors_total;
    int64_t timestamp;
} mtcli_statistics_t;

// Конфигурация сервера
typedef struct {
    int server_port;
    int max_connections;
    bool ipv6_enabled;
    int log_level;
    char log_file[MTCLI_MAX_PATH];
    int workers;
    bool aes_ni_enabled;
    char secrets[10][128];  // До 10 секретов
    int secrets_count;
} mtcli_config_data_t;

// Коды ошибок CLI
typedef enum {
    MTCLI_OK = 0,
    MTCLI_ERR_CONNECTION = 1,
    MTCLI_ERR_AUTH = 2,
    MTCLI_ERR_TIMEOUT = 3,
    MTCLI_ERR_INVALID_RESPONSE = 4,
    MTCLI_ERR_SERVER = 5,
    MTCLI_ERR_INVALID_ARGS = 6,
    MTCLI_ERR_NOT_FOUND = 7,
    MTCLI_ERR_PERMISSION = 8,
    MTCLI_ERR_UNKNOWN = 99
} mtcli_error_code_t;

// Структура ошибки CLI
typedef struct {
    mtcli_error_code_t code;
    char message[256];
    char details[512];
    int http_status;
} mtcli_error_t;

// Секрет
typedef struct {
    char hash[128];
    char description[256];
    int64_t created_at;
    uint64_t uses;
    bool active;
} mtcli_secret_t;

// Лог запись
typedef struct {
    int64_t timestamp;
    char level[16];
    char message[1024];
    char source[128];
} mtcli_log_entry_t;

// Результат выполнения команды
typedef struct {
    int exit_code;
    char error[512];
    char* output;           // Динамический вывод
    size_t output_size;
} mtcli_result_t;

// Инициализация конфигурации по умолчанию
void mtcli_config_init(mtcli_config_t *config);

// Обработка ошибок
void mtcli_error_init(mtcli_error_t *error);
void mtcli_error_set(mtcli_error_t *error, mtcli_error_code_t code, const char *message, const char *details);
void mtcli_error_set_http(mtcli_error_t *error, int http_status, const char *message);
const char* mtcli_error_code_to_string(mtcli_error_code_t code);
void mtcli_error_print(mtcli_error_t *error);
int mtcli_error_to_exit_code(mtcli_error_code_t code);

// Парсинг аргументов командной строки
int mtcli_parse_args(int argc, char **argv, mtcli_config_t *config, 
                     mtcli_command_t *cmd, char **cmd_args);

// Определение типа команды
mtcli_command_t mtcli_parse_command(const char *cmd_str);

// Выполнение команд
mtcli_result_t mtcli_cmd_status(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_stats(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_config(mtcli_config_t *config, const char *action, const char *key, const char *value);
mtcli_result_t mtcli_cmd_secrets(mtcli_config_t *config, const char *action, const char *secret, const char *description);
mtcli_result_t mtcli_cmd_logs(mtcli_config_t *config, const char *level, int tail, bool follow);
mtcli_result_t mtcli_cmd_connections(mtcli_config_t *config, const char *filter);
mtcli_result_t mtcli_cmd_ratelimit(mtcli_config_t *config, const char *action, const char *key, int max_requests, int window);
mtcli_result_t mtcli_cmd_health(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_metrics(mtcli_config_t *config, const char *format);
mtcli_result_t mtcli_cmd_reload(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_restart(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_stop(mtcli_config_t *config);
mtcli_result_t mtcli_cmd_help(const char *command);
mtcli_result_t mtcli_cmd_version(void);

// REST API вызовы
int mtcli_rest_call(mtcli_config_t *config, const char *method, const char *endpoint,
                    const char *body, char *response, size_t response_size);

// gRPC вызовы (заглушки, реализация через grpc-server.c)
int mtcli_grpc_call_status(mtcli_config_t *config, mtcli_server_status_t *status);
int mtcli_grpc_call_stats(mtcli_config_t *config, mtcli_statistics_t *stats);
int mtcli_grpc_call_config(mtcli_config_t *config, mtcli_config_data_t *config_data);

// Утилиты
void mtcli_result_free(mtcli_result_t *result);
void mtcli_print_result(mtcli_result_t *result);
char* mtcli_format_bytes(uint64_t bytes, char *buffer, size_t buffer_size);
char* mtcli_format_uptime(uint64_t ms, char *buffer, size_t buffer_size);
void mtcli_print_json(const char *json);
void mtcli_print_table(const char **headers, int num_headers, const char ***rows, int num_rows);

// Интерактивный режим
int mtcli_interactive_mode(mtcli_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // MTCLI_H
