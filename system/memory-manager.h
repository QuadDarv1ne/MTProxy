#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>

// Типы аллокаторов
typedef enum {
    ALLOCATOR_TYPE_STANDARD = 0,    // Стандартный malloc/free
    ALLOCATOR_TYPE_POOL,           // Пул аллокатор
    ALLOCATOR_TYPE_ARENA,          // Arena аллокатор
    ALLOCATOR_TYPE_SLAB,           // Slab аллокатор
    ALLOCATOR_TYPE_BUMP,           // Bump аллокатор
    ALLOCATOR_TYPE_CUSTOM          // Пользовательский аллокатор
} allocator_type_t;

// Стратегии размещения
typedef enum {
    ALLOCATION_STRATEGY_FIRST_FIT = 0,
    ALLOCATION_STRATEGY_BEST_FIT,
    ALLOCATION_STRATEGY_WORST_FIT,
    ALLOCATION_STRATEGY_NEXT_FIT
} allocation_strategy_t;

// Статистика аллокатора
typedef struct {
    size_t total_allocated_bytes;
    size_t current_allocated_bytes;
    size_t peak_allocated_bytes;
    size_t total_allocation_count;
    size_t total_deallocation_count;
    size_t failed_allocation_count;
    double avg_allocation_time_us;
    double avg_deallocation_time_us;
    size_t memory_fragmentation_ratio; // в процентах
    size_t cache_hit_rate;             // в процентах
} memory_stats_t;

// Конфигурация менеджера памяти
typedef struct {
    // Основные параметры
    size_t initial_heap_size;
    size_t max_heap_size;
    allocator_type_t default_allocator_type;
    allocation_strategy_t allocation_strategy;
    
    // Оптимизации
    int enable_thread_local_caching;
    int enable_memory_prefetching;
    int enable_compaction;
    int enable_statistics;
    
    // Защита памяти
    int enable_memory_guard_pages;
    int enable_double_free_detection;
    int enable_use_after_free_detection;
    int enable_buffer_overflow_detection;
    
    // Профилирование
    int enable_profiling;
    int profile_sample_rate;
    char profile_output_file[256];
    
    // NUMA поддержка
    int enable_numa_locality;
    int numa_node_preference;
} memory_manager_config_t;

// Дескриптор памяти
typedef struct memory_block {
    void *address;
    size_t size;
    size_t requested_size;
    allocator_type_t allocator_type;
    int is_allocated;
    unsigned long long allocation_id;
    unsigned long long timestamp;
    struct memory_block *next;
    struct memory_block *prev;
} memory_block_t;

// Пул аллокатор
typedef struct {
    memory_block_t *free_blocks;
    memory_block_t *used_blocks;
    size_t block_size;
    size_t pool_size;
    size_t allocated_count;
    size_t free_count;
    void *memory_pool;
} memory_pool_t;

// Arena аллокатор
typedef struct {
    void *arena_start;
    void *arena_end;
    void *current_position;
    size_t arena_size;
    size_t used_space;
    int is_initialized;
} memory_arena_t;

// Контекст менеджера памяти
typedef struct {
    // Конфигурация
    memory_manager_config_t config;
    
    // Аллокаторы
    struct {
        memory_pool_t *pools;
        int pool_count;
        memory_arena_t *arenas;
        int arena_count;
        void *standard_heap;
    } allocators;
    
    // Статистика
    memory_stats_t global_stats;
    memory_stats_t *per_thread_stats;
    int thread_count;
    
    // Синхронизация
    void *global_mutex;
    void **thread_mutexes;
    
    // Профилирование
    struct {
        unsigned long long *allocation_timestamps;
        size_t *allocation_sizes;
        int *allocator_types;
        size_t sample_count;
        size_t max_samples;
    } profiler;
    
    // Состояние
    int is_initialized;
    int is_running;
    unsigned long long allocation_counter;
    unsigned long long error_counter;
} memory_manager_t;

// Инициализация и конфигурация
memory_manager_t* memory_manager_init(const memory_manager_config_t *config);
int memory_manager_configure(memory_manager_t *manager, 
                            const memory_manager_config_t *config);
void memory_manager_cleanup(memory_manager_t *manager);

// Основные функции аллокации
void* memory_allocate(memory_manager_t *manager, size_t size);
void* memory_callocate(memory_manager_t *manager, size_t count, size_t size);
void* memory_reallocate(memory_manager_t *manager, void *ptr, size_t new_size);
void memory_deallocate(memory_manager_t *manager, void *ptr);

// Типизированные аллокации
#define MEMORY_ALLOCATE(manager, type) \
    (type*)memory_allocate(manager, sizeof(type))

#define MEMORY_ALLOCATE_ARRAY(manager, type, count) \
    (type*)memory_allocate(manager, sizeof(type) * (count))

#define MEMORY_NEW(manager, type) \
    MEMORY_ALLOCATE(manager, type)

#define MEMORY_NEW_ARRAY(manager, type, count) \
    MEMORY_ALLOCATE_ARRAY(manager, type, count)

// Пул аллокаторы
memory_pool_t* memory_pool_create(memory_manager_t *manager, 
                                 size_t block_size, size_t pool_size);
void* memory_pool_allocate(memory_pool_t *pool);
void memory_pool_deallocate(memory_pool_t *pool, void *ptr);
void memory_pool_destroy(memory_pool_t *pool);

// Arena аллокаторы
memory_arena_t* memory_arena_create(memory_manager_t *manager, size_t size);
void* memory_arena_allocate(memory_arena_t *arena, size_t size);
void memory_arena_reset(memory_arena_t *arena);
void memory_arena_destroy(memory_arena_t *arena);

// Информация и диагностика
const memory_stats_t* memory_get_stats(memory_manager_t *manager);
const memory_manager_config_t* memory_get_config(memory_manager_t *manager);
void memory_print_stats(memory_manager_t *manager);
void memory_reset_stats(memory_manager_t *manager);

// Мониторинг и профилирование
int memory_start_profiling(memory_manager_t *manager);
int memory_stop_profiling(memory_manager_t *manager);
int memory_dump_profile(memory_manager_t *manager, const char *filename);

// Управление фрагментацией
int memory_compact(memory_manager_t *manager);
size_t memory_get_fragmentation_ratio(memory_manager_t *manager);
void memory_optimize_layout(memory_manager_t *manager);

// Безопасность
int memory_enable_protection(memory_manager_t *manager);
int memory_disable_protection(memory_manager_t *manager);
int memory_verify_integrity(memory_manager_t *manager);
void memory_set_corruption_handler(void (*handler)(void *ptr, size_t size));

// Интеграция с существующими системами
int memory_manager_integrate_with_malloc(memory_manager_t *manager);
void* memory_replace_malloc(size_t size);
void* memory_replace_calloc(size_t count, size_t size);
void* memory_replace_realloc(void *ptr, size_t new_size);
void memory_replace_free(void *ptr);

// Утилиты
size_t memory_get_block_size(void *ptr);
allocator_type_t memory_get_allocator_type(void *ptr);
unsigned long long memory_get_allocation_id(void *ptr);
double memory_get_current_time_us(void);

// Глобальные функции
void memory_manager_set_global(memory_manager_t *manager);
memory_manager_t* memory_manager_get_global(void);

#endif // MEMORY_MANAGER_H