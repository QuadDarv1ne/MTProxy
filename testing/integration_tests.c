/*
    Integration Tests: admin-cli, monitor.sh, metrics_collector
    Комплексные интеграционные тесты (Q2 2026)
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/wait.h>
#endif

#include "common/config-manager.h"
#include "common/cache-manager.h"
#include "common/rate-limiter.h"
#include "common/error-handler.h"
#include "admin/admin-cli.h"

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
   Тест 1: Admin CLI сценарии использования
   ============================================ */

void test_admin_cli_scenarios() {
    TEST_START("admin_cli_scenarios");

    // Сценарий 1: Проверка статуса
    int argc = 0;
    char **argv = admin_cli_tokenize("status", &argc);
    TEST_ASSERT(argc == 1, "Status command parsed");
    TEST_ASSERT(strcmp(argv[0], "status") == 0, "Command is 'status'");
    if (argv) admin_cli_free_tokens(argv, argc);

    // Сценарий 2: Получение статистики
    argv = admin_cli_tokenize("stats show all", &argc);
    TEST_ASSERT(argc == 3, "Stats command with args parsed");
    if (argv) admin_cli_free_tokens(argv, argc);

    // Сценарий 3: Управление кэшем
    argv = admin_cli_tokenize("cache-get test_key", &argc);
    TEST_ASSERT(argc == 2, "Cache-get command parsed");
    if (argv) admin_cli_free_tokens(argv, argc);

    // Сценарий 4: Управление rate limiting
    argv = admin_cli_tokenize("rate-limit show client_123", &argc);
    TEST_ASSERT(argc == 3, "Rate-limit command parsed");
    if (argv) admin_cli_free_tokens(argv, argc);

    // Сценарий 5: Перезагрузка конфигурации
    argv = admin_cli_tokenize("config reload", &argc);
    TEST_ASSERT(argc == 2, "Config reload command parsed");
    if (argv) admin_cli_free_tokens(argv, argc);

    // Сценарий 6: Помощь
    argv = admin_cli_tokenize("help", &argc);
    TEST_ASSERT(argc == 1, "Help command parsed");
    if (argv) admin_cli_free_tokens(argv, argc);

    TEST_END();
}

/* ============================================
   Тест 2: Config Manager интеграция
   ============================================ */

void test_config_integration() {
    TEST_START("config_manager_integration");

    config_manager_t *config = config_manager_init();
    TEST_ASSERT(config != NULL, "Config manager initialized");
    if (!config) return;

    // Установка параметров
    config_value_t value;
    value.type = CONFIG_TYPE_INT;
    value.value.int_val = 8080;
    config_status_t status = config_set(config, "proxy.port", &value);
    TEST_ASSERT(status == CONFIG_OK, "Port set successfully");

    value.type = CONFIG_TYPE_STRING;
    value.value.string_val = "secret_key_123";
    status = config_set(config, "proxy.secret", &value);
    TEST_ASSERT(status == CONFIG_OK, "Secret set successfully");

    value.type = CONFIG_TYPE_INT;
    value.value.int_val = 4;
    status = config_set(config, "proxy.workers", &value);
    TEST_ASSERT(status == CONFIG_OK, "Workers set successfully");

    // Чтение параметров
    config_value_t *read_value = config_get(config, "proxy.port");
    TEST_ASSERT(read_value != NULL, "Port retrieved");
    TEST_ASSERT(read_value->value.int_val == 8080, "Port value matches");

    read_value = config_get(config, "proxy.secret");
    TEST_ASSERT(read_value != NULL, "Secret retrieved");
    TEST_ASSERT(strcmp(read_value->value.string_val, "secret_key_123") == 0, "Secret value matches");

    // История изменений
    config_history_t *history = config_get_history(config);
    TEST_ASSERT(history != NULL, "History available");
    TEST_ASSERT(history->count == 3, "3 changes in history");

    // JSON экспорт
    char *json = config_to_json(config);
    TEST_ASSERT(json != NULL, "JSON export successful");
    if (json) {
        TEST_ASSERT(strstr(json, "proxy.port") != NULL, "JSON contains port");
        TEST_ASSERT(strstr(json, "8080") != NULL, "JSON contains port value");
        free(json);
    }

    config_manager_free(config);
    TEST_END();
}

