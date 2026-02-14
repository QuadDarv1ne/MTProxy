#include "memory-manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

// Глобальный менеджер памяти
static memory_manager_t *global_memory_manager = NULL;

// Платформенно-зависимые функции
#ifdef _WIN32
    static void* platform_allocate_pages(size_t size) {
        return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    
    static void platform_free_pages(void *ptr, size_t size) {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
    
    static void* platform_create_mutex(void) {
        CRITICAL_SECTION *cs = malloc(sizeof(CRITICAL_SECTION));
        if (cs) {
            InitializeCriticalSection(cs);
        }
        return cs;
    }
    
    static void platform_destroy_mutex(void *mutex) {
        if (mutex) {
            DeleteCriticalSection((CRITICAL_SECTION*)mutex);
            free(mutex);
        }
    }
    
    static void platform_lock_mutex(void *mutex) {
        if (mutex) {
            EnterCriticalSection((CRITICAL_SECTION*)mutex);
        }
    }
    
    static void platform_unlock_mutex(void *mutex) {
        if (mutex) {
            LeaveCriticalSection((CRITICAL_SECTION*)mutex);
        }
    }
#else
    static void* platform_allocate_pages(size_t size) {
        return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    
    static void platform_free_pages(void *ptr, size_t size) {
        munmap(ptr, size);
    }
    
    static void* platform_create_mutex(void) {
        pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
        if (mutex) {
            pthread_mutex_init(mutex, NULL);
        }
        return mutex;
    }
    
    static void platform_destroy_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_destroy((pthread_mutex_t*)mutex);
            free(mutex);
        }
    }
    
    static void platform_lock_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_lock((pthread_mutex_t*)mutex);
        }
    }
    
    static void platform_unlock_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_unlock((pthread_mutex_t*)mutex);
        }
    }
#endif

// Получение текущего времени в микросекундах
double memory_get_current_time_us(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000000.0 / frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
#endif
}

// Инициализация менеджера памяти
memory_manager_t* memory_manager_init(const memory_manager_config_t *config) {
    memory_manager_t *manager = calloc(1, sizeof(memory_manager_t));
    if (!manager) {
        return NULL;
    }
    
    // Конфигурация по умолчанию
    memory_manager_config_t default_config = {
        .initial_heap_size = 64 * 1024 * 1024,    // 64MB
        .max_heap_size = 1024 * 1024 * 1024,      // 1GB
        .default_allocator_type = ALLOCATOR_TYPE_STANDARD,
        .allocation_strategy = ALLOCATION_STRATEGY_FIRST_FIT,
        .enable_thread_local_caching = 1,
        .enable_memory_prefetching = 1,
        .enable_compaction = 1,
        .enable_statistics = 1,
        .enable_memory_guard_pages = 0,
        .enable_double_free_detection = 1,
        .enable_use_after_free_detection = 1,
        .enable_buffer_overflow_detection = 1,
        .enable_profiling = 0,
        .profile_sample_rate = 100,
        .enable_numa_locality = 0,
        .numa_node_preference = 0
    };
    
    // Применение пользовательской конфигурации
    if (config) {
        manager->config = *config;
    } else {
        manager->config = default_config;
    }
    
    // Инициализация мьютексов
    manager->global_mutex = platform_create_mutex();
    if (!manager->global_mutex) {
        free(manager);
        return NULL;
    }
    
    // Инициализация стандартного хипа
    manager->allocators.standard_heap = platform_allocate_pages(manager->config.initial_heap_size);
    if (!manager->allocators.standard_heap) {
        platform_destroy_mutex(manager->global_mutex);
        free(manager);
        return NULL;
    }
    
    // Инициализация профайлера
    if (manager->config.enable_profiling) {
        manager->profiler.max_samples = 10000;
        manager->profiler.allocation_timestamps = calloc(manager->profiler.max_samples, sizeof(unsigned long long));
        manager->profiler.allocation_sizes = calloc(manager->profiler.max_samples, sizeof(size_t));
        manager->profiler.allocator_types = calloc(manager->profiler.max_samples, sizeof(int));
        if (!manager->profiler.allocation_timestamps || 
            !manager->profiler.allocation_sizes || 
            !manager->profiler.allocator_types) {
            memory_manager_cleanup(manager);
            return NULL;
        }
    }
    
    manager->is_initialized = 1;
    manager->is_running = 1;
    manager->allocation_counter = 0;
    manager->error_counter = 0;
    
    return manager;
}

