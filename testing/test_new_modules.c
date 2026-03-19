/*
    Тесты для новых модулей MTProxy
    Тестирование: cache-manager, rate-limiter, error-handler, config-manager
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "common/cache-manager.h"
#include "common/rate-limiter.h"
#include "common/error-handler.h"
#include "common/config-manager.h"

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
   Тесты Cache Manager
   ============================================ */

void test_cache_basic() {
    TEST_START("cache_basic");

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
    if (!cache) return;

    // Put/Get
    const char *key = "test_key";
    const char *value = "test_value";

    cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
    TEST_ASSERT(status == CACHE_OK, "Cache put successful");

    // Get
    void *result = NULL;
    size_t result_size = 0;
    status = cache_get(cache, key, &result, &result_size);
    TEST_ASSERT(status == CACHE_OK, "Cache get successful");
    TEST_ASSERT(result != NULL, "Result not NULL");
    TEST_ASSERT(strcmp((char*)result, value) == 0, "Value matches");
    
    if (result) free(result);
    
    // Exists
    TEST_ASSERT(cache_exists(cache, key) == 1, "Key exists");
    TEST_ASSERT(cache_exists(cache, "nonexistent") == 0, "Nonexistent key not found");
    
    // Delete
    status = cache_delete(cache, key);
    TEST_ASSERT(status == CACHE_OK, "Cache delete successful");
    TEST_ASSERT(cache_exists(cache, key) == 0, "Key deleted");

    printf("  Before cleanup...\n");
    cache_manager_cleanup(cache);
    printf("  After cleanup...\n");
    TEST_END();
}

void test_cache_ttl() {
    TEST_START("cache_ttl");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_TTL,
        .max_entries = 100,
        .max_size_mb = 10,
        .default_ttl_sec = 1,  // 1 second TTL
        .enable_locking = 0,
        .enable_partitioning = 0
    };

    cache_manager_t *cache = cache_manager_init(&config);
    TEST_ASSERT(cache != NULL, "Cache initialized");
    if (!cache) return;
    
    const char *key = "ttl_key";
    const char *value = "ttl_value";
    
    cache_put(cache, key, value, strlen(value) + 1);
    TEST_ASSERT(cache_exists(cache, key) == 1, "Key exists initially");
    
    // Wait for TTL to expire
    printf("  Waiting for TTL expiration (2 seconds)...\n");
    #ifdef _WIN32
    Sleep(2000);
    #else
    usleep(2000000);
    #endif
    
    TEST_ASSERT(cache_exists(cache, key) == 0, "Key expired after TTL");
    
    cache_manager_cleanup(cache);
    TEST_END();
}

/* ============================================
   Тесты Rate Limiter
   ============================================ */

void test_ratelimit_basic() {
    TEST_START("ratelimit_basic");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_FIXED_WINDOW,
        .max_requests = 5,
        .window_seconds = 60
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    TEST_ASSERT(limiter != NULL, "Rate limiter initialized");

    const char *client = "test_client";

    // Should allow up to max_requests
    for (int i = 0; i < 5; i++) {
        rate_limit_status_t status = rate_limit_check(limiter, client);
        TEST_ASSERT(status == RATE_LIMIT_OK,
                   i < 5 ? "Request allowed" : "Request blocked");
    }

    // Next request should be blocked
    rate_limit_status_t status = rate_limit_check(limiter, client);
    TEST_ASSERT(status == RATE_LIMIT_EXCEEDED, "Request blocked after limit");

    rate_limiter_cleanup(limiter);
    TEST_END();
}

void test_ratelimit_whitelist() {
    TEST_START("ratelimit_whitelist");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_FIXED_WINDOW,
        .max_requests = 2,
        .window_seconds = 60,
        .enable_whitelist = 1
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);

    const char *whitelisted = "vip_client";
    rate_limit_add_to_whitelist(limiter, whitelisted);

    // Whitelisted client should never be blocked
    for (int i = 0; i < 10; i++) {
        rate_limit_status_t status = rate_limit_check(limiter, whitelisted);
        TEST_ASSERT(status == RATE_LIMIT_OK, "Whitelisted request always allowed");
    }

    rate_limit_remove_from_whitelist(limiter, whitelisted);
    rate_limiter_cleanup(limiter);
    TEST_END();
}

void test_ratelimit_blacklist() {
    TEST_START("ratelimit_blacklist");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_FIXED_WINDOW,
        .max_requests = 100,
        .window_seconds = 60,
        .enable_blacklist = 1
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);

    const char *blocked = "bad_client";
    rate_limit_add_to_blacklist(limiter, blocked);

    // Blacklisted client should always be blocked
    rate_limit_status_t status = rate_limit_check(limiter, blocked);
    TEST_ASSERT(status == RATE_LIMIT_EXCEEDED, "Blacklisted request always blocked");

    rate_limiter_cleanup(limiter);
    TEST_END();
}

/* ============================================
   Тесты Error Handler
   ============================================ */

