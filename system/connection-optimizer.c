/*
    Оптимизация соединений для MTProxy
    Пул соединений и оптимизация производительности
*/

#include "connection-optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Глобальный оптимизатор соединений
static connection_optimizer_t *g_conn_optimizer = NULL;

// Вспомогательные функции
static int initialize_connection_pool(connection_optimizer_t *optimizer);
static int initialize_memory_pool(connection_optimizer_t *optimizer);
static void cleanup_connection_pool(connection_optimizer_t *optimizer);
static void cleanup_memory_pool(connection_optimizer_t *optimizer);

// Инициализация
connection_optimizer_t* conn_opt_init(const conn_opt_config_t *config) {
    connection_optimizer_t *optimizer = (connection_optimizer_t*)malloc(sizeof(connection_optimizer_t));
    if (!optimizer) {
        return NULL;
    }
    
    // Обнуление структуры
    memset(optimizer, 0, sizeof(connection_optimizer_t));
    
    // Конфигурация по умолчанию
    optimizer->config.max_connections = 10000;
    optimizer->config.min_idle_connections = 10;
    optimizer->config.max_idle_connections = 100;
    optimizer->config.connection_timeout_sec = 300;
    optimizer->config.enable_keepalive = 1;
    optimizer->config.keepalive_interval_sec = 60;
    optimizer->config.memory_pool_size = 1024 * 1024 * 16; // 16MB
    optimizer->config.enable_compression = 1;
    optimizer->config.compression_threshold = 1024; // 1KB
    optimizer->config.enable_multiplexing = 1;
    
    // Применение пользовательской конфигурации
    if (config) {
        if (config->max_connections > 0) 
            optimizer->config.max_connections = config->max_connections;
        if (config->min_idle_connections >= 0) 
            optimizer->config.min_idle_connections = config->min_idle_connections;
        if (config->max_idle_connections > 0) 
            optimizer->config.max_idle_connections = config->max_idle_connections;
        if (config->connection_timeout_sec > 0) 
            optimizer->config.connection_timeout_sec = config->connection_timeout_sec;
        optimizer->config.enable_keepalive = config->enable_keepalive;
        if (config->keepalive_interval_sec > 0) 
            optimizer->config.keepalive_interval_sec = config->keepalive_interval_sec;
        if (config->memory_pool_size > 0) 
            optimizer->config.memory_pool_size = config->memory_pool_size;
        optimizer->config.enable_compression = config->enable_compression;
        if (config->compression_threshold > 0) 
            optimizer->config.compression_threshold = config->compression_threshold;
        optimizer->config.enable_multiplexing = config->enable_multiplexing;
    }
    
    // Инициализация пула соединений
    if (initialize_connection_pool(optimizer) != 0) {
        free(optimizer);
        return NULL;
    }
    
    // Инициализация пула памяти
    if (initialize_memory_pool(optimizer) != 0) {
        cleanup_connection_pool(optimizer);
        free(optimizer);
        return NULL;
    }
    
    optimizer->is_initialized = 1;
    optimizer->stats.init_time = 0; // будет реальное время
    
    g_conn_optimizer = optimizer;
    return optimizer;
}

static int initialize_connection_pool(connection_optimizer_t *optimizer) {
    if (!optimizer) return -1;
    
    optimizer->pool.connections = (connection_entry_t*)calloc(optimizer->config.max_connections, 
                                                           sizeof(connection_entry_t));
    if (!optimizer->pool.connections) {
        return -1;
    }
    
    optimizer->pool.total_capacity = optimizer->config.max_connections;
    optimizer->pool.active_count = 0;
    optimizer->pool.idle_count = 0;
    optimizer->pool.created_count = 0;
    optimizer->pool.reused_count = 0;
    
    // Инициализация очереди свободных соединений
    optimizer->pool.free_list = (int*)malloc(sizeof(int) * optimizer->config.max_connections);
    if (!optimizer->pool.free_list) {
        free(optimizer->pool.connections);
        return -1;
    }
    
    // Заполнение очереди свободных соединений
    for (int i = 0; i < optimizer->config.max_connections; i++) {
        optimizer->pool.free_list[i] = i;
    }
    optimizer->pool.free_count = optimizer->config.max_connections;
    
    return 0;
}

static int initialize_memory_pool(connection_optimizer_t *optimizer) {
    if (!optimizer) return -1;
    
    optimizer->memory_pool.base_ptr = malloc(optimizer->config.memory_pool_size);
    if (!optimizer->memory_pool.base_ptr) {
        return -1;
    }
    
    optimizer->memory_pool.size = optimizer->config.memory_pool_size;
    optimizer->memory_pool.used = 0;
    optimizer->memory_pool.alignment = 64; // 64-byte alignment for performance
    
    return 0;
}

// Очистка
void conn_opt_cleanup(connection_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    cleanup_connection_pool(optimizer);
    cleanup_memory_pool(optimizer);
    
    optimizer->is_initialized = 0;
    if (g_conn_optimizer == optimizer) {
        g_conn_optimizer = NULL;
    }
    
    free(optimizer);
}