// Конфигурация менеджера
int memory_manager_configure(memory_manager_t *manager, 
                            const memory_manager_config_t *config) {
    if (!manager || !config) {
        return -1;
    }
    
    platform_lock_mutex(manager->global_mutex);
    manager->config = *config;
    platform_unlock_mutex(manager->global_mutex);
    
    return 0;
}

// Основная функция аллокации
void* memory_allocate(memory_manager_t *manager, size_t size) {
    if (!manager || !manager->is_initialized || size == 0) {
        return NULL;
    }
    
    double start_time = memory_get_current_time_us();
    
    platform_lock_mutex(manager->global_mutex);
    
    void *ptr = NULL;
    
    // Выбор стратегии аллокации
    switch (manager->config.default_allocator_type) {
        case ALLOCATOR_TYPE_STANDARD:
            ptr = malloc(size);
            break;
            
        case ALLOCATOR_TYPE_POOL:
            // TODO: Реализация пулов
            ptr = malloc(size);
            break;
            
        case ALLOCATOR_TYPE_ARENA:
            // TODO: Реализация arena
            ptr = malloc(size);
            break;
            
        default:
            ptr = malloc(size);
            break;
    }
    
    if (ptr) {
        // Обновление статистики
        manager->global_stats.total_allocated_bytes += size;
        manager->global_stats.current_allocated_bytes += size;
        manager->global_stats.total_allocation_count++;
        
        if (manager->global_stats.current_allocated_bytes > manager->global_stats.peak_allocated_bytes) {
            manager->global_stats.peak_allocated_bytes = manager->global_stats.current_allocated_bytes;
        }
        
        // Добавление защитных байтов если включено
        if (manager->config.enable_buffer_overflow_detection) {
            // Добавляем 16 байт для детекции переполнения
            char *guard_ptr = (char*)ptr + size;
            for (int i = 0; i < 16; i++) {
                guard_ptr[i] = 0xDE;
            }
        }
    } else {
        manager->global_stats.failed_allocation_count++;
        manager->error_counter++;
    }
    
    double end_time = memory_get_current_time_us();
    manager->global_stats.avg_allocation_time_us = 
        (manager->global_stats.avg_allocation_time_us * (manager->global_stats.total_allocation_count - 1) + 
         (end_time - start_time)) / manager->global_stats.total_allocation_count;
    
    platform_unlock_mutex(manager->global_mutex);
    
    return ptr;
}

