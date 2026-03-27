/*
    Глобальные ограничения памяти MTProxy
    Защита от переполнения оперативной памяти (OOM Protection)
*/

#ifndef MEMORY_LIMITS_H
#define MEMORY_LIMITS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================
// Глобальные лимиты памяти (настраиваемые)
// ============================================

// Лимиты по умолчанию (могут быть переопределены через CFLAGS)
#ifndef MAX_CACHE_SIZE_MB
#define MAX_CACHE_SIZE_MB 128
#endif

#ifndef MAX_POOL_SIZE_MB
#define MAX_POOL_SIZE_MB 64
#endif

// Лимиты для различных подсистем
#define MAX_CRYPTO_CACHE_MB      16
#define MAX_DH_CACHE_ENTRIES     1000
#define MAX_AES_KEY_CACHE_ENTRIES 1000
#define MAX_CONNECTION_POOL_SIZE 500
#define MAX_RATE_LIMIT_ENTRIES   10000
#define MAX_ERROR_TRACKING_SIZE_MB 8
#define MAX_LOG_BUFFER_MB        16
#define MAX_METRIC_ENTRIES       5000
#define MAX_TRACE_SPANS        10000

// Агрегатные лимиты
#define MAX_TOTAL_MANAGED_MEMORY_MB (MAX_CACHE_SIZE_MB + MAX_POOL_SIZE_MB + \
                                     MAX_CRYPTO_CACHE_MB + MAX_LOG_BUFFER_MB)

// Лимиты для одного выделения
#define MAX_SINGLE_ALLOCATION_MB   32
#define MAX_SINGLE_ALLOCATION_BYTES (MAX_SINGLE_ALLOCATION_MB * 1024 * 1024)

// Пороги тревоги
#define MEMORY_WARNING_THRESHOLD_PERCENT 80
#define MEMORY_CRITICAL_THRESHOLD_PERCENT 95

// ============================================
// Структуры для отслеживания памяти
// ============================================

typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
    int warning_triggered;
    int critical_triggered;
} memory_tracker_t;

// ============================================
// Функции отслеживания и ограничения
// ============================================

// Инициализация трекера памяти
int memory_tracker_init(memory_tracker_t *tracker);

// Отслеживание выделения
void* tracked_malloc(size_t size, memory_tracker_t *tracker);
void* tracked_calloc(size_t nmemb, size_t size, memory_tracker_t *tracker);
void* tracked_realloc(void *ptr, size_t size, memory_tracker_t *tracker);
void tracked_free(void *ptr, memory_tracker_t *tracker);

// Проверка лимитов
int check_memory_limit(size_t requested_size, memory_tracker_t *tracker);
int is_memory_warning(void);
int is_memory_critical(void);

// Получение статистики
void get_memory_stats(size_t *current, size_t *peak, size_t *limit);
void print_memory_usage(void);

// Глобальный трекер (extern для использования в других модулях)
extern memory_tracker_t g_memory_tracker;

// Макросы для замены стандартных функций выделения
// Внимание: используется __real_* для избежания рекурсивного вызова
#ifdef ENABLE_MEMORY_TRACKING
#ifdef __GNUC__
#define malloc(s)  __real_malloc(s)
#define calloc(n,s) __real_calloc(n, s)
#define realloc(p,s) __real_realloc(p, s)
#define free(p) __real_free(p)
#else
// Для не-GCC компиляторов используем обёртки
void* __wrap_malloc(size_t size);
void* __wrap_calloc(size_t nmemb, size_t size);
void* __wrap_realloc(void *ptr, size_t size);
void __wrap_free(void *ptr);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif // MEMORY_LIMITS_H
