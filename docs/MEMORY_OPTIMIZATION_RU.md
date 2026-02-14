# Оптимизация работы с памятью MTProxy

## Обзор

Система оптимизации памяти предоставляет расширенные возможности управления памятью с поддержкой различных стратегий аллокации, профилирования и защиты от ошибок работы с памятью.

## Архитектура

### Основные компоненты

1. **Memory Manager** (`memory-manager.h/.c`) - основной менеджер памяти
2. **Memory Pool** - пулы фиксированного размера
3. **Memory Arena** - arena аллокаторы
4. **Memory Optimizer** - оптимизатор использования памяти

### Типы аллокаторов

```c
typedef enum {
    ALLOCATOR_TYPE_STANDARD = 0,    // Стандартный malloc/free
    ALLOCATOR_TYPE_POOL,           // Пул аллокатор
    ALLOCATOR_TYPE_ARENA,          // Arena аллокатор
    ALLOCATOR_TYPE_SLAB,           // Slab аллокатор
    ALLOCATOR_TYPE_BUMP,           // Bump аллокатор
    ALLOCATOR_TYPE_CUSTOM          // Пользовательский аллокатор
} allocator_type_t;
```

## Основные функции

### Инициализация
```c
// Конфигурация по умолчанию
memory_manager_t* manager = memory_manager_init(NULL);

// С пользовательской конфигурацией
memory_manager_config_t config = {
    .initial_heap_size = 128 * 1024 * 1024,  // 128MB
    .max_heap_size = 1024 * 1024 * 1024,     // 1GB
    .default_allocator_type = ALLOCATOR_TYPE_POOL,
    .enable_thread_local_caching = 1,
    .enable_memory_prefetching = 1,
    .enable_statistics = 1,
    .enable_double_free_detection = 1
};

memory_manager_t* manager = memory_manager_init(&config);
```

### Базовые операции
```c
// Аллокация
void *ptr = memory_allocate(manager, 1024);

// Calloc
void *ptr = memory_callocate(manager, 10, sizeof(int));

// Realloc
void *new_ptr = memory_reallocate(manager, ptr, 2048);

// Освобождение
memory_deallocate(manager, ptr);
```

### Типизированные макросы
```c
// Аллокация одного объекта
MyStruct *obj = MEMORY_ALLOCATE(manager, MyStruct);

// Аллокация массива
int *array = MEMORY_ALLOCATE_ARRAY(manager, int, 100);

// Альтернативные макросы
MyStruct *obj = MEMORY_NEW(manager, MyStruct);
int *array = MEMORY_NEW_ARRAY(manager, int, 100);
```

## Пул аллокаторы

### Создание пула
```c
// Создание пула для объектов размером 64 байта
memory_pool_t *pool = memory_pool_create(manager, 64, 1000);

// Аллокация из пула
void *obj = memory_pool_allocate(pool);

// Освобождение в пул
memory_pool_deallocate(pool, obj);

// Уничтожение пула
memory_pool_destroy(pool);
```

### Преимущества пулов:
- Быстрая аллокация/деаллокация
- Отсутствие фрагментации
- Предсказуемое время выполнения
- Эффективное использование кэша

## Arena аллокаторы

### Создание arena
```c
// Создание arena размером 1MB
memory_arena_t *arena = memory_arena_create(manager, 1024 * 1024);

// Аллокация из arena
void *buffer = memory_arena_allocate(arena, 1024);

// Сброс arena (освобождение всей памяти)
memory_arena_reset(arena);

// Уничтожение arena
memory_arena_destroy(arena);
```

### Преимущества arena:
- Очень быстрая аллокация
- Нулевые накладные расходы на деаллокацию
- Идеальна для временных объектов
- Отлично подходит для парсинга и обработки данных

## Статистика и мониторинг

### Получение статистики
```c
const memory_stats_t* stats = memory_get_stats(manager);

printf("Total allocated: %zu bytes\n", stats->total_allocated_bytes);
printf("Current allocated: %zu bytes\n", stats->current_allocated_bytes);
printf("Peak allocated: %zu bytes\n", stats->peak_allocated_bytes);
printf("Failed allocations: %zu\n", stats->failed_allocation_count);
printf("Avg allocation time: %.2f μs\n", stats->avg_allocation_time_us);
```

### Печать статистики
```c
memory_print_stats(manager);
```

### Сброс статистики
```c
memory_reset_stats(manager);
```

## Защита памяти

### Обнаружение ошибок
```c
memory_manager_config_t config = {
    .enable_double_free_detection = 1,
    .enable_use_after_free_detection = 1,
    .enable_buffer_overflow_detection = 1,
    .enable_memory_guard_pages = 1
};
```

