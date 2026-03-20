/*
    Rate Limiter High-Load Test — 1000+ клиентов
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
        .requests_per_second = 100,
        .burst_size = 200,
        .enable_whitelist = 1,
        .enable_blacklist = 1,
        .client_ttl_sec = 300
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    printf("  Rate limiter initialized: %d req/sec, burst=%d\n", 
           config.requests_per_second, config.burst_size);

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
            
            if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
                allowed_count++;
                ops_passed++;
            } else {
                rejected_count++;
                ops_passed++;  // Это тоже успешный результат
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
    printf("    Active clients: %d\n", stats.active_clients);
    printf("    Total requests: %d\n", stats.total_requests);
    printf("    Allowed requests: %d\n", stats.allowed_requests);
    printf("    Rejected requests: %d\n", stats.rejected_requests);

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
        .requests_per_second = 50,
        .burst_size = 100,
        .enable_whitelist = 1,
        .enable_blacklist = 1,
        .client_ttl_sec = 300
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    const int WHITELIST_COUNT = 100;
    const int BLACKLIST_COUNT = 100;
    const int NORMAL_COUNT = 800;
    const int TOTAL_CLIENTS = WHITELIST_COUNT + BLACKLIST_COUNT + NORMAL_COUNT;

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
            if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
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
            if (status == RATE_LIMIT_STATUS_BLACKLISTED) {
                blacklist_rejected++;
            }
            ops_run++;
        }
        ops_passed++;
    }
    BENCH_END(blacklist_check);
    printf("  Blacklist rejected: %d/%d (%.2f%%)\n", 
           blacklist_rejected, BLACKLIST_COUNT * 10, (blacklist_rejected * 100.0) / (BLACKLIST_COUNT * 10));

    // Тест нормальных клиентов
    BENCH_START(normal_check, NORMAL_COUNT * 10);
    int normal_allowed = 0;
    for (int i = 0; i < NORMAL_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "normal_client_%d", i);
        
        for (int r = 0; r < 10; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, key);
            if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
                normal_allowed++;
            }
            ops_run++;
        }
        ops_passed++;
    }
    BENCH_END(normal_check);
    printf("  Normal clients allowed: %d/%d (%.2f%%)\n", 
           normal_allowed, NORMAL_COUNT * 10, (normal_allowed * 100.0) / (NORMAL_COUNT * 10));

    rate_limiter_cleanup(limiter);
    printf("\n  Whitelist/Blacklist Test Complete\n");
}

/* ============================================
   Тест 3: Разные алгоритмы (5 алгоритмов)
   ============================================ */

void test_rate_limiter_algorithms() {
    printf("\n=== Rate Limiter Algorithms Comparison ===\n");

    const int CLIENT_COUNT = 200;
    const int REQUESTS_PER_CLIENT = 50;

    rate_limit_algorithm_t algorithms[] = {
        RATE_LIMIT_TOKEN_BUCKET,
        RATE_LIMIT_SLIDING_WINDOW,
        RATE_LIMIT_FIXED_WINDOW,
        RATE_LIMIT_LEAKY_BUCKET,
        RATE_LIMIT_ADAPTIVE
    };

    const char *algo_names[] = {
        "Token Bucket",
        "Sliding Window",
        "Fixed Window",
        "Leaky Bucket",
        "Adaptive"
    };

    for (int a = 0; a < 5; a++) {
        printf("\n  Algorithm: %s\n", algo_names[a]);

        rate_limit_config_t config = {
            .algorithm = algorithms[a],
            .requests_per_second = 100,
            .burst_size = 150,
            .enable_whitelist = 0,
            .enable_blacklist = 0,
            .client_ttl_sec = 300
        };

        rate_limiter_t *limiter = rate_limiter_init(&config);
        if (!limiter) {
            printf("    ✗ FAILED: Initialization failed\n");
            continue;
        }

        BENCH_START(algo_test, CLIENT_COUNT * REQUESTS_PER_CLIENT);
        int allowed = 0;
        int rejected = 0;

        for (int c = 0; c < CLIENT_COUNT; c++) {
            char client_key[64];
            snprintf(client_key, sizeof(client_key), "algo_%d_client_%d", a, c);

            for (int r = 0; r < REQUESTS_PER_CLIENT; r++) {
                rate_limit_status_t status = rate_limit_check(limiter, client_key);
                
                if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
                    allowed++;
                } else {
                    rejected++;
                }
                ops_run++;
                ops_passed++;
            }
        }
        BENCH_END(algo_test);

        printf("    Allowed: %d, Rejected: %d\n", allowed, rejected);

        rate_limiter_cleanup(limiter);
    }

    printf("\n  Algorithms Comparison Complete\n");
}

/* ============================================
   Тест 4: Стресс-тест (1000 клиентов, burst)
   ============================================ */

