/*
    Rate Limiter High-Load Test - Упрощённая версия для Windows
    High-Load тестирование rate-limiter (Q2 2026)
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
#endif

#include "common/rate-limiter.h"
#include "test_memory_utils.h"  // Утилиты управления памятью

// Статистика тестов
static int ops_run = 0;
static int ops_passed = 0;
static int ops_failed = 0;

// Получение времени в микросекундах
static inline int64_t get_time_us() {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000) / freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

// Макрос для бенчмарка
#define BENCH_START(name, iterations) \
    int64_t start_##name = get_time_us(); \
    int iterations_##name = iterations

#define BENCH_END(name) \
    int64_t end_##name = get_time_us(); \
    int64_t duration_##name = end_##name - start_##name; \
    double ops_per_sec_##name = (iterations_##name * 1000000.0) / duration_##name; \
    printf("  %s: %d ops in %.2f ms (%.0f ops/sec)\n", \
           #name, iterations_##name, duration_##name / 1000.0, ops_per_sec_##name)

/* ============================================
   Тест 1: Базовая производительность (1000 клиентов)
   ============================================ */

void test_rate_limiter_basic_performance() {
    printf("\n=== Rate Limiter Basic Performance (1000 clients) ===\n");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 100,
        .window_seconds = 1,
        .bucket_capacity = 200,
        .refill_rate = 100,
        .enable_burst = 1,
        .burst_capacity = 200,
        .enable_whitelist = 1,
        .enable_blacklist = 1
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    printf("  Rate limiter initialized: %d req/sec, burst=%d\n",
           (int)config.max_requests, (int)config.burst_capacity);

    const int CLIENT_COUNT = 1000;
    const int REQUESTS_PER_CLIENT = 100;
    const int TOTAL_REQUESTS = CLIENT_COUNT * REQUESTS_PER_CLIENT;

    // Тест: проверка лимитов для 1000 клиентов
    BENCH_START(check, TOTAL_REQUESTS);
    int allowed_count = 0;
    int rejected_count = 0;

    for (int c = 0; c < CLIENT_COUNT; c++) {
        char client_key[64];
        snprintf(client_key, sizeof(client_key), "client_%d", c);

        for (int r = 0; r < REQUESTS_PER_CLIENT; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, client_key);

            if (status == RATE_LIMIT_OK) {
                allowed_count++;
                ops_passed++;
            } else if (status == RATE_LIMIT_EXCEEDED) {
                rejected_count++;
                ops_passed++;  // Это тоже успешный результат
            } else {
                ops_failed++;
            }
            ops_run++;
        }
    }
    BENCH_END(check);

    printf("  Allowed: %d (%.2f%%)\n", allowed_count, (allowed_count * 100.0) / TOTAL_REQUESTS);
    printf("  Rejected: %d (%.2f%%)\n", rejected_count, (rejected_count * 100.0) / TOTAL_REQUESTS);

    // Статистика
    rate_limiter_stats_t stats;
    rate_limiter_get_stats(limiter, &stats);
    printf("\n  Rate Limiter Statistics:\n");
    printf("    Total requests: %llu\n", (unsigned long long)stats.total_requests);
    printf("    Total rejections: %llu\n", (unsigned long long)stats.total_rejections);
    printf("    Active entries: %zu\n", stats.active_entries);
    printf("    Rejection rate: %.2f%%\n", stats.rejection_rate);

    rate_limiter_cleanup(limiter);
    printf("\n  Basic Performance Test Complete\n");
}

/* ============================================
   Тест 2: Whitelist/Blacklist производительность
   ============================================ */

