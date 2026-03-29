/*
    Cache No-Copy Test - Тестирование оптимизации cache_get_ref
    Сравнение производительности с копированием и без
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #include <sys/time.h>
#endif

#include "common/cache-manager.h"
#include "testing/test_memory_utils.h"

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

// Тест 1: Сравнение производительности (с копированием vs без)
void test_cache_copy_vs_no_copy() {
    printf("\n=== Cache Copy vs No-Copy Performance Test ===\n");
    
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 50000,
        .max_size_mb = 100,
        .default_ttl_sec = 300,
        .enable_locking = 0,
        .enable_partitioning = 1,
        .partition_count = 8
    };
    
    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ERROR: Failed to initialize cache\n");
        ops_failed++;
        return;
    }
    
    const int ENTRIES = 10000;
    const int ITERATIONS = 50000;
    const size_t DATA_SIZE = 256;
    
    // Предварительная запись данных
    printf("  Preparing %d entries...\n", ENTRIES);
    for (int i = 0; i < ENTRIES; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        
        char *value = malloc(DATA_SIZE);
        snprintf(value, DATA_SIZE, "value_%d_data_string", i);
        
        cache_put(cache, key, value, strlen(value) + 1);
        free(value);
    }
    
    printf("  Cache entries: %zu\n", cache->stats.current_entries);
    
    // Тест 1: Чтение с копированием (cache_get)
    int64_t start_copy = get_time_us();
    
    for (int i = 0; i < ITERATIONS; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i % ENTRIES);
        
        void *data = NULL;
        size_t size = 0;
        cache_status_t status = cache_get(cache, key, &data, &size);
        
        if (status == CACHE_OK && data) {
            // Имитация работы с данными
            volatile char sum = 0;
            for (size_t j = 0; j < size; j++) {
                sum += ((char*)data)[j];
            }
            (void)sum;
            
            free(data);  // Освобождение копии
        }
        
        ops_run++;
    }
    
    int64_t duration_copy = get_time_us() - start_copy;
    double ops_per_sec_copy = (ITERATIONS * 1000000.0) / duration_copy;
    
    printf("\n  Cache Get (with copy):\n");
    printf("    %d ops in %.2f ms (%.0f ops/sec)\n",
           ITERATIONS, duration_copy / 1000.0, ops_per_sec_copy);
    printf("    Avg latency: %.2f ns/op\n", (double)duration_copy / ITERATIONS * 1000);
    
    // Тест 2: Чтение без копирования (cache_get_no_copy)
    int64_t start_no_copy = get_time_us();
    
    for (int i = 0; i < ITERATIONS; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i % ENTRIES);
        
        const void *data = NULL;
        size_t size = 0;
        cache_status_t status = cache_get_no_copy(cache, key, &data, &size);
        
        if (status == CACHE_OK && data) {
            // Имитация работы с данными (только чтение)
            volatile char sum = 0;
            for (size_t j = 0; j < size; j++) {
                sum += ((const char*)data)[j];
            }
            (void)sum;
            // Нет free() - данные не копировались!
        }
        
        ops_run++;
    }
    
    int64_t duration_no_copy = get_time_us() - start_no_copy;
    double ops_per_sec_no_copy = (ITERATIONS * 1000000.0) / duration_no_copy;
    
    printf("\n  Cache Get (no copy):\n");
    printf("    %d ops in %.2f ms (%.0f ops/sec)\n",
           ITERATIONS, duration_no_copy / 1000.0, ops_per_sec_no_copy);
    printf("    Avg latency: %.2f ns/op\n", (double)duration_no_copy / ITERATIONS * 1000);
    
    // Сравнение
    double speedup = (double)duration_copy / duration_no_copy;
    printf("\n  Speedup: %.2fx faster (%.1f%% improvement)\n",
           speedup, (speedup - 1) * 100);
    
    cache_manager_cleanup(cache);
    printf("\n  Copy vs No-Copy Test Complete\n");
    
    ops_passed++;
}

// Тест 2: Использование cache_get_ref
void test_cache_get_ref() {
    printf("\n=== Cache Get Ref Test ===\n");
    
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 1000,
        .max_size_mb = 10,
        .default_ttl_sec = 60,
        .enable_locking = 0
    };
    
    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ERROR: Failed to initialize cache\n");
        ops_failed++;
        return;
    }
    
    // Запись тестовых данных
    const int TEST_ENTRIES = 100;
    for (int i = 0; i < TEST_ENTRIES; i++) {
        char key[64];
        snprintf(key, sizeof(key), "ref_key_%d", i);
        
        char value[128];
        snprintf(value, sizeof(value), "ref_value_%d_data", i);
        
        cache_put(cache, key, value, strlen(value) + 1);
    }
    
    printf("  Written %d entries\n", TEST_ENTRIES);
    
    // Чтение через cache_get_ref
    int hit_count = 0;
    int64_t start = get_time_us();
    
    for (int i = 0; i < TEST_ENTRIES * 10; i++) {
        char key[64];
        snprintf(key, sizeof(key), "ref_key_%d", i % TEST_ENTRIES);
        
        size_t size = 0;
        const void *data = cache_get_ref(cache, key, &size);
        
        if (data) {
            hit_count++;
            
            // Чтение данных (без модификации!)
            volatile char first_byte = ((const char*)data)[0];
            (void)first_byte;
        }
        
        ops_run++;
    }
    
    int64_t duration = get_time_us() - start;
    double ops_per_sec = ((TEST_ENTRIES * 10) * 1000000.0) / duration;
    
    printf("  Read %d entries x10 in %.2f ms (%.0f ops/sec)\n",
           TEST_ENTRIES, duration / 1000.0, ops_per_sec);
    printf("  Hit rate: %d%% (%d/%d)\n",
           (hit_count * 100) / (TEST_ENTRIES * 10), hit_count, TEST_ENTRIES * 10);
    
    cache_manager_cleanup(cache);
    printf("\n  Get Ref Test Complete\n");
    
    ops_passed++;
}

// Тест 3: Стресс-тест с mixed операциями
void test_cache_mixed_stress() {
    printf("\n=== Cache Mixed Stress Test ===\n");
    
    cache_config_t config = {
        .type = CACHE_TYPE_MEMORY,
        .policy = CACHE_LRU,
        .max_entries = 5000,
        .max_size_mb = 50,
        .default_ttl_sec = 300,
        .enable_locking = 0
    };
    
    cache_manager_t *cache = cache_manager_init(&config);
    if (!cache) {
        printf("  ERROR: Failed to initialize cache\n");
        ops_failed++;
        return;
    }
    
    const int ITERATIONS = 20000;
    const int MAX_KEY = 1000;
    
    int64_t start = get_time_us();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int key_idx = i % MAX_KEY;
        char key[64];
        snprintf(key, sizeof(key), "stress_key_%d", key_idx);
        
        if (i % 3 == 0) {
            // Write (33%)
            char value[128];
            snprintf(value, sizeof(value), "stress_value_%d", i);
            cache_put(cache, key, value, strlen(value) + 1);
        } else {
            // Read no-copy (67%)
            size_t size = 0;
            const void *data = cache_get_ref(cache, key, &size);
            
            if (data) {
                // Чтение данных
                volatile char first_byte = ((const char*)data)[0];
                (void)first_byte;
            }
        }
        
        ops_run++;
    }
    
    int64_t duration = get_time_us() - start;
    double ops_per_sec = (ITERATIONS * 1000000.0) / duration;
    
    printf("  Mixed stress: %d ops in %.2f ms (%.0f ops/sec)\n",
           ITERATIONS, duration / 1000.0, ops_per_sec);
    
    // Статистика
    cache_stats_t stats;
    cache_get_stats(cache, &stats);
    
    printf("\n  Cache Statistics:\n");
    printf("    Current entries: %zu\n", stats.current_entries);
    printf("    Hits: %lld\n", stats.hits);
    printf("    Misses: %lld\n", stats.misses);
    printf("    Hit rate: %.1f%%\n", stats.hit_rate);
    
    cache_manager_cleanup(cache);
    printf("\n  Mixed Stress Test Complete\n");
    
    ops_passed++;
}

// Главная функция
int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("  Cache No-Copy Optimization Test Suite\n");
    printf("===========================================\n\n");
    
    TEST_PRINT_MEMORY_USAGE("Start");
    
    // Тест 1: Сравнение производительности
    TEST_WITH_MEMORY_CLEANUP(test_cache_copy_vs_no_copy);
    
    // Тест 2: Использование cache_get_ref
    TEST_WITH_MEMORY_CLEANUP(test_cache_get_ref);
    
    // Тест 3: Стресс-тест
    TEST_WITH_MEMORY_CLEANUP(test_cache_mixed_stress);
    
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
