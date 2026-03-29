/*
 * benchmark_cache_performance.c — Бенчмарки производительности cache-manager
 *
 * Тесты производительности кэша (100K+ операций):
 * - 100K операций кэширования (get/set)
 * - 100K операций с различными алгоритмами (LRU, LFU, FIFO, TTL)
 * - Кэш performance под нагрузкой
 * - Hit rate тесты
 * - Multi-thread тесты
 *
 * Запуск:
 *   ./benchmark-cache-performance
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "common/cache-manager.h"

#define BENCH_100K  100000
#define BENCH_500K  500000
#define BENCH_1M    1000000
#define THREAD_COUNT 8
#define KEY_SIZE    64
#define VALUE_SIZE  256

/* ============================================================================
 * Benchmark statistics
 * ============================================================================ */

typedef struct {
    double ops_per_sec;
    double total_time_ms;
    double avg_time_per_op_us;
    size_t total_operations;
    long long hits;
    long long misses;
    double hit_rate;
    size_t bytes_processed;
    double throughput_mbps;
} cache_benchmark_stats_t;

static cache_benchmark_stats_t g_stats = {0};

/* ============================================================================
 * Timing helpers
 * ============================================================================ */

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void print_cache_stats(const char *test_name, cache_benchmark_stats_t *stats) {
    printf("\n  %-45s\n", test_name);
    printf("  Operations:      %12zu\n", stats->total_operations);
    printf("  Total time:      %12.2f ms\n", stats->total_time_ms);
    printf("  Ops/sec:         %12.0f\n", stats->ops_per_sec);
    printf("  Avg time/op:     %12.2f μs\n", stats->avg_time_per_op_us);
    printf("  Cache hits:      %12lld\n", stats->hits);
    printf("  Cache misses:    %12lld\n", stats->misses);
    printf("  Hit rate:        %11.2f%%\n", stats->hit_rate);
    if (stats->bytes_processed > 0) {
        printf("  Throughput:      %12.2f MB/s\n", stats->throughput_mbps);
        printf("  Bytes processed: %12zu KB\n", stats->bytes_processed / 1024);
    }
}

/* ============================================================================
 * Test 1: 100K Cache Set Operations (LRU)
 * ============================================================================ */

static void benchmark_100k_cache_set_lru(void) {
    cache_benchmark_stats_t stats = {0};
    cache_manager_t cache;
    double start, end;

    printf("\n[BENCH] 100K Cache SET Operations (LRU)...\n");

    // Инициализация кэша
    cache_config_t config = {0};
    config.type = CACHE_TYPE_MEMORY;
    config.policy = CACHE_LRU;
    config.max_entries = 50000;
    config.max_size_mb = 256;
    config.default_ttl_sec = 3600;
    config.enable_statistics = 1;

    if (cache_manager_init(&cache, &config) != 0) {
        printf("  ERROR: Failed to initialize cache manager\n");
        return;
    }

    char key[KEY_SIZE];
    char value[VALUE_SIZE];

    start = get_time_ms();

    // SET operations
    for (int i = 0; i < BENCH_100K; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d_data", i);
        cache_manager_set(&cache, key, value, strlen(value) + 1);
        stats.total_operations++;
    }

    end = get_time_ms();
    double total_time = end - start;

    stats.total_time_ms = total_time;
    stats.ops_per_sec = stats.total_operations / (total_time / 1000.0);
    stats.avg_time_per_op_us = (total_time / stats.total_operations) * 1000.0;
    stats.bytes_processed = stats.total_operations * (KEY_SIZE + VALUE_SIZE);
    stats.throughput_mbps = (stats.bytes_processed / 1024.0 / 1024.0) / (total_time / 1000.0);

    // Получение статистики
    cache_stats_t cache_stats = {0};
    cache_manager_get_stats(&cache, &cache_stats);
    stats.hits = cache_stats.hits;
    stats.misses = cache_stats.misses;
    stats.hit_rate = cache_stats.hit_rate * 100.0;

    print_cache_stats("100K cache SET (LRU)", &stats);

    cache_manager_cleanup(&cache);
    g_stats = stats;
}

/* ============================================================================
 * Test 2: 100K Cache Get Operations (LRU)
 * ============================================================================ */

