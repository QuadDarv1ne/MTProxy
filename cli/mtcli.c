/*
    MTProxy CLI - Cross-platform Command Line Interface
    Реализация утилиты для управления и мониторинга MTProxy сервера
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include "cli/mtcli.h"
#include "common/utils.h"

// ============================================================================
// Обработка ошибок
// ============================================================================

void mtcli_error_init(mtcli_error_t *error) {
    if (!error) return;
    memset(error, 0, sizeof(mtcli_error_t));
    error->code = MTCLI_OK;
    error->http_status = 0;
}

void mtcli_error_set(mtcli_error_t *error, mtcli_error_code_t code, const char *message, const char *details) {
    if (!error) return;
    error->code = code;
    if (message) {
        snprintf(error->message, sizeof(error->message), "%s", message);
    }
    if (details) {
        snprintf(error->details, sizeof(error->details), "%s", details);
    }
}

void mtcli_error_set_http(mtcli_error_t *error, int http_status, const char *message) {
    if (!error) return;
    error->http_status = http_status;
    
    // Маппинг HTTP статусов на коды ошибок CLI
    if (http_status == 0) {
        error->code = MTCLI_ERR_CONNECTION;
        if (!message) snprintf(error->message, sizeof(error->message), "Connection failed");
    } else if (http_status == 401 || http_status == 403) {
        error->code = MTCLI_ERR_AUTH;
        if (!message) snprintf(error->message, sizeof(error->message), "Authentication failed");
    } else if (http_status == 404) {
        error->code = MTCLI_ERR_NOT_FOUND;
        if (!message) snprintf(error->message, sizeof(error->message), "Resource not found");
    } else if (http_status == 408) {
        error->code = MTCLI_ERR_TIMEOUT;
        if (!message) snprintf(error->message, sizeof(error->message), "Request timeout");
    } else if (http_status >= 500) {
        error->code = MTCLI_ERR_SERVER;
        if (!message) snprintf(error->message, sizeof(error->message), "Server error");
    } else {
        error->code = MTCLI_ERR_UNKNOWN;
        if (!message) snprintf(error->message, sizeof(error->message), "Unknown error");
    }
    
    if (message) {
        snprintf(error->message, sizeof(error->message), "%s (HTTP %d)", message, http_status);
    }
}

const char* mtcli_error_code_to_string(mtcli_error_code_t code) {
    switch (code) {
        case MTCLI_OK: return "OK";
        case MTCLI_ERR_CONNECTION: return "Connection Error";
        case MTCLI_ERR_AUTH: return "Authentication Error";
        case MTCLI_ERR_TIMEOUT: return "Timeout";
        case MTCLI_ERR_INVALID_RESPONSE: return "Invalid Response";
        case MTCLI_ERR_SERVER: return "Server Error";
        case MTCLI_ERR_INVALID_ARGS: return "Invalid Arguments";
        case MTCLI_ERR_NOT_FOUND: return "Not Found";
        case MTCLI_ERR_PERMISSION: return "Permission Denied";
        case MTCLI_ERR_UNKNOWN: return "Unknown Error";
        default: return "Unknown";
    }
}

void mtcli_error_print(mtcli_error_t *error) {
    if (!error || error->code == MTCLI_OK) return;
    
    fprintf(stderr, "\n❌ Error: %s\n", mtcli_error_code_to_string(error->code));
    
    if (error->message[0] != '\0') {
        fprintf(stderr, "   Message: %s\n", error->message);
    }
    
    if (error->details[0] != '\0') {
        fprintf(stderr, "   Details: %s\n", error->details);
    }
    
    if (error->http_status > 0) {
        fprintf(stderr, "   HTTP Status: %d\n", error->http_status);
    }
    
    fprintf(stderr, "\n");
}

int mtcli_error_to_exit_code(mtcli_error_code_t code) {
    switch (code) {
        case MTCLI_OK: return 0;
        case MTCLI_ERR_INVALID_ARGS: return 1;
        case MTCLI_ERR_NOT_FOUND: return 2;
        case MTCLI_ERR_PERMISSION: return 3;
        case MTCLI_ERR_AUTH: return 4;
        case MTCLI_ERR_TIMEOUT: return 5;
        case MTCLI_ERR_CONNECTION: return 6;
        case MTCLI_ERR_SERVER: return 7;
        case MTCLI_ERR_INVALID_RESPONSE: return 8;
        default: return 99;
    }
}

// ============================================================================
// Инициализация конфигурации
// ============================================================================

void mtcli_config_init(mtcli_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(mtcli_config_t));
    snprintf(config->host, sizeof(config->host), "localhost");
    config->rest_port = 8080;
    config->grpc_port = 50051;
    config->server_port = 8888;
    config->mode = MTCLI_MODE_REST;
    config->verbose = false;
    config->json_output = false;
    config->insecure = false;
    config->timeout_ms = 5000;
}

// ============================================================================
// Парсинг аргументов командной строки
// ============================================================================

static void print_usage(void) {
    printf("MTProxy CLI v%s - утилита для управления MTProxy сервером\n\n", MTCLI_VERSION);
    printf("Использование:\n");
    printf("  mtproxy-cli <command> [options]\n\n");
    printf("Команды:\n");
    printf("  status              Показать статус сервера\n");
    printf("  stats               Показать статистику\n");
    printf("  config get          Получить конфигурацию\n");
    printf("  config set <key> <value>  Установить параметр\n");
    printf("  secrets list        Список секретов\n");
    printf("  secrets add --secret <secret> [--desc <description>]\n");
    printf("  secrets remove <hash>  Удалить секрет\n");
    printf("  logs [--level <level>] [--tail <n>] [--follow]\n");
    printf("  connections [active|all]\n");
    printf("  ratelimit status|add|remove <key>\n");
    printf("  health              Проверка здоровья\n");
    printf("  metrics [json|prometheus]\n");
    printf("  reload              Перезагрузить конфигурацию\n");
    printf("  restart             Перезапустить сервер\n");
    printf("  stop                Остановить сервер\n");
    printf("  ml-stats            ML статистика\n");
    printf("  ml-anomaly [status] ML детекция аномалий\n");
    printf("  ml-predict [metric] ML прогнозирование\n");
    printf("  help [command]      Показать справку\n");
    printf("  version             Показать версию\n\n");
    printf("Опции:\n");
    printf("  --host <host>       Хост сервера (по умолчанию: localhost)\n");
    printf("  --port <port>       Порт REST API (по умолчанию: 8080)\n");
    printf("  --grpc-port <port>  Порт gRPC (по умолчанию: 50051)\n");
    printf("  --api-key <key>     API ключ для аутентификации\n");
    printf("  --mode <mode>       Режим: rest, grpc, direct (по умолчанию: rest)\n");
    printf("  --json              Вывод в формате JSON\n");
    printf("  --verbose, -v       Подробный вывод\n");
    printf("  --timeout <ms>      Таймаут подключения (по умолчанию: 5000)\n");
    printf("  --insecure          Не проверять SSL\n");
    printf("\nПримеры:\n");
    printf("  mtproxy-cli status\n");
    printf("  mtproxy-cli stats --json\n");
    printf("  mtproxy-cli config get\n");
    printf("  mtproxy-cli secrets add --secret dd1234567890abcdef --desc \"My proxy\"\n");
    printf("  mtproxy-cli logs --level error --tail 50\n");
    printf("  mtproxy-cli --host 192.168.1.100 --port 8080 status\n");
}

int mtcli_parse_args(int argc, char **argv, mtcli_config_t *config, 
                     mtcli_command_t *cmd, char ***cmd_args) {
    if (!config || !cmd || argc < 1) return -1;
    
    mtcli_config_init(config);
    *cmd = MTCLI_CMD_UNKNOWN;
    *cmd_args = NULL;
    
    int i = 1;  // Пропускаем имя программы
    int arg_index = 0;
    
    // Выделяем память для аргументов команды
    *cmd_args = malloc(sizeof(char*) * (argc + 1));
    if (!*cmd_args) return -1;
    memset(*cmd_args, 0, sizeof(char*) * (argc + 1));
    
    while (i < argc) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            snprintf(config->host, sizeof(config->host), "%s", argv[++i]);
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config->rest_port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--grpc-port") == 0 && i + 1 < argc) {
            config->grpc_port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--api-key") == 0 && i + 1 < argc) {
            snprintf(config->api_key, sizeof(config->api_key), "%s", argv[++i]);
        }
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "rest") == 0) config->mode = MTCLI_MODE_REST;
            else if (strcmp(argv[i], "grpc") == 0) config->mode = MTCLI_MODE_GRPC;
            else if (strcmp(argv[i], "direct") == 0) config->mode = MTCLI_MODE_DIRECT;
        }
        else if (strcmp(argv[i], "--json") == 0) {
            config->json_output = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            config->verbose = true;
        }
        else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            config->timeout_ms = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--insecure") == 0) {
            config->insecure = true;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            *cmd = MTCLI_CMD_HELP;
            return 0;
        }
        else if (strcmp(argv[i], "--version") == 0) {
            *cmd = MTCLI_CMD_VERSION;
            return 0;
        }
        else if (argv[i][0] != '-') {
            // Это команда или аргумент команды
            if (*cmd == MTCLI_CMD_UNKNOWN) {
                *cmd = mtcli_parse_command(argv[i]);
            } else {
                (*cmd_args)[arg_index++] = argv[i];
            }
        }
        i++;
    }
    
    return 0;
}

mtcli_command_t mtcli_parse_command(const char *cmd_str) {
    if (!cmd_str) return MTCLI_CMD_UNKNOWN;

    if (strcmp(cmd_str, "status") == 0) return MTCLI_CMD_STATUS;
    if (strcmp(cmd_str, "stats") == 0) return MTCLI_CMD_STATS;
    if (strcmp(cmd_str, "config") == 0) return MTCLI_CMD_CONFIG;
    if (strcmp(cmd_str, "secrets") == 0) return MTCLI_CMD_SECRETS;
    if (strcmp(cmd_str, "logs") == 0) return MTCLI_CMD_LOGS;
    if (strcmp(cmd_str, "connections") == 0) return MTCLI_CMD_CONNECTIONS;
    if (strcmp(cmd_str, "ratelimit") == 0) return MTCLI_CMD_RATELIMIT;
    if (strcmp(cmd_str, "health") == 0) return MTCLI_CMD_HEALTH;
    if (strcmp(cmd_str, "metrics") == 0) return MTCLI_CMD_METRICS;
    if (strcmp(cmd_str, "reload") == 0) return MTCLI_CMD_RELOAD;
    if (strcmp(cmd_str, "restart") == 0) return MTCLI_CMD_RESTART;
    if (strcmp(cmd_str, "stop") == 0) return MTCLI_CMD_STOP;
    if (strcmp(cmd_str, "help") == 0) return MTCLI_CMD_HELP;
    if (strcmp(cmd_str, "version") == 0) return MTCLI_CMD_VERSION;
    if (strcmp(cmd_str, "ml-stats") == 0) return MTCLI_CMD_ML_STATS;
    if (strcmp(cmd_str, "ml-anomaly") == 0) return MTCLI_CMD_ML_ANOMALY;
    if (strcmp(cmd_str, "ml-predict") == 0) return MTCLI_CMD_ML_PREDICT;

    return MTCLI_CMD_UNKNOWN;
}

// ============================================================================
// HTTP клиент для REST API
// ============================================================================

static int http_create_socket(const char *host, int port) {
    int sock = -1;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
#endif
    
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return -1;
    }
    
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) continue;
        
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // Успешное подключение
        }
        
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        sock = -1;
    }
    
    freeaddrinfo(res);
    return sock;
}

static int http_send_request(int sock, const char *method, const char *path,
                             const char *host, const char *api_key,
                             const char *body) {
    if (!method || !path || !host) return -1;
    
    char request[4096] = {0};
    int offset = 0;
    
    // Строка запроса
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "%s %s HTTP/1.1\r\n", method, path);
    
    // Заголовки
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "Host: %s\r\n", host);
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "Connection: close\r\n");
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "Accept: application/json\r\n");
    
    if (api_key && api_key[0]) {
        offset += snprintf(request + offset, sizeof(request) - offset,
                           "X-API-Key: %s\r\n", api_key);
    }
    
    if (body && strlen(body) > 0) {
        offset += snprintf(request + offset, sizeof(request) - offset,
                           "Content-Type: application/json\r\n");
        offset += snprintf(request + offset, sizeof(request) - offset,
                           "Content-Length: %zu\r\n", strlen(body));
    }
    
    // Конец заголовков
    offset += snprintf(request + offset, sizeof(request) - offset, "\r\n");
    
    // Тело запроса
    if (body && strlen(body) > 0) {
        offset += snprintf(request + offset, sizeof(request) - offset, "%s", body);
    }
    
    // Отправка
    return send(sock, request, strlen(request), 0);
}

static int http_receive_response(int sock, char *response, size_t response_size) {
    size_t total_received = 0;
    int bytes_read;
    
    while (total_received < response_size - 1) {
        bytes_read = recv(sock, response + total_received, 
                         response_size - total_received - 1, 0);
        
        if (bytes_read <= 0) break;  // Конец соединения или ошибка
        
        total_received += bytes_read;
    }
    
    response[total_received] = '\0';
    return (int)total_received;
}

// ============================================================================
// REST API вызовы
// ============================================================================

int mtcli_rest_call(mtcli_config_t *config, const char *method, const char *endpoint,
                    const char *body, char *response, size_t response_size) {
    if (!config || !method || !endpoint || !response) return -1;
    
    // Формируем полный путь
    char path[MTCLI_MAX_PATH];
    snprintf(path, sizeof(path), "%s", endpoint);
    
    // Создаем сокет
    int sock = http_create_socket(config->host, config->rest_port);
    if (sock < 0) {
        snprintf(response, response_size, 
                 "{\"error\": \"Failed to connect to %s:%d\"}", 
                 config->host, config->rest_port);
        return -1;
    }
    
    // Отправляем запрос
    if (http_send_request(sock, method, path, config->host, config->api_key, body) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        snprintf(response, response_size, "{\"error\": \"Failed to send request\"}");
        return -1;
    }
    
    // Получаем ответ
    int received = http_receive_response(sock, response, response_size);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return received;
}

// ============================================================================
// Реализация команд
// ============================================================================

mtcli_result_t mtcli_cmd_status(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    
    int ret = mtcli_rest_call(config, "GET", "/api/v1/server/status", NULL, 
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;

    if (!config->json_output) {
        /* Форматируем JSON для удобного чтения */
        mtcli_print_json(response);
        free(result.output);
        result.output = NULL;
    }

    return result;
}

