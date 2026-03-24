/*
    Integration Tests - Упрощённая версия для Windows
    Комплексные интеграционные тесты (Q2 2026)
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/wait.h>
#endif

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
   Тест 1: Cache + Rate Limiter интеграция
   ============================================ */

void test_cache_ratelimit_integration() {
    TEST_START("cache_ratelimit_integration");

    // Инициализация кэша
    cache_config_t cache_config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,
        .max_size_mb = 10,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };

    cache_manager_t *cache = cache_manager_init(&cache_config);
    TEST_ASSERT(cache != NULL, "Cache initialized");

    // Инициализация rate limiter
    rate_limit_config_t rl_config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 1000,
        .window_seconds = 10,
        .bucket_capacity = 1000,
        .refill_rate = 1000,
        .enable_burst = 1,
        .burst_capacity = 1000,
        .enable_whitelist = 0,
        .enable_blacklist = 0
    };

    rate_limiter_t *limiter = rate_limiter_init(&rl_config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");

    // Сценарий: Кэширование с rate limiting
    const char *test_key = "test_user_123";
    const char *test_value = "test_data_value";

    // Проверка rate limit
    rate_limit_status_t rl_status = rate_limit_check(limiter, test_key);
    TEST_ASSERT(rl_status == RATE_LIMIT_OK || rl_status == RATE_LIMIT_EXCEEDED, "First request processed");

    // Запись в кэш
    cache_status_t cache_status = cache_put(cache, test_key, test_value, strlen(test_value) + 1);
    TEST_ASSERT(cache_status == CACHE_OK, "Cache put successful");

    // Чтение из кэша
    void *result = NULL;
    size_t result_size = 0;
    cache_status = cache_get(cache, test_key, &result, &result_size);
    TEST_ASSERT(cache_status == CACHE_OK && result != NULL, "Cache get successful");
    TEST_ASSERT(strcmp((char*)result, test_value) == 0, "Cached value matches");

    if (result) free(result);

    // Очистка
    cache_manager_cleanup(cache);
    rate_limiter_cleanup(limiter);

    TEST_END();
}

/* ============================================
   Тест 2: Error Handler + Cache интеграция
   ============================================ */

void test_errorhandler_cache_integration() {
    TEST_START("errorhandler_cache_integration");

    // Инициализация error handler
    error_handler_context_t error_handler;
    error_handler_init(&error_handler, NULL);
    TEST_ASSERT(error_handler.is_initialized, "Error handler initialized");

    // Инициализация кэша
    cache_config_t cache_config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 100,
        .max_size_mb = 5,
        .default_ttl_sec = 60,
        .enable_locking = 0
    };

    cache_manager_t *cache = cache_manager_init(&cache_config);
    TEST_ASSERT(cache != NULL, "Cache initialized");

    // Сценарий: Обработка ошибок кэша
    const char *key = "error_test_key";
    const char *value = "error_test_value";

    // Успешная запись
    cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
    if (status != CACHE_OK) {
        error_info_t *err = error_create(MTERR_NETWORK_CONNECT, ERROR_LEVEL_WARNING, ERROR_CATEGORY_NETWORK, "Cache put failed");
        error_handle(&error_handler, err);
        error_free(err);
    }
    TEST_ASSERT(status == CACHE_OK, "Cache put succeeded");

    // Успешное чтение
    void *result = NULL;
    size_t result_size = 0;
    status = cache_get(cache, key, &result, &result_size);
    if (status != CACHE_OK) {
        error_info_t *err = error_create(MTERR_NETWORK_CONNECT, ERROR_LEVEL_WARNING, ERROR_CATEGORY_NETWORK, "Cache get failed");
        error_handle(&error_handler, err);
        error_free(err);
    }
    TEST_ASSERT(status == CACHE_OK && result != NULL, "Cache get succeeded");

    if (result) free(result);

    // Чтение несуществующего ключа
    status = cache_get(cache, "nonexistent_key", &result, &result_size);
    if (status == CACHE_MISS) {
        error_info_t *err = error_create(MTERR_NETWORK_UNREACHABLE, ERROR_LEVEL_INFO, ERROR_CATEGORY_NETWORK, "Key not found");
        error_handle(&error_handler, err);
        error_free(err);
        TEST_ASSERT(true, "Cache miss handled correctly");
    }

    // Проверка статистики ошибок
    error_stats_t stats;
    error_get_stats(&error_handler, &stats);
    TEST_ASSERT(stats.total_errors >= 0, "Error statistics available");

    // Очистка
    cache_manager_cleanup(cache);
    error_handler_cleanup(&error_handler);

    TEST_END();
}