static void benchmark_100k_cache_get_lru(void) {
    cache_benchmark_stats_t stats = {0};
    cache_manager_t cache;
    double start, end;
    char value[VALUE_SIZE];
    size_t value_len;

    printf("\n[BENCH] 100K Cache GET Operations (LRU)...\n");

    // Инициализация кэша
    cache_config_t config = {0};
    config.type = CACHE_TYPE_MEMORY;
    config.policy = CACHE_LRU;
    config.max_entries = 50000;
    config.max_size_mb = 256;
    config.default_ttl_sec = 3600;
    config.enable_statistics = 1;

    if (cache_manager_init(&cache, &config) != 0) {
        printf("  ERROR: Failed to initialize cache manager\n");
        return;
    }

    // Предварительное заполнение кэша
    char key[KEY_SIZE];
    char set_value[VALUE_SIZE];
    for (int i = 0; i < BENCH_100K; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(set_value, sizeof(set_value), "value_%d_data", i);
        cache_manager_set(&cache, key, set_value, strlen(set_value) + 1);
    }

    start = get_time_ms();

    // GET operations
    for (int i = 0; i < BENCH_100K; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        if (cache_manager_get(&cache, key, value, sizeof(value), &value_len) == CACHE_OK) {
            stats.hits++;
        } else {
            stats.misses++;
        }
        stats.total_operations++;
    }

    end = get_time_ms();
    double total_time = end - start;

    stats.total_time_ms = total_time;
    stats.ops_per_sec = stats.total_operations / (total_time / 1000.0);
    stats.avg_time_per_op_us = (total_time / stats.total_operations) * 1000.0;
    stats.bytes_processed = stats.total_operations * (KEY_SIZE + VALUE_SIZE);
    stats.throughput_mbps = (stats.bytes_processed / 1024.0 / 1024.0) / (total_time / 1000.0);
    stats.hit_rate = (stats.hits * 100.0) / stats.total_operations;

    print_cache_stats("100K cache GET (LRU)", &stats);

    cache_manager_cleanup(&cache);
    g_stats = stats;
}

/* ============================================================================
 * Test 3: 100K Mixed Operations (50% Set, 50% Get)
 * ============================================================================ */

static void benchmark_100k_cache_mixed(void) {
    cache_benchmark_stats_t stats = {0};
    cache_manager_t cache;
    double start, end;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    size_t value_len;

    printf("\n[BENCH] 100K Cache Mixed Operations (50%% SET, 50%% GET)...\n");

    // Инициализация кэша
    cache_config_t config = {0};
    config.type = CACHE_TYPE_MEMORY;
    config.policy = CACHE_LRU;
    config.max_entries = 50000;
    config.max_size_mb = 256;
    config.default_ttl_sec = 3600;
    config.enable_statistics = 1;

    if (cache_manager_init(&cache, &config) != 0) {
        printf("  ERROR: Failed to initialize cache manager\n");
        return;
    }

    start = get_time_ms();

    // Mixed operations
    for (int i = 0; i < BENCH_100K; i++) {
        snprintf(key, sizeof(key), "key_%d", i % 50000);  // Повторяющиеся ключи для hit rate

        if (i % 2 == 0) {
            // SET
            snprintf(value, sizeof(value), "value_%d_data", i);
            cache_manager_set(&cache, key, value, strlen(value) + 1);
        } else {
            // GET
            if (cache_manager_get(&cache, key, value, sizeof(value), &value_len) == CACHE_OK) {
                stats.hits++;
            } else {
                stats.misses++;
            }
        }
        stats.total_operations++;
    }

    end = get_time_ms();
    double total_time = end - start;

    stats.total_time_ms = total_time;
    stats.ops_per_sec = stats.total_operations / (total_time / 1000.0);
    stats.avg_time_per_op_us = (total_time / stats.total_operations) * 1000.0;
    stats.bytes_processed = stats.total_operations * (KEY_SIZE + VALUE_SIZE);
    stats.throughput_mbps = (stats.bytes_processed / 1024.0 / 1024.0) / (total_time / 1000.0);
    stats.hit_rate = (stats.hits * 100.0) / (stats.hits + stats.misses);

    print_cache_stats("100K cache mixed (50/50)", &stats);

    cache_manager_cleanup(&cache);
    g_stats = stats;
}

/* ============================================================================
 * Test 4: Cache Algorithm Comparison (LRU vs LFU vs FIFO)
 * ============================================================================ */