mtcli_result_t mtcli_cmd_stats(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};

    int ret = mtcli_rest_call(config, "GET", "/api/v1/statistics", NULL,
                              response, sizeof(response));

    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }

    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;

    if (!config->json_output) {
        /* Форматируем JSON для удобного чтения */
        mtcli_print_json(response);
        free(result.output);
        result.output = NULL;
    }

    return result;
}

mtcli_result_t mtcli_cmd_config(mtcli_config_t *config, const char *action, 
                                 const char *key, const char *value) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[256] = "/api/v1/config";
    char body[1024] = {0};
    
    if (!action) {
        // GET конфигурации
        int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else if (strcmp(action, "set") == 0 && key && value) {
        // SET конфигурации
        snprintf(body, sizeof(body), "{\"%s\": \"%s\"}", key, value);
        int ret = mtcli_rest_call(config, "PUT", endpoint, body,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Invalid config action: %s", action);
        return result;
    }

    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;

    if (!config->json_output) {
        /* Форматируем JSON для удобного чтения */
        mtcli_print_json(response);
        free(result.output);
        result.output = NULL;
    }

    return result;
}

mtcli_result_t mtcli_cmd_secrets(mtcli_config_t *config, const char *action,
                                  const char *secret, const char *description) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[256] = "/api/v1/secrets";
    char body[512] = {0};
    
    if (!action || strcmp(action, "list") == 0) {
        // Список секретов
        int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else if (strcmp(action, "add") == 0 && secret) {
        // Добавление секрета
        if (description) {
            snprintf(body, sizeof(body), "{\"secret\": \"%s\", \"description\": \"%s\"}",
                     secret, description);
        } else {
            snprintf(body, sizeof(body), "{\"secret\": \"%s\"}", secret);
        }
        int ret = mtcli_rest_call(config, "POST", endpoint, body,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else if (strcmp(action, "remove") == 0 && secret) {
        // Удаление секрета
        char path[256];
        snprintf(path, sizeof(path), "/api/v1/secrets/%s", secret);
        int ret = mtcli_rest_call(config, "DELETE", path, NULL,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Invalid secrets action: %s", 
                 action ? action : "null");
        return result;
    }

    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;

    if (!config->json_output) {
        /* Форматируем JSON для удобного чтения */
        mtcli_print_json(response);
        free(result.output);
        result.output = NULL;
    }

    return result;
}

mtcli_result_t mtcli_cmd_logs(mtcli_config_t *config, const char *level,
                               int tail, bool follow) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[512] = "/api/v1/logs?";
    
    // Формируем query параметры
    char params[256] = {0};
    int offset = 0;
    
    if (level) {
        offset += snprintf(params + offset, sizeof(params) - offset,
                           "level=%s&", level);
    }
    if (tail > 0) {
        offset += snprintf(params + offset, sizeof(params) - offset,
                           "tail=%d&", tail);
    }
    
    strncat(endpoint, params, sizeof(endpoint) - strlen(endpoint) - 1);
    
    // Удаляем последний '&' если есть
    size_t len = strlen(endpoint);
    if (len > 0 && endpoint[len-1] == '&') {
        endpoint[len-1] = '\0';
    }
    if (len > 0 && endpoint[len-1] == '?') {
        endpoint[len-1] = '\0';
    }
    
    int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_connections(mtcli_config_t *config, const char *filter) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[256] = "/api/v1/connections";
    
    if (filter) {
        snprintf(endpoint, sizeof(endpoint), "/api/v1/connections?filter=%s", filter);
    }
    
    int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_ratelimit(mtcli_config_t *config, const char *action,
                                    const char *key, int max_requests, int window) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[256] = "/api/v1/ratelimit";
    char body[256] = {0};
    
    if (!action || strcmp(action, "status") == 0) {
        int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else if (strcmp(action, "add") == 0 && key) {
        snprintf(body, sizeof(body), 
                 "{\"key\": \"%s\", \"max_requests\": %d, \"window\": %d}",
                 key, max_requests, window);
        int ret = mtcli_rest_call(config, "POST", endpoint, body,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else if (strcmp(action, "remove") == 0 && key) {
        char path[256];
        snprintf(path, sizeof(path), "/api/v1/ratelimit/%s", key);
        int ret = mtcli_rest_call(config, "DELETE", path, NULL,
                                  response, sizeof(response));
        if (ret < 0) {
            result.exit_code = 1;
            snprintf(result.error, sizeof(result.error), "Request failed");
            return result;
        }
    } else {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Invalid ratelimit action");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_health(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    
    int ret = mtcli_rest_call(config, "GET", "/api/v1/health", NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_metrics(mtcli_config_t *config, const char *format) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char endpoint[256] = "/api/v1/metrics";
    
    if (format && strcmp(format, "prometheus") == 0) {
        snprintf(endpoint, sizeof(endpoint), "/api/v1/metrics?format=prometheus");
    }
    
    int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_reload(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    
    int ret = mtcli_rest_call(config, "POST", "/api/v1/server/reload", NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_restart(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    
    int ret = mtcli_rest_call(config, "POST", "/api/v1/server/restart", NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_stop(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    
    int ret = mtcli_rest_call(config, "POST", "/api/v1/server/stop", NULL,
                              response, sizeof(response));
    
    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "Request failed");
        return result;
    }
    
    result.output = strdup(response);
    result.output_size = strlen(response);
    result.exit_code = 0;
    
    return result;
}

mtcli_result_t mtcli_cmd_help(const char *command) {
    mtcli_result_t result = {0};
    char output[4096] = {0};

    if (command) {
        /* Справка по конкретной команде */
        const char *help_text = NULL;
        
        if (strcmp(command, "status") == 0) {
            help_text = "status - Получить статистику прокси\n"
                        "  Возвращает: активные соединения, трафик, CPU, память\n";
        } else if (strcmp(command, "config") == 0) {
            help_text = "config - Управление конфигурацией\n"
                        "  Действия: get, set <key> <value>\n";
        } else if (strcmp(command, "secrets") == 0) {
            help_text = "secrets - Управление секретами\n"
                        "  Действия: list, add <secret>, remove <secret>\n";
        } else if (strcmp(command, "logs") == 0) {
            help_text = "logs - Просмотр логов\n"
                        "  Опции: --level <level>, --tail <n>, --follow\n";
        } else if (strcmp(command, "help") == 0) {
            help_text = "help - Показать справку\n"
                        "  Использование: help [command]\n";
        } else if (strcmp(command, "version") == 0) {
            help_text = "version - Показать версию\n";
        } else {
            snprintf(output, sizeof(output), "Unknown command: %s\nUse 'help' for available commands.\n", command);
            result.output = strdup(output);
            result.output_size = strlen(output);
            result.exit_code = 0;
            return result;
        }
        
        snprintf(output, sizeof(output), "Help for command: %s\n\n%s", command, help_text);
    } else {
        print_usage();
        result.output = strdup("See usage information above");
        result.output_size = strlen(result.output);
        result.exit_code = 0;
        return result;
    }

    result.output = strdup(output);
    result.output_size = strlen(output);
    result.exit_code = 0;

    return result;
}

mtcli_result_t mtcli_cmd_version(void) {
    mtcli_result_t result = {0};
    char output[256] = {0};

    snprintf(output, sizeof(output), "MTProxy CLI v%s\n", MTCLI_VERSION);

    result.output = strdup(output);
    result.output_size = strlen(output);
    result.exit_code = 0;

    return result;
}

// ============================================================================
// ML команды
// ============================================================================

mtcli_result_t mtcli_cmd_ml_stats(mtcli_config_t *config) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char output[4096] = {0};
    int offset = 0;

    // Получаем ML статистику через REST API
    int ret = mtcli_rest_call(config, "GET", "/api/v1/ml/stats", NULL,
                              response, sizeof(response));

    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "ML stats request failed");
        return result;
    }

    // Форматируем вывод
    offset += snprintf(output + offset, sizeof(output) - offset, 
                       "=== ML Statistics ===\n\n");
    
    // Парсим JSON ответ (упрощённо)
    if (strstr(response, "anomaly_detection")) {
        offset += snprintf(output + offset, sizeof(output) - offset,
                           "Anomaly Detection:\n");
        offset += snprintf(output + offset, sizeof(output) - offset,
                           "  Status: Active\n");
    }
    if (strstr(response, "predictive_analytics")) {
        offset += snprintf(output + offset, sizeof(output) - offset,
                           "Predictive Analytics:\n");
        offset += snprintf(output + offset, sizeof(output) - offset,
                           "  Status: Active\n");
    }
    
    offset += snprintf(output + offset, sizeof(output) - offset,
                       "\nRaw response:\n%s\n", response);

    result.output = strdup(output);
    result.output_size = strlen(output);
    result.exit_code = 0;

    return result;
}

mtcli_result_t mtcli_cmd_ml_anomaly(mtcli_config_t *config, const char *action) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char output[4096] = {0};
    int offset = 0;
    char endpoint[256] = {0};

    if (!action) action = "status";
    
    snprintf(endpoint, sizeof(endpoint), "/api/v1/ml/anomaly/%s", action);

    int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                              response, sizeof(response));

    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "ML anomaly request failed");
        return result;
    }

    offset += snprintf(output + offset, sizeof(output) - offset,
                       "=== Anomaly Detection ===\n\n");
    offset += snprintf(output + offset, sizeof(output) - offset,
                       "Action: %s\n\n", action);
    offset += snprintf(output + offset, sizeof(output) - offset,
                       "Response:\n%s\n", response);

    result.output = strdup(output);
    result.output_size = strlen(output);
    result.exit_code = 0;

    return result;
}

mtcli_result_t mtcli_cmd_ml_predict(mtcli_config_t *config, const char *metric) {
    mtcli_result_t result = {0};
    char response[MTCLI_MAX_RESPONSE] = {0};
    char output[4096] = {0};
    int offset = 0;
    char endpoint[256] = {0};

    if (!metric) metric = "connections";
    
    snprintf(endpoint, sizeof(endpoint), "/api/v1/ml/predict?metric=%s", metric);

    int ret = mtcli_rest_call(config, "GET", endpoint, NULL,
                              response, sizeof(response));

    if (ret < 0) {
        result.exit_code = 1;
        snprintf(result.error, sizeof(result.error), "ML predict request failed");
        return result;
    }

    offset += snprintf(output + offset, sizeof(output) - offset,
                       "=== Predictive Analytics ===\n\n");
    offset += snprintf(output + offset, sizeof(output) - offset,
                       "Metric: %s\n\n", metric);
    offset += snprintf(output + offset, sizeof(output) - offset,
                       "Prediction:\n%s\n", response);

    result.output = strdup(output);
    result.output_size = strlen(output);
    result.exit_code = 0;

    return result;
}

// ============================================================================
// Утилиты
// ============================================================================

void mtcli_result_free(mtcli_result_t *result) {
    if (!result) return;
    if (result->output) {
        free(result->output);
        result->output = NULL;
    }
}

void mtcli_print_result(mtcli_result_t *result) {
    if (!result) return;
    
    if (result->exit_code != 0 && result->error[0]) {
        fprintf(stderr, "Error: %s\n", result->error);
    }
    
    if (result->output) {
        printf("%s\n", result->output);
    }
}

char* mtcli_format_bytes(uint64_t bytes, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 32) return NULL;
    
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    snprintf(buffer, buffer_size, "%.2f %s", size, units[unit_index]);
    return buffer;
}

char* mtcli_format_uptime(uint64_t ms, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 64) return NULL;
    
    uint64_t seconds = ms / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    if (days > 0) {
        snprintf(buffer, buffer_size, "%dd %dh %dm %ds", days, hours, minutes, seconds);
    } else if (hours > 0) {
        snprintf(buffer, buffer_size, "%dh %dm %ds", hours, minutes, seconds);
    } else if (minutes > 0) {
        snprintf(buffer, buffer_size, "%dm %ds", minutes, seconds);
    } else {
        snprintf(buffer, buffer_size, "%ds", seconds);
    }
    
    return buffer;
}

void mtcli_print_json(const char *json) {
    if (!json) return;
    
    /* Простое форматирование JSON: добавляем отступы для { } [ ] */
    int indent = 0;
    const char *p = json;
    
    while (*p) {
        /* Пропускаем пробелы в начале строки */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            p++;
        }
        
        if (*p == '\0') break;
        
        /* Добавляем отступ */
        for (int i = 0; i < indent; i++) {
            putchar(' ');
        }
        
        /* Выводим символы до конца элемента */
        while (*p && *p != ',' && *p != '}' && *p != ']') {
            putchar(*p);
            if (*p == '{' || *p == '[') {
                indent += 2;
                putchar('\n');
                break;
            }
            p++;
        }
        
        /* Закрывающие скобки */
        if (*p == '}' || *p == ']') {
            indent -= 2;
            if (indent < 0) indent = 0;
            putchar('\n');
            for (int i = 0; i < indent; i++) {
                putchar(' ');
            }
            putchar(*p);
            p++;
        }
        
        /* Запятая и новая строка */
        if (*p == ',') {
            putchar(',');
            p++;
        }
        
        putchar('\n');
    }
    putchar('\n');
}