static void cleanup_connection_pool(connection_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->pool.connections) return;
    
    free(optimizer->pool.connections);
    optimizer->pool.connections = NULL;
    
    if (optimizer->pool.free_list) {
        free(optimizer->pool.free_list);
        optimizer->pool.free_list = NULL;
    }
}

static void cleanup_memory_pool(connection_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->memory_pool.base_ptr) return;
    
    free(optimizer->memory_pool.base_ptr);
    optimizer->memory_pool.base_ptr = NULL;
}

// Управление соединениями
connection_entry_t* conn_opt_acquire_connection(connection_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->is_initialized) return NULL;
    
    // Попытка получить соединение из пула
    if (optimizer->pool.free_count > 0) {
        int idx = optimizer->pool.free_list[--optimizer->pool.free_count];
        connection_entry_t *conn = &optimizer->pool.connections[idx];
        
        // Сброс состояния соединения
        conn->id = idx;
        conn->state = CONN_STATE_ACTIVE;
        conn->last_used = 0; // будет реальное время
        conn->bytes_sent = 0;
        conn->bytes_received = 0;
        conn->reused_count++;
        
        optimizer->pool.active_count++;
        optimizer->pool.idle_count--;
        optimizer->pool.reused_count++;
        
        optimizer->stats.acquired_connections++;
        return conn;
    }
    
    // Если пул исчерпан, возвращаем NULL
    optimizer->stats.acquire_failures++;
    return NULL;
}

int conn_opt_release_connection(connection_optimizer_t *optimizer, connection_entry_t *conn) {
    if (!optimizer || !conn || !optimizer->is_initialized) return -1;
    
    // Проверка, принадлежит ли соединение этому пулу
    ptrdiff_t diff = conn - optimizer->pool.connections;
    if (diff < 0 || diff >= optimizer->pool.total_capacity) {
        return -1;
    }
    
    // Проверка состояния соединения
    if (conn->state != CONN_STATE_ACTIVE) {
        return -1;
    }
    
    // Возврат в пул
    conn->state = CONN_STATE_IDLE;
    conn->last_used = 0; // будет реальное время
    
    if (optimizer->pool.free_count < optimizer->pool.total_capacity) {
        optimizer->pool.free_list[optimizer->pool.free_count++] = conn->id;
        optimizer->pool.active_count--;
        optimizer->pool.idle_count++;
        
        optimizer->stats.released_connections++;
        return 0;
    }
    
    // Если очередь свободных соединений переполнена, закрываем соединение
    optimizer->stats.closed_connections++;
    return -1;
}

// Управление памятью
void* conn_opt_alloc(connection_optimizer_t *optimizer, size_t size) {
    if (!optimizer || !optimizer->is_initialized || size == 0) return NULL;
    
    // Выравнивание размера
    size_t aligned_size = (size + optimizer->memory_pool.alignment - 1) & 
                          ~(optimizer->memory_pool.alignment - 1);
    
    if (optimizer->memory_pool.used + aligned_size > optimizer->memory_pool.size) {
        // Если пул памяти исчерпан, используем malloc
        optimizer->stats.malloc_fallbacks++;
        return malloc(size);
    }
    
    void *ptr = (char*)optimizer->memory_pool.base_ptr + optimizer->memory_pool.used;
    optimizer->memory_pool.used += aligned_size;
    
    optimizer->stats.allocated_bytes += aligned_size;
    return ptr;
}

void conn_opt_free(connection_optimizer_t *optimizer, void *ptr, size_t size) {
    if (!optimizer || !ptr) return;
    
    // Проверяем, находится ли указатель в нашем пуле
    char *pool_start = (char*)optimizer->memory_pool.base_ptr;
    char *pool_end = pool_start + optimizer->memory_pool.size;
    char *ptr_char = (char*)ptr;
    
    if (ptr_char >= pool_start && ptr_char < pool_end) {
        // Это внутренний пул - ничего не делаем (мы не можем освободить конкретный блок)
        // В реальной реализации нужно использовать более сложный менеджер памяти
        optimizer->stats.freed_bytes += size;
    } else {
        // Это malloc-выделенная память - освобождаем
        free(ptr);
    }
}

// Многопоточная безопасность
int conn_opt_enable_thread_safety(connection_optimizer_t *optimizer) {
    if (!optimizer) return -1;
    
    optimizer->thread_safety_enabled = 1;
    return 0;
}

int conn_opt_disable_thread_safety(connection_optimizer_t *optimizer) {
    if (!optimizer) return -1;
    
    optimizer->thread_safety_enabled = 0;
    return 0;
}