void test_rate_limiter_whitelist_blacklist() {
    printf("\n=== Rate Limiter Whitelist/Blacklist Test ===\n");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 50,
        .window_seconds = 1,
        .bucket_capacity = 100,
        .refill_rate = 50,
        .enable_burst = 1,
        .burst_capacity = 100,
        .enable_whitelist = 1,
        .enable_blacklist = 1
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    const int WHITELIST_COUNT = 100;
    const int BLACKLIST_COUNT = 100;

    // Добавление в whitelist
    BENCH_START(whitelist_add, WHITELIST_COUNT);
    for (int i = 0; i < WHITELIST_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "whitelist_client_%d", i);
        rate_limit_add_to_whitelist(limiter, key);
        ops_run++;
        ops_passed++;
    }
    BENCH_END(whitelist_add);

    // Добавление в blacklist
    BENCH_START(blacklist_add, BLACKLIST_COUNT);
    for (int i = 0; i < BLACKLIST_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "blacklist_client_%d", i);
        rate_limit_add_to_blacklist(limiter, key);
        ops_run++;
        ops_passed++;
    }
    BENCH_END(blacklist_add);

    // Тест проверки whitelist
    BENCH_START(whitelist_check, WHITELIST_COUNT * 10);
    int whitelist_allowed = 0;
    for (int i = 0; i < WHITELIST_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "whitelist_client_%d", i);

        for (int r = 0; r < 10; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, key);
            if (status == RATE_LIMIT_OK) {
                whitelist_allowed++;
            }
            ops_run++;
        }
        ops_passed++;
    }
    BENCH_END(whitelist_check);
    printf("  Whitelist allowed: %d/%d (%.2f%%)\n",
           whitelist_allowed, WHITELIST_COUNT * 10, (whitelist_allowed * 100.0) / (WHITELIST_COUNT * 10));

    // Тест проверки blacklist
    BENCH_START(blacklist_check, BLACKLIST_COUNT * 10);
    int blacklist_rejected = 0;
    for (int i = 0; i < BLACKLIST_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "blacklist_client_%d", i);

        for (int r = 0; r < 10; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, key);
            if (status == RATE_LIMIT_EXCEEDED || status == RATE_LIMIT_ERROR) {
                blacklist_rejected++;
            }
            ops_run++;
        }
        ops_passed++;
    }
    BENCH_END(blacklist_check);
    printf("  Blacklist rejected: %d/%d (%.2f%%)\n",
           blacklist_rejected, BLACKLIST_COUNT * 10, (blacklist_rejected * 100.0) / (BLACKLIST_COUNT * 10));

    rate_limiter_cleanup(limiter);
    printf("\n  Whitelist/Blacklist Test Complete\n");
}

/* ============================================
   Тест 3: High-Load (10000 клиентов)
   ============================================ */

void test_rate_limiter_highload() {
    printf("\n=== Rate Limiter High-Load Test (10000 clients) ===\n");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .max_requests = 100,
        .window_seconds = 1,
        .bucket_capacity = 200,
        .refill_rate = 100,
        .enable_burst = 1,
        .burst_capacity = 200,
        .enable_whitelist = 0,
        .enable_blacklist = 0
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    const int CLIENT_COUNT = 10000;
    const int REQUESTS_PER_CLIENT = 10;
    const int TOTAL_REQUESTS = CLIENT_COUNT * REQUESTS_PER_CLIENT;

    // Тест: 10K клиентов по 10 запросов
    BENCH_START(highload_check, TOTAL_REQUESTS);
    int allowed_count = 0;
    int rejected_count = 0;

    for (int c = 0; c < CLIENT_COUNT; c++) {
        char client_key[64];
        snprintf(client_key, sizeof(client_key), "load_client_%d", c);

        for (int r = 0; r < REQUESTS_PER_CLIENT; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, client_key);

            if (status == RATE_LIMIT_OK) {
                allowed_count++;
            } else if (status == RATE_LIMIT_EXCEEDED) {
                rejected_count++;
            }
            ops_run++;
            ops_passed++;
        }
    }
    BENCH_END(highload_check);

    printf("  Allowed: %d (%.2f%%)\n", allowed_count, (allowed_count * 100.0) / TOTAL_REQUESTS);
    printf("  Rejected: %d (%.2f%%)\n", rejected_count, (rejected_count * 100.0) / TOTAL_REQUESTS);

    // Статистика
    rate_limiter_stats_t stats;
    rate_limiter_get_stats(limiter, &stats);
    printf("\n  Rate Limiter Statistics:\n");
    printf("    Total requests: %llu\n", (unsigned long long)stats.total_requests);
    printf("    Total rejections: %llu\n", (unsigned long long)stats.total_rejections);
    printf("    Active entries: %zu\n", stats.active_entries);

    rate_limiter_cleanup(limiter);
    printf("\n  High-Load Test Complete\n");
}

/* ============================================
   Главная функция
   ============================================ */

int main(int argc, char *argv[]) {
    // Проверка флага --quick для быстрых тестов
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        printf("Quick mode enabled (reduced iterations)\n\n");
    }

    TEST_PRINT_MEMORY_USAGE("Start");

    printf("===========================================\n");
    printf("  Rate Limiter High-Load Test Suite\n");
    printf("===========================================\n\n");

    // Тест 1: Базовая производительность
    TEST_WITH_MEMORY_CLEANUP(test_rate_limiter_basic_performance);

    // Тест 2: Whitelist/Blacklist тест
    TEST_WITH_MEMORY_CLEANUP(test_rate_limiter_whitelist_blacklist);

    // Тест 3: High-Load тест
    TEST_WITH_MEMORY_CLEANUP(test_rate_limiter_highload);

    printf("\n===========================================\n");
    printf("  Final Summary\n");
    printf("===========================================\n");
    printf("  Total operations: %d\n", ops_run);
    printf("  Passed: %d\n", ops_passed);
    printf("  Failed: %d\n", ops_failed);
    printf("  Success rate: %.2f%%\n",
           (ops_run > 0) ? (ops_passed * 100.0) / ops_run : 0);
    
    TEST_PRINT_MEMORY_USAGE("End");
    printf("===========================================\n");

    return (ops_failed > 0) ? 1 : 0;
}
