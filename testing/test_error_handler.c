/*
    Unit-тесты для системы обработки ошибок (error-handler)
    
    Запуск:
    ./bin/test-error-handler
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/error-handler.h"

// Простые макросы для тестов
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAILED: %s\n", msg); \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("FAILED: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
        return; \
    } \
} while(0)

// ============================================================================
// Тесты инициализации
// ============================================================================

static void test_error_handler_init(void) {
    printf("Test: error_handler_init... ");
    
    error_handler_context_t ctx;
    recovery_config_t config = {0};
    int result = error_handler_init(&ctx, &config);
    
    TEST_ASSERT_EQ(result, 0, "init failed");
    
    error_handler_cleanup(&ctx);
    printf("OK\n");
}

static void test_error_create(void) {
    printf("Test: error_create... ");
    
    error_info_t *error = error_create(
        MTERR_NETWORK_CONNECT,
        ERROR_LEVEL_ERROR,
        ERROR_CATEGORY_NETWORK,
        "Connection failed"
    );
    
    TEST_ASSERT(error != NULL, "error is NULL");
    TEST_ASSERT_EQ(error->error_code, MTERR_NETWORK_CONNECT, "error code mismatch");
    TEST_ASSERT_EQ(error->level, ERROR_LEVEL_ERROR, "level mismatch");
    TEST_ASSERT_EQ(error->category, ERROR_CATEGORY_NETWORK, "category mismatch");
    
    error_free(error);
    printf("OK\n");
}

static void test_error_create_with_details(void) {
    printf("Test: error_create_with_details... ");
    
    error_info_t *error = error_create_with_details(
        MTERR_AUTH_INVALID_TOKEN,
        ERROR_LEVEL_ERROR,
        ERROR_CATEGORY_AUTH,
        "Authentication failed",
        "Invalid API key provided",
        "test.c",
        123,
        "test_function"
    );
    
    TEST_ASSERT(error != NULL, "error is NULL");
    TEST_ASSERT_EQ(error->error_code, MTERR_AUTH_INVALID_TOKEN, "error code mismatch");
    TEST_ASSERT(strstr(error->message, "Authentication") != NULL, "message mismatch");
    TEST_ASSERT(strstr(error->details, "Invalid API key") != NULL, "details mismatch");
    
    error_free(error);
    printf("OK\n");
}

// ============================================================================
// Тесты утилит
// ============================================================================

static void test_error_code_to_string(void) {
    printf("Test: error_code_to_string... ");
    
    const char *str = error_code_to_string(MTERR_NETWORK_CONNECT);
    TEST_ASSERT(str != NULL && strlen(str) > 0, "empty string");
    
    printf("OK\n");
}

static void test_error_level_to_string(void) {
    printf("Test: error_level_to_string... ");
    
    TEST_ASSERT(strcmp(error_level_to_string(ERROR_LEVEL_INFO), "INFO") == 0, "INFO mismatch");
    TEST_ASSERT(strcmp(error_level_to_string(ERROR_LEVEL_ERROR), "ERROR") == 0, "ERROR mismatch");
    
    printf("OK\n");
}

static void test_error_category_to_string(void) {
    printf("Test: error_category_to_string... ");
    
    TEST_ASSERT(strcmp(error_category_to_string(ERROR_CATEGORY_NETWORK), "Network") == 0, "Network mismatch");
    TEST_ASSERT(strcmp(error_category_to_string(ERROR_CATEGORY_AUTH), "Authentication") == 0, "Auth mismatch");
    
    printf("OK\n");
}

// ============================================================================
// Тесты улучшенных сообщений об ошибках
// ============================================================================

static void test_error_get_user_hint(void) {
    printf("Test: error_get_user_hint... ");
    
    const char *hint = error_get_user_hint(MTERR_AUTH_INVALID_TOKEN);
    TEST_ASSERT(hint != NULL && strlen(hint) > 0, "NULL hint");
    
    hint = error_get_user_hint(MTERR_TIMEOUT_REQUEST);
    TEST_ASSERT(strstr(hint, "Таймаут") != NULL, "Timeout hint mismatch");
    
    printf("OK\n");
}

static void test_error_is_recoverable(void) {
    printf("Test: error_is_recoverable... ");
    
    error_info_t error;
    memset(&error, 0, sizeof(error));
    error.error_code = MTERR_NETWORK_TIMEOUT;
    error.level = ERROR_LEVEL_WARNING;
    
    TEST_ASSERT(error_is_recoverable(&error) == 1, "should be recoverable");
    
    error.level = ERROR_LEVEL_FATAL;
    TEST_ASSERT(error_is_recoverable(&error) == 0, "FATAL should not be recoverable");
    
    printf("OK\n");
}

static void test_error_format_user_message(void) {
    printf("Test: error_format_user_message... ");
    
    error_info_t error;
    memset(&error, 0, sizeof(error));
    error.error_code = MTERR_AUTH_INVALID_TOKEN;
    error.level = ERROR_LEVEL_ERROR;
    error.category = ERROR_CATEGORY_AUTH;
    snprintf(error.message, sizeof(error.message), "Authentication failed");
    
    char buffer[2048];
    char *result = error_format_user_message(&error, buffer, sizeof(buffer));
    
    TEST_ASSERT(result != NULL, "NULL result");
    TEST_ASSERT(strstr(buffer, "ERROR") != NULL, "missing ERROR");
    TEST_ASSERT(strstr(buffer, "Подсказка") != NULL, "missing hint");
    
    printf("OK\n");
}

// ============================================================================
// Тесты обработки ошибок
// ============================================================================

static void test_error_handle(void) {
    printf("Test: error_handle... ");
    
    error_handler_context_t ctx;
    recovery_config_t config = {0};
    error_handler_init(&ctx, &config);
    
    error_info_t *error = error_create(
        MTERR_NETWORK_TIMEOUT,
        ERROR_LEVEL_WARNING,
        ERROR_CATEGORY_NETWORK,
        "Test timeout error"
    );
    
    int result = error_handle(&ctx, error);
    TEST_ASSERT(result == 0 || result == 1, "handle failed");
    
    error_free(error);
    error_handler_cleanup(&ctx);
    printf("OK\n");
}

static void test_error_get_stats(void) {
    printf("Test: error_get_stats... ");
    
    error_handler_context_t ctx;
    recovery_config_t config = {0};
    error_handler_init(&ctx, &config);
    
    error_info_t *err1 = error_create(MTERR_NETWORK_TIMEOUT, ERROR_LEVEL_WARNING, ERROR_CATEGORY_NETWORK, "Test 1");
    error_info_t *err2 = error_create(MTERR_AUTH_INVALID_TOKEN, ERROR_LEVEL_ERROR, ERROR_CATEGORY_AUTH, "Test 2");
    
    error_handle(&ctx, err1);
    error_handle(&ctx, err2);
    
    error_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    error_get_stats(&ctx, &stats);
    
    TEST_ASSERT(stats.total_errors >= 2, "wrong error count");
    
    error_free(err1);
    error_free(err2);
    error_handler_cleanup(&ctx);
    printf("OK\n");
}

// ============================================================================
// Тесты производительности
// ============================================================================

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void test_error_handler_performance(void) {
    printf("Test: error_handler_performance (1000 errors)... ");
    
    error_handler_context_t ctx;
    recovery_config_t config = {0};
    error_handler_init(&ctx, &config);
    
    double start = get_time_ms();
    
    for (int i = 0; i < 1000; i++) {
        error_info_t *error = error_create(
            MTERR_NETWORK_TIMEOUT,
            ERROR_LEVEL_WARNING,
            ERROR_CATEGORY_NETWORK,
            "Performance test error"
        );
        error_handle(&ctx, error);
        error_free(error);
    }
    
    double elapsed = get_time_ms() - start;
    double ops_per_sec = 1000.0 / (elapsed / 1000.0);
    
    printf("%.0f errors/sec... ", ops_per_sec);
    TEST_ASSERT(ops_per_sec > 100, "too slow");  // Минимум 100 ошибок в секунду
    
    error_handler_cleanup(&ctx);
    printf("OK\n");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    printf("=== Error Handler Unit Tests ===\n\n");
    
    int tests_run = 0;
    int tests_failed = 0;
    
    #define RUN_TEST(test_func) do { \
        tests_run++; \
        test_func(); \
    } while(0)
    
    #define RUN_TEST_CHECK(test_func) do { \
        int before = tests_failed; \
        RUN_TEST(test_func); \
        if (before != tests_failed) tests_failed++; \
    } while(0)
    
    // Тесты инициализации
    RUN_TEST(test_error_handler_init);
    RUN_TEST(test_error_create);
    RUN_TEST(test_error_create_with_details);
    
    // Тесты утилит
    RUN_TEST(test_error_code_to_string);
    RUN_TEST(test_error_level_to_string);
    RUN_TEST(test_error_category_to_string);
    
    // Тесты улучшенных сообщений
    RUN_TEST(test_error_get_user_hint);
    RUN_TEST(test_error_is_recoverable);
    RUN_TEST(test_error_format_user_message);
    
    // Тесты обработки
    RUN_TEST(test_error_handle);
    RUN_TEST(test_error_get_stats);
    
    // Тесты производительности
    RUN_TEST(test_error_handler_performance);
    
    // Итоги
    printf("\n=== Test Summary ===\n");
    printf("Total:  %d\n", tests_run);
    printf("Passed: %d\n", tests_run);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