void mtcli_print_table(const char **headers, int num_headers, 
                       const char ***rows, int num_rows) {
    if (!headers || !rows || num_headers <= 0 || num_rows <= 0) return;
    
    // Вычисляем ширину колонок
    int *col_widths = calloc(num_headers, sizeof(int));
    if (!col_widths) return;
    
    for (int i = 0; i < num_headers; i++) {
        col_widths[i] = (int)strlen(headers[i]);
    }
    
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_headers; col++) {
            int len = (int)strlen(rows[row][col]);
            if (len > col_widths[col]) {
                col_widths[col] = len;
            }
        }
    }
    
    // Печатаем заголовок
    printf("|");
    for (int i = 0; i < num_headers; i++) {
        printf(" %- *s |", col_widths[i], headers[i]);
    }
    printf("\n");
    
    // Разделитель
    printf("|");
    for (int i = 0; i < num_headers; i++) {
        for (int j = 0; j < col_widths[i] + 2; j++) {
            printf("-");
        }
        printf("|");
    }
    printf("\n");
    
    // Печатаем строки
    for (int row = 0; row < num_rows; row++) {
        printf("|");
        for (int col = 0; col < num_headers; col++) {
            printf(" %- *s |", col_widths[col], rows[row][col]);
        }
        printf("\n");
    }
    
    free(col_widths);
}

