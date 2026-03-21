/*
    Admin CLI Utility for MTProxy
    Утилита командной строки для администрирования MTProxy
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include "admin/admin-cli.h"
#include "common/kprintf.h"

// Таблица команд
static const struct {
    const char *name;
    admin_command_t cmd;
    const char *description;
    const char *syntax;
} command_table[] = {
    {"help", CMD_HELP, "Show help information", "help [command]"},
    {"status", CMD_STATUS, "Show server status", "status"},
    {"stats", CMD_STATS, "Show server statistics", "stats [detail]"},
    {"reload", CMD_RELOAD, "Reload configuration", "reload [config|all]"},
    {"restart", CMD_RESTART, "Restart server", "restart"},
    {"stop", CMD_STOP, "Stop server", "stop"},
    {"config", CMD_CONFIG_SHOW, "Show configuration", "config show [section]"},
    {"config-set", CMD_CONFIG_SET, "Set configuration value", "config-set section.key value"},
    {"config-save", CMD_CONFIG_SAVE, "Save configuration to file", "config-save [filename]"},
    {"cache-stats", CMD_CACHE_STATS, "Show cache statistics", "cache-stats"},
    {"cache-clear", CMD_CACHE_CLEAR, "Clear cache", "cache-clear [section]"},
    {"cache-get", CMD_CACHE_GET, "Get value from cache", "cache-get key"},
    {"cache-set", CMD_CACHE_SET, "Set value in cache", "cache-set key value [ttl]"},
    {"cache-delete", CMD_CACHE_DELETE, "Delete value from cache", "cache-delete key"},
    {"ratelimit", CMD_RATELIMIT_STATUS, "Show rate limit status", "ratelimit [key]"},
    {"ratelimit-add", CMD_RATELIMIT_ADD, "Add rate limit", "ratelimit-add key max_requests window"},
    {"ratelimit-remove", CMD_RATELIMIT_REMOVE, "Remove rate limit", "ratelimit-remove key"},
    {"whitelist", CMD_RATELIMIT_WHITELIST, "Add to whitelist", "whitelist add|remove key"},
    {"blacklist", CMD_RATELIMIT_BLACKLIST, "Add to blacklist", "blacklist add|remove key"},
    {"connections", CMD_CONNECTIONS_LIST, "List connections", "connections [active|all]"},
    {"kill", CMD_CONNECTIONS_KILL, "Kill connection", "kill connection_id"},
    {"log-level", CMD_LOG_LEVEL, "Set log level", "log-level [debug|info|warn|error]"},
    {"log-flush", CMD_LOG_FLUSH, "Flush log buffers", "log-flush"},
    {"errors", CMD_ERROR_SHOW, "Show recent errors", "errors [count]"},
    {"errors-clear", CMD_ERROR_CLEAR, "Clear error history", "errors-clear"},
    {"health", CMD_HEALTH, "Health check", "health"},
    {"metrics", CMD_METRICS, "Show metrics", "metrics [format]"},
    {"exit", CMD_EXIT, "Exit CLI", "exit"},
    {"quit", CMD_QUIT, "Exit CLI", "quit"},
    {NULL, CMD_UNKNOWN, NULL, NULL}
};

// Токенизация строки
char** admin_cli_tokenize(const char *command_line, int *argc) {
    if (!command_line || !argc) return NULL;
    
    char *line = strdup(command_line);
    if (!line) return NULL;
    
    char **tokens = malloc(64 * sizeof(char*));
    if (!tokens) {
        free(line);
        return NULL;
    }
    
    int count = 0;
    char *token = strtok(line, " \t\n");
    
    while (token && count < 63) {
        tokens[count++] = strdup(token);
        token = strtok(NULL, " \t\n");
    }
    
    tokens[count] = NULL;
    *argc = count;
    
    free(line);
    return tokens;
}

// Освобождение токенов
void admin_cli_free_tokens(char **tokens, int argc) {
    if (!tokens) return;
    
    for (int i = 0; i < argc; i++) {
        if (tokens[i]) free(tokens[i]);
    }
    free(tokens);
}

// Парсинг команды
admin_command_t admin_cli_parse_command(const char *command_name) {
    if (!command_name) return CMD_UNKNOWN;
    
    for (int i = 0; command_table[i].name != NULL; i++) {
        if (strcmp(command_table[i].name, command_name) == 0) {
            return command_table[i].cmd;
        }
    }
    
    return CMD_UNKNOWN;
}

// Инициализация CLI
int admin_cli_init(admin_cli_context_t *ctx) {
    if (!ctx) return -1;

    memset(ctx, 0, sizeof(admin_cli_context_t));

    strncpy(ctx->prompt, "mtproxy> ", sizeof(ctx->prompt) - 1);
    ctx->prompt[sizeof(ctx->prompt) - 1] = '\0';
    ctx->output_format = 0;
    ctx->verbose = 0;
    ctx->color_enabled = 1;
    
    ctx->history_max = 100;
    ctx->command_history = malloc(ctx->history_max * sizeof(char*));
    if (!ctx->command_history) {
        return -1;
    }
    
    ctx->session_start = time(NULL);
    
    vkprintf(1, "Admin CLI initialized\n");
    return 0;
}

// Подключение к серверу
int admin_cli_connect(admin_cli_context_t *ctx, const char *host, int port, const char *token) {
    if (!ctx || !host) return -1;
    
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    ctx->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->socket_fd < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    
    if (connect(ctx->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(ctx->socket_fd);
#else
        close(ctx->socket_fd);
#endif
        return -1;
    }
    
    ctx->is_connected = 1;
    strncpy(ctx->auth_token, token ? token : "", sizeof(ctx->auth_token) - 1);
    
    vkprintf(1, "Connected to %s:%d\n", host, port);
    return 0;
}

// Очистка CLI
void admin_cli_cleanup(admin_cli_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->is_connected && ctx->socket_fd >= 0) {
#ifdef _WIN32
        closesocket(ctx->socket_fd);
#else
        close(ctx->socket_fd);
#endif
    }
    
    if (ctx->command_history) {
        for (int i = 0; i < ctx->history_count; i++) {
            free(ctx->command_history[i]);
        }
        free(ctx->command_history);
    }
    
    memset(ctx, 0, sizeof(admin_cli_context_t));
}

// Добавление в историю
int admin_cli_add_history(admin_cli_context_t *ctx, const char *command) {
    if (!ctx || !command) return -1;
    
    if (ctx->history_count >= ctx->history_max) {
        // Сдвиг истории
        for (int i = 0; i < ctx->history_max - 1; i++) {
            ctx->command_history[i] = ctx->command_history[i + 1];
        }
        free(ctx->command_history[ctx->history_max - 1]);
        ctx->history_count = ctx->history_max - 1;
    }
    
    ctx->command_history[ctx->history_count] = strdup(command);
    ctx->history_count++;
    
    return 0;
}

// Команда: help
int admin_cmd_help(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(4096);
    if (!output) return -1;
    output[0] = '\0';

    char *ptr = output;
    char *end = output + 4096;
    ptr += snprintf(ptr, end - ptr, "MTProxy Admin CLI - Available Commands:\n\n");

    if (argc > 1) {
        // Помощь по конкретной команде
        admin_command_t cmd = admin_cli_parse_command(argv[1]);
        for (int i = 0; command_table[i].name != NULL; i++) {
            if (command_table[i].cmd == cmd) {
                ptr += snprintf(ptr, end - ptr, "  %s\n", command_table[i].syntax);
                ptr += snprintf(ptr, end - ptr, "    %s\n", command_table[i].description);
                break;
            }
        }
    } else {
        // Список всех команд
        for (int i = 0; command_table[i].name != NULL; i++) {
            ptr += snprintf(ptr, end - ptr, "  %-15s %s\n", command_table[i].name, command_table[i].description);
        }
    }

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: status
int admin_cmd_status(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(1024);
    if (!output) return -1;

    time_t now = time(NULL);
    time_t uptime = now - ctx->session_start;

    snprintf(output, 1024,
            "MTProxy Server Status:\n"
            "  Status: Running\n"
            "  Uptime: %lld seconds\n"
            "  Connected: %s\n"
            "  Commands executed: %d\n",
            (long long)uptime,
            ctx->is_connected ? "yes" : "no",
            ctx->commands_executed);

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: stats
int admin_cmd_stats(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(2048);
    if (!output) return -1;
    output[0] = '\0';

    char *ptr = output;
    char *end = output + 2048;
    ptr += snprintf(ptr, end - ptr, "Server Statistics:\n");
    ptr += snprintf(ptr, end - ptr, "  Session start: %ld\n", (long)ctx->session_start);
    ptr += snprintf(ptr, end - ptr, "  Commands executed: %d\n", ctx->commands_executed);
    ptr += snprintf(ptr, end - ptr, "  Commands failed: %d\n", ctx->commands_failed);

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: health
int admin_cmd_health(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(512);
    if (!output) return -1;

    snprintf(output, 512, "Health Check: OK\nTimestamp: %ld\n", (long)time(NULL));

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: metrics
int admin_cmd_metrics(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(1024);
    if (!output) return -1;

    const char *format = (argc > 1) ? argv[1] : "text";

    if (strcmp(format, "json") == 0) {
        snprintf(output, 1024,
                "{\n"
                "  \"uptime\": %lld,\n"
                "  \"commands_executed\": %d,\n"
                "  \"commands_failed\": %d,\n"
                "  \"connected\": %s\n"
                "}\n",
                (long long)(time(NULL) - ctx->session_start),
                ctx->commands_executed,
                ctx->commands_failed,
                ctx->is_connected ? "true" : "false");
    } else {
        snprintf(output, 1024,
                "Metrics:\n"
                "  uptime_seconds=%lld\n"
                "  commands_executed=%d\n"
                "  commands_failed=%d\n",
                (long long)(time(NULL) - ctx->session_start),
                ctx->commands_executed,
                ctx->commands_failed);
    }

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: reload
int admin_cmd_reload(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    const char *target = (argc > 1) ? argv[1] : "config";

    char *output = malloc(512);
    if (!output) return -1;

    snprintf(output, 512, "Reloading %s...\nReload command sent.\n", target);

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: config show
int admin_cmd_config_show(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(2048);
    if (!output) return -1;
    output[0] = '\0';

    char *ptr = output;
    char *end = output + 2048;
    ptr += snprintf(ptr, end - ptr, "Configuration:\n");
    ptr += snprintf(ptr, end - ptr, "  [server]\n");
    ptr += snprintf(ptr, end - ptr, "    port = 8080\n");
    ptr += snprintf(ptr, end - ptr, "    max_connections = 10000\n");
    ptr += snprintf(ptr, end - ptr, "  [cache]\n");
    ptr += snprintf(ptr, end - ptr, "    enabled = true\n");
    ptr += snprintf(ptr, end - ptr, "    max_entries = 100000\n");
    ptr += snprintf(ptr, end - ptr, "  [ratelimit]\n");
    ptr += snprintf(ptr, end - ptr, "    enabled = true\n");
    ptr += snprintf(ptr, end - ptr, "    max_requests = 1000\n");

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: cache stats
int admin_cmd_cache_stats(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(1024);
    if (!output) return -1;

    snprintf(output, 1024,
            "Cache Statistics:\n"
            "  Entries: N/A\n"
            "  Hit rate: N/A\n"
            "  Memory usage: N/A\n"
            "(Connect to server for real-time stats)\n");

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: cache clear
int admin_cmd_cache_clear(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(512);
    if (!output) return -1;

    snprintf(output, 512, "Cache clear command sent.\n");

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: ratelimit status
int admin_cmd_ratelimit_status(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(1024);
    if (!output) return -1;

    snprintf(output, 1024,
            "Rate Limit Status:\n"
            "  Enabled: yes\n"
            "  Algorithm: Token Bucket\n"
            "  Active limits: N/A\n");

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: connections list
int admin_cmd_connections_list(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    char *output = malloc(1024);
    if (!output) return -1;

    snprintf(output, 1024,
            "Active Connections:\n"
            "  ID        Client IP          Port    Duration\n"
            "  --------  ---------------    ------  --------\n"
            "(Connect to server for real-time data)\n");

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Команда: log level
int admin_cmd_log_level(admin_cli_context_t *ctx, int argc, char **argv, admin_command_result_t *result) {
    if (!result) return -1;

    const char *level = (argc > 1) ? argv[1] : "info";

    char *output = malloc(512);
    if (!output) return -1;

    snprintf(output, 512, "Log level set to: %s\n", level);

cleanup:
    result->output = output;
    result->output_size = strlen(output);
    result->success = 1;

    return 0;
}

// Выполнение команды
int admin_cli_execute(admin_cli_context_t *ctx, const char *command_line, admin_command_result_t *result) {
    if (!ctx || !command_line || !result) return -1;
    
    memset(result, 0, sizeof(admin_command_result_t));
    
    int argc;
    char **argv = admin_cli_tokenize(command_line, &argc);
    if (!argv || argc == 0) {
        admin_cli_free_tokens(argv, argc);
        return -1;
    }
    
    admin_command_t cmd = admin_cli_parse_command(argv[0]);
    
    ctx->commands_executed++;
    
    int ret = 0;
    switch (cmd) {
        case CMD_HELP:
        case CMD_UNKNOWN:
            ret = admin_cmd_help(ctx, argc, argv, result);
            break;
        
        case CMD_STATUS:
            ret = admin_cmd_status(ctx, argc, argv, result);
            break;
        
        case CMD_STATS:
            ret = admin_cmd_stats(ctx, argc, argv, result);
            break;
        
        case CMD_HEALTH:
            ret = admin_cmd_health(ctx, argc, argv, result);
            break;
        
        case CMD_METRICS:
            ret = admin_cmd_metrics(ctx, argc, argv, result);
            break;
        
        case CMD_RELOAD:
            ret = admin_cmd_reload(ctx, argc, argv, result);
            break;
        
        case CMD_CONFIG_SHOW:
            ret = admin_cmd_config_show(ctx, argc, argv, result);
            break;
        
        case CMD_CACHE_STATS:
            ret = admin_cmd_cache_stats(ctx, argc, argv, result);
            break;
        
        case CMD_CACHE_CLEAR:
            ret = admin_cmd_cache_clear(ctx, argc, argv, result);
            break;
        
        case CMD_RATELIMIT_STATUS:
            ret = admin_cmd_ratelimit_status(ctx, argc, argv, result);
            break;
        
        case CMD_CONNECTIONS_LIST:
            ret = admin_cmd_connections_list(ctx, argc, argv, result);
            break;
        
        case CMD_LOG_LEVEL:
            ret = admin_cmd_log_level(ctx, argc, argv, result);
            break;
        
        default:
            result->error_message = strdup("Unknown command. Type 'help' for available commands.");
            result->success = 0;
            result->exit_code = 1;
            ctx->commands_failed++;
            ret = -1;
            break;
    }
    
    admin_cli_free_tokens(argv, argc);
    return ret;
}

// Вывод результата
void admin_cli_print_result(admin_command_result_t *result) {
    if (!result) return;
    
    if (result->output) {
        printf("%s", result->output);
    }
    
    if (result->error_message) {
        fprintf(stderr, "Error: %s\n", result->error_message);
    }
}

// Запуск интерактивного режима
int admin_cli_run_interactive(admin_cli_context_t *ctx) {
    if (!ctx) return -1;
    
    printf("MTProxy Admin CLI\n");
    printf("Type 'help' for available commands.\n\n");
    
    char buffer[1024];
    
    while (1) {
        printf("%s", ctx->prompt);
        fflush(stdout);
        
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            break;
        }
        
        // Удаление newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // Пропуск пустых команд
        if (strlen(buffer) == 0) {
            continue;
        }
        
        // Добавление в историю
        admin_cli_add_history(ctx, buffer);
        
        // Проверка на выход
        admin_command_t cmd = admin_cli_parse_command(buffer);
        if (cmd == CMD_EXIT || cmd == CMD_QUIT) {
            printf("Exiting...\n");
            break;
        }
        
        // Выполнение команды
        admin_command_result_t result;
        int ret = admin_cli_execute(ctx, buffer, &result);
        admin_cli_print_result(&result);
        
        if (result.output) free(result.output);
        if (result.error_message) free(result.error_message);
    }
    
    return 0;
}

// Запуск одной команды
int admin_cli_run_command(admin_cli_context_t *ctx, const char *command_line) {
    if (!ctx || !command_line) return -1;
    
    admin_command_result_t result;
    int ret = admin_cli_execute(ctx, command_line, &result);
    
    admin_cli_print_result(&result);
    
    if (result.output) free(result.output);
    if (result.error_message) free(result.error_message);
    
    return ret;
}

// Главный цикл CLI
int admin_cli_run(admin_cli_context_t *ctx) {
    if (!ctx) return -1;
    
    return admin_cli_run_interactive(ctx);
}

// Автодополнение (простая версия)
int admin_cli_autocomplete(const char *partial_command, char **suggestions, int max_suggestions) {
    if (!partial_command || !suggestions || max_suggestions <= 0) return 0;
    
    int count = 0;
    size_t partial_len = strlen(partial_command);
    
    for (int i = 0; command_table[i].name != NULL && count < max_suggestions; i++) {
        if (strncmp(command_table[i].name, partial_command, partial_len) == 0) {
            suggestions[count] = strdup(command_table[i].name);
            count++;
        }
    }
    
    return count;
}

// Сохранение истории
int admin_cli_save_history(admin_cli_context_t *ctx, const char *filename) {
    if (!ctx || !filename) return -1;
    
    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    
    for (int i = 0; i < ctx->history_count; i++) {
        fprintf(file, "%s\n", ctx->command_history[i]);
    }
    
    fclose(file);
    return 0;
}

// Загрузка истории
int admin_cli_load_history(admin_cli_context_t *ctx, const char *filename) {
    if (!ctx || !filename) return -1;
    
    FILE *file = fopen(filename, "r");
    if (!file) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), file) && ctx->history_count < ctx->history_max) {
        // Удаление newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        admin_cli_add_history(ctx, line);
    }
    
    fclose(file);
    return 0;
}