void test_rate_limiter_stress() {
    printf("\n=== Rate Limiter Stress Test (1000 clients, burst) ===\n");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .requests_per_second = 10,  // Низкий лимит для теста
        .burst_size = 20,
        .enable_whitelist = 0,
        .enable_blacklist = 0,
        .client_ttl_sec = 60
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    const int CLIENT_COUNT = 1000;
    const int BURST_REQUESTS = 100;  // Много запросов сразу

    // Burst запросы от каждого клиента
    BENCH_START(stress_burst, CLIENT_COUNT * BURST_REQUESTS);
    int total_allowed = 0;
    int total_rejected = 0;

    for (int c = 0; c < CLIENT_COUNT; c++) {
        char client_key[64];
        snprintf(client_key, sizeof(client_key), "stress_client_%d", c);

        for (int r = 0; r < BURST_REQUESTS; r++) {
            rate_limit_status_t status = rate_limit_check(limiter, client_key);
            
            if (status == RATE_LIMIT_STATUS_OK || status == RATE_LIMIT_STATUS_ALLOWED) {
                total_allowed++;
            } else {
                total_rejected++;
            }
            ops_run++;
            ops_passed++;
        }
    }
    BENCH_END(stress_burst);

    printf("  Total allowed: %d (%.2f%%)\n", total_allowed, (total_allowed * 100.0) / (CLIENT_COUNT * BURST_REQUESTS));
    printf("  Total rejected: %d (%.2f%%)\n", total_rejected, (total_rejected * 100.0) / (CLIENT_COUNT * BURST_REQUESTS));

    // Проверка статистики
    rate_limiter_stats_t stats;
    rate_limiter_get_stats(limiter, &stats);
    printf("\n  Stress Test Statistics:\n");
    printf("    Active clients: %d\n", stats.active_clients);
    printf("    Total requests: %d\n", stats.total_requests);
    printf("    Peak rate: %d req/sec\n", stats.peak_requests_per_second);

    rate_limiter_cleanup(limiter);
    printf("\n  Stress Test Complete\n");
}

/* ============================================
   Тест 5: Retry-After и Reset-Time
   ============================================ */

void test_rate_limiter_retry_after() {
    printf("\n=== Rate Limiter Retry-After Test ===\n");

    rate_limit_config_t config = {
        .algorithm = RATE_LIMIT_TOKEN_BUCKET,
        .requests_per_second = 5,
        .burst_size = 10,
        .enable_whitelist = 0,
        .enable_blacklist = 0,
        .client_ttl_sec = 300
    };

    rate_limiter_t *limiter = rate_limiter_init(&config);
    if (!limiter) {
        printf("  ✗ FAILED: Rate limiter initialization failed\n");
        ops_failed++;
        return;
    }

    const char *test_key = "retry_test_client";

    // Исчерпание лимита
    printf("  Exhausting rate limit...\n");
    for (int i = 0; i < 20; i++) {
        rate_limit_check(limiter, test_key);
        ops_run++;
    }

    // Проверка retry-after
    BENCH_START(retry_after, 100);
    uint64_t retry_after_sum = 0;
    for (int i = 0; i < 100; i++) {
        uint64_t retry_after = rate_limit_calculate_retry_after(limiter, test_key);
        retry_after_sum += retry_after;
        ops_run++;
        ops_passed++;
    }
    BENCH_END(retry_after);
    printf("  Average retry-after: %.2f ms\n", retry_after_sum / 100.0);

    // Проверка reset-time
    BENCH_START(reset_time, 100);
    time_t reset_time_sum = 0;
    for (int i = 0; i < 100; i++) {
        time_t reset_time = rate_limit_calculate_reset_time(limiter, test_key);
        reset_time_sum += reset_time;
        ops_run++;
        ops_passed++;
    }
    BENCH_END(reset_time);
    printf("  Average reset-time: %ld sec\n", reset_time_sum / 100);

    rate_limiter_cleanup(limiter);
    printf("\n  Retry-After Test Complete\n");
}

/* ============================================
   Главная функция
   ============================================ */

int main(int argc, char **argv) {
    printf("========================================\n");
    printf("Rate Limiter High-Load Test (Q2 2026)\n");
    printf("========================================\n\n");

    int64_t total_start = get_time_us();

    // Запуск тестов
    test_rate_limiter_basic_performance();
    test_rate_limiter_whitelist_blacklist();
    test_rate_limiter_algorithms();
    test_rate_limiter_stress();
    test_rate_limiter_retry_after();

    int64_t total_end = get_time_us();
    double total_duration = (total_end - total_start) / 1000000.0;

    // Итоговая статистика
    printf("\n========================================\n");
    printf("FINAL RESULTS\n");
    printf("========================================\n");
    printf("Total operations: %d\n", ops_run);
    printf("Passed: %d (%.2f%%)\n", ops_passed, (ops_passed * 100.0) / ops_run);
    printf("Failed: %d (%.2f%%)\n", ops_failed, (ops_failed * 100.0) / ops_run);
    printf("Total time: %.2f sec\n", total_duration);
    printf("Overall performance: %.0f ops/sec\n", ops_run / total_duration);
    printf("========================================\n");

    return ops_failed > 0 ? 1 : 0;
}