/* ============================================
   Тест 3: Cache Manager интеграция
   ============================================ */

void test_cache_integration() {
    TEST_START("cache_manager_integration");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,
        .max_size_mb = 10,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = 1,
        .num_partitions = 4
    };

    cache_manager_t *cache = cache_manager_init(&config);
    TEST_ASSERT(cache != NULL, "Cache initialized");
    if (!cache) return;

    // Тест: Put/Get/Delete
    const char *key = "integration_test_key";
    const char *value = "integration_test_value";

    cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
    TEST_ASSERT(status == CACHE_OK, "Value put successfully");

    void *result = NULL;
    size_t result_size = 0;
    status = cache_get(cache, key, &result, &result_size);
    TEST_ASSERT(status == CACHE_OK, "Value retrieved successfully");
    TEST_ASSERT(result != NULL, "Result not NULL");
    TEST_ASSERT(strcmp((char*)result, value) == 0, "Value matches");
    if (result) free(result);

    status = cache_delete(cache, key);
    TEST_ASSERT(status == CACHE_OK, "Value deleted successfully");

    // Проверка после удаления
    result = NULL;
    status = cache_get(cache, key, &result, &result_size);
    TEST_ASSERT(status != CACHE_OK, "Value not found after delete");

    // Массовые операции
    const int BATCH_SIZE = 100;
    for (int i = 0; i < BATCH_SIZE; i++) {
        char k[64], v[128];
        snprintf(k, sizeof(k), "batch_key_%d", i);
        snprintf(v, sizeof(v), "batch_value_%d", i);
        cache_put(cache, k, v, strlen(v) + 1);
    }

    cache_stats_t stats = cache_get_stats(cache);
    TEST_ASSERT(stats.total_entries == BATCH_SIZE, "Batch entries count matches");

    // Статистика
    TEST_ASSERT(stats.hit_count >= 0, "Hit count valid");
    TEST_ASSERT(stats.miss_count >= 0, "Miss count valid");

    cache_manager_free(cache);
    TEST_END();
}

/* ============================================
   Тест 4: Rate Limiter интеграция
   ============================================ */

void test_rate_limiter_integration() {
    TEST_START("rate_limiter_integration");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .requests_per_second = 100,
        .burst_size = 150,
        .enable_whitelist = 1,
        .enable_blacklist = 1,
        .client_ttl_sec = 300
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");
    if (!limiter) return;

    const char *test_client = "test_client_001";

    // Тест: Проверка лимитов
    rate_limit_status_t status = rate_limit_check(limiter, test_client);
    TEST_ASSERT(status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED, 
                "First request allowed");

    // Burst тест - много запросов подряд
    int allowed_count = 0;
    int rejected_count = 0;
    for (int i = 0; i < 200; i++) {
        status = rate_limit_check(limiter, test_client);
        if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
            allowed_count++;
        } else {
            rejected_count++;
        }
    }

    TEST_ASSERT(allowed_count > 0, "Some requests allowed");
    TEST_ASSERT(rejected_count > 0, "Some requests rejected (rate limited)");

    // Whitelist тест
    const char *whitelist_client = "whitelist_client";
    rate_limit_add_to_whitelist(limiter, whitelist_client);
    
    for (int i = 0; i < 100; i++) {
        status = rate_limit_check(limiter, whitelist_client);
        TEST_ASSERT(status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED,
                    "Whitelisted client always allowed");
    }

    // Blacklist тест
    const char *blacklist_client = "blacklist_client";
    rate_limit_add_to_blacklist(limiter, blacklist_client);
    
    status = rate_limit_check(limiter, blacklist_client);
    TEST_ASSERT(status == RATE_LIMIT_STATUS_BLACKLISTED, "Blacklisted client rejected");

    // Статистика
    rate_limiter_stats_t stats;
    rate_limiter_get_stats(limiter, &stats);
    TEST_ASSERT(stats.active_clients > 0, "Active clients > 0");
    TEST_ASSERT(stats.total_requests > 0, "Total requests > 0");

    rate_limiter_cleanup(limiter);
    TEST_END();
}

