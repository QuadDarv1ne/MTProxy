/*
 * Продвинутый менеджер оптимизации для MTProxy
 * Интеграция NUMA, io_uring и DPDK для максимальной производительности
 */

#ifndef _ADVANCED_OPTIMIZER_H_
#define _ADVANCED_OPTIMIZER_H_

#include <stdint.h>
#include "numa-allocator.h"
#include "io-uring-interface.h"
#include "dpdk-interface.h"

// Уровни оптимизации
typedef enum {
    OPTIMIZATION_LEVEL_BASIC = 0,      // Базовая оптимизация
    OPTIMIZATION_LEVEL_STANDARD = 1,   // Стандартная оптимизация
    OPTIMIZATION_LEVEL_ADVANCED = 2,   // Продвинутая оптимизация
    OPTIMIZATION_LEVEL_MAXIMUM = 3     // Максимальная оптимизация
} optimization_level_t;

// Типы оптимизаций
typedef enum {
    OPTIMIZATION_TYPE_NONE = 0,
    OPTIMIZATION_TYPE_NUMA = 1,
    OPTIMIZATION_TYPE_IO_URING = 2,
    OPTIMIZATION_TYPE_DPDK = 4,
    OPTIMIZATION_TYPE_ALL = 7
} optimization_type_t;

// Статистика продвинутой оптимизации
typedef struct {
    long long total_optimizations_applied;
    long long numa_optimizations;
    long long io_uring_operations;
    long long dpdk_packets_processed;
    long long performance_improvements;
    long long resource_efficiency_gains;
    optimization_level_t current_level;
    int optimizations_enabled;
} advanced_optimization_stats_t;

// Конфигурация продвинутой оптимизации
typedef struct {
    optimization_level_t optimization_level;
    int enable_numa_optimization;
    int enable_io_uring;
    int enable_dpdk;
    int auto_tuning_enabled;
    int performance_monitoring_enabled;
    int resource_efficiency_target;
    int cpu_affinity_enabled;
    int memory_pool_size_mb;
    int connection_pool_size;
} advanced_optimization_config_t;

// Контекст продвинутой оптимизации
typedef struct {
    advanced_optimization_config_t config;
    advanced_optimization_stats_t stats;
    numa_context_t numa_ctx;
    io_uring_context_t io_uring_ctx;
    dpdk_context_t dpdk_ctx;
    int initialized;
    int performance_score;
    int resource_utilization;
    int stability_score;
} advanced_optimizer_context_t;

// Структура для метрик производительности
typedef struct {
    long long cpu_usage_percent;
    long long memory_usage_mb;
    long long network_throughput_mbps;
    long long connection_count;
    long long packet_loss_rate;
    long long latency_ms;
    int optimization_recommendations;
    int stability_indicator;
} performance_metrics_t;

// Функции инициализации
int advanced_optimizer_init(advanced_optimizer_context_t *ctx);
int advanced_optimizer_init_with_config(advanced_optimizer_context_t *ctx, 
                                       const advanced_optimization_config_t *config);
void advanced_optimizer_cleanup(advanced_optimizer_context_t *ctx);

// Функции управления оптимизациями
int advanced_optimizer_enable_optimization(advanced_optimizer_context_t *ctx, 
                                          optimization_type_t opt_type);
int advanced_optimizer_disable_optimization(advanced_optimizer_context_t *ctx, 
                                           optimization_type_t opt_type);
int advanced_optimizer_set_level(advanced_optimizer_context_t *ctx, 
                                optimization_level_t level);
optimization_level_t advanced_optimizer_get_current_level(advanced_optimizer_context_t *ctx);

// Функции работы с производительностью
int advanced_optimizer_monitor_performance(advanced_optimizer_context_t *ctx, 
                                          performance_metrics_t *metrics);
int advanced_optimizer_auto_tune(advanced_optimizer_context_t *ctx);
int advanced_optimizer_apply_recommendations(advanced_optimizer_context_t *ctx);

// Функции NUMA-оптимизации
int advanced_optimizer_bind_thread_to_numa_node(advanced_optimizer_context_t *ctx, 
                                               int thread_id, int node_id);
int advanced_optimizer_optimize_memory_allocation(advanced_optimizer_context_t *ctx, 
                                                 size_t size, void **ptr);
int advanced_optimizer_get_optimal_numa_node(advanced_optimizer_context_t *ctx, 
                                            int thread_id);

// Функции io_uring-оптимизации
int advanced_optimizer_setup_async_io(advanced_optimizer_context_t *ctx);
int advanced_optimizer_submit_async_operation(advanced_optimizer_context_t *ctx, 
                                             io_uring_operation_t op_type,
                                             int fd, void *buffer, size_t size);
int advanced_optimizer_process_completed_operations(advanced_optimizer_context_t *ctx, 
                                                   int timeout_ms);

// Функции DPDK-оптимизации
int advanced_optimizer_init_dpdk_networking(advanced_optimizer_context_t *ctx);
int advanced_optimizer_process_network_packets(advanced_optimizer_context_t *ctx, 
                                              int max_packets);
int advanced_optimizer_optimize_packet_processing(advanced_optimizer_context_t *ctx);

// Функции статистики
advanced_optimization_stats_t advanced_optimizer_get_stats(advanced_optimizer_context_t *ctx);
void advanced_optimizer_reset_stats(advanced_optimizer_context_t *ctx);

// Функции конфигурации
void advanced_optimizer_get_config(advanced_optimizer_context_t *ctx, 
                                  advanced_optimization_config_t *config);
int advanced_optimizer_update_config(advanced_optimizer_context_t *ctx, 
                                    const advanced_optimization_config_t *new_config);

// Вспомогательные функции
int advanced_optimizer_is_available(void);
int advanced_optimizer_get_performance_score(advanced_optimizer_context_t *ctx);
int advanced_optimizer_get_stability_score(advanced_optimizer_context_t *ctx);
const char* advanced_optimizer_get_level_string(optimization_level_t level);

#endif