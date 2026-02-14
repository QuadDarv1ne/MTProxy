/*
 * NUMA-совместимый аллокатор памяти для MTProxy
 * Оптимизирует распределение памяти с учетом архитектуры NUMA
 */

#ifndef _NUMA_ALLOCATOR_H_
#define _NUMA_ALLOCATOR_H_

#include <stdint.h>

// Типы памяти NUMA
typedef enum {
    NUMA_TYPE_LOCAL = 0,    // Локальная память текущего узла
    NUMA_TYPE_REMOTE = 1,   // Удаленная память других узлов
    NUMA_TYPE_INTERLEAVED = 2, // Чередующаяся память
    NUMA_TYPE_PREFERRED = 3    // Предпочтительная память
} numa_memory_type_t;

// Статистика NUMA
typedef struct {
    long long local_allocations;
    long long remote_allocations;
    long long allocation_failures;
    long long memory_migrations;
    int current_node;
    int total_nodes;
} numa_stats_t;

// Конфигурация NUMA
typedef struct {
    int enable_numa_optimization;
    int preferred_node;
    int interleave_nodes;
    int enable_migration;
    int migration_threshold_mb;
} numa_config_t;

// Контекст NUMA
typedef struct {
    numa_config_t config;
    numa_stats_t stats;
    int numa_available;
    int max_nodes;
} numa_context_t;

// Функции инициализации
int numa_init(numa_context_t *ctx);
int numa_init_with_config(numa_context_t *ctx, const numa_config_t *config);
void numa_cleanup(numa_context_t *ctx);

// Функции аллокации памяти
void* numa_malloc(size_t size, numa_memory_type_t type);
void* numa_calloc(size_t count, size_t size, numa_memory_type_t type);
void numa_free(void *ptr, size_t size);

// Функции управления памятью
int numa_bind_memory_to_node(void *ptr, size_t size, int node_id);
int numa_move_memory_to_node(void *ptr, size_t size, int target_node);
int numa_get_current_node(void);
int numa_get_node_for_address(void *ptr);

// Функции статистики
numa_stats_t numa_get_stats(numa_context_t *ctx);
void numa_reset_stats(numa_context_t *ctx);

// Функции конфигурации
int numa_set_preferred_node(int node_id);
int numa_enable_interleaving(int *node_list, int node_count);
void numa_get_config(numa_context_t *ctx, numa_config_t *config);

#endif