void test_error_handler_basic() {
    TEST_START("error_handler_basic");
    
    recovery_config_t config = {
        .strategy = RECOVERY_RETRY,
        .max_retries = 3,
        .retry_delay_ms = 100,
        .enable_circuit_breaker = 1,
        .circuit_breaker_threshold = 5
    };
    
    error_handler_context_t ctx;
    int result = error_handler_init(&ctx, &config);
    TEST_ASSERT(result == 0, "Error handler initialized");
    
    // Create and handle error
    error_info_t *error = ERROR_CREATE(MTERR_NETWORK_CONNECT, 
                                       ERROR_LEVEL_ERROR,
                                       ERROR_CATEGORY_NETWORK,
                                       "Connection failed");
    TEST_ASSERT(error != NULL, "Error created");
    
    result = error_handle(&ctx, error);
    TEST_ASSERT(result == 0, "Error handled");
    
    // Check last error
    error_info_t last_error;
    result = error_get_last(&ctx, &last_error);
    TEST_ASSERT(result == 0, "Got last error");
    TEST_ASSERT(last_error.error_code == MTERR_NETWORK_CONNECT, "Error code matches");
    
    error_free(error);
    error_handler_cleanup(&ctx);
    TEST_END();
}

void test_error_circuit_breaker() {
    TEST_START("error_circuit_breaker");
    
    recovery_config_t config = {
        .strategy = RECOVERY_RETRY,
        .max_retries = 1,
        .enable_circuit_breaker = 1,
        .circuit_breaker_threshold = 3,
        .circuit_breaker_timeout_ms = 1000
    };
    
    error_handler_context_t ctx;
    error_handler_init(&ctx, &config);
    
    // Record failures to trip circuit breaker
    for (int i = 0; i < 3; i++) {
        error_circuit_breaker_record_failure(&ctx);
    }
    
    TEST_ASSERT(error_circuit_breaker_can_execute(&ctx) == 0, 
               "Circuit breaker is open");
    
    // Wait for timeout
    printf("  Waiting for circuit breaker timeout...\n");
    #ifdef _WIN32
    Sleep(1100);
    #else
    usleep(1100000);
    #endif
    
    TEST_ASSERT(error_circuit_breaker_can_execute(&ctx) == 1, 
               "Circuit breaker allows test request");
    
    error_handler_cleanup(&ctx);
    TEST_END();
}

/* ============================================
   Тесты Config Manager
   ============================================ */

void test_config_manager_basic() {
    TEST_START("config_manager_basic");
    
    int result = config_manager_init("/tmp/mtproxy_test.conf");
    TEST_ASSERT(result == 0, "Config manager initialized");
    
    // Register parameter
    int max_connections = 1000;
    result = config_manager_register_parameter(
        "network", "max_connections",
        CONFIG_TYPE_INT, &max_connections, sizeof(int),
        1, "1000", "Maximum connections"
    );
    TEST_ASSERT(result == 0, "Parameter registered");
    
    // Get parameter
    int value;
    result = config_manager_get_parameter("network", "max_connections", 
                                         &value, sizeof(int));
    TEST_ASSERT(result == 0, "Parameter retrieved");
    TEST_ASSERT(value == 1000, "Value matches");
    
    // Set parameter
    int new_value = 2000;
    result = config_manager_set_parameter("network", "max_connections",
                                         &new_value, sizeof(int));
    TEST_ASSERT(result == 0, "Parameter updated");
    
    // Verify update
    result = config_manager_get_parameter("network", "max_connections",
                                         &value, sizeof(int));
    TEST_ASSERT(value == 2000, "Updated value matches");
    
    config_manager_cleanup();
    TEST_END();
}

void test_config_manager_json() {
    TEST_START("config_manager_json");
    
    config_manager_init("/tmp/mtproxy_test.conf");
    
    // Register and set parameters
    int port = 8080;
    config_manager_register_parameter("server", "port", CONFIG_TYPE_INT,
                                     &port, sizeof(int), 1, "8080", "Server port");
    
    // Export to JSON
    int result = config_manager_export_to_json("/tmp/mtproxy_test.json", 0);
    TEST_ASSERT(result == 0, "JSON export successful");
    
    // Verify file exists (simple check)
    FILE *f = fopen("/tmp/mtproxy_test.json", "r");
    TEST_ASSERT(f != NULL, "JSON file created");
    if (f) fclose(f);
    
    config_manager_cleanup();
    TEST_END();
}

/* ============================================
   Main
   ============================================ */

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("  MTProxy New Modules Test Suite\n");
    printf("===========================================\n");
    
    // Cache Manager Tests
    printf("\n--- Cache Manager Tests ---\n");
    test_cache_basic();
    test_cache_ttl();
    
    // Rate Limiter Tests
    printf("\n--- Rate Limiter Tests ---\n");
    test_ratelimit_basic();
    test_ratelimit_whitelist();
    test_ratelimit_blacklist();
    
    // Error Handler Tests
    printf("\n--- Error Handler Tests ---\n");
    test_error_handler_basic();
    test_error_circuit_breaker();
    
    // Config Manager Tests
    printf("\n--- Config Manager Tests ---\n");
    test_config_manager_basic();
    test_config_manager_json();
    
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