### Типы обнаруживаемых ошибок:
- **Double free** - повторное освобождение
- **Use after free** - использование после освобождения
- **Buffer overflow** - переполнение буфера
- **Memory corruption** - повреждение памяти

## Профилирование

### Включение профилирования
```c
memory_manager_config_t config = {
    .enable_profiling = 1,
    .profile_sample_rate = 100,  // Профилировать каждую 100-ю аллокацию
    .profile_output_file = "/tmp/memory_profile.txt"
};

// Запуск профилирования
memory_start_profiling(manager);

// Остановка и сохранение
memory_stop_profiling(manager);
memory_dump_profile(manager, "/tmp/final_profile.txt");
```

## Оптимизация фрагментации

### Компактификация
```c
// Запуск компактификации
memory_compact(manager);

// Получение уровня фрагментации
size_t fragmentation = memory_get_fragmentation_ratio(manager);
printf("Memory fragmentation: %zu%%\n", fragmentation);

// Оптимизация размещения
memory_optimize_layout(manager);
```

## Интеграция с существующими системами

### Замена стандартных функций
```c
// Интеграция с malloc/free
memory_manager_integrate_with_malloc(manager);

// Теперь стандартные функции будут использовать наш менеджер
void *ptr = malloc(1024);  // Использует memory_manager
free(ptr);                 // Использует memory_manager
```

## Конфигурация через переменные окружения

```bash
# Установка размера хипа
export MT_MEMORY_HEAP_SIZE=256MB

# Выбор типа аллокатора
export MT_MEMORY_ALLOCATOR=POOL

# Включение профилирования
export MT_MEMORY_PROFILE=1

# Установка уровня детализации
export MT_MEMORY_DEBUG_LEVEL=2
```

## Пример использования

```c
#include "system/memory-manager.h"

int main() {
    // Инициализация менеджера
    memory_manager_config_t config = {
        .initial_heap_size = 64 * 1024 * 1024,
        .default_allocator_type = ALLOCATOR_TYPE_POOL,
        .enable_statistics = 1,
        .enable_double_free_detection = 1
    };
    
    memory_manager_t *manager = memory_manager_init(&config);
    if (!manager) {
        fprintf(stderr, "Failed to initialize memory manager\n");
        return -1;
    }
    
    // Установка глобального менеджера
    memory_manager_set_global(manager);
    
    // Создание пула для часто используемых объектов
    memory_pool_t *connection_pool = memory_pool_create(manager, 
                                                       sizeof(connection_t), 1000);
    
    // Создание arena для временных буферов
    memory_arena_t *buffer_arena = memory_arena_create(manager, 1024 * 1024);
    
    // Использование памяти
    connection_t *conn = memory_pool_allocate(connection_pool);
    char *buffer = memory_arena_allocate(buffer_arena, 1024);
    
    // Работа с данными...
    
    // Освобождение
    memory_pool_deallocate(connection_pool, conn);
    memory_arena_reset(buffer_arena);  // Быстрое освобождение всей arena
    
    // Печать статистики
    memory_print_stats(manager);
    
    // Очистка
    memory_pool_destroy(connection_pool);
    memory_arena_destroy(buffer_arena);
    memory_manager_cleanup(manager);
    
    return 0;
}
```

## Производительность

### Ожидаемые улучшения:
- **Пул аллокатор**: 5-10x ускорение по сравнению с malloc
- **Arena аллокатор**: 10-50x ускорение для временных объектов
- **Снижение фрагментации**: до 90% уменьшение фрагментации
- **Обнаружение ошибок**: автоматическое выявление memory leaks

### Тестирование производительности:
```bash
# Компиляция тестов
make memory-benchmark

# Запуск бенчмарка
./bin/memory-benchmark --allocator=pool --iterations=1000000 --object-size=64
```

## Мониторинг в реальном времени

```c
// Периодический мониторинг
while (running) {
    const memory_stats_t *stats = memory_get_stats(manager);
    
    if (stats->current_allocated_bytes > THRESHOLD) {
        printf("Memory usage warning: %zu bytes\n", stats->current_allocated_bytes);
        memory_compact(manager);
    }
    
    if (stats->failed_allocation_count > 0) {
        printf("Memory allocation failures detected: %zu\n", 
               stats->failed_allocation_count);
    }
    
    sleep(1);
}
```

## Совместимость

- **Windows**: Поддержка VirtualAlloc/VirtualFree
- **Linux**: Поддержка mmap/munmap
- **macOS**: Совместимость через mmap
- **FreeBSD**: Поддержка через системные вызовы

## Безопасность

- Защита от переполнения буфера
- Обнаружение двойного освобождения
- Защита от использования после освобождения
- Верификация целостности памяти
- Защитные страницы памяти