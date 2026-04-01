/*
    MTProxy CLI Tests
    Тесты для CLI утилиты
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cli/mtcli.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации конфигурации
// ============================================================================

static int test_mtcli_config_init(void) {
    TEST_START();
    
    mtcli_config_t config;
    mtcli_config_init(&config);
    
    ASSERT(strcmp(config.host, "localhost") == 0, "Host должен быть localhost");
    ASSERT(config.rest_port == 8080, "REST порт должен быть 8080");
    ASSERT(config.grpc_port == 50051, "gRPC порт должен быть 50051");
    ASSERT(config.mode == MTCLI_MODE_REST, "Режим по умолчанию REST");
    ASSERT(config.verbose == false, "Verbose должен быть выключен");
    ASSERT(config.json_output == false, "JSON вывод должен быть выключен");
    ASSERT(config.timeout_ms == 5000, "Таймаут должен быть 5000мс");
    
    TEST_END();
}

static int test_mtcli_config_init_null(void) {
    TEST_START();
    
    // Проверка на NULL
    mtcli_config_init(NULL);  // Не должно крашиться
    
    TEST_END();
}

// ============================================================================
// Тесты парсинга команд
// ============================================================================

static int test_mtcli_parse_command(void) {
    TEST_START();
    
    ASSERT(mtcli_parse_command("status") == MTCLI_CMD_STATUS, "status");
    ASSERT(mtcli_parse_command("stats") == MTCLI_CMD_STATS, "stats");
    ASSERT(mtcli_parse_command("config") == MTCLI_CMD_CONFIG, "config");
    ASSERT(mtcli_parse_command("secrets") == MTCLI_CMD_SECRETS, "secrets");
    ASSERT(mtcli_parse_command("logs") == MTCLI_CMD_LOGS, "logs");
    ASSERT(mtcli_parse_command("connections") == MTCLI_CMD_CONNECTIONS, "connections");
    ASSERT(mtcli_parse_command("ratelimit") == MTCLI_CMD_RATELIMIT, "ratelimit");
    ASSERT(mtcli_parse_command("health") == MTCLI_CMD_HEALTH, "health");
    ASSERT(mtcli_parse_command("metrics") == MTCLI_CMD_METRICS, "metrics");
    ASSERT(mtcli_parse_command("reload") == MTCLI_CMD_RELOAD, "reload");
    ASSERT(mtcli_parse_command("restart") == MTCLI_CMD_RESTART, "restart");
    ASSERT(mtcli_parse_command("stop") == MTCLI_CMD_STOP, "stop");
    ASSERT(mtcli_parse_command("help") == MTCLI_CMD_HELP, "help");
    ASSERT(mtcli_parse_command("version") == MTCLI_CMD_VERSION, "version");
    ASSERT(mtcli_parse_command("unknown") == MTCLI_CMD_UNKNOWN, "unknown команда");
    ASSERT(mtcli_parse_command(NULL) == MTCLI_CMD_UNKNOWN, "NULL команда");
    ASSERT(mtcli_parse_command("") == MTCLI_CMD_UNKNOWN, "Пустая команда");
    
    TEST_END();
}

// ============================================================================
// Тесты парсинга аргументов
// ============================================================================

static int test_mtcli_parse_args_basic(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "status", "--host", "192.168.1.100", "--port", "9090"};
    int argc = 6;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_STATUS, "Команда status");
    ASSERT(strcmp(config.host, "192.168.1.100") == 0, "Host установлен");
    ASSERT(config.rest_port == 9090, "Порт установлен");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

static int test_mtcli_parse_args_json(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "stats", "--json", "--verbose"};
    int argc = 4;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_STATS, "Команда stats");
    ASSERT(config.json_output == true, "JSON вывод включен");
    ASSERT(config.verbose == true, "Verbose включен");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

static int test_mtcli_parse_args_mode(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "status", "--mode", "grpc", "--grpc-port", "6000"};
    int argc = 6;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_STATUS, "Команда status");
    ASSERT(config.mode == MTCLI_MODE_GRPC, "Режим gRPC");
    ASSERT(config.grpc_port == 6000, "gRPC порт установлен");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

static int test_mtcli_parse_args_api_key(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "config", "get", "--api-key", "test-key-123"};
    int argc = 6;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_CONFIG, "Команда config");
    ASSERT(strcmp(config.api_key, "test-key-123") == 0, "API ключ установлен");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

static int test_mtcli_parse_args_help(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "--help"};
    int argc = 2;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_HELP, "Команда help");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

static int test_mtcli_parse_args_version(void) {
    TEST_START();
    
    char *argv[] = {"mtproxy-cli", "--version"};
    int argc = 2;
    
    mtcli_config_t config;
    mtcli_command_t cmd;
    char **cmd_args = NULL;
    
    int ret = mtcli_parse_args(argc, argv, &config, &cmd, &cmd_args);
    
    ASSERT(ret == 0, "Парсинг успешен");
    ASSERT(cmd == MTCLI_CMD_VERSION, "Команда version");
    
    if (cmd_args) free(cmd_args);
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_mtcli_format_bytes(void) {
    TEST_START();
    
    char buffer[64];
    
    mtcli_format_bytes(0, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "0.00 B") == 0, "0 байт");
    
    mtcli_format_bytes(1024, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1.00 KB") == 0, "1 KB");
    
    mtcli_format_bytes(1048576, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1.00 MB") == 0, "1 MB");
    
    mtcli_format_bytes(1073741824, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1.00 GB") == 0, "1 GB");
    
    TEST_END();
}

static int test_mtcli_format_uptime(void) {
    TEST_START();
    
    char buffer[64];
    
    mtcli_format_uptime(1000, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1s") == 0, "1 секунда");
    
    mtcli_format_uptime(61000, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1m 1s") == 0, "1 минута 1 секунда");
    
    mtcli_format_uptime(3661000, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1h 1m 1s") == 0, "1 час 1 минута 1 секунда");
    
    mtcli_format_uptime(90061000, buffer, sizeof(buffer));
    ASSERT(strcmp(buffer, "1d 1h 1m 1s") == 0, "1 день 1 час 1 минута 1 секунда");
    
    TEST_END();
}

static int test_mtcli_result_free(void) {
    TEST_START();
    
    mtcli_result_t result = {0};
    result.output = strdup("test output");
    result.output_size = 11;
    
    mtcli_result_free(&result);
    
    ASSERT(result.output == NULL, "output освобождён");
    
    // Проверка на NULL
    mtcli_result_free(NULL);
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy CLI Tests");
    
    // Конфигурация
    test_run(test_mtcli_config_init);
    test_run(test_mtcli_config_init_null);
    
    // Парсинг команд
    test_run(test_mtcli_parse_command);
    
    // Парсинг аргументов
    test_run(test_mtcli_parse_args_basic);
    test_run(test_mtcli_parse_args_json);
    test_run(test_mtcli_parse_args_mode);
    test_run(test_mtcli_parse_args_api_key);
    test_run(test_mtcli_parse_args_help);
    test_run(test_mtcli_parse_args_version);
    
    // Утилиты
    test_run(test_mtcli_format_bytes);
    test_run(test_mtcli_format_uptime);
    test_run(test_mtcli_result_free);
    
    return test_finish();
}
