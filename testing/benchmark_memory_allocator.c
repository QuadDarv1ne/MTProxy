/*
 * benchmark_memory_allocator.c — Бенчмарки производительности аллокаторов
 *
 * Сравнение:
 * - standard malloc/free
 * - jemalloc (если доступен)
 * - tcmalloc (если доступен)
 *
 * Тесты:
 * - malloc/free производительность
 * - aligned malloc производительность
 * - fragmentation тест
 * - multi-thread тест
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "common/memory-allocator.h"

#define BENCH_ITERATIONS 100000
#define THREAD_COUNT 4

// Статистика
typedef struct {
    double malloc_ops_per_sec;
    double free_ops_per_sec;
    double aligned_ops_per_sec;
    double total_time_ms;
    size_t total_allocated;
    size_t peak_memory;
} benchmark_stats_t;

static benchmark_stats_t g_stats = {0};

// Тест 1: Базовая производительность malloc/free
static void benchmark_malloc_free(void) {
    void *ptrs[BENCH_ITERATIONS];
    clock_t start, end;
    
    printf("Test 1: malloc/free (%d iterations)...\n", BENCH_ITERATIONS);
    start = clock();
    
    // Выделение
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        ptrs[i] = mt_malloc(64 + (i % 256));
    }
    
    end = clock();
    double alloc_time = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    g_stats.malloc_ops_per_sec = BENCH_ITERATIONS / alloc_time * 1000;
    
    printf("  Allocation: %.2f ms (%.0f ops/sec)\n", 
           alloc_time, g_stats.malloc_ops_per_sec);
    
    // Освобождение
    start = clock();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        mt_free(ptrs[i]);
    }
    end = clock();
    
    double free_time = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    g_stats.free_ops_per_sec = BENCH_ITERATIONS / free_time * 1000;
    
    printf("  Free:       %.2f ms (%.0f ops/sec)\n", 
           free_time, g_stats.free_ops_per_sec);
}

// Тест 2: Производительность aligned malloc
static void benchmark_aligned_malloc(void) {
    void *ptrs[BENCH_ITERATIONS / 10];
    clock_t start, end;
    
    printf("\nTest 2: aligned malloc (%d iterations, 64-byte align)...\n", 
           BENCH_ITERATIONS / 10);
    start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS / 10; i++) {
        ptrs[i] = mt_malloc_aligned(1024, 64);
    }
    
    end = clock();
    double alloc_time = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    g_stats.aligned_ops_per_sec = (BENCH_ITERATIONS / 10) / alloc_time * 1000;
    
    printf("  Aligned Alloc: %.2f ms (%.0f ops/sec)\n", 
           alloc_time, g_stats.aligned_ops_per_sec);
    
    // Освобождение
    for (int i = 0; i < BENCH_ITERATIONS / 10; i++) {
        mt_free_aligned(ptrs[i]);
    }
}

// Тест 3: Фрагментация памяти
static void benchmark_fragmentation(void) {
    void *ptrs[1000];
    size_t total = 0;
    
    printf("\nTest 3: Fragmentation test...\n");
    
    // Выделение разных размеров
    for (int i = 0; i < 1000; i++) {
        size_t size = 16 << (i % 10);  // 16 байт .. 16 КБ
        ptrs[i] = mt_malloc(size);
        if (ptrs[i]) {
            total += size;
            memset(ptrs[i], 0, size);
        }
    }
    
    printf("  Allocated: %zu KB\n", total / 1024);
    g_stats.total_allocated = total;
    
    // Освобождение каждого второго
    int freed_count = 0;
    for (int i = 0; i < 1000; i += 2) {
        if (ptrs[i]) {
            mt_free(ptrs[i]);
            freed_count++;
        }
    }
    
    printf("  Freed %d blocks (creating fragmentation)\n", freed_count);
    
    // Попытка выделить большой блок
    void *large = mt_malloc(1024 * 1024);  // 1 MB
    if (large) {
        printf("  Large allocation (1 MB): SUCCESS\n");
        mt_free(large);
    } else {
        printf("  Large allocation (1 MB): FAILED (fragmentation)\n");
    }
    
    // Cleanup
    for (int i = 1; i < 1000; i += 2) {
        if (ptrs[i]) mt_free(ptrs[i]);
    }
    
    // Компактизация
    mt_compact_memory();
}

// Многопоточный тест
typedef struct {
    int thread_id;
    int iterations;
    double time_ms;
    size_t allocated;
} thread_arg_t;

static void* thread_alloc_worker(void *arg) {
    thread_arg_t *targ = (thread_arg_t*)arg;
    void *ptrs[1000];
    clock_t start = clock();
    
    for (int i = 0; i < targ->iterations; i++) {
        size_t size = 64 + (i % 512);
        ptrs[i % 1000] = mt_malloc(size);
        if (ptrs[i % 1000]) {
            targ->allocated += size;
            memset(ptrs[i % 1000], 0, size);
        }
        
        // Освобождение каждого 10-го
        if (i % 10 == 0 && i > 0) {
            int idx = (i - 10) % 1000;
            if (ptrs[idx]) {
                mt_free(ptrs[idx]);
                ptrs[idx] = NULL;
            }
        }
    }
    
    // Cleanup
    for (int i = 0; i < 1000; i++) {
        if (ptrs[i]) mt_free(ptrs[i]);
    }
    
    clock_t end = clock();
    targ->time_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    
    return NULL;
}

static void benchmark_multithreaded(void) {
    pthread_t threads[THREAD_COUNT];
    thread_arg_t args[THREAD_COUNT];
    
    printf("\nTest 4: Multi-threaded test (%d threads)...\n", THREAD_COUNT);
    
    clock_t start = clock();
    
    // Создание потоков
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].thread_id = i;
        args[i].iterations = BENCH_ITERATIONS / THREAD_COUNT;
        args[i].allocated = 0;
        pthread_create(&threads[i], NULL, thread_alloc_worker, &args[i]);
    }
    
    // Ожидание завершения
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    double total_time = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    
    // Статистика
    size_t total_allocated = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        total_allocated += args[i].allocated;
        printf("  Thread %d: %.2f ms, %zu KB allocated\n",
               i, args[i].time_ms, args[i].allocated / 1024);
    }
    
    printf("  Total: %.2f ms, %zu KB\n", total_time, total_allocated / 1024);
    printf("  Combined throughput: %.0f ops/sec\n", 
           BENCH_ITERATIONS / total_time * 1000);
}

// Пиковое потребление памяти
static void benchmark_peak_memory(void) {
    void *ptrs[10000];
    size_t peak = 0;
    size_t current = 0;
    
    printf("\nTest 5: Peak memory test...\n");
    
    for (int i = 0; i < 10000; i++) {
        size_t size = 256 + (i % 1024);
        ptrs[i] = mt_malloc(size);
        if (ptrs[i]) {
            current += size;
            if (current > peak) {
                peak = current;
            }
            memset(ptrs[i], 0, size);
        }
    }
    
    printf("  Peak memory: %zu KB\n", peak / 1024);
    g_stats.peak_memory = peak;
    
    // Cleanup
    for (int i = 0; i < 10000; i++) {
        if (ptrs[i]) mt_free(ptrs[i]);
    }
}

// Итоги
static void print_summary(void) {
    printf("\n=== Benchmark Summary ===\n");
    printf("Allocator: ");
#ifdef USE_JEMALLOC
    printf("jemalloc\n");
#elif defined(USE_TCMALLOC)
    printf("tcmalloc\n");
#else
    printf("standard malloc (default)\n");
#endif
    
    printf("\nPerformance:\n");
    printf("  malloc ops/sec:  %.0f\n", g_stats.malloc_ops_per_sec);
    printf("  free ops/sec:    %.0f\n", g_stats.free_ops_per_sec);
    printf("  aligned ops/sec: %.0f\n", g_stats.aligned_ops_per_sec);
    
    printf("\nMemory:\n");
    printf("  Total allocated: %zu KB\n", g_stats.total_allocated / 1024);
    printf("  Peak memory:     %zu KB\n", g_stats.peak_memory / 1024);
    
    printf("\nRecommendations:\n");
    if (g_stats.malloc_ops_per_sec < 500000) {
        printf("  - Consider using jemalloc for better performance\n");
    }
    if (g_stats.peak_memory > 100 * 1024) {
        printf("  - High memory usage, consider memory pooling\n");
    }
}

int main(int argc, char *argv[]) {
    printf("=== Memory Allocator Benchmark ===\n");
    printf("Iterations: %d, Threads: %d\n\n", BENCH_ITERATIONS, THREAD_COUNT);
    
    benchmark_malloc_free();
    benchmark_aligned_malloc();
    benchmark_fragmentation();
    benchmark_multithreaded();
    benchmark_peak_memory();
    
    print_summary();
    
    return 0;
}
