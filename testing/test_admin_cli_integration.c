/*
    Integration Tests for MTProxy Admin CLI
    Интеграционные тесты для утилиты администрирования
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#include "admin/admin-cli.h"
#include "common/config-manager.h"
#include "common/cache-manager.h"
#include "common/rate-limiter.h"
#include "common/error-handler.h"

// Статистика тестов
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Макросы для тестирования
#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAILED: %s\n", message); \
        } \
    } while(0)

#define TEST_START(name) \
    printf("\n=== Test: %s ===\n", name)

#define TEST_END() \
    printf("Completed: %d run, %d passed, %d failed\n", \
           tests_run, tests_passed, tests_failed)

/* ============================================
   Тесты интеграции CLI с модулями
   ============================================ */

void test_cli_command_parsing() {
    TEST_START("cli_command_parsing");
    
    int argc = 0;
    char **argv = NULL;
    
    // Тест 1: простая команда
    argv = admin_cli_tokenize("status", &argc);
    TEST_ASSERT(argc == 1, "Single command parsed");
    TEST_ASSERT(strcmp(argv[0], "status") == 0, "Command name matches");
    if (argv) admin_cli_free_tokens(argv, argc);
    
    // Тест 2: команда с аргументами
    argv = admin_cli_tokenize("config show section", &argc);
    TEST_ASSERT(argc == 3, "Command with args parsed");
    TEST_ASSERT(strcmp(argv[0], "config") == 0, "First arg matches");
    TEST_ASSERT(strcmp(argv[1], "show") == 0, "Second arg matches");
    TEST_ASSERT(strcmp(argv[2], "section") == 0, "Third arg matches");
    if (argv) admin_cli_free_tokens(argv, argc);
    
    // Тест 3: команда с несколькими пробелами
    argv = admin_cli_tokenize("cache-get   key_name", &argc);
    TEST_ASSERT(argc == 2, "Command with multiple spaces parsed");
    TEST_ASSERT(strcmp(argv[0], "cache-get") == 0, "First arg matches");
    TEST_ASSERT(strcmp(argv[1], "key_name") == 0, "Second arg matches");
    if (argv) admin_cli_free_tokens(argv, argc);
    
    // Тест 4: пустая строка
    argv = admin_cli_tokenize("", &argc);
    TEST_ASSERT(argc == 0, "Empty string parsed");
    if (argv) admin_cli_free_tokens(argv, argc);
    
    // Тест 5: NULL проверка
    argv = admin_cli_tokenize(NULL, &argc);
    TEST_ASSERT(argv == NULL, "NULL input returns NULL");
    
    TEST_END();
}

void test_cli_command_lookup() {
    TEST_START("cli_command_lookup");
    
    // Тест известных команд
    admin_command_t cmd = admin_cli_parse_command("help");
    TEST_ASSERT(cmd == CMD_HELP, "help command recognized");
    
    cmd = admin_cli_parse_command("status");
    TEST_ASSERT(cmd == CMD_STATUS, "status command recognized");
    
    cmd = admin_cli_parse_command("stats");
    TEST_ASSERT(cmd == CMD_STATS, "stats command recognized");
    
    cmd = admin_cli_parse_command("config");
    TEST_ASSERT(cmd == CMD_CONFIG_SHOW, "config command recognized");
    
    cmd = admin_cli_parse_command("config-set");
    TEST_ASSERT(cmd == CMD_CONFIG_SET, "config-set command recognized");
    
    cmd = admin_cli_parse_command("cache-stats");
    TEST_ASSERT(cmd == CMD_CACHE_STATS, "cache-stats command recognized");
    
    cmd = admin_cli_parse_command("ratelimit");
    TEST_ASSERT(cmd == CMD_RATELIMIT_STATUS, "ratelimit command recognized");
    
    cmd = admin_cli_parse_command("connections");
    TEST_ASSERT(cmd == CMD_CONNECTIONS_LIST, "connections command recognized");
    
    cmd = admin_cli_parse_command("health");
    TEST_ASSERT(cmd == CMD_HEALTH, "health command recognized");
    
    // Тест неизвестной команды
    cmd = admin_cli_parse_command("unknown-command");
    TEST_ASSERT(cmd == CMD_UNKNOWN, "unknown command returns CMD_UNKNOWN");
    
    TEST_END();
}