// Calloc
void* memory_callocate(memory_manager_t *manager, size_t count, size_t size) {
    size_t total_size = count * size;
    void *ptr = memory_allocate(manager, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Realloc
void* memory_reallocate(memory_manager_t *manager, void *ptr, size_t new_size) {
    if (!manager || !manager->is_initialized) {
        return realloc(ptr, new_size);
    }
    
    if (!ptr) {
        return memory_allocate(manager, new_size);
    }
    
    if (new_size == 0) {
        memory_deallocate(manager, ptr);
        return NULL;
    }
    
    // Получение текущего размера (приблизительно)
    size_t old_size = malloc_usable_size(ptr);
    
    platform_lock_mutex(manager->global_mutex);
    
    void *new_ptr = realloc(ptr, new_size);
    
    if (new_ptr) {
        // Обновление статистики
        if (new_size > old_size) {
            manager->global_stats.current_allocated_bytes += (new_size - old_size);
        } else {
            manager->global_stats.current_allocated_bytes -= (old_size - new_size);
        }
        
        if (manager->global_stats.current_allocated_bytes > manager->global_stats.peak_allocated_bytes) {
            manager->global_stats.peak_allocated_bytes = manager->global_stats.current_allocated_bytes;
        }
        
        manager->global_stats.total_allocation_count++;
    } else {
        manager->global_stats.failed_allocation_count++;
        manager->error_counter++;
    }
    
    platform_unlock_mutex(manager->global_mutex);
    
    return new_ptr;
}

// Освобождение памяти
void memory_deallocate(memory_manager_t *manager, void *ptr) {
    if (!ptr) return;
    
    if (!manager || !manager->is_initialized) {
        free(ptr);
        return;
    }
    
    double start_time = memory_get_current_time_us();
    
    platform_lock_mutex(manager->global_mutex);
    
    // Проверка на double free
    if (manager->config.enable_double_free_detection) {
        // TODO: Реализовать проверку на повторное освобождение
    }
    
    // Получение размера для статистики
    size_t size = malloc_usable_size(ptr);
    
    free(ptr);
    
    // Обновление статистики
    manager->global_stats.current_allocated_bytes -= size;
    manager->global_stats.total_deallocation_count++;
    
    double end_time = memory_get_current_time_us();
    manager->global_stats.avg_deallocation_time_us = 
        (manager->global_stats.avg_deallocation_time_us * (manager->global_stats.total_deallocation_count - 1) + 
         (end_time - start_time)) / manager->global_stats.total_deallocation_count;
    
    platform_unlock_mutex(manager->global_mutex);
}

// Пул аллокатор
memory_pool_t* memory_pool_create(memory_manager_t *manager, 
                                 size_t block_size, size_t pool_size) {
    if (!manager || block_size == 0 || pool_size == 0) {
        return NULL;
    }
    
    memory_pool_t *pool = calloc(1, sizeof(memory_pool_t));
    if (!pool) return NULL;
    
    pool->block_size = block_size;
    pool->pool_size = pool_size;
    pool->memory_pool = malloc(block_size * pool_size);
    
    if (!pool->memory_pool) {
        free(pool);
        return NULL;
    }
    
    // Инициализация свободных блоков
    pool->free_blocks = NULL;
    char *pool_ptr = (char*)pool->memory_pool;
    
    for (size_t i = 0; i < pool_size; i++) {
        memory_block_t *block = (memory_block_t*)(pool_ptr + i * block_size);
        block->address = block;
        block->size = block_size;
        block->is_allocated = 0;
        block->next = pool->free_blocks;
        pool->free_blocks = block;
        pool->free_count++;
    }
    
    return pool;
}

// Аллокация из пула
void* memory_pool_allocate(memory_pool_t *pool) {
    if (!pool || !pool->free_blocks) {
        return NULL;
    }
    
    memory_block_t *block = pool->free_blocks;
    pool->free_blocks = block->next;
    block->is_allocated = 1;
    pool->allocated_count++;
    pool->free_count--;
    
    return block->address;
}

// Освобождение в пул
void memory_pool_deallocate(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return;
    
    memory_block_t *block = (memory_block_t*)ptr;
    block->is_allocated = 0;
    block->next = pool->free_blocks;
    pool->free_blocks = block;
    pool->allocated_count--;
    pool->free_count++;
}

// Уничтожение пула
void memory_pool_destroy(memory_pool_t *pool) {
    if (!pool) return;
    
    if (pool->memory_pool) {
        free(pool->memory_pool);
    }
    free(pool);
}

// Arena аллокатор
memory_arena_t* memory_arena_create(memory_manager_t *manager, size_t size) {
    if (!manager || size == 0) {
        return NULL;
    }
    
    memory_arena_t *arena = calloc(1, sizeof(memory_arena_t));
    if (!arena) return NULL;
    
    arena->arena_size = size;
    arena->arena_start = malloc(size);
    
    if (!arena->arena_start) {
        free(arena);
        return NULL;
    }
    
    arena->arena_end = (char*)arena->arena_start + size;
    arena->current_position = arena->arena_start;
    arena->used_space = 0;
    arena->is_initialized = 1;
    
    return arena;
}

// Аллокация из arena
void* memory_arena_allocate(memory_arena_t *arena, size_t size) {
    if (!arena || !arena->is_initialized || size == 0) {
        return NULL;
    }
    
    // Выравнивание по 8 байтам
    size_t aligned_size = (size + 7) & ~7;
    
    if ((char*)arena->current_position + aligned_size > (char*)arena->arena_end) {
        return NULL; // Недостаточно места
    }
    
    void *ptr = arena->current_position;
    arena->current_position = (char*)arena->current_position + aligned_size;
    arena->used_space += aligned_size;
    
    return ptr;
}

// Сброс arena
void memory_arena_reset(memory_arena_t *arena) {
    if (!arena || !arena->is_initialized) return;
    
    arena->current_position = arena->arena_start;
    arena->used_space = 0;
}

// Уничтожение arena
void memory_arena_destroy(memory_arena_t *arena) {
    if (!arena) return;
    
    if (arena->arena_start) {
        free(arena->arena_start);
    }
    free(arena);
}

// Получение статистики
const memory_stats_t* memory_get_stats(memory_manager_t *manager) {
    if (!manager) return NULL;
    return &manager->global_stats;
}

// Получение конфигурации
const memory_manager_config_t* memory_get_config(memory_manager_t *manager) {
    if (!manager) return NULL;
    return &manager->config;
}

// Печать статистики
void memory_print_stats(memory_manager_t *manager) {
    if (!manager) return;
    
    const memory_stats_t *stats = &manager->global_stats;
    
    printf("=== Memory Manager Statistics ===\n");
    printf("Total allocated: %zu bytes\n", stats->total_allocated_bytes);
    printf("Current allocated: %zu bytes\n", stats->current_allocated_bytes);
    printf("Peak allocated: %zu bytes\n", stats->peak_allocated_bytes);
    printf("Total allocations: %zu\n", stats->total_allocation_count);
    printf("Total deallocations: %zu\n", stats->total_deallocation_count);
    printf("Failed allocations: %zu\n", stats->failed_allocation_count);
    printf("Avg allocation time: %.2f μs\n", stats->avg_allocation_time_us);
    printf("Avg deallocation time: %.2f μs\n", stats->avg_deallocation_time_us);
    printf("Error count: %llu\n", manager->error_counter);
    printf("=================================\n");
}

// Сброс статистики
void memory_reset_stats(memory_manager_t *manager) {
    if (!manager) return;
    
    memset(&manager->global_stats, 0, sizeof(memory_stats_t));
    manager->allocation_counter = 0;
    manager->error_counter = 0;
}

// Очистка менеджера
void memory_manager_cleanup(memory_manager_t *manager) {
    if (!manager) return;
    
    // Очистка профайлера
    if (manager->profiler.allocation_timestamps) {
        free(manager->profiler.allocation_timestamps);
    }
    if (manager->profiler.allocation_sizes) {
        free(manager->profiler.allocation_sizes);
    }
    if (manager->profiler.allocator_types) {
        free(manager->profiler.allocator_types);
    }
    
    // Очистка стандартного хипа
    if (manager->allocators.standard_heap) {
        platform_free_pages(manager->allocators.standard_heap, manager->config.initial_heap_size);
    }
    
    // Очистка мьютексов
    platform_destroy_mutex(manager->global_mutex);
    
    // Очистка пулов и arena
    // TODO: Реализовать очистку пулов и arena
    
    free(manager);
}

// Установка глобального менеджера
void memory_manager_set_global(memory_manager_t *manager) {
    global_memory_manager = manager;
}

// Получение глобального менеджера
memory_manager_t* memory_manager_get_global(void) {
    return global_memory_manager;
}