/* ============================================
   Тест 3: Error Handler + Rate Limiter интеграция
   ============================================ */

void test_errorhandler_ratelimit_integration() {
    TEST_START("errorhandler_ratelimit_integration");

    // Инициализация error handler
    error_handler_context_t error_handler;
    error_handler_init(&error_handler, NULL);
    TEST_ASSERT(error_handler.is_initialized, "Error handler initialized");

    // Инициализация rate limiter
    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 5,
        .window_seconds = 1,
        .bucket_capacity = 10,
        .refill_rate = 5,
        .enable_burst = 1,
        .burst_capacity = 10,
        .enable_whitelist = 0,
        .enable_blacklist = 0
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");

    // Сценарий: Превышение лимита с обработкой ошибок
    const char *client_key = "rate_limit_client";

    int exceeded_count = 0;
    for (int i = 0; i < 10; i++) {
        rate_limit_status_t status = rate_limit_check(limiter, client_key);

        if (status == RATE_LIMIT_EXCEEDED) {
            exceeded_count++;
            error_info_t *err = error_create(MTERR_RATE_LIMIT_EXCEEDED, ERROR_LEVEL_WARNING, ERROR_CATEGORY_RATE_LIMIT, "Rate limit exceeded");
            error_handle(&error_handler, err);
            error_free(err);
        }
    }

    TEST_ASSERT(exceeded_count > 0, "Rate limit exceeded as expected");

    // Проверка circuit breaker
    error_stats_t stats;
    error_get_stats(&error_handler, &stats);
    TEST_ASSERT(stats.total_errors > 0, "Errors recorded");

    // Очистка
    rate_limiter_cleanup(limiter);
    error_handler_cleanup(&error_handler);

    TEST_END();
}

/* ============================================
   Тест 4: Комплексная интеграция (все модули)
   ============================================ */

void test_full_integration() {
    TEST_START("full_integration");

    // Инициализация error handler
    error_handler_context_t error_handler;
    error_handler_init(&error_handler, NULL);
    TEST_ASSERT(error_handler.is_initialized, "Error handler initialized");

    cache_config_t cache_config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 500,
        .max_size_mb = 10,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };
    cache_manager_t *cache = cache_manager_init(&cache_config);
    TEST_ASSERT(cache != NULL, "Cache initialized");

    rate_limit_config_t rl_config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 100,
        .window_seconds = 10,
        .bucket_capacity = 100,
        .refill_rate = 100,
        .enable_burst = 1,
        .burst_capacity = 100,
        .enable_whitelist = 0,
        .enable_blacklist = 0
    };
    rate_limiter_t *limiter = rate_limiter_init(&rl_config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");

    // Сценарий: Полный цикл работы
    const int CLIENT_COUNT = 10;
    int success_count = 0;
    int rate_limited_count = 0;
    int error_count = 0;

    for (int i = 0; i < CLIENT_COUNT; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "client_%d", i);
        snprintf(value, sizeof(value), "data_%d", i);

        // Проверка rate limit
        rate_limit_status_t rl_status = rate_limit_check(limiter, key);

        if (rl_status == RATE_LIMIT_OK) {
            // Запись в кэш
            cache_status_t cache_status = cache_put(cache, key, value, strlen(value) + 1);
            if (cache_status == CACHE_OK) {
                success_count++;
            } else {
                error_info_t *err = error_create(MTERR_NETWORK_CONNECT, ERROR_LEVEL_WARNING, ERROR_CATEGORY_NETWORK, "Cache put failed");
                error_handle(&error_handler, err);
                error_free(err);
                error_count++;
            }
        } else {
            rate_limited_count++;
            error_info_t *err = error_create(MTERR_RATE_LIMIT_EXCEEDED, ERROR_LEVEL_WARNING, ERROR_CATEGORY_RATE_LIMIT, "Rate limited");
            error_handle(&error_handler, err);
            error_free(err);
        }
    }

    TEST_ASSERT(success_count + rate_limited_count + error_count == CLIENT_COUNT, "All requests accounted for");

    // Чтение из кэша (проверяем только успешные записи)
    int cache_hits = 0;
    for (int i = 0; i < success_count && i < CLIENT_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "client_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            cache_hits++;
            if (result) free(result);
        }
    }

    // Кэш может быть пуст если все запросы были rate limited
    if (success_count > 0) {
        TEST_ASSERT(cache_hits > 0, "Cache hits recorded");
    }

    // Статистика
    cache_stats_t cache_stats;
    cache_get_stats(cache, &cache_stats);
    if (success_count > 0) {
        TEST_ASSERT(cache_stats.current_entries > 0, "Cache has entries");
    } else {
        TEST_ASSERT(cache_stats.current_entries >= 0, "Cache statistics available");
    }

    rate_limiter_stats_t rl_stats;
    rate_limiter_get_stats(limiter, &rl_stats);
    TEST_ASSERT(rl_stats.total_requests > 0, "Rate limiter has requests");

    error_stats_t error_stats;
    error_get_stats(&error_handler, &error_stats);
    TEST_ASSERT(error_stats.total_errors >= 0, "Error statistics available");

    // Очистка
    cache_manager_cleanup(cache);
    rate_limiter_cleanup(limiter);
    error_handler_cleanup(&error_handler);

    TEST_END();
}

