/*
    Упрощенная система оптимизации производительности MTProxy
    Без зависимостей от стандартной библиотеки C
*/

#ifndef SIMPLE_PERFORMANCE_OPTIMIZER_H
#define SIMPLE_PERFORMANCE_OPTIMIZER_H

// Базовые типы
typedef unsigned int uint32_t;
typedef unsigned long long size_t;
typedef long long time_t;
typedef int pthread_t;

// Конфигурация
#define DEFAULT_THREAD_POOL_SIZE 8
#define MAX_THREAD_POOL_SIZE 64
#define MEMORY_POOL_SIZE (64 * 1024 * 1024)
#define CPU_CACHE_LINE_SIZE 64

// Статусы
typedef enum {
    OPTIMIZATION_STATUS_DISABLED = 0,
    OPTIMIZATION_STATUS_ENABLED,
    OPTIMIZATION_STATUS_ACTIVE,
    OPTIMIZATION_STATUS_ERROR
} optimization_status_t;

// Основная структура
typedef struct simple_perf_optimizer {
    // Конфигурация
    int enable_numa_optimization;
    int enable_memory_pooling;
    int enable_cpu_affinity;
    int thread_pool_size;
    size_t memory_pool_size;
    
    // Статистика
    long long total_connections;
    long long total_bytes_processed;
    long long active_threads;
    long long memory_used;
    double avg_processing_time;
    
    // Статус
    optimization_status_t overall_status;
    long long start_time;
    
    // Метрики производительности
    double cpu_usage_percent;
    long long packets_processed;
    long long bytes_throughput;
    double latency_us;
} simple_perf_optimizer_t;

// Инициализация
simple_perf_optimizer_t* simple_perf_init(void);
int simple_perf_configure(simple_perf_optimizer_t *opt, int thread_count, size_t mem_pool_size);
void simple_perf_cleanup(simple_perf_optimizer_t *opt);

// Оптимизации
int simple_perf_apply_cpu_affinity(simple_perf_optimizer_t *opt);
int simple_perf_enable_memory_pooling(simple_perf_optimizer_t *opt);
int simple_perf_optimize_connection_distribution(simple_perf_optimizer_t *opt);

// Мониторинг
void simple_perf_collect_metrics(simple_perf_optimizer_t *opt);
void simple_perf_get_report(simple_perf_optimizer_t *opt, char *buffer, size_t buffer_size);
int simple_perf_is_degraded(simple_perf_optimizer_t *opt);

// Утилиты
uint32_t simple_perf_hash_connection(uint32_t connection_id);
int simple_perf_get_cpu_count(void);
double simple_perf_get_time_ms(void);
void simple_perf_sleep_ms(int milliseconds);

// Статистика
void simple_perf_print_stats(simple_perf_optimizer_t *opt);
void simple_perf_reset_stats(simple_perf_optimizer_t *opt);

#endif // SIMPLE_PERFORMANCE_OPTIMIZER_H