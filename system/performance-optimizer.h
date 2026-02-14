/*
    Система оптимизации производительности MTProxy
    Содержит NUMA-память, memory pooling, CPU affinity и мониторинг
*/

#ifndef PERFORMANCE_OPTIMIZER_H
#define PERFORMANCE_OPTIMIZER_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация производительности
#define DEFAULT_THREAD_POOL_SIZE 8
#define MAX_THREAD_POOL_SIZE 64
#define MEMORY_POOL_SIZE (64 * 1024 * 1024)  // 64MB
#define BUFFER_POOL_SIZE (32 * 1024 * 1024)   // 32MB
#define MAX_CONNECTIONS_PER_THREAD 1000
#define CPU_CACHE_LINE_SIZE 64

// Типы оптимизаций
typedef enum {
    OPTIMIZATION_TYPE_NONE = 0,
    OPTIMIZATION_TYPE_NUMA_MEMORY,
    OPTIMIZATION_TYPE_MEMORY_POOLING,
    OPTIMIZATION_TYPE_CPU_AFFINITY,
    OPTIMIZATION_TYPE_CONNECTION_SHARDING,
    OPTIMIZATION_TYPE_ZERO_COPY,
    OPTIMIZATION_TYPE_ASYNC_IO
} optimization_type_t;

// Статус оптимизации
typedef enum {
    OPTIMIZATION_STATUS_DISABLED = 0,
    OPTIMIZATION_STATUS_ENABLED,
    OPTIMIZATION_STATUS_ACTIVE,
    OPTIMIZATION_STATUS_ERROR
} optimization_status_t;

// NUMA информация
typedef struct numa_info {
    int node_count;
    int current_node;
    size_t node_memory_size;
    int *cpu_list;
    int cpu_count;
} numa_info_t;

// Memory pool
typedef struct memory_pool {
    void *memory_start;
    size_t pool_size;
    size_t used_size;
    size_t block_size;
    int *free_blocks;
    int free_block_count;
    optimization_status_t status;
} memory_pool_t;

// Thread pool
typedef struct thread_worker {
    int thread_id;
    int cpu_core;
    int numa_node;
    void *work_queue;
    optimization_status_t status;
    long long processed_connections;
    long long processed_bytes;
} thread_worker_t;

typedef struct thread_pool {
    thread_worker_t *workers;
    int worker_count;
    int active_workers;
    optimization_status_t status;
    // Статистика
    long long total_connections;
    long long total_bytes_processed;
    double avg_processing_time;
} thread_pool_t;

// Connection sharding
typedef struct connection_shard {
    int shard_id;
    int thread_id;
    uint32_t connection_hash;
    int connection_count;
    optimization_status_t status;
} connection_shard_t;

// Performance metrics
typedef struct performance_metrics {
    // CPU metrics
    double cpu_usage_percent;
    double cpu_frequency_mhz;
    int cache_misses;
    
    // Memory metrics
    size_t memory_used_bytes;
    size_t memory_cached_bytes;
    int page_faults;
    
    // Network metrics
    long long packets_processed;
    long long bytes_throughput;
    double latency_us;
    
    // Thread metrics
    int context_switches;
    int lock_contentions;
    
    // NUMA metrics
    long long numa_local_accesses;
    long long numa_remote_accesses;
} performance_metrics_t;

// Основная структура оптимизатора
typedef struct performance_optimizer {
    // Конфигурация
    int enable_numa_optimization;
    int enable_memory_pooling;
    int enable_cpu_affinity;
    int thread_pool_size;
    size_t memory_pool_size;
    
    // Компоненты
    numa_info_t numa_info;
    memory_pool_t *memory_pools;
    thread_pool_t thread_pool;
    connection_shard_t *connection_shards;
    performance_metrics_t metrics;
    
    // Статус
    optimization_status_t overall_status;
    long long optimization_start_time;
    
    // Статистика
    long long total_optimizations_applied;
    long long performance_improvements;
} performance_optimizer_t;

// Инициализация и управление
performance_optimizer_t* perf_optimizer_init(void);
int perf_optimizer_configure(performance_optimizer_t *opt, int thread_count, size_t mem_pool_size);
void perf_optimizer_cleanup(performance_optimizer_t *opt);

// NUMA оптимизации
int perf_detect_numa_topology(performance_optimizer_t *opt);
int perf_bind_thread_to_numa_node(int thread_id, int numa_node);
void* perf_numa_malloc(size_t size, int numa_node);
void perf_numa_free(void *ptr, size_t size);

// Memory pooling
memory_pool_t* perf_create_memory_pool(size_t pool_size, size_t block_size);
void* perf_pool_alloc(memory_pool_t *pool, size_t size);
void perf_pool_free(memory_pool_t *pool, void *ptr);
int perf_pool_stats(memory_pool_t *pool, char *buffer, size_t buffer_size);

// Thread pooling
int perf_create_thread_pool(performance_optimizer_t *opt, int thread_count);
int perf_assign_work_to_thread(thread_pool_t *pool, void *work_item);
int perf_get_thread_load_balance(thread_pool_t *pool);
void perf_thread_pool_stats(thread_pool_t *pool, char *buffer, size_t buffer_size);

// Connection sharding
int perf_init_connection_sharding(performance_optimizer_t *opt, int shard_count);
int perf_get_shard_for_connection(uint32_t connection_id, int total_shards);
int perf_distribute_connections(performance_optimizer_t *opt);

// Мониторинг производительности
void perf_collect_metrics(performance_optimizer_t *opt);
void perf_get_performance_report(performance_optimizer_t *opt, char *buffer, size_t buffer_size);
int perf_is_performance_degraded(performance_optimizer_t *opt);

// Оптимизации
int perf_apply_cpu_affinity(performance_optimizer_t *opt);
int perf_enable_zero_copy_operations(performance_optimizer_t *opt);
int perf_optimize_memory_access_patterns(performance_optimizer_t *opt);
int perf_tune_network_buffers(performance_optimizer_t *opt);

// Утилиты
uint32_t perf_hash_connection_id(uint32_t connection_id);
int perf_get_cpu_count(void);
int perf_get_numa_node_count(void);
double perf_get_current_time_ms(void);
void perf_sleep_ms(int milliseconds);

// Статистика и отладка
void perf_print_detailed_stats(performance_optimizer_t *opt);
void perf_reset_statistics(performance_optimizer_t *opt);
int perf_export_metrics_to_file(performance_optimizer_t *opt, const char *filename);

#endif // PERFORMANCE_OPTIMIZER_H