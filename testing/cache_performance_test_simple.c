/*
    Cache Performance Test - Упрощённая версия для Windows
    Performance тестирование cache-manager (Q2 2026)
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

#include "common/cache-manager.h"
#include "test_memory_utils.h"  // Утилиты управления памятью

// Статистика тестов
static int ops_run = 0;
static int ops_passed = 0;
static int ops_failed = 0;
static int quick_mode = 0;  // Флаг для быстрых тестов

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
   Тест 1: Базовая производительность (оптимизировано)
   ============================================ */

void test_cache_basic_performance() {
    // Оптимизация: уменьшение итераций для Windows и quick режима
    #ifdef _WIN32
    int put_iterations = quick_mode ? 1000 : 5000;
    int get_iterations = quick_mode ? 1000 : 5000;
    int mixed_iterations = quick_mode ? 2000 : 5000;
    int max_entries = quick_mode ? 5000 : 50000;
    #else
    int put_iterations = quick_mode ? 5000 : 50000;
    int get_iterations = quick_mode ? 5000 : 50000;
    int mixed_iterations = quick_mode ? 10000 : 50000;
    int max_entries = quick_mode ? 10000 : 50000;
    #endif
    
    printf("\n=== Cache Basic Performance Test (%s) ===\n", 
           quick_mode ? "quick mode" : "full mode");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = max_entries,
        .max_size_mb = quick_mode ? 10 : 100,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = quick_mode ? 1 : 1,
        .partition_count = quick_mode ? 4 : 8
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    printf("  Cache initialized: %d partitions, %d max entries\n",
           config.partition_count, config.max_entries);

    // Тест 1: Put операции
    BENCH_START(put, put_iterations);
    for (int i = 0; i < put_iterations; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d_data", i);

        cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
        if (status == CACHE_OK) {
            ops_passed++;
        } else {
            ops_failed++;
        }
        ops_run++;
    }
    BENCH_END(put);

    // Тест 2: Get операции
    BENCH_START(get, get_iterations);
    int hit_count = 0;
    for (int i = 0; i < get_iterations; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            hit_count++;
            ops_passed++;
        } else {
            ops_failed++;
        }
        if (result) free(result);
        ops_run++;
    }
    BENCH_END(get);
    printf("  Hit rate: %.2f%% (%d/%d)\n",
           (hit_count * 100.0) / get_iterations, hit_count, get_iterations);

    // Тест 3: Смешанные операции
    BENCH_START(mixed, mixed_iterations);
    for (int i = 0; i < mixed_iterations; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "key_%d", i % max_entries);

        if (i % 2 == 0) {
            // Write
            snprintf(value, sizeof(value), "updated_value_%d", i);
            cache_put(cache, key, value, strlen(value) + 1);
        } else {
            // Read
            void *result = NULL;
            size_t result_size = 0;
            cache_get(cache, key, &result, &result_size);
            if (result) free(result);
        }
        ops_run++;
        ops_passed++;
    }
    BENCH_END(mixed);

    // Статистика кэша
    cache_stats_t stats;
    cache_get_stats(cache, &stats);
    printf("\n  Cache Statistics:\n");
    printf("    Current entries: %zu\n", stats.current_entries);
    printf("    Hits: %lld\n", stats.hits);
    printf("    Misses: %lld\n", stats.misses);
    printf("    Evictions: %lld\n", stats.evictions);
    printf("    Hit rate: %.2f%%\n", stats.hit_rate);

    cache_manager_cleanup(cache);
    printf("\n  Basic Performance Test Complete\n");
}

/* ============================================
   Тест 2: Стресс-тест LRU (100K записей)
   ============================================ */

