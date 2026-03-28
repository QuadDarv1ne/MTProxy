/*
    Реализация глобального отслеживания памяти MTProxy
    OOM Protection (Out Of Memory)
*/

#define _FILE_OFFSET_BITS 64

#include "common/memory-limits.h"
#include "common/kprintf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Глобальный трекер
memory_tracker_t g_memory_tracker = {0};

// Простая реализация для Windows (без POSIX)
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Получение текущего использования памяти процесса
__attribute__((unused))
static size_t get_process_memory_usage(void) {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return (size_t)(status.ullTotalPhys - status.ullAvailPhys);
    }
    return 0;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (size_t)(pages * page_size);
    }
    return 0;
#endif
}

// Инициализация трекера памяти
int memory_tracker_init(memory_tracker_t *tracker) {
    if (!tracker) return -1;

    memset(tracker, 0, sizeof(memory_tracker_t));
    tracker->warning_triggered = 0;
    tracker->critical_triggered = 0;
    tracker->peak_usage = 0;

    vkprintf(1, "Memory tracker initialized: limits - cache=%dMB, pool=%dMB, total=%dMB\n",
             MAX_CACHE_SIZE_MB, MAX_POOL_SIZE_MB, MAX_TOTAL_MANAGED_MEMORY_MB);

    return 0;
}

// Проверка лимита перед выделением
int check_memory_limit(size_t requested_size, memory_tracker_t *tracker) {
    if (!tracker) return 0;

    // Проверка на максимальное单次 выделение
    if (requested_size > MAX_SINGLE_ALLOCATION_BYTES) {
        fprintf(stderr, "ERROR: Allocation too large: %zu bytes (max: %d MB)\n",
                requested_size, MAX_SINGLE_ALLOCATION_MB);
        return 0;
    }

    // Проверка общего лимита
    size_t new_usage = tracker->current_usage + requested_size;
    size_t max_limit = (size_t)MAX_TOTAL_MANAGED_MEMORY_MB * 1024 * 1024;

    if (new_usage > max_limit) {
        if (!tracker->warning_triggered) {
            fprintf(stderr, "WARNING: Memory usage approaching limit (%zu/%zu MB)\n",
                    tracker->current_usage / (1024*1024), max_limit / (1024*1024));
            tracker->warning_triggered = 1;
        }

        if (new_usage > max_limit * MEMORY_CRITICAL_THRESHOLD_PERCENT / 100) {
            if (!tracker->critical_triggered) {
                fprintf(stderr, "CRITICAL: Memory limit exceeded! Blocking allocation.\n");
                tracker->critical_triggered = 1;
            }
            return 0;
        }
    }

    return 1;
}

// Отслеживаемое выделение
void* tracked_malloc(size_t size, memory_tracker_t *tracker) {
    if (!tracker || size == 0) return NULL;

    if (!check_memory_limit(size, tracker)) {
        return NULL;
    }

#if defined(__GNUC__) && !defined(_WIN32)
    void *ptr = __real_malloc(size);
#else
    void *ptr = malloc(size);
#endif
    if (ptr) {
        tracker->total_allocated += size;
        tracker->current_usage += size;
        tracker->allocation_count++;

        if (tracker->current_usage > tracker->peak_usage) {
            tracker->peak_usage = tracker->current_usage;
        }
    }

    return ptr;
}

// Отслеживаемое calloc
void* tracked_calloc(size_t nmemb, size_t size, memory_tracker_t *tracker) {
    if (!tracker || nmemb == 0 || size == 0) return NULL;

    size_t total_size = nmemb * size;
    if (!check_memory_limit(total_size, tracker)) {
        return NULL;
    }

#if defined(__GNUC__) && !defined(_WIN32)
    void *ptr = __real_calloc(nmemb, size);
#else
    void *ptr = calloc(nmemb, size);
#endif
    if (ptr) {
        tracker->total_allocated += total_size;
        tracker->current_usage += total_size;
        tracker->allocation_count++;

        if (tracker->current_usage > tracker->peak_usage) {
            tracker->peak_usage = tracker->current_usage;
        }
    }

    return ptr;
}

// Отслеживаемое realloc
void* tracked_realloc(void *ptr, size_t size, memory_tracker_t *tracker) {
    if (!tracker) return NULL;

    if (size == 0) {
        if (ptr) tracked_free(ptr, tracker);
        return NULL;
    }

    if (!check_memory_limit(size, tracker)) {
        return NULL;
    }

#if defined(__GNUC__) && !defined(_WIN32)
    void *new_ptr = __real_realloc(ptr, size);
#else
    void *new_ptr = realloc(ptr, size);
#endif
    if (new_ptr && ptr) {
        // Корректировка usage на разницу
        if (size > 0) {
            tracker->current_usage += size;
        }
        tracker->allocation_count++;
    }

    return new_ptr;
}

// Отслеживаемое освобождение
void tracked_free(void *ptr, memory_tracker_t *tracker) {
    if (!ptr || !tracker) return;

    // В production реализации нужно хранить метаданные о размере
    // Для простоты используем эвристику: уменьшаем на среднее значение
    size_t avg_alloc_size = tracker->allocation_count > 0 
        ? tracker->total_allocated / tracker->allocation_count 
        : 256; // default fallback
    
    if (tracker->current_usage >= avg_alloc_size) {
        tracker->current_usage -= avg_alloc_size;
    } else {
        tracker->current_usage = 0;
    }
    
    tracker->free_count++;
    tracker->total_freed += avg_alloc_size;

#if defined(__GNUC__) && !defined(_WIN32)
    __real_free(ptr);
#else
    free(ptr);
#endif
}

// Проверка статуса памяти
int is_memory_warning(void) {
    return g_memory_tracker.warning_triggered;
}

int is_memory_critical(void) {
    return g_memory_tracker.critical_triggered;
}

// Получение статистики
void get_memory_stats(size_t *current, size_t *peak, size_t *limit) {
    if (current) *current = g_memory_tracker.current_usage;
    if (peak) *peak = g_memory_tracker.peak_usage;
    if (limit) *limit = (size_t)MAX_TOTAL_MANAGED_MEMORY_MB * 1024 * 1024;
}

// Печать использования памяти
void print_memory_usage(void) {
    printf("\n========== Memory Usage ==========\n");
    printf("Current:  %zu MB\n", g_memory_tracker.current_usage / (1024*1024));
    printf("Peak:     %zu MB\n", g_memory_tracker.peak_usage / (1024*1024));
    printf("Limit:    %d MB\n", MAX_TOTAL_MANAGED_MEMORY_MB);
    printf("Allocs:   %zu\n", g_memory_tracker.allocation_count);
    printf("Frees:    %zu\n", g_memory_tracker.free_count);
    printf("Warning:  %s\n", g_memory_tracker.warning_triggered ? "YES" : "NO");
    printf("Critical: %s\n", g_memory_tracker.critical_triggered ? "YES" : "NO");
    printf("===================================\n\n");
}