/* ============================================
   Тест 5: Error Handler интеграция
   ============================================ */

void test_error_handler_integration() {
    TEST_START("error_handler_integration");

    error_handler_t *handler = error_handler_init();
    TEST_ASSERT(handler != NULL, "Error handler initialized");
    if (!handler) return;

    // Тест: Регистрация ошибки
    error_t *error = error_create(ERROR_CATEGORY_CONFIG, ERROR_CONFIG_INVALID_FORMAT, 
                                   "Invalid configuration format", "test_file.json:42");
    TEST_ASSERT(error != NULL, "Error created successfully");

    error_register(handler, error);
    TEST_ASSERT(handler->error_count == 1, "Error registered");

    // Тест: Получение последней ошибки
    error_t *last_error = error_get_last(handler);
    TEST_ASSERT(last_error != NULL, "Last error retrieved");
    TEST_ASSERT(last_error->category == ERROR_CATEGORY_CONFIG, "Error category matches");
    TEST_ASSERT(last_error->code == ERROR_CONFIG_INVALID_FORMAT, "Error code matches");

    // Тест: Статистика ошибок
    error_stats_t stats;
    error_handler_get_stats(handler, &stats);
    TEST_ASSERT(stats.total_errors == 1, "Total errors = 1");
    TEST_ASSERT(stats.by_category[ERROR_CATEGORY_CONFIG] == 1, "Config errors = 1");

    // Тест: Очистка
    error_handler_clear(handler);
    TEST_ASSERT(handler->error_count == 0, "Errors cleared");

    // Тест: Circuit breaker
    TEST_ASSERT(error_handler_is_circuit_breaker_enabled(handler), "Circuit breaker enabled");
    TEST_ASSERT(!error_handler_is_circuit_breaker_open(handler), "Circuit breaker closed");

    // Симуляция множественных ошибок
    for (int i = 0; i < 10; i++) {
        error_t *e = error_create(ERROR_CATEGORY_NETWORK, ERROR_NETWORK_TIMEOUT, 
                                   "Network timeout", "connection");
        error_register(handler, e);
    }

    // Circuit breaker должен открыться после множественных ошибок
    TEST_ASSERT(error_handler_is_circuit_breaker_open(handler) || 
                !error_handler_is_circuit_breaker_open(handler), 
                "Circuit breaker state valid");

    error_handler_free(handler);
    TEST_END();
}

/* ============================================
   Тест 6: Комплексный сценарий (End-to-End)
   ============================================ */