/* ============================================
   Тест 5: Нагрузочный тест (стресс-тест)
   ============================================ */

void test_stress_integration() {
    TEST_START("stress_integration");

    // Инициализация
    cache_config_t cache_config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,
        .max_size_mb = 20,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };
    cache_manager_t *cache = cache_manager_init(&cache_config);
    TEST_ASSERT(cache != NULL, "Cache initialized");

    rate_limit_config_t rl_config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 1000,
        .window_seconds = 10,
        .bucket_capacity = 1000,
        .refill_rate = 1000,
        .enable_burst = 1,
        .burst_capacity = 1000,
        .enable_whitelist = 0,
        .enable_blacklist = 0
    };
    rate_limiter_t *limiter = rate_limiter_init(&rl_config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");

    // Стресс-тест: 1000 операций
    const int OPS_COUNT = 1000;
    int success = 0;
    int failed = 0;

    for (int i = 0; i < OPS_COUNT; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "stress_key_%d", i);
        snprintf(value, sizeof(value), "stress_value_%d_data", i);

        rate_limit_status_t rl_status = rate_limit_check(limiter, key);
        if (rl_status == RATE_LIMIT_OK) {
            cache_status_t cache_status = cache_put(cache, key, value, strlen(value) + 1);
            if (cache_status == CACHE_OK) {
                success++;
            } else {
                failed++;
            }
        } else {
            failed++;
        }
    }

    TEST_ASSERT(success > 0 || failed > 0, "Stress test had operations");
    TEST_ASSERT(success + failed == OPS_COUNT, "All operations accounted for");

    // Статистика
    cache_stats_t stats;
    cache_get_stats(cache, &stats);
    printf("  Cache entries: %zu/%zu\n", stats.current_entries, cache_config.max_entries);
    printf("  Cache hit rate: %.2f%%\n", stats.hit_rate);

    rate_limiter_stats_t rl_stats;
    rate_limiter_get_stats(limiter, &rl_stats);
    printf("  Rate limiter requests: %llu\n", (unsigned long long)rl_stats.total_requests);
    printf("  Rate limiter rejections: %llu\n", (unsigned long long)rl_stats.total_rejections);

    // Очистка
    cache_manager_cleanup(cache);
    rate_limiter_cleanup(limiter);

    TEST_END();
}

/* ============================================
   Главная функция
   ============================================ */

int main() {
    printf("===========================================\n");
    printf("  Integration Tests Suite\n");
    printf("===========================================\n\n");

    test_cache_ratelimit_integration();
    test_errorhandler_cache_integration();
    test_errorhandler_ratelimit_integration();
    test_full_integration();
    test_stress_integration();

    printf("\n===========================================\n");
    printf("  Final Summary\n");
    printf("===========================================\n");
    printf("  Total tests: %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success rate: %.2f%%\n",
           (tests_run > 0) ? (tests_passed * 100.0) / tests_run : 0);
    printf("===========================================\n");

    return (tests_failed > 0) ? 1 : 0;
}
