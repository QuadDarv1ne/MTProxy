/*
 * Система оптимизации использования памяти для MTProxy
 * Различные стратегии выделения и управления памятью
 */

#ifndef _MEMORY_OPTIMIZER_H_
#define _MEMORY_OPTIMIZER_H_

#include <stdint.h>

// Стратегии выделения памяти
typedef enum {
    MEM_STRATEGY_DEFAULT = 0,        // Стандартное выделение
    MEM_STRATEGY_POOL = 1,           // Пул памяти
    MEM_STRATEGY_MMAP = 2,           // Использование mmap
    MEM_STRATEGY_NUMA_AWARE = 3,     // NUMA-ориентированное выделение
    MEM_STRATEGY_HUGEPAGE = 4,       // Использование больших страниц
    MEM_STRATEGY_CACHE_FRIENDLY = 5  // Кэш-эффективное выделение
} memory_allocation_strategy_t;

// Состояния оптимизатора памяти
typedef enum {
    MEM_OPT_STATUS_UNINITIALIZED = 0,
    MEM_OPT_STATUS_INITIALIZED = 1,
    MEM_OPT_STATUS_ACTIVE = 2,
    MEM_OPT_STATUS_ERROR = 3
} memory_optimizer_status_t;

// Типы памяти
typedef enum {
    MEM_TYPE_REGULAR = 0,     // Обычная память
    MEM_TYPE_TEMPORARY = 1,   // Временная память
    MEM_TYPE_PERMANENT = 2,   // Постоянная память
    MEM_TYPE_BUFFER = 3,      // Буферная память
    MEM_TYPE_SHARED = 4       // Разделяемая память
} memory_type_t;

// Статистика оптимизации памяти
typedef struct {
    long long allocations_count;
    long long deallocations_count;
    long long current_allocated_bytes;
    long long peak_allocated_bytes;
    long long saved_bytes;
    long long reused_buffers_count;
    long long fragmentation_events;
    memory_optimizer_status_t current_status;
    memory_allocation_strategy_t current_strategy;
    double memory_efficiency_ratio;
    double fragmentation_percent;
} memory_optimizer_stats_t;

// Конфигурация оптимизатора памяти
typedef struct {
    int enable_memory_optimization;
    memory_allocation_strategy_t default_strategy;
    int enable_pool_allocator;
    int enable_large_pages;
    int enable_numa_awareness;
    int enable_cache_friendly_alloc;
    int pool_initial_size;
    int pool_max_size;
    int pool_growth_step;
    int buffer_preallocation_size;
    int enable_buffer_reuse;
    int buffer_recycling_threshold;
    int enable_fragmentation_control;
    int fragmentation_check_interval;
    int memory_limit_mb;
    int gc_enabled;
    int gc_interval_ms;
} memory_optimizer_config_t;

// Контекст оптимизатора памяти
typedef struct {
    memory_optimizer_config_t config;
    memory_optimizer_stats_t stats;
    memory_optimizer_status_t status;
    memory_allocation_strategy_t current_strategy;
    void *allocator_handle;
    void *memory_pools;
    void *buffer_manager;
    int initialized;
    long long last_gc_time;
    int num_threads;
    int numa_nodes_count;
    int *numa_node_mapping;
    uint64_t *thread_local_pools;
} memory_optimizer_context_t;

// Блок информации о выделенной памяти
typedef struct {
    void *ptr;
    size_t size;
    memory_type_t mem_type;
    memory_allocation_strategy_t alloc_strategy;
    long long allocation_time;
    int thread_id;
    int is_free;
    uint32_t checksum;
} memory_block_info_t;

// Функции инициализации
int mem_opt_init(memory_optimizer_context_t *ctx);
int mem_opt_init_with_config(memory_optimizer_context_t *ctx, 
                           const memory_optimizer_config_t *config);
void mem_opt_cleanup(memory_optimizer_context_t *ctx);

// Функции выделения памяти
void* mem_opt_malloc(size_t size);
void* mem_opt_calloc(size_t nmemb, size_t size);
void* mem_opt_realloc(void *ptr, size_t size);
void mem_opt_free(void *ptr);

// Функции выделения с указанием стратегии
void* mem_opt_malloc_with_strategy(size_t size, 
                                 memory_allocation_strategy_t strategy);
void* mem_opt_malloc_with_type(size_t size, 
                             memory_type_t mem_type);

// Функции управления пулами памяти
int mem_opt_create_memory_pool(size_t initial_size, size_t max_size);
void* mem_opt_pool_alloc(size_t size);
void mem_opt_pool_free(void *ptr);
int mem_opt_destroy_memory_pool(void);

// Функции управления буферами
void* mem_opt_buffer_acquire(size_t size);
int mem_opt_buffer_release(void *buffer);
int mem_opt_buffer_recycle(void *buffer);
int mem_opt_buffer_resize(void *buffer, size_t new_size);

// Функции NUMA-оптимизации
int mem_opt_bind_to_numa_node(int node_id);
int mem_opt_allocate_on_node(size_t size, int node_id);
int mem_opt_get_current_numa_node(void);

// Функции контроля фрагментации
int mem_opt_defragment_memory(void);
int mem_opt_check_fragmentation(void);
double mem_opt_get_fragmentation_level(void);

// Функции сборки мусора
int mem_opt_run_garbage_collection(void);
int mem_opt_enable_garbage_collection(int enable);
int mem_opt_set_gc_interval(int interval_ms);

// Функции статистики
memory_optimizer_stats_t mem_opt_get_stats(memory_optimizer_context_t *ctx);
void mem_opt_reset_stats(memory_optimizer_context_t *ctx);
void mem_opt_print_stats(void);

// Функции конфигурации
void mem_opt_get_config(memory_optimizer_context_t *ctx, 
                      memory_optimizer_config_t *config);
int mem_opt_update_config(memory_optimizer_context_t *ctx, 
                        const memory_optimizer_config_t *new_config);

// Вспомогательные функции
int mem_opt_is_available(void);
memory_allocation_strategy_t mem_opt_get_default_strategy(void);
int mem_opt_set_default_strategy(memory_allocation_strategy_t strategy);
double mem_opt_get_efficiency_ratio(void);
int mem_opt_estimate_memory_savings(size_t requested_size);

#endif