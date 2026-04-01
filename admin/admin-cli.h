/*
    Admin CLI Utility for MTProxy
    Утилита командной строки для администрирования MTProxy
*/

#ifndef ADMIN_CLI_H
#define ADMIN_CLI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Команды CLI
typedef enum {
    CMD_UNKNOWN = 0,
    CMD_HELP,
    CMD_STATUS,
    CMD_STATS,
    CMD_RELOAD,
    CMD_RESTART,
    CMD_STOP,
    CMD_CONFIG_SHOW,
    CMD_CONFIG_SET,
    CMD_CONFIG_SAVE,
    CMD_CACHE_STATS,
    CMD_CACHE_CLEAR,
    CMD_CACHE_GET,
    CMD_CACHE_SET,
    CMD_CACHE_DELETE,
    CMD_RATELIMIT_STATUS,
    CMD_RATELIMIT_ADD,
    CMD_RATELIMIT_REMOVE,
    CMD_RATELIMIT_WHITELIST,
    CMD_RATELIMIT_BLACKLIST,
    CMD_CONNECTIONS_LIST,
    CMD_CONNECTIONS_KILL,
    CMD_LOG_LEVEL,
    CMD_LOG_FLUSH,
    CMD_ERROR_SHOW,
    CMD_ERROR_CLEAR,
    CMD_HEALTH,
    CMD_METRICS,
    CMD_EXIT,
    CMD_QUIT
} admin_command_t;

// Контекст CLI
typedef struct {
    int is_interactive;
    char prompt[64];
    char hostname[128];
    int port;
    char auth_token[256];
    
    // Соединение
    int socket_fd;
    int is_connected;
    
    // Формат вывода
    int output_format;  // 0=text, 1=json, 2=table
    int verbose;
    int color_enabled;
    
    // История команд
    char **command_history;
    int history_count;
    int history_max;
    
    // Статистика сессии
    int commands_executed;
    int commands_failed;
    time_t session_start;
} admin_cli_context_t;

// Результат выполнения команды
typedef struct {
    int success;
    int exit_code;
    char *output;
    size_t output_size;
    char *error_message;
    time_t execution_time_ms;
} admin_command_result_t;

// Инициализация
int admin_cli_init(admin_cli_context_t *ctx);
int admin_cli_connect(admin_cli_context_t *ctx, const char *host, int port, const char *token);
void admin_cli_cleanup(admin_cli_context_t *ctx);

// Запуск CLI
int admin_cli_run(admin_cli_context_t *ctx);
int admin_cli_run_interactive(admin_cli_context_t *ctx);
int admin_cli_run_command(admin_cli_context_t *ctx, const char *command_line);

// Парсинг команд
admin_command_t admin_cli_parse_command(const char *command_name);
int admin_cli_execute(admin_cli_context_t *ctx, const char *command_line, admin_command_result_t *result);

// Обработчики команд
int admin_cmd_help(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_status(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_stats(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_reload(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_config_show(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_config_set(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_cache_stats(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_cache_clear(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_ratelimit_status(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_connections_list(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_log_level(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_health(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);
int admin_cmd_metrics(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result);

// Утилиты
char** admin_cli_tokenize(const char *command_line, int *argc);
void admin_cli_free_tokens(char **tokens, int argc);
void admin_cli_print_result(admin_command_result_t *result);
char* admin_cli_format_json(const char *data);
char* admin_cli_format_table(const char **headers, int num_columns, const char **rows, int num_rows);

// Автодополнение
int admin_cli_autocomplete(const char *partial_command, char **suggestions, int max_suggestions);

// История команд
int admin_cli_add_history(admin_cli_context_t *ctx, const char *command);
int admin_cli_save_history(admin_cli_context_t *ctx, const char *filename);
int admin_cli_load_history(admin_cli_context_t *ctx, const char *filename);

// Вспомогательные макросы
#define ADMIN_CLI_RESULT_INIT {0, 0, NULL, 0, NULL, 0}

#define ADMIN_CLI_RETURN_SUCCESS(result) \
    do { (result)->success = 1; (result)->exit_code = 0; return 0; } while(0)

#define ADMIN_CLI_RETURN_ERROR(result, msg) \
    do { (result)->success = 0; (result)->exit_code = 1; (result)->error_message = strdup(msg); return 1; } while(0)

#ifdef __cplusplus
}
#endif

#endif // ADMIN_CLI_H
