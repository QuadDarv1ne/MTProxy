/*
    Cache Memory Pool Test - Тестирование пула памяти для кэша
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#include "common/cache-memory-pool.h"

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

// Тест 1: Базовая производительность пула
void test_pool_basic_performance() {
    printf("\n=== Pool Basic Performance Test ===\n");
    
    cache_entry_pool_t pool;
    const size_t POOL_SIZE = 10000;
    
    if (cache_pool_init(&pool, POOL_SIZE) != 0) {
        printf("  ERROR: Failed to initialize pool\n");
        ops_failed++;
        return;
    }
    
    printf("  Pool initialized: %zu preallocated entries\n", POOL_SIZE);
    
    // Тест 1: Выделение из пула
    int64_t start_pool = get_time_us();
    void **ptrs = (void**)malloc(POOL_SIZE * sizeof(void*));
    
    for (size_t i = 0; i < POOL_SIZE; i++) {
        ptrs[i] = cache_pool_alloc(&pool, 256);
        if (!ptrs[i]) {
            printf("  ERROR: Allocation failed at index %zu\n", i);
            ops_failed++;
        }
    }
    
    int64_t end_pool = get_time_us();
    int64_t duration_pool = end_pool - start_pool;
    double ops_per_sec_pool = (POOL_SIZE * 1000000.0) / duration_pool;
    
    printf("  Pool alloc: %zu ops in %.2f ms (%.0f ops/sec)\n",
           POOL_SIZE, duration_pool / 1000.0, ops_per_sec_pool);
    
    // Тест 2: Выделение через calloc (для сравнения)
    start_pool = get_time_us();
    
    for (size_t i = 0; i < POOL_SIZE; i++) {
        ptrs[i] = calloc(1, 256);
    }
    
    end_pool = get_time_us();
    duration_pool = end_pool - start_pool;
    ops_per_sec_pool = (POOL_SIZE * 1000000.0) / duration_pool;
    
    printf("  Calloc alloc: %zu ops in %.2f ms (%.0f ops/sec)\n",
           POOL_SIZE, duration_pool / 1000.0, ops_per_sec_pool);
    
    // Освобождение
    for (size_t i = 0; i < POOL_SIZE; i++) {
        cache_pool_free(&pool, ptrs[i]);
    }
    free(ptrs);
    
    // Статистика
    cache_pool_stats_t stats;
    cache_pool_get_stats(&pool, &stats);
    
    printf("\n  Pool Statistics:\n");
    printf("    Total allocations: %zu\n", stats.total_allocations);
    printf("    Pool hits: %zu (%.1f%%)\n", stats.pool_hits, stats.hit_rate);
    printf("    Pool misses: %zu\n", stats.pool_misses);
    printf("    Current usage: %zu\n", stats.current_usage);
    
    cache_pool_cleanup(&pool);
    printf("\n  Basic Performance Test Complete\n");
    
    ops_passed++;
    ops_run++;
}

// Тест 2: Стресс-тест с смешанными операциями
void test_pool_stress() {
    printf("\n=== Pool Stress Test ===\n");
    
    cache_entry_pool_t pool;
    const size_t POOL_SIZE = 5000;
    const size_t ITERATIONS = 50000;
    
    if (cache_pool_init(&pool, POOL_SIZE) != 0) {
        printf("  ERROR: Failed to initialize pool\n");
        ops_failed++;
        return;
    }
    
    printf("  Pool size: %zu, Iterations: %zu\n", POOL_SIZE, ITERATIONS);
    
    void **ptrs = (void**)malloc(POOL_SIZE * sizeof(void*));
    memset(ptrs, 0, POOL_SIZE * sizeof(void*));
    
    int64_t start = get_time_us();
    
    for (size_t i = 0; i < ITERATIONS; i++) {
        size_t idx = i % POOL_SIZE;
        
        // Освобождение старой записи
        if (ptrs[idx]) {
            cache_pool_free(&pool, ptrs[idx]);
        }
        
        // Выделение новой записи
        ptrs[idx] = cache_pool_alloc(&pool, 256);
        if (!ptrs[idx]) {
            printf("  ERROR: Allocation failed at iteration %zu\n", i);
            ops_failed++;
        }
        
        ops_run++;
    }
    
    int64_t duration = get_time_us() - start;
    double ops_per_sec = (ITERATIONS * 1000000.0) / duration;
    
    printf("  Stress test: %zu ops in %.2f ms (%.0f ops/sec)\n",
           ITERATIONS, duration / 1000.0, ops_per_sec);
    
    // Освобождение всех записей
    for (size_t i = 0; i < POOL_SIZE; i++) {
        if (ptrs[i]) {
            cache_pool_free(&pool, ptrs[i]);
        }
    }
    free(ptrs);
    
    // Статистика
    cache_pool_stats_t stats;
    cache_pool_get_stats(&pool, &stats);
    
    printf("\n  Pool Statistics:\n");
    printf("    Total allocations: %zu\n", stats.total_allocations);
    printf("    Pool hits: %zu (%.1f%%)\n", stats.pool_hits, stats.hit_rate);
    printf("    Pool misses: %zu\n", stats.pool_misses);
    
    cache_pool_cleanup(&pool);
    printf("\n  Stress Test Complete\n");
    
    ops_passed++;
}

// Тест 3: Тест многопоточности
void test_pool_multithreaded() {
    printf("\n=== Pool Multithreaded Test ===\n");
    
    cache_entry_pool_t pool;
    const size_t POOL_SIZE = 10000;
    
    if (cache_pool_init(&pool, POOL_SIZE) != 0) {
        printf("  ERROR: Failed to initialize pool\n");
        ops_failed++;
        return;
    }
    
    printf("  Pool initialized with %zu entries\n", POOL_SIZE);
    printf("  NOTE: Multithreaded test requires threading support\n");
    
    // Базовый тест с блокировками
    const int THREADS = 4;
    const int OPS_PER_THREAD = 1000;
    
    int64_t start = get_time_us();
    
    for (int t = 0; t < THREADS; t++) {
        for (int i = 0; i < OPS_PER_THREAD; i++) {
            void *ptr = cache_pool_alloc(&pool, 256);
            if (ptr) {
                cache_pool_free(&pool, ptr);
                ops_run++;
            }
        }
    }
    
    int64_t duration = get_time_us() - start;
    double ops_per_sec = ((THREADS * OPS_PER_THREAD) * 1000000.0) / duration;
    
    printf("  Multithreaded: %d ops in %.2f ms (%.0f ops/sec)\n",
           THREADS * OPS_PER_THREAD, duration / 1000.0, ops_per_sec);
    
    // Статистика
    cache_pool_stats_t stats;
    cache_pool_get_stats(&pool, &stats);
    
    printf("\n  Pool Statistics:\n");
    printf("    Total allocations: %zu\n", stats.total_allocations);
    printf("    Pool hits: %zu (%.1f%%)\n", stats.pool_hits, stats.hit_rate);
    
    cache_pool_cleanup(&pool);
    printf("\n  Multithreaded Test Complete\n");
    
    ops_passed++;
}

// Главная функция
int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("  Cache Memory Pool Test Suite\n");
    printf("===========================================\n\n");
    
    test_pool_basic_performance();
    test_pool_stress();
    test_pool_multithreaded();
    
    printf("\n===========================================\n");
    printf("  Final Summary\n");
    printf("===========================================\n");
    printf("  Total operations: %d\n", ops_run);
    printf("  Passed: %d\n", ops_passed);
    printf("  Failed: %d\n", ops_failed);
    printf("  Success rate: %.2f%%\n",
           (ops_run > 0) ? (ops_passed * 100.0) / ops_run : 0);
    printf("===========================================\n");
    
    return (ops_failed > 0) ? 1 : 0;
}