static void benchmark_cache_algorithms_comparison(void) {
    cache_manager_t cache;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    size_t value_len;

    printf("\n[BENCH] Cache Algorithm Comparison...\n");

    cache_config_t configs[] = {
        {.type = CACHE_TYPE_MEMORY, .policy = CACHE_LRU,  .max_entries = 10000, .max_size_mb = 64, .default_ttl_sec = 3600, .enable_statistics = 1},
        {.type = CACHE_TYPE_MEMORY, .policy = CACHE_LFU,  .max_entries = 10000, .max_size_mb = 64, .default_ttl_sec = 3600, .enable_statistics = 1},
        {.type = CACHE_MEMORY,      .policy = CACHE_FIFO, .max_entries = 10000, .max_size_mb = 64, .default_ttl_sec = 3600, .enable_statistics = 1},
        {.type = CACHE_TYPE_MEMORY, .policy = CACHE_TTL,  .max_entries = 10000, .max_size_mb = 64, .default_ttl_sec = 60,   .enable_statistics = 1}
    };

    const char *policy_names[] = {"LRU", "LFU", "FIFO", "TTL"};

    for (int p = 0; p < 4; p++) {
        if (cache_manager_init(&cache, &configs[p]) != 0) {
            printf("  ERROR: Failed to initialize cache (%s)\n", policy_names[p]);
            continue;
        }

        double start = get_time_ms();
        long long hits = 0, misses = 0;
        size_t ops = 0;

        // Zipf-like access pattern (80/20 rule)
        for (int i = 0; i < 100000; i++) {
            int key_idx;
            if (i % 100 < 80) {
                // 80% обращений к 20% ключей (hot keys)
                key_idx = i % 2000;
            } else {
                // 20% обращений к 80% ключей (cold keys)
                key_idx = 2000 + (i % 8000);
            }

            snprintf(key, sizeof(key), "key_%d", key_idx);
            snprintf(value, sizeof(value), "value_%d", key_idx);

            if (i % 2 == 0) {
                cache_manager_set(&cache, key, value, strlen(value) + 1);
            } else {
                if (cache_manager_get(&cache, key, value, sizeof(value), &value_len) == CACHE_OK) {
                    hits++;
                } else {
                    misses++;
                }
            }
            ops++;
        }

        double end = get_time_ms();
        double total_time = end - start;

        cache_stats_t cache_stats = {0};
        cache_manager_get_stats(&cache, &cache_stats);

        printf("\n  %-20s:\n", policy_names[p]);
        printf("    Operations:    %10zu\n", ops);
        printf("    Total time:    %10.2f ms\n", total_time);
        printf("    Ops/sec:       %10.0f\n", ops / (total_time / 1000.0));
        printf("    Hit rate:      %10.2f%%\n", (hits * 100.0) / (hits + misses));
        printf("    Evictions:     %10lld\n", cache_stats.evictions);

        cache_manager_cleanup(&cache);
    }
}

/* ============================================================================
 * Test 5: Multi-threaded Cache Performance
 * ============================================================================ */

typedef struct {
    cache_manager_t *cache;
    int thread_id;
    size_t iterations;
    long long hits;
    long long misses;
    size_t ops_completed;
    double time_ms;
} cache_thread_data_t;

static void *cache_bench_worker(void *arg) {
    cache_thread_data_t *data = (cache_thread_data_t *)arg;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    size_t value_len;

    double start = get_time_ms();

    for (size_t i = 0; i < data->iterations; i++) {
        snprintf(key, sizeof(key), "key_%d", (data->thread_id * data->iterations + i) % 10000);
        snprintf(value, sizeof(value), "value_%zu", i);

        if (i % 2 == 0) {
            cache_manager_set(data->cache, key, value, strlen(value) + 1);
        } else {
            if (cache_manager_get(data->cache, key, value, sizeof(value), &value_len) == CACHE_OK) {
                data->hits++;
            } else {
                data->misses++;
            }
        }
        data->ops_completed++;
    }

    data->time_ms = get_time_ms() - start;
    return NULL;
}