// Статистика
void conn_opt_get_stats(connection_optimizer_t *optimizer, conn_opt_stats_t *stats) {
    if (!optimizer || !stats) return;
    
    *stats = optimizer->stats;
    
    // Добавляем дополнительные вычисляемые метрики
    if (stats->acquired_connections > 0) {
        stats->reuse_ratio = (double)optimizer->pool.reused_count / stats->acquired_connections;
    }
    
    stats->pool_utilization = (double)(optimizer->pool.total_capacity - optimizer->pool.free_count) / 
                              optimizer->pool.total_capacity;
    
    stats->idle_connections = optimizer->pool.idle_count;
    stats->active_connections = optimizer->pool.active_count;
    stats->memory_pool_usage = optimizer->memory_pool.used;
    stats->memory_pool_total = optimizer->memory_pool.size;
}

void conn_opt_print_stats(connection_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    conn_opt_stats_t stats;
    conn_opt_get_stats(optimizer, &stats);
    
    printf("=== Connection Optimizer Statistics ===\n");
    printf("Acquired connections: %lld\n", stats.acquired_connections);
    printf("Released connections: %lld\n", stats.released_connections);
    printf("Acquire failures: %lld\n", stats.acquire_failures);
    printf("Closed connections: %lld\n", stats.closed_connections);
    printf("Malloc fallbacks: %lld\n", stats.malloc_fallbacks);
    printf("Allocated bytes: %lld\n", stats.allocated_bytes);
    printf("Freed bytes: %lld\n", stats.freed_bytes);
    printf("Reuse ratio: %.2f\n", stats.reuse_ratio);
    printf("Pool utilization: %.2f%%\n", stats.pool_utilization * 100);
    printf("Active connections: %d\n", stats.active_connections);
    printf("Idle connections: %d\n", stats.idle_connections);
    printf("Memory pool usage: %zu/%zu bytes\n", 
           stats.memory_pool_usage, stats.memory_pool_total);
    printf("========================================\n");
}

// Утилиты
double conn_opt_get_efficiency_score(connection_optimizer_t *optimizer) {
    if (!optimizer || optimizer->stats.acquired_connections == 0) return 0.0;
    
    // Оценка эффективности на основе отношения переиспользованных соединений к общему числу
    double reuse_score = (double)optimizer->pool.reused_count / optimizer->stats.acquired_connections;
    
    // Оценка использования пула
    double pool_util_score = optimizer->pool.active_count / (double)optimizer->pool.total_capacity;
    
    // Комбинированная оценка (можно настраивать веса)
    return (reuse_score * 0.7) + (pool_util_score * 0.3);
}

int conn_opt_resize_pool(connection_optimizer_t *optimizer, int new_size) {
    if (!optimizer || !optimizer->is_initialized || new_size <= 0) return -1;
    
    // В реальной реализации: увеличение пула соединений
    // Для простоты возвращаем ошибку, так как это требует сложной перестройки
    return -1;
}

// Оптимизация производительности
int conn_opt_apply_performance_tuning(connection_optimizer_t *optimizer, 
                                     conn_performance_tuning_t *tuning) {
    if (!optimizer || !tuning) return -1;
    
    // Применение настроек производительности
    if (tuning->max_connections > 0) {
        optimizer->config.max_connections = tuning->max_connections;
    }
    
    if (tuning->min_idle_connections >= 0) {
        optimizer->config.min_idle_connections = tuning->min_idle_connections;
    }
    
    if (tuning->max_idle_connections > 0) {
        optimizer->config.max_idle_connections = tuning->max_idle_connections;
    }
    
    if (tuning->timeout_seconds > 0) {
        optimizer->config.connection_timeout_sec = tuning->timeout_seconds;
    }
    
    if (tuning->keepalive_interval > 0) {
        optimizer->config.keepalive_interval_sec = tuning->keepalive_interval;
    }
    
    optimizer->config.enable_keepalive = tuning->enable_keepalive;
    optimizer->config.enable_compression = tuning->enable_compression;
    optimizer->config.enable_multiplexing = tuning->enable_multiplexing;
    
    return 0;
}

// Управление нагрузкой
int conn_opt_adjust_for_load(connection_optimizer_t *optimizer, int current_load) {
    if (!optimizer) return -1;
    
    // Адаптивная настройка параметров в зависимости от нагрузки
    if (current_load > 80) {
        // Высокая нагрузка - увеличиваем пул
        optimizer->config.max_idle_connections *= 1.5;
        if (optimizer->config.max_idle_connections > 500) {
            optimizer->config.max_idle_connections = 500;
        }
    } else if (current_load < 20) {
        // Низкая нагрузка - уменьшаем пул
        optimizer->config.max_idle_connections /= 1.5;
        if (optimizer->config.max_idle_connections < optimizer->config.min_idle_connections) {
            optimizer->config.max_idle_connections = optimizer->config.min_idle_connections;
        }
    }
    
    return 0;
}

// Сброс статистики
void conn_opt_reset_stats(connection_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    memset(&optimizer->stats, 0, sizeof(conn_opt_stats_t));
    optimizer->stats.init_time = 0; // будет реальное время
}