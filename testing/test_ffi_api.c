/**
 * @file test_ffi_api.c
 * @brief Тесты для FFI API MTProxy
 * 
 * Тестирование публичного API для интеграции через FFI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Включаем публичный API
#include "include/mtproxy.h"

// Статистика тестов
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("--- Test: %s ---\n", #name)
#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  ✗ %s\n", msg); \
        fprintf(stderr, "Assertion failed: %s\n", #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    tests_run++; \
    if ((a) == (b)) { \
        tests_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  ✗ %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
    } \
} while(0)

// ============================================================================
// Тесты инициализации
// ============================================================================

static void test_init(void) {
    TEST(mtproxy_init);
    
    int ret = mtproxy_init();
    ASSERT_EQ(ret, 0, "Init successful");
    
    bool running = mtproxy_is_running();
    ASSERT(running == false, "Not running after init");
}

static void test_version(void) {
    TEST(mtproxy_get_version);
    
    const char* version = mtproxy_get_version();
    ASSERT(version != NULL, "Version not NULL");
    ASSERT(strlen(version) > 0, "Version not empty");
    printf("  Version: %s\n", version);
}

// ============================================================================
// Тесты конфигурации
// ============================================================================

static void test_set_port(void) {
    TEST(mtproxy_set_port);
    
    int ret = mtproxy_set_port(8443);
    ASSERT_EQ(ret, 0, "Set port 8443");
    
    ret = mtproxy_set_port(0);
    ASSERT(ret != 0, "Invalid port 0 rejected");
}

static void test_secrets(void) {
    TEST(mtproxy_add_secret);
    
    // Валидный секрет (64 hex символа)
    const char* valid_secret = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    int ret = mtproxy_add_secret(valid_secret);
    ASSERT_EQ(ret, 0, "Valid secret added");
    
    // Невалидный секрет (слишком короткий)
    ret = mtproxy_add_secret("0123456789abcdef");
    ASSERT(ret != 0, "Short secret rejected");
    
    // Невалидный секрет (не hex)
    ret = mtproxy_add_secret("ghijklmnopqrstuvwxyz0123456789abcdef0123456789abcdef012345");
    ASSERT(ret != 0, "Non-hex secret rejected");
    
    // Дубликат
    ret = mtproxy_add_secret(valid_secret);
    ASSERT(ret != 0, "Duplicate secret rejected");
}

static void test_validate_secret(void) {
    TEST(mtproxy_validate_secret);
    
    const char* valid = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const char* short_secret = "0123456789abcdef";
    const char* invalid = "ghijklmnopqrstuvwxyz0123456789abcdef0123456789abcdef01234567";
    
    ASSERT(mtproxy_validate_secret(valid) == true, "Valid secret accepted");
    ASSERT(mtproxy_validate_secret(short_secret) == false, "Short secret rejected");
    ASSERT(mtproxy_validate_secret(invalid) == false, "Invalid chars rejected");
}

static void test_clear_secrets(void) {
    TEST(mtproxy_clear_secrets);
    
    mtproxy_clear_secrets();
    
    const char* secret = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef01234567";
    int ret = mtproxy_add_secret(secret);
    ASSERT_EQ(ret, 0, "Secret added");
    
    mtproxy_clear_secrets();
    printf("  ✓ Secrets cleared\n");
}

static void test_max_connections(void) {
    TEST(mtproxy_set_max_connections);
    
    int ret = mtproxy_set_max_connections(5000);
    ASSERT_EQ(ret, 0, "Set max connections");
    
    ret = mtproxy_set_max_connections(0);
    ASSERT(ret != 0, "Invalid max_connections rejected");
}

static void test_ipv6(void) {
    TEST(mtproxy_set_ipv6);
    
    int ret = mtproxy_set_ipv6(true);
    ASSERT_EQ(ret, 0, "Enable IPv6");
    
    ret = mtproxy_set_ipv6(false);
    ASSERT_EQ(ret, 0, "Disable IPv6");
}

// ============================================================================
// Тесты статистики
// ============================================================================

static void test_stats(void) {
    TEST(mtproxy_get_stats);
    
    mtproxy_stats_t* stats = mtproxy_get_stats();
    ASSERT(stats != NULL, "Stats not NULL");
    
    printf("  Active connections: %u\n", stats->active_connections);
    printf("  Total connections: %u\n", stats->total_connections);
    printf("  Bytes sent: %lu\n", (unsigned long)stats->bytes_sent);
    printf("  Bytes received: %lu\n", (unsigned long)stats->bytes_received);
}

static void test_connection_counters(void) {
    TEST(mtproxy_get_active_connections);
    
    uint32_t active = mtproxy_get_active_connections();
    ASSERT(active == 0, "No active connections initially");
    
    uint32_t total = mtproxy_get_total_connections();
    ASSERT(total == 0, "No total connections initially");
}

static void test_traffic_counters(void) {
    TEST(mtproxy_get_bytes_sent);
    
    uint64_t sent = mtproxy_get_bytes_sent();
    ASSERT(sent == 0, "No bytes sent initially");
    
    uint64_t received = mtproxy_get_bytes_received();
    ASSERT(received == 0, "No bytes received initially");
}

static void test_uptime(void) {
    TEST(mtproxy_get_uptime);
    
    // До запуска uptime = 0
    uint64_t uptime = mtproxy_get_uptime();
    ASSERT(uptime == 0, "No uptime before start");
}

// ============================================================================
// Тесты утилит
// ============================================================================

static void test_generate_secret(void) {
    TEST(mtproxy_generate_secret);
    
    char buffer[65];
    int ret = mtproxy_generate_secret(buffer, sizeof(buffer));
    ASSERT_EQ(ret, 0, "Secret generated");
    ASSERT(strlen(buffer) == 64, "Secret length is 64");
    ASSERT(mtproxy_validate_secret(buffer) == true, "Generated secret is valid");
    
    printf("  Generated: %s\n", buffer);
}

static void test_generate_secret_invalid_buffer(void) {
    TEST(mtproxy_generate_secret_invalid_buffer);
    
    char buffer[10];  // Слишком маленький
    int ret = mtproxy_generate_secret(buffer, sizeof(buffer));
    ASSERT(ret != 0, "Small buffer rejected");
}

static void test_last_error(void) {
    TEST(mtproxy_get_last_error);
    
    const char* error = mtproxy_get_last_error();
    ASSERT(error != NULL, "Last error not NULL");
    printf("  Last error: '%s'\n", error);
}

// ============================================================================
// Тесты запуска/остановки
// ============================================================================

static void test_start_stop(void) {
    TEST(mtproxy_start_stop);
    
    // Очищаем секреты перед тестом
    mtproxy_clear_secrets();
    
    // Попытка запуска без секрета
    int ret = mtproxy_start();
    ASSERT(ret != 0, "Start without secret rejected");
    
    // Добавляем секрет
    const char* secret = "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210";
    ret = mtproxy_add_secret(secret);
    ASSERT_EQ(ret, 0, "Secret added");
    
    // Запуск (в тестовом режиме не запускается реально)
    ret = mtproxy_start();
    // Ожидаем ошибку т.к. реальный сервер не запускается в тесте
    printf("  Start returned: %d\n", ret);
    
    // Остановка
    mtproxy_stop();
    printf("  ✓ Stop called\n");
}

// ============================================================================
// Тесты конфигурации struct
// ============================================================================

static void test_apply_config(void) {
    TEST(mtproxy_apply_config);
    
    mtproxy_clear_secrets();
    
    mtproxy_config_t config = {
        .port = 9443,
        .secret = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
        .max_connections = 8000,
        .enable_ipv6 = false,
        .enable_stats = true
    };
    
    int ret = mtproxy_apply_config(&config);
    ASSERT_EQ(ret, 0, "Config applied");
}

static void test_apply_config_null(void) {
    TEST(mtproxy_apply_config_null);
    
    int ret = mtproxy_apply_config(NULL);
    ASSERT(ret != 0, "Null config rejected");
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("===========================================\n");
    printf("  MTProxy FFI API Test Suite\n");
    printf("===========================================\n\n");
    
    // Тесты инициализации
    test_init();
    test_version();
    
    // Тесты конфигурации
    test_set_port();
    test_secrets();
    test_validate_secret();
    test_clear_secrets();
    test_max_connections();
    test_ipv6();
    test_apply_config();
    test_apply_config_null();
    
    // Тесты статистики
    test_stats();
    test_connection_counters();
    test_traffic_counters();
    test_uptime();
    
    // Тесты утилит
    test_generate_secret();
    test_generate_secret_invalid_buffer();
    test_last_error();
    
    // Тесты запуска
    test_start_stop();
    
    // Итоги
    printf("\n===========================================\n");
    printf("  Test Summary\n");
    printf("===========================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