static void benchmark_multithread_cache(void) {
    cache_benchmark_stats_t stats = {0};
    cache_manager_t cache;
    pthread_t threads[THREAD_COUNT];
    cache_thread_data_t thread_data[THREAD_COUNT];
    char key[KEY_SIZE];
    char value[VALUE_SIZE];

    printf("\n[BENCH] Multi-threaded Cache Performance (%d threads)...\n", THREAD_COUNT);

    // Инициализация кэша
    cache_config_t config = {0};
    config.type = CACHE_TYPE_MEMORY;
    config.policy = CACHE_LRU;
    config.max_entries = 50000;
    config.max_size_mb = 256;
    config.default_ttl_sec = 3600;
    config.enable_statistics = 1;
    config.enable_locking = 1;  // Thread-safe

    if (cache_manager_init(&cache, &config) != 0) {
        printf("  ERROR: Failed to initialize cache manager\n");
        return;
    }

    // Предварительное заполнение
    for (int i = 0; i < 10000; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        cache_manager_set(&cache, key, value, strlen(value) + 1);
    }

    size_t ops_per_thread = 100000 / THREAD_COUNT;
    double start = get_time_ms();

    // Создание потоков
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].cache = &cache;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = ops_per_thread;
        thread_data[i].hits = 0;
        thread_data[i].misses = 0;
        thread_data[i].ops_completed = 0;
        thread_data[i].time_ms = 0;

        pthread_create(&threads[i], NULL, cache_bench_worker, &thread_data[i]);
    }

    // Ожидание потоков
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
        stats.hits += thread_data[i].hits;
        stats.misses += thread_data[i].misses;
        stats.total_operations += thread_data[i].ops_completed;
    }

    double end = get_time_ms();
    double total_time = end - start;

    stats.total_time_ms = total_time;
    stats.ops_per_sec = stats.total_operations / (total_time / 1000.0);
    stats.avg_time_per_op_us = (total_time / stats.total_operations) * 1000.0;
    stats.hit_rate = (stats.hits * 100.0) / (stats.hits + stats.misses);

    print_cache_stats("Multi-thread cache (8 threads)", &stats);
    printf("  Thread count:    %12d\n", THREAD_COUNT);
    printf("  Ops per thread:  %12zu\n", ops_per_thread);

    cache_manager_cleanup(&cache);
    g_stats = stats;
}

/* ============================================================================
 * Test 6: 500K Cache Operations (Stress Test)
 * ============================================================================ */

static void benchmark_500k_cache_stress(void) {
    cache_benchmark_stats_t stats = {0};
    cache_manager_t cache;
    double start, end;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    size_t value_len;

    printf("\n[BENCH] 500K Cache Stress Test...\n");

    // Инициализация кэша с ограниченным размером (для evictions)
    cache_config_t config = {0};
    config.type = CACHE_TYPE_MEMORY;
    config.policy = CACHE_LRU;
    config.max_entries = 10000;  // Ограничено для тестирования evictions
    config.max_size_mb = 32;
    config.default_ttl_sec = 3600;
    config.enable_statistics = 1;

    if (cache_manager_init(&cache, &config) != 0) {
        printf("  ERROR: Failed to initialize cache manager\n");
        return;
    }

    start = get_time_ms();

    for (int i = 0; i < BENCH_500K; i++) {
        snprintf(key, sizeof(key), "key_%d", i % 50000);
        snprintf(value, sizeof(value), "value_%d_stress_test_data", i);

        if (i % 3 == 0) {
            // SET
            cache_manager_set(&cache, key, value, strlen(value) + 1);
        } else if (i % 3 == 1) {
            // GET
            if (cache_manager_get(&cache, key, value, sizeof(value), &value_len) == CACHE_OK) {
                stats.hits++;
            } else {
                stats.misses++;
            }
        } else {
            // DELETE (каждая 3-я операция)
            if (i > 1000) {  // Не удалять сразу
                cache_delete(&cache, key);
            }
        }
        stats.total_operations++;
    }

    end = get_time_ms();
    double total_time = end - start;

    stats.total_time_ms = total_time;
    stats.ops_per_sec = stats.total_operations / (total_time / 1000.0);
    stats.avg_time_per_op_us = (total_time / stats.total_operations) * 1000.0;
    stats.bytes_processed = stats.total_operations * (KEY_SIZE + VALUE_SIZE);
    stats.throughput_mbps = (stats.bytes_processed / 1024.0 / 1024.0) / (total_time / 1000.0);
    stats.hit_rate = (stats.hits * 100.0) / (stats.hits + stats.misses);

    // Получение статистики
    cache_stats_t cache_stats = {0};
    cache_manager_get_stats(&cache, &cache_stats);

    print_cache_stats("500K cache stress test", &stats);
    printf("  Evictions:       %12lld\n", cache_stats.evictions);

    cache_manager_cleanup(&cache);
    g_stats = stats;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║        MTProxy Cache Performance Benchmark Suite             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Configuration:\n");
    printf("  - 100K = %d operations\n", BENCH_100K);
    printf("  - 500K = %d operations\n", BENCH_500K);
    printf("  - Threads: %d\n", THREAD_COUNT);
    printf("  - Key size: %d bytes\n", KEY_SIZE);
    printf("  - Value size: %d bytes\n", VALUE_SIZE);
    printf("\n");

    // Run all benchmarks
    benchmark_100k_cache_set_lru();
    benchmark_100k_cache_get_lru();
    benchmark_100k_cache_mixed();
    benchmark_cache_algorithms_comparison();
    benchmark_multithread_cache();
    benchmark_500k_cache_stress();

    // Summary
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    Benchmark Complete                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