// ============================================================================
// Интерактивный режим
// ============================================================================

int mtcli_interactive_mode(mtcli_config_t *config) {
    char line[MTCLI_MAX_COMMAND];
    char *args[64];
    mtcli_command_t cmd;
    
    printf("MTProxy CLI v%s - интерактивный режим\n", MTCLI_VERSION);
    printf("Введите 'help' для справки, 'exit' для выхода\n\n");
    
    while (1) {
        printf("mtproxy> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        // Удаляем newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Пропускаем пустые строки
        if (strlen(line) == 0) {
            continue;
        }
        
        // Парсим команду
        cmd = mtcli_parse_command(line);
        
        if (cmd == MTCLI_CMD_EXIT || cmd == MTCLI_CMD_QUIT) {
            break;
        } else if (cmd == MTCLI_CMD_HELP) {
            mtcli_result_t result = mtcli_cmd_help(NULL);
            mtcli_print_result(&result);
            mtcli_result_free(&result);
        } else if (cmd == MTCLI_CMD_VERSION) {
            mtcli_result_t result = mtcli_cmd_version();
            mtcli_print_result(&result);
            mtcli_result_free(&result);
        } else if (cmd == MTCLI_CMD_QUIT || cmd == MTCLI_CMD_EXIT) {
            printf("Exiting MTProxy CLI\n");
            break;
        } else if (cmd != MTCLI_CMD_UNKNOWN) {
            /* Выполнение команд в интерактивном режиме */
            printf("Command '%s' requires arguments. Use non-interactive mode.\n", line);
        } else {
            printf("Unknown command: %s\nType 'help' for available commands.\n", line);
        }
    }

    return 0;
}

// ============================================================================
// Точка входа
// ============================================================================

int main(int argc, char **argv) {
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    // Парсим аргументы
    if (mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args) != 0) {
        fprintf(stderr, "Error parsing arguments\n");
        return 1;
    }
    
    // Выполняем команду
    mtcli_result_t result = {0};
    
    switch (cmd) {
        case MTCLI_CMD_STATUS:
            result = mtcli_cmd_status(&config);
            break;
        case MTCLI_CMD_STATS:
            result = mtcli_cmd_stats(&config);
            break;
        case MTCLI_CMD_CONFIG:
            result = mtcli_cmd_config(&config, cmd_args ? cmd_args[0] : NULL,
                                      cmd_args ? cmd_args[1] : NULL,
                                      cmd_args ? cmd_args[2] : NULL);
            break;
        case MTCLI_CMD_SECRETS:
            result = mtcli_cmd_secrets(&config, cmd_args ? cmd_args[0] : NULL,
                                       cmd_args ? cmd_args[1] : NULL,
                                       cmd_args ? cmd_args[2] : NULL);
            break;
        case MTCLI_CMD_LOGS:
            result = mtcli_cmd_logs(&config, "error", 50, false);
            break;
        case MTCLI_CMD_CONNECTIONS:
            result = mtcli_cmd_connections(&config, cmd_args ? cmd_args[0] : NULL);
            break;
        case MTCLI_CMD_RATELIMIT:
            result = mtcli_cmd_ratelimit(&config, cmd_args ? cmd_args[0] : NULL,
                                         cmd_args ? cmd_args[1] : NULL, 100, 60);
            break;
        case MTCLI_CMD_HEALTH:
            result = mtcli_cmd_health(&config);
            break;
        case MTCLI_CMD_METRICS:
            result = mtcli_cmd_metrics(&config, cmd_args ? cmd_args[0] : NULL);
            break;
        case MTCLI_CMD_RELOAD:
            result = mtcli_cmd_reload(&config);
            break;
        case MTCLI_CMD_RESTART:
            result = mtcli_cmd_restart(&config);
            break;
        case MTCLI_CMD_STOP:
            result = mtcli_cmd_stop(&config);
            break;
        case MTCLI_CMD_HELP:
            result = mtcli_cmd_help(cmd_args ? cmd_args[0] : NULL);
            break;
        case MTCLI_CMD_VERSION:
            result = mtcli_cmd_version();
            break;
        case MTCLI_CMD_ML_STATS:
            result = mtcli_cmd_ml_stats(&config);
            break;
        case MTCLI_CMD_ML_ANOMALY:
            result = mtcli_cmd_ml_anomaly(&config, cmd_args ? cmd_args[0] : NULL);
            break;
        case MTCLI_CMD_ML_PREDICT:
            result = mtcli_cmd_ml_predict(&config, cmd_args ? cmd_args[0] : NULL);
            break;
        case MTCLI_CMD_UNKNOWN:
        default:
            if (argc > 1) {
                fprintf(stderr, "Unknown command: %s\n", argv[1]);
            }
            print_usage();
            if (cmd_args) free(cmd_args);
            return 1;
    }
    
    // Вывод результата
    mtcli_print_result(&result);
    mtcli_result_free(&result);
    
    if (cmd_args) free(cmd_args);
    
    return result.exit_code;
}
