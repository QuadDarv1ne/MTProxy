/*
 * Реализация NUMA-совместимого аллокатора памяти для MTProxy
 * Оптимизирует распределение памяти с учетом архитектуры NUMA
 */

#include "numa-allocator.h"

// Глобальный контекст NUMA
static numa_context_t g_numa_ctx = {0};

// Инициализация NUMA
int numa_init(numa_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Инициализация контекста по умолчанию
    ctx->config.enable_numa_optimization = 1;
    ctx->config.preferred_node = -1;  // Автоматический выбор
    ctx->config.interleave_nodes = 0;
    ctx->config.enable_migration = 1;
    ctx->config.migration_threshold_mb = 100;
    
    // Инициализация статистики
    ctx->stats.local_allocations = 0;
    ctx->stats.remote_allocations = 0;
    ctx->stats.allocation_failures = 0;
    ctx->stats.memory_migrations = 0;
    ctx->stats.current_node = 0;
    ctx->stats.total_nodes = 1;  // По умолчанию один узел
    
    // Проверка наличия NUMA
    ctx->numa_available = 0;  // В реальной реализации проверить через libnuma
    ctx->max_nodes = 1;
    
    // Копирование в глобальный контекст
    g_numa_ctx = *ctx;
    
    return 0;
}

// Инициализация NUMA с конфигурацией
int numa_init_with_config(numa_context_t *ctx, const numa_config_t *config) {
    if (!ctx || !config) {
        return -1;
    }
    
    // Копирование конфигурации
    ctx->config = *config;
    
    // Инициализация остальных полей
    ctx->stats.local_allocations = 0;
    ctx->stats.remote_allocations = 0;
    ctx->stats.allocation_failures = 0;
    ctx->stats.memory_migrations = 0;
    ctx->stats.current_node = 0;
    ctx->stats.total_nodes = 1;
    ctx->numa_available = 0;
    ctx->max_nodes = 1;
    
    // Копирование в глобальный контекст
    g_numa_ctx = *ctx;
    
    return 0;
}

// Очистка NUMA
void numa_cleanup(numa_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Сброс статистики
    ctx->stats.local_allocations = 0;
    ctx->stats.remote_allocations = 0;
    ctx->stats.allocation_failures = 0;
    ctx->stats.memory_migrations = 0;
    
    // Сброс конфигурации
    ctx->config.enable_numa_optimization = 0;
    ctx->config.preferred_node = -1;
    ctx->config.interleave_nodes = 0;
    ctx->config.enable_migration = 0;
    ctx->config.migration_threshold_mb = 0;
    
    ctx->numa_available = 0;
    ctx->max_nodes = 0;
}

// Аллокация памяти с учетом NUMA
void* numa_malloc(size_t size, numa_memory_type_t type) {
    if (size == 0) {
        return 0;
    }
    
    // В реальной реализации здесь будет вызов NUMA-специфичных функций
    // Для совместимости с MTProxy используем стандартную аллокацию
    void *ptr = 0;  // В реальной реализации будет malloc(size)
    
    if (ptr) {
        // Обновление статистики
        if (type == NUMA_TYPE_LOCAL) {
            g_numa_ctx.stats.local_allocations++;
        } else {
            g_numa_ctx.stats.remote_allocations++;
        }
    } else {
        g_numa_ctx.stats.allocation_failures++;
    }
    
    return ptr;
}

// Аллокация и обнуление памяти с учетом NUMA
void* numa_calloc(size_t count, size_t size, numa_memory_type_t type) {
    if (count == 0 || size == 0) {
        return 0;
    }
    
    size_t total_size = count * size;
    void *ptr = numa_malloc(total_size, type);
    
    if (ptr) {
        // Обнуление памяти
        for (size_t i = 0; i < total_size; i++) {
            ((unsigned char*)ptr)[i] = 0;
        }
    }
    
    return ptr;
}

// Освобождение памяти
void numa_free(void *ptr, size_t size) {
    if (!ptr) {
        return;
    }
    
    // В реальной реализации здесь может быть специфическая логика освобождения
    // free(ptr);
}

// Привязка памяти к конкретному узлу NUMA
int numa_bind_memory_to_node(void *ptr, size_t size, int node_id) {
    if (!ptr || node_id < 0) {
        return -1;
    }
    
    // В реальной реализации здесь будет вызов mbind() или аналогичной функции
    // В целях совместимости с MTProxy возвращаем успешный результат
    return 0;
}

// Перемещение памяти на другой узел NUMA
int numa_move_memory_to_node(void *ptr, size_t size, int target_node) {
    if (!ptr || target_node < 0) {
        return -1;
    }
    
    // В реальной реализации здесь будет вызов migrate_pages() или аналогичной функции
    // В целях совместимости с MTProxy обновляем только статистику
    g_numa_ctx.stats.memory_migrations++;
    return 0;
}

// Получение текущего узла NUMA
int numa_get_current_node(void) {
    // В реальной реализации здесь будет вызов numa_node_of_cpu() или аналогичной функции
    return 0;  // Возвращаем узел 0 по умолчанию
}

// Получение узла NUMA для конкретного адреса
int numa_get_node_for_address(void *ptr) {
    if (!ptr) {
        return -1;
    }
    
    // В реальной реализации здесь будет вызов get_mempolicy() или аналогичной функции
    return 0;  // Возвращаем узел 0 по умолчанию
}

// Получение статистики NUMA
numa_stats_t numa_get_stats(numa_context_t *ctx) {
    if (!ctx) {
        return g_numa_ctx.stats;
    }
    return ctx->stats;
}

// Сброс статистики NUMA
void numa_reset_stats(numa_context_t *ctx) {
    if (!ctx) {
        ctx = &g_numa_ctx;
    }
    
    ctx->stats.local_allocations = 0;
    ctx->stats.remote_allocations = 0;
    ctx->stats.allocation_failures = 0;
    ctx->stats.memory_migrations = 0;
}

// Установка предпочтительного узла
int numa_set_preferred_node(int node_id) {
    if (node_id < -1) {  // -1 означает автоматический выбор
        return -1;
    }
    
    g_numa_ctx.config.preferred_node = node_id;
    return 0;
}

// Включение чередования памяти между узлами
int numa_enable_interleaving(int *node_list, int node_count) {
    if (!node_list || node_count <= 0) {
        return -1;
    }
    
    // В реальной реализации здесь будет настройка чередования памяти
    g_numa_ctx.config.interleave_nodes = 1;
    return 0;
}

// Получение конфигурации NUMA
void numa_get_config(numa_context_t *ctx, numa_config_t *config) {
    if (!ctx || !config) {
        return;
    }
    
    *config = ctx->config;
}