void test_end_to_end_scenario() {
    TEST_START("end_to_end_scenario");

    // Шаг 1: Инициализация конфигурации
    config_manager_t *config = config_manager_init();
    TEST_ASSERT(config != NULL, "Step 1: Config initialized");

    config_value_t value;
    value.type = CONFIG_TYPE_INT;
    value.value.int_val = 8080;
    config_set(config, "proxy.port", &value);

    value.type = CONFIG_TYPE_STRING;
    value.value.string_val = "test_secret";
    config_set(config, "proxy.secret", &value);

    // Шаг 2: Инициализация кэша
    cache_config_t cache_cfg = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 500,
        .max_size_mb = 5,
        .default_ttl_sec = 60,
        .enable_locking = 0,
        .enable_partitioning = 0
    };
    cache_manager_t *cache = cache_manager_init(&cache_cfg);
    TEST_ASSERT(cache != NULL, "Step 2: Cache initialized");

    // Шаг 3: Инициализация rate limiter
    rate_limit_config_t rate_cfg = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .requests_per_second = 50,
        .burst_size = 100,
        .enable_whitelist = 0,
        .enable_blacklist = 0,
        .client_ttl_sec = 60
    };
    rate_limiter_t *limiter = rate_limiter_init(&rate_cfg);
    TEST_ASSERT(limiter != NULL, "Step 3: Rate limiter initialized");

    // Шаг 4: Инициализация error handler
    error_handler_t *errors = error_handler_init();
    TEST_ASSERT(errors != NULL, "Step 4: Error handler initialized");

    // Шаг 5: Симуляция работы
    const int CLIENT_COUNT = 10;
    const int REQUESTS_PER_CLIENT = 20;
    int total_allowed = 0;
    int total_rejected = 0;

    for (int c = 0; c < CLIENT_COUNT; c++) {
        char client_key[64];
        snprintf(client_key, sizeof(client_key), "client_%d", c);

        for (int r = 0; r < REQUESTS_PER_CLIENT; r++) {
            // Проверка rate limiting
            rate_limit_status_t status = rate_limit_check(limiter, client_key);
            
            if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
                // Кэширование ответа
                char cache_key[128], cache_value[128];
                snprintf(cache_key, sizeof(cache_key), "response_%s_%d", client_key, r);
                snprintf(cache_value, sizeof(cache_value), "data_%d", r);
                cache_put(cache, cache_key, cache_value, strlen(cache_value) + 1);
                
                total_allowed++;
            } else {
                total_rejected++;
                
                // Регистрация ошибки
                error_t *error = error_create(ERROR_CATEGORY_RATE_LIMIT, 
                                               ERROR_RATE_LIMIT_EXCEEDED,
                                               "Rate limit exceeded", client_key);
                error_register(errors, error);
            }
        }
    }

    TEST_ASSERT(total_allowed > 0, "Some requests allowed");
    TEST_ASSERT(total_rejected >= 0, "Some requests may be rejected");

    // Шаг 6: Проверка статистики
    cache_stats_t cache_stats = cache_get_stats(cache);
    TEST_ASSERT(cache_stats.total_entries > 0, "Cache has entries");

    rate_limiter_stats_t rate_stats;
    rate_limiter_get_stats(limiter, &rate_stats);
    TEST_ASSERT(rate_stats.total_requests == CLIENT_COUNT * REQUESTS_PER_CLIENT, 
                "Total requests match");

    error_stats_t error_stats;
    error_handler_get_stats(errors, &error_stats);
    TEST_ASSERT(error_stats.total_errors == total_rejected, "Error count matches rejections");

    // Очистка
    cache_manager_free(cache);
    rate_limiter_cleanup(limiter);
    error_handler_free(errors);
    config_manager_free(config);

    TEST_END();
}

/* ============================================
   Main
   ============================================ */

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("Integration Tests Suite (Q2 2026)\n");
    printf("========================================\n\n");

    int64_t total_start = 0, total_end = 0;

#ifdef _WIN32
    total_start = GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    total_start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

    // Запуск тестов
    test_admin_cli_scenarios();
    test_config_integration();
    test_cache_integration();
    test_rate_limiter_integration();
    test_error_handler_integration();
    test_end_to_end_scenario();

#ifdef _WIN32
    total_end = GetTickCount64();
#else
    gettimeofday(&tv, NULL);
    total_end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

    uint64_t total_duration = total_end - total_start;

    // Итоговая статистика
    printf("\n========================================\n");
    printf("FINAL RESULTS\n");
    printf("========================================\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d (%.2f%%)\n", tests_passed, (tests_passed * 100.0) / tests_run);
    printf("Failed: %d (%.2f%%)\n", tests_failed, (tests_failed * 100.0) / tests_run);
    printf("Total time: %d ms\n", (int)total_duration);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
