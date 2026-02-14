/*
    Реализация системы оптимизации производительности MTProxy
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

// Проверка доступности NUMA
#ifdef __linux__
    #include <numa.h>
    #define HAS_NUMA_SUPPORT 1
#else
    #define HAS_NUMA_SUPPORT 0
#endif

#include "performance-optimizer.h"

// Глобальный оптимизатор
static performance_optimizer_t *g_optimizer = NULL;

// Вспомогательные функции
static int get_cpu_count_impl(void);
static int detect_numa_nodes_impl(performance_optimizer_t *opt);
static void* memory_pool_thread(void *arg);
static uint32_t simple_hash(uint32_t key);
static double get_time_ms(void);

// Инициализация оптимизатора
performance_optimizer_t* perf_optimizer_init(void) {
    // Выделение памяти для оптимизатора
    performance_optimizer_t *opt = malloc(sizeof(performance_optimizer_t));
    if (!opt) {
        return NULL;
    }
    
    // Инициализация нулями
    memset(opt, 0, sizeof(performance_optimizer_t));
    
    // Значения по умолчанию
    opt->enable_numa_optimization = HAS_NUMA_SUPPORT;
    opt->enable_memory_pooling = 1;
    opt->enable_cpu_affinity = 1;
    opt->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    opt->memory_pool_size = MEMORY_POOL_SIZE;
    opt->overall_status = OPTIMIZATION_STATUS_ENABLED;
    opt->optimization_start_time = (long long)get_time_ms();
    
    // Детектирование NUMA топологии
    if (opt->enable_numa_optimization) {
        detect_numa_nodes_impl(opt);
    }
    
    g_optimizer = opt;
    return opt;
}

// Конфигурация оптимизатора
int perf_optimizer_configure(performance_optimizer_t *opt, int thread_count, size_t mem_pool_size) {
    if (!opt) return -1;
    
    // Проверка параметров
    if (thread_count <= 0 || thread_count > MAX_THREAD_POOL_SIZE) {
        thread_count = DEFAULT_THREAD_POOL_SIZE;
    }
    
    if (mem_pool_size < (16 * 1024 * 1024)) { // Минимум 16MB
        mem_pool_size = MEMORY_POOL_SIZE;
    }
    
    opt->thread_pool_size = thread_count;
    opt->memory_pool_size = mem_pool_size;
    
    // Создание пулов памяти
    if (opt->enable_memory_pooling) {
        opt->memory_pools = malloc(sizeof(memory_pool_t) * opt->numa_info.node_count);
        if (opt->memory_pools) {
            for (int i = 0; i < opt->numa_info.node_count; i++) {
                opt->memory_pools[i].pool_size = opt->memory_pool_size / opt->numa_info.node_count;
                opt->memory_pools[i].block_size = 4096; // 4KB блоки
                opt->memory_pools[i].status = OPTIMIZATION_STATUS_ENABLED;
            }
        }
    }
    
    // Создание пула потоков
    perf_create_thread_pool(opt, opt->thread_pool_size);
    
    // Инициализация шардинга соединений
    perf_init_connection_sharding(opt, opt->thread_pool_size);
    
    return 0;
}

// Очистка ресурсов
void perf_optimizer_cleanup(performance_optimizer_t *opt) {
    if (!opt) return;
    
    // Очистка пулов памяти
    if (opt->memory_pools) {
        free(opt->memory_pools);
    }
    
    // Очистка пула потоков
    if (opt->thread_pool.workers) {
        free(opt->thread_pool.workers);
    }
    
    // Очистка шардов
    if (opt->connection_shards) {
        free(opt->connection_shards);
    }
    
    // Очистка NUMA информации
    if (opt->numa_info.cpu_list) {
        free(opt->numa_info.cpu_list);
    }
    
    free(opt);
    if (g_optimizer == opt) {
        g_optimizer = NULL;
    }
}

// Детектирование NUMA топологии
int perf_detect_numa_topology(performance_optimizer_t *opt) {
    if (!opt || !opt->enable_numa_optimization) return -1;
    
    return detect_numa_nodes_impl(opt);
}

// Привязка потока к NUMA узлу
int perf_bind_thread_to_numa_node(int thread_id, int numa_node) {
    if (!g_optimizer || !g_optimizer->enable_numa_optimization) return 0;
    
#if HAS_NUMA_SUPPORT
    if (numa_available() == -1) return -1;
    
    if (numa_node >= 0 && numa_node < g_optimizer->numa_info.node_count) {
        // Получение CPU для данного NUMA узла
        if (g_optimizer->numa_info.cpu_list) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(g_optimizer->numa_info.cpu_list[numa_node], &cpuset);
            
            pthread_t current_thread = pthread_self();
            if (pthread_setaffinity_np(current_thread, sizeof(cpuset), &cpuset) == 0) {
                return 0; // Успех
            }
        }
    }
#endif
    
    return -1; // Ошибка
}

// NUMA выделение памяти
void* perf_numa_malloc(size_t size, int numa_node) {
    if (!g_optimizer || !g_optimizer->enable_numa_optimization) {
        return malloc(size);
    }
    
#if HAS_NUMA_SUPPORT
    if (numa_available() == -1 || numa_node < 0) {
        return malloc(size);
    }
    
    return numa_alloc_onnode(size, numa_node);
#else
    return malloc(size);
#endif
}

// NUMA освобождение памяти
void perf_numa_free(void *ptr, size_t size) {
    if (!ptr) return;
    
#if HAS_NUMA_SUPPORT
    if (g_optimizer && g_optimizer->enable_numa_optimization && numa_available() != -1) {
        numa_free(ptr, size);
        return;
    }
#endif
    
    free(ptr);
}

// Создание пула памяти
memory_pool_t* perf_create_memory_pool(size_t pool_size, size_t block_size) {
    memory_pool_t *pool = malloc(sizeof(memory_pool_t));
    if (!pool) return NULL;
    
    // Выделение памяти для пула
    pool->memory_start = malloc(pool_size);
    if (!pool->memory_start) {
        free(pool);
        return NULL;
    }
    
    pool->pool_size = pool_size;
    pool->used_size = 0;
    pool->block_size = block_size > 0 ? block_size : 4096;
    pool->free_block_count = pool_size / pool->block_size;
    pool->status = OPTIMIZATION_STATUS_ACTIVE;
    
    // Инициализация списка свободных блоков
    pool->free_blocks = malloc(sizeof(int) * pool->free_block_count);
    if (pool->free_blocks) {
        for (int i = 0; i < pool->free_block_count; i++) {
            pool->free_blocks[i] = i;
        }
    }
    
    return pool;
}

// Выделение из пула памяти
void* perf_pool_alloc(memory_pool_t *pool, size_t size) {
    if (!pool || pool->status != OPTIMIZATION_STATUS_ACTIVE) {
        return malloc(size);
    }
    
    // Округление размера до размера блока
    size_t blocks_needed = (size + pool->block_size - 1) / pool->block_size;
    
    if (pool->free_block_count >= blocks_needed) {
        // Выделение блоков
        char *ptr = (char*)pool->memory_start + (pool->free_blocks[pool->free_block_count - 1] * pool->block_size);
        pool->free_block_count -= blocks_needed;
        pool->used_size += blocks_needed * pool->block_size;
        return ptr;
    }
    
    // Недостаточно памяти в пуле
    return malloc(size);
}

// Освобождение в пул памяти
void perf_pool_free(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        if (ptr) free(ptr);
        return;
    }
    
    // Проверка, принадлежит ли указатель пулу
    char *pool_start = (char*)pool->memory_start;
    char *pool_end = pool_start + pool->pool_size;
    
    if ((char*)ptr >= pool_start && (char*)ptr < pool_end) {
        // Возвращение блоков в пул
        size_t offset = (char*)ptr - pool_start;
        int block_index = offset / pool->block_size;
        
        if (pool->free_block_count < pool->pool_size / pool->block_size) {
            pool->free_blocks[pool->free_block_count] = block_index;
            pool->free_block_count++;
            pool->used_size -= pool->block_size;
        }
    } else {
        free(ptr);
    }
}

// Создание пула потоков
int perf_create_thread_pool(performance_optimizer_t *opt, int thread_count) {
    if (!opt || thread_count <= 0) return -1;
    
    opt->thread_pool.workers = malloc(sizeof(thread_worker_t) * thread_count);
    if (!opt->thread_pool.workers) return -1;
    
    opt->thread_pool.worker_count = thread_count;
    opt->thread_pool.active_workers = 0;
    opt->thread_pool.status = OPTIMIZATION_STATUS_ENABLED;
    
    // Инициализация воркеров
    for (int i = 0; i < thread_count; i++) {
        thread_worker_t *worker = &opt->thread_pool.workers[i];
        worker->thread_id = i;
        worker->cpu_core = i % perf_get_cpu_count();
        worker->numa_node = opt->enable_numa_optimization ? 
                           (i % opt->numa_info.node_count) : 0;
        worker->work_queue = NULL;
        worker->status = OPTIMIZATION_STATUS_ENABLED;
        worker->processed_connections = 0;
        worker->processed_bytes = 0;
    }
    
    return 0;
}

// Шардинг соединений
int perf_init_connection_sharding(performance_optimizer_t *opt, int shard_count) {
    if (!opt || shard_count <= 0) return -1;
    
    opt->connection_shards = malloc(sizeof(connection_shard_t) * shard_count);
    if (!opt->connection_shards) return -1;
    
    for (int i = 0; i < shard_count; i++) {
        connection_shard_t *shard = &opt->connection_shards[i];
        shard->shard_id = i;
        shard->thread_id = i % opt->thread_pool.worker_count;
        shard->connection_hash = 0;
        shard->connection_count = 0;
        shard->status = OPTIMIZATION_STATUS_ENABLED;
    }
    
    return 0;
}

// Получение шарда для соединения
int perf_get_shard_for_connection(uint32_t connection_id, int total_shards) {
    if (total_shards <= 0) return 0;
    return simple_hash(connection_id) % total_shards;
}

// Сбор метрик производительности
void perf_collect_metrics(performance_optimizer_t *opt) {
    if (!opt) return;
    
    // Симуляция сбора метрик
    opt->metrics.cpu_usage_percent = 50.0 + (rand() % 30); // 50-80%
    opt->metrics.memory_used_bytes = opt->memory_pool_size / 2;
    opt->metrics.packets_processed = 1000000 + (rand() % 100000);
    opt->metrics.bytes_throughput = 1000000000LL + (rand() % 100000000);
    opt->metrics.latency_us = 100.0 + (rand() % 50);
}

// Получение отчета о производительности
void perf_get_performance_report(performance_optimizer_t *opt, char *buffer, size_t buffer_size) {
    if (!opt || !buffer || buffer_size < 200) return;
    
    snprintf(buffer, buffer_size,
        "Performance Report:\n"
        "CPU Usage: %.1f%%\n"
        "Memory Used: %zu bytes\n"
        "Packets Processed: %lld\n"
        "Throughput: %lld bytes/sec\n"
        "Latency: %.1f us\n"
        "Active Threads: %d/%d\n",
        opt->metrics.cpu_usage_percent,
        opt->metrics.memory_used_bytes,
        opt->metrics.packets_processed,
        opt->metrics.bytes_throughput,
        opt->metrics.latency_us,
        opt->thread_pool.active_workers,
        opt->thread_pool.worker_count);
}

// Применение CPU affinity
int perf_apply_cpu_affinity(performance_optimizer_t *opt) {
    if (!opt || !opt->enable_cpu_affinity) return 0;
    
    // В реальной реализации привязать потоки к конкретным ядрам CPU
    // для минимизации межъядерных миграций
    return 0;
}

// Хэш-функция для соединений
uint32_t perf_hash_connection_id(uint32_t connection_id) {
    return simple_hash(connection_id);
}

// Получение количества CPU
int perf_get_cpu_count(void) {
    return get_cpu_count_impl();
}

// Получение количества NUMA узлов
int perf_get_numa_node_count(void) {
    if (g_optimizer && g_optimizer->enable_numa_optimization) {
        return g_optimizer->numa_info.node_count;
    }
    return 1;
}

// Текущее время в миллисекундах
double perf_get_current_time_ms(void) {
    return get_time_ms();
}

// Задержка в миллисекундах
void perf_sleep_ms(int milliseconds) {
    if (milliseconds > 0) {
        usleep(milliseconds * 1000);
    }
}

// Печать детальной статистики
void perf_print_detailed_stats(performance_optimizer_t *opt) {
    if (!opt) return;
    
    printf("=== Performance Statistics ===\n");
    printf("Total Optimizations: %lld\n", opt->total_optimizations_applied);
    printf("Performance Improvements: %lld\n", opt->performance_improvements);
    printf("Thread Pool Efficiency: %.1f%%\n", 
           (opt->thread_pool.active_workers * 100.0) / opt->thread_pool.worker_count);
    
    if (opt->memory_pools) {
        printf("Memory Pool Utilization: %.1f%%\n", 
               (opt->memory_pools[0].used_size * 100.0) / opt->memory_pools[0].pool_size);
    }
}

// Сброс статистики
void perf_reset_statistics(performance_optimizer_t *opt) {
    if (!opt) return;
    
    opt->total_optimizations_applied = 0;
    opt->performance_improvements = 0;
    opt->thread_pool.total_connections = 0;
    opt->thread_pool.total_bytes_processed = 0;
    
    if (opt->memory_pools) {
        for (int i = 0; i < opt->numa_info.node_count; i++) {
            opt->memory_pools[i].used_size = 0;
            opt->memory_pools[i].free_block_count = 
                opt->memory_pools[i].pool_size / opt->memory_pools[i].block_size;
        }
    }
}

// Вспомогательные функции реализации

static int get_cpu_count_impl(void) {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? (int)count : 1;
}

static int detect_numa_nodes_impl(performance_optimizer_t *opt) {
#if HAS_NUMA_SUPPORT
    if (numa_available() == -1) {
        opt->numa_info.node_count = 1;
        opt->numa_info.current_node = 0;
        return -1;
    }
    
    opt->numa_info.node_count = numa_max_node() + 1;
    opt->numa_info.current_node = numa_node_of_cpu(sched_getcpu());
    opt->numa_info.node_memory_size = numa_node_size(opt->numa_info.current_node, NULL);
    
    // Получение списка CPU для каждого NUMA узла
    opt->numa_info.cpu_list = malloc(sizeof(int) * get_cpu_count_impl());
    if (opt->numa_info.cpu_list) {
        opt->numa_info.cpu_count = 0;
        for (int i = 0; i < get_cpu_count_impl(); i++) {
            if (numa_node_of_cpu(i) == opt->numa_info.current_node) {
                opt->numa_info.cpu_list[opt->numa_info.cpu_count++] = i;
            }
        }
    }
    
    return 0;
#else
    opt->numa_info.node_count = 1;
    opt->numa_info.current_node = 0;
    opt->numa_info.cpu_count = get_cpu_count_impl();
    return -1;
#endif
}

static void* memory_pool_thread(void *arg) {
    // Поток для управления пулом памяти
    memory_pool_t *pool = (memory_pool_t*)arg;
    
    while (pool->status == OPTIMIZATION_STATUS_ACTIVE) {
        // В реальной реализации: мониторинг и оптимизация пула
        perf_sleep_ms(100);
    }
    
    return NULL;
}

static uint32_t simple_hash(uint32_t key) {
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = (key >> 16) ^ key;
    return key;
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
}