void test_config_manager_integration() {
    TEST_START("config_manager_integration");
    
    config_manager_init(NULL);
    
    // Тест 1: установка значения
    config_value_t val;
    val.type = CONFIG_INT;
    val.value.int_val = 42;
    
    int result = config_manager_set("test.int_value", &val, 0);
    TEST_ASSERT(result == 0, "Config value set successfully");
    
    // Тест 2: получение значения
    config_value_t *retrieved = config_manager_get("test.int_value");
    TEST_ASSERT(retrieved != NULL, "Config value retrieved");
    TEST_ASSERT(retrieved->type == CONFIG_INT, "Config type matches");
    TEST_ASSERT(retrieved->value.int_val == 42, "Config value matches");
    
    // Тест 3: установка строкового значения
    config_value_t str_val;
    str_val.type = CONFIG_STRING;
    str_val.value.string_val = strdup("test_string");
    
    result = config_manager_set("test.string_value", &str_val, 0);
    TEST_ASSERT(result == 0, "String config value set successfully");
    
    // Тест 4: получение строкового значения
    retrieved = config_manager_get("test.string_value");
    TEST_ASSERT(retrieved != NULL, "String config value retrieved");
    TEST_ASSERT(retrieved->type == CONFIG_STRING, "String config type matches");
    TEST_ASSERT(strcmp(retrieved->value.string_val, "test_string") == 0, "String value matches");
    
    // Тест 5: несуществующий ключ
    retrieved = config_manager_get("nonexistent.key");
    TEST_ASSERT(retrieved == NULL, "Nonexistent key returns NULL");
    
    // Очистка
    config_manager_cleanup();
    if (str_val.value.string_val) free(str_val.value.string_val);
    
    TEST_END();
}

void test_cache_manager_integration() {
    TEST_START("cache_manager_integration");
    
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 100,
        .max_size_mb = 10,
        .default_ttl_sec = 60,
        .enable_locking = 0,
        .enable_partitioning = 0
    };
    
    cache_manager_t *cache = cache_manager_init(&config);
    TEST_ASSERT(cache != NULL, "Cache initialized");
    if (!cache) {
        TEST_END();
        return;
    }
    
    // Тест 1: put/get
    const char *key = "cli:test_key";
    const char *value = "test_value_data";
    
    cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
    TEST_ASSERT(status == CACHE_OK, "Cache put successful");
    
    void *result = NULL;
    size_t result_size = 0;
    status = cache_get(cache, key, &result, &result_size);
    TEST_ASSERT(status == CACHE_OK, "Cache get successful");
    TEST_ASSERT(result != NULL, "Result not NULL");
    TEST_ASSERT(strcmp((char*)result, value) == 0, "Value matches");
    
    if (result) free(result);
    
    // Тест 2: delete
    status = cache_delete(cache, key);
    TEST_ASSERT(status == CACHE_OK, "Cache delete successful");
    
    // Тест 3: get после delete
    result = NULL;
    status = cache_get(cache, key, &result, &result_size);
    TEST_ASSERT(status != CACHE_OK, "Cache get after delete fails");
    if (result) free(result);
    
    cache_manager_cleanup(cache);
    
    TEST_END();
}

