/*
    Cache Performance Test - 100K+ операций
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
   Тест 1: Базовая производительность (100K операций)
   ============================================ */

void test_cache_basic_performance() {
    printf("\n=== Cache Basic Performance Test (100K ops) ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 50000,
        .max_size_mb = 100,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = 1,
        .num_partitions = 8
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    printf("  Cache initialized: %d partitions, %d max entries\n", 
           config.num_partitions, config.max_entries);

    // Тест 1: Put операции (50K записей)
    BENCH_START(put, 50000);
    for (int i = 0; i < 50000; i++) {
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

    // Тест 2: Get операции (50K чтений)
    BENCH_START(get, 50000);
    int hit_count = 0;
    for (int i = 0; i < 50000; i++) {
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
           (hit_count * 100.0) / 50000, hit_count, 50000);

    // Тест 3: Смешанные операции (25K read + 25K write)
    BENCH_START(mixed, 50000);
    for (int i = 0; i < 50000; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "key_%d", i % 50000);
        
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
    cache_stats_t stats = cache_get_stats(cache);
    printf("\n  Cache Statistics:\n");
    printf("    Total entries: %d\n", stats.total_entries);
    printf("    Hit count: %d\n", stats.hit_count);
    printf("    Miss count: %d\n", stats.miss_count);
    printf("    Evictions: %d\n", stats.evictions);
    printf("    Memory used: %.2f MB\n", stats.memory_used_bytes / (1024.0 * 1024.0));

    cache_manager_free(cache);
    printf("\n  Basic Performance Test Complete\n");
}

/* ============================================
   Тест 2: Производительность с TTL
   ============================================ */

void test_cache_ttl_performance() {
    printf("\n=== Cache TTL Performance Test ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 10000,
        .max_size_mb = 50,
        .default_ttl_sec = 2,  // Короткий TTL для теста
        .enable_locking = 0,
        .enable_partitioning = 1,
        .num_partitions = 4
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    // Запись 10K записей с TTL
    BENCH_START(put_ttl, 10000);
    for (int i = 0; i < 10000; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "ttl_key_%d", i);
        snprintf(value, sizeof(value), "ttl_value_%d", i);
        
        cache_status_t status = cache_put_ttl(cache, key, value, strlen(value) + 1, 2);
        if (status == CACHE_OK) {
            ops_passed++;
        } else {
            ops_failed++;
        }
        ops_run++;
    }
    BENCH_END(put_ttl);

    // Ожидание истечения TTL
    printf("  Waiting for TTL expiration (3 sec)...\n");
    sleep(3);

    // Чтение - все записи должны истечь
    int expired_count = 0;
    BENCH_START(get_expired, 10000);
    for (int i = 0; i < 10000; i++) {
        char key[64];
        snprintf(key, sizeof(key), "ttl_key_%d", i);
        
        void *result = NULL;
        size_t result_size = 0;
        cache_status_t status = cache_get(cache, key, &result, &result_size);
        
        if (status != CACHE_OK || result == NULL) {
            expired_count++;
        }
        if (result) free(result);
        ops_run++;
    }
    BENCH_END(get_expired);
    printf("  Expired entries: %d/%d (%.2f%%)\n", 
           expired_count, 10000, (expired_count * 100.0) / 10000);

    cache_manager_free(cache);
    printf("\n  TTL Performance Test Complete\n");
}

/* ============================================
   Тест 3: Массовые операции (Batch)
   ============================================ */

void test_cache_batch_operations() {
    printf("\n=== Cache Batch Operations Test ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 20000,
        .max_size_mb = 50,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = 1,
        .num_partitions = 4
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    const int BATCH_SIZE = 1000;
    const int BATCH_COUNT = 20;
    const int TOTAL_OPS = BATCH_SIZE * BATCH_COUNT;

    // Batch Put
    BENCH_START(batch_put, TOTAL_OPS);
    for (int b = 0; b < BATCH_COUNT; b++) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            char key[64], value[128];
            snprintf(key, sizeof(key), "batch_%d_key_%d", b, i);
            snprintf(value, sizeof(value), "batch_%d_value_%d", b, i);
            
            cache_status_t status = cache_put(cache, key, value, strlen(value) + 1);
            if (status == CACHE_OK) {
                ops_passed++;
            } else {
                ops_failed++;
            }
            ops_run++;
        }
    }
    BENCH_END(batch_put);

    // Batch Get
    BENCH_START(batch_get, TOTAL_OPS);
    int batch_hit_count = 0;
    for (int b = 0; b < BATCH_COUNT; b++) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            char key[64];
            snprintf(key, sizeof(key), "batch_%d_key_%d", b, i);
            
            void *result = NULL;
            size_t result_size = 0;
            cache_status_t status = cache_get(cache, key, &result, &result_size);
            
            if (status == CACHE_OK && result != NULL) {
                batch_hit_count++;
                ops_passed++;
            } else {
                ops_failed++;
            }
            if (result) free(result);
            ops_run++;
        }
    }
    BENCH_END(batch_get);
    printf("  Batch hit rate: %.2f%% (%d/%d)\n", 
           (batch_hit_count * 100.0) / TOTAL_OPS, batch_hit_count, TOTAL_OPS);

    // Batch Delete
    BENCH_START(batch_delete, TOTAL_OPS);
    for (int b = 0; b < BATCH_COUNT; b++) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            char key[64];
            snprintf(key, sizeof(key), "batch_%d_key_%d", b, i);
            
            cache_status_t status = cache_delete(cache, key);
            if (status == CACHE_OK) {
                ops_passed++;
            } else {
                ops_failed++;
            }
            ops_run++;
        }
    }
    BENCH_END(batch_delete);

    cache_manager_free(cache);
    printf("\n  Batch Operations Test Complete\n");
}

/* ============================================
   Тест 4: Стресс-тест (LRU eviction)
   ============================================ */

void test_cache_lru_stress() {
    printf("\n=== Cache LRU Stress Test ===\n");

    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,  // Маленький лимит для eviction
        .max_size_mb = 10,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = 0
    };

    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ✗ FAILED: Cache initialization failed\n");
        ops_failed++;
        return;
    }

    const int TOTAL_WRITES = 10000;

    // Запись больше чем max_entries (должен работать LRU eviction)
    BENCH_START(lru_write, TOTAL_WRITES);
    for (int i = 0; i < TOTAL_WRITES; i++) {
        char key[64], value[128];
        snprintf(key, sizeof(key), "stress_key_%d", i);
        snprintf(value, sizeof(value), "stress_value_%d_longer_data", i);
        
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
    cache_stats_t stats = cache_get_stats(cache);
    printf("\n  LRU Statistics:\n");
    printf("    Total entries: %d (max: %d)\n", stats.total_entries, config.max_entries);
    printf("    Evictions: %d\n", stats.evictions);
    printf("    Expected evictions: ~%d\n", TOTAL_WRITES - config.max_entries);

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
    printf("  Recent entries hit rate: %.2f%% (%d/500)\n", 
           (recent_hit * 100.0) / 500, recent_hit);

    cache_manager_free(cache);
    printf("\n  LRU Stress Test Complete\n");
}

/* ============================================
   Главная функция
   ============================================ */

int main(int argc, char **argv) {
    printf("========================================\n");
    printf("Cache Performance Test Suite (Q2 2026)\n");
    printf("========================================\n\n");

    int64_t total_start = get_time_us();

    // Запуск тестов
    test_cache_basic_performance();
    test_cache_ttl_performance();
    test_cache_batch_operations();
    test_cache_lru_stress();

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
