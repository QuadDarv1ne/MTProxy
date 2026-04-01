/*
 * Комплексный оптимизатор производительности для MTProxy
 * Интеграция векторизованной криптографии, адаптивного пула соединений и оптимизации памяти
 */

#ifndef _PERFORMANCE_OPTIMIZER_H_
#define _PERFORMANCE_OPTIMIZER_H_

#include "memory-optimizer.h"
#include "../crypto/vectorized-crypto.h"
#include "../conn_pool/adaptive-connection-pool.h"

// Состояния комплексного оптимизатора
typedef enum {
    PERF_OPT_STATUS_UNINITIALIZED = 0,
    PERF_OPT_STATUS_INITIALIZED = 1,
    PERF_OPT_STATUS_ACTIVE = 2,
    PERF_OPT_STATUS_PAUSED = 3,
    PERF_OPT_STATUS_ERROR = 4
} performance_optimizer_status_t;

// Компоненты оптимизации
typedef enum {
    PERF_COMPONENT_VECTOR_CRYPTO = 0,
    PERF_COMPONENT_CONN_POOL = 1,
    PERF_COMPONENT_MEMORY_OPT = 2,
    PERF_COMPONENT_ALL = 3
} performance_component_t;

// Уровни оптимизации
typedef enum {
    PERF_LEVEL_OFF = 0,      // Без оптимизации
    PERF_LEVEL_BASIC = 1,    // Базовая оптимизация
    PERF_LEVEL_ADVANCED = 2, // Продвинутая оптимизация
    PERF_LEVEL_MAX = 3       // Максимальная оптимизация
} performance_level_t;

// Статистика комплексного оптимизатора
typedef struct {
    long long total_performance_improvements;
    long long vector_crypto_improvements;
    long long conn_pool_improvements;
    long long memory_opt_improvements;
    double overall_performance_gain_percent;
    performance_optimizer_status_t current_status;
    performance_level_t current_level;
    int active_components;
    long long last_optimization_time;
    double cpu_utilization_before;
    double cpu_utilization_after;
    double memory_utilization_before;
    double memory_utilization_after;
} performance_optimizer_stats_t;

// Конфигурация комплексного оптимизатора
typedef struct {
    int enable_vectorized_crypto;
    int enable_adaptive_connection_pool;
    int enable_memory_optimization;
    performance_level_t optimization_level;
    int enable_auto_tuning;
    int auto_tune_interval_ms;
    int enable_monitoring;
    int monitoring_interval_ms;
    int enable_profiling;
    int profile_output_interval_ms;
    int enable_component_cooperation;
    int cooperative_optimization;
    int enable_dynamic_adjustment;
    int dynamic_adjustment_interval_ms;
    double target_cpu_utilization;
    double target_memory_utilization;
    int enable_resource_sharing;
    int gc_enabled;
} performance_optimizer_config_t;

// Контекст комплексного оптимизатора
typedef struct {
    performance_optimizer_config_t config;
    performance_optimizer_stats_t stats;
    performance_optimizer_status_t status;
    memory_optimizer_context_t mem_opt_ctx;
    vectorized_crypto_context_t vec_crypto_ctx;
    connection_pool_context_t conn_pool_ctx;
    int initialized;
    long long last_tune_time;
    int active_components_count;
    performance_level_t current_level;
    int *component_statuses;
    void *monitoring_handle;
    void *profiling_handle;
    int monitoring_active;
    int profiling_active;
} performance_optimizer_context_t;

// Функции инициализации
int perf_opt_init(performance_optimizer_context_t *ctx);
int perf_opt_init_with_config(performance_optimizer_context_t *ctx, 
                            const performance_optimizer_config_t *config);
void perf_opt_cleanup(performance_optimizer_context_t *ctx);

// Функции управления компонентами
int perf_opt_enable_component(performance_optimizer_context_t *ctx, 
                           performance_component_t component, 
                           int enable);
int perf_opt_is_component_enabled(performance_optimizer_context_t *ctx, 
                               performance_component_t component);
int perf_opt_set_performance_level(performance_optimizer_context_t *ctx, 
                                performance_level_t level);

// Функции оптимизации
int perf_opt_run_optimization_cycle(performance_optimizer_context_t *ctx);
int perf_opt_auto_tune(performance_optimizer_context_t *ctx);
int perf_opt_dynamic_adjust(performance_optimizer_context_t *ctx);
int perf_opt_cooperative_optimization(performance_optimizer_context_t *ctx);

// Функции мониторинга и профилирования
int perf_opt_start_monitoring(performance_optimizer_context_t *ctx);
int perf_opt_stop_monitoring(performance_optimizer_context_t *ctx);
int perf_opt_start_profiling(performance_optimizer_context_t *ctx);
int perf_opt_stop_profiling(performance_optimizer_context_t *ctx);
int perf_opt_collect_performance_data(performance_optimizer_context_t *ctx);

// Функции статистики
performance_optimizer_stats_t perf_opt_get_stats(performance_optimizer_context_t *ctx);
void perf_opt_reset_stats(performance_optimizer_context_t *ctx);
void perf_opt_print_performance_report(performance_optimizer_context_t *ctx);

// Функции конфигурации
void perf_opt_get_config(performance_optimizer_context_t *ctx, 
                       performance_optimizer_config_t *config);
int perf_opt_update_config(performance_optimizer_context_t *ctx, 
                         const performance_optimizer_config_t *new_config);

// Вспомогательные функции
int perf_opt_is_available(void);
performance_level_t perf_opt_get_current_level(void);
double perf_opt_get_overall_performance_gain(void);
int perf_opt_calculate_optimization_score(performance_optimizer_context_t *ctx);

// Интеграционные функции
int perf_opt_integrate_with_mtproto(void);
int perf_opt_apply_optimizations(void);
int perf_opt_verify_optimizations(void);

#endif