void test_rate_limiter_integration() {
    TEST_START("rate_limiter_integration");
    
    rate_limiter_config_t config = {
        .algorithm = RATE_LIMITER_TOKEN_BUCKET,
        .default_max_requests = 10,
        .default_window_sec = 60,
        .enable_whitelist = 1,
        .enable_blacklist = 1
    };
    
    rate_limiter_t *limiter = rate_limiter_init(&config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");
    if (!limiter) {
        TEST_END();
        return;
    }
    
    const char *client_key = "cli:test_client";
    
    // Тест 1: разрешение запроса
    rate_limit_result_t result = rate_limiter_check(limiter, client_key);
    TEST_ASSERT(result == RATE_LIMIT_ALLOW, "First request allowed");
    
    // Тест 2: добавление в whitelist
    int status = rate_limiter_whitelist_add(limiter, client_key);
    TEST_ASSERT(status == 0, "Client added to whitelist");
    
    result = rate_limiter_check(limiter, client_key);
    TEST_ASSERT(result == RATE_LIMIT_ALLOW, "Whitelisted client allowed");
    
    // Тест 3: удаление из whitelist
    status = rate_limiter_whitelist_remove(limiter, client_key);
    TEST_ASSERT(status == 0, "Client removed from whitelist");
    
    // Тест 4: добавление в blacklist
    status = rate_limiter_blacklist_add(limiter, client_key);
    TEST_ASSERT(status == 0, "Client added to blacklist");
    
    result = rate_limiter_check(limiter, client_key);
    TEST_ASSERT(result == RATE_LIMIT_DENY, "Blacklisted client denied");
    
    // Тест 5: удаление из blacklist
    status = rate_limiter_blacklist_remove(limiter, client_key);
    TEST_ASSERT(status == 0, "Client removed from blacklist");
    
    rate_limiter_cleanup(limiter);
    
    TEST_END();
}

void test_error_handler_integration() {
    TEST_START("error_handler_integration");
    
    error_handler_config_t config = {
        .enable_circuit_breaker = 1,
        .circuit_breaker_threshold = 5,
        .circuit_breaker_timeout_sec = 30,
        .enable_retry = 1,
        .max_retries = 3,
        .retry_delay_ms = 100
    };
    
    error_handler_t *handler = error_handler_init(&config);
    TEST_ASSERT(handler != NULL, "Error handler initialized");
    if (!handler) {
        TEST_END();
        return;
    }
    
    // Тест 1: регистрация ошибки
    error_t *err = error_create(ERROR_CONNECTION, "Test connection error", NULL);
    TEST_ASSERT(err != NULL, "Error created");
    
    error_register(handler, err);
    TEST_ASSERT(handler->stats.total_errors > 0, "Error registered in stats");
    
    // Тест 2: проверка категории
    TEST_ASSERT(err->category == ERROR_CONNECTION, "Error category matches");
    
    // Тест 3: получение последних ошибок
    error_t **recent = error_get_recent(handler, 5);
    TEST_ASSERT(recent != NULL, "Recent errors retrieved");
    
    free(recent);
    error_free(err);
    
    // Тест 4: circuit breaker
    TEST_ASSERT(error_circuit_breaker_is_open(handler) == 0, "Circuit breaker initially closed");
    
    // Тест 5: очистка
    error_handler_cleanup(handler);
    
    TEST_END();
}

/* ============================================
   Main
   ============================================ */

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("  MTProxy Admin CLI Integration Tests\n");
    printf("===========================================\n");
    
    // CLI Command Parsing Tests
    printf("\n--- CLI Command Parsing Tests ---\n");
    test_cli_command_parsing();
    
    // CLI Command Lookup Tests
    printf("\n--- CLI Command Lookup Tests ---\n");
    test_cli_command_lookup();
    
    // Config Manager Integration Tests
    printf("\n--- Config Manager Integration Tests ---\n");
    test_config_manager_integration();
    
    // Cache Manager Integration Tests
    printf("\n--- Cache Manager Integration Tests ---\n");
    test_cache_manager_integration();
    
    // Rate Limiter Integration Tests
    printf("\n--- Rate Limiter Integration Tests ---\n");
    test_rate_limiter_integration();
    
    // Error Handler Integration Tests
    printf("\n--- Error Handler Integration Tests ---\n");
    test_error_handler_integration();
    
    // Summary
    printf("\n===========================================\n");
    printf("  Test Summary\n");
    printf("===========================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