void test_cache_lru_stress() {
    printf("\n=== Cache LRU Stress Test (100K writes) ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 50000,
        .max_size_mb = 50,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    const int TOTAL_WRITES = 100000;

    // Запись 100K записей (должны начаться вытеснения)
    BENCH_START(lru_write, TOTAL_WRITES);
    for (int i = 0; i < TOTAL_WRITES; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "stress_key_%d", i);
        snprintf(value, sizeof(value), "stress_value_%d_data", i);

        cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
        if (status == CACHE_OK) {
            ops_passed++;
        } else {
            ops_failed++;
        }
        ops_run++;
    }
    BENCH_END(lru_write);

    // Проверка статистики
    cache_stats_t stats;
    cache_get_stats(cache, &stats);
    printf("\n  LRU Statistics:\n");
    printf("    Current entries: %zu (max: %zu)\n", stats.current_entries, config.max_entries);
    printf("    Evictions: %lld\n", stats.evictions);
    printf("    Expected evictions: ~%d\n", TOTAL_WRITES - (int)config.max_entries);

    // Чтение последних записей (должны быть в кэше)
    int recent_hit = 0;
    BENCH_START(lru_read_recent, 500);
    for (int i = TOTAL_WRITES - 500; i < TOTAL_WRITES; i++) {
        char key[64];
        snprintf(key, sizeof(key), "stress_key_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            recent_hit++;
        }
        if (result) free(result);
        ops_run++;
    }
    BENCH_END(lru_read_recent);
    printf("  Recent hit rate: %.2f%% (%d/500)\n",
           (recent_hit * 100.0) / 500, recent_hit);

    // Чтение старых записей (должны быть вытеснены)
    int old_hit = 0;
    BENCH_START(lru_read_old, 500);
    for (int i = 0; i < 500; i++) {
        char key[64];
        snprintf(key, sizeof(key), "stress_key_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            old_hit++;
        }
        if (result) free(result);
        ops_run++;
    }
    BENCH_END(lru_read_old);
    printf("  Old hit rate: %.2f%% (%d/500) - expected low\n",
           (old_hit * 100.0) / 500, old_hit);

    cache_manager_cleanup(cache);
    printf("\n  LRU Stress Test Complete\n");
}

/* ============================================
   Тест 3: Операции с TTL
   ============================================ */

void test_cache_ttl() {
    printf("\n=== Cache TTL Test ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_TTL,
        .max_entries = 10000,
        .max_size_mb = 10,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        return;
    }

    // Запись с коротким TTL (2 секунды)
    printf("  Writing entries with 2-second TTL...\n");
    for (int i = 0; i < 1000; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "ttl_key_%d", i);
        snprintf(value, sizeof(value), "ttl_value_%d", i);

        cache_status_t status = cache_put_with_ttl(cache, key, value, strlen(value) + 1, 2);
        if (status == CACHE_OK) {
            ops_passed++;
        } else {
            ops_failed++;
        }
        ops_run++;
    }
    printf("  ✓ Written 1000 entries\n");

    // Чтение сразу (должны быть доступны)
    int hit_before = 0;
    for (int i = 0; i < 100; i++) {
        char key[64];
        snprintf(key, sizeof(key), "ttl_key_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            hit_before++;
        }
        if (result) free(result);
    }
    printf("  Hit rate before TTL: %d%% (%d/100)\n", hit_before, hit_before);

    // Ожидание истечения TTL
    printf("  Waiting for TTL expiration (3 seconds)...\n");
    sleep(3);

    // Чтение после TTL (должны быть удалены)
    int hit_after = 0;
    for (int i = 0; i < 100; i++) {
        char key[64];
        snprintf(key, sizeof(key), "ttl_key_%d", i);

        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);

        if (status == CACHE_OK && result != NULL) {
            hit_after++;
        }
        if (result) free(result);
    }
    printf("  Hit rate after TTL: %d%% (%d/100) - expected 0\n", hit_after, hit_after);

    cache_manager_cleanup(cache);
    printf("\n  TTL Test Complete\n");
}

/* ============================================
   Главная функция
   ============================================ */

int main(int argc, char *argv[]) {
    // Проверка флага --quick для быстрых тестов
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        quick_mode = 1;
        printf("Quick mode enabled (reduced iterations)\n\n");
    }

    TEST_PRINT_MEMORY_USAGE("Start");

    printf("===========================================\n");
    printf("  Cache Performance Test Suite\n");
    printf("===========================================\n\n");

    // Тест 1: Базовая производительность
    TEST_WITH_MEMORY_CLEANUP(test_cache_basic_performance);

    // Тест 2: LRU стресс-тест
    TEST_WITH_MEMORY_CLEANUP(test_cache_lru_stress);

    // Тест 3: TTL тест
    TEST_WITH_MEMORY_CLEANUP(test_cache_ttl);

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
