/*
    Test Memory Utilities - Утилиты для управления памятью в тестах
    Оптимизация потребления памяти для Windows
*/

#ifndef TEST_MEMORY_UTILS_H
#define TEST_MEMORY_UTILS_H

#ifdef _WIN32
    #include <windows.h>
    
    /*
     * Освобождение памяти между тестами (Windows)
     * HeapCompact уменьшает фрагментацию и возвращает память ОС
     */
    static inline void test_memory_compact(void) {
        HeapCompact(GetProcessHeap(), 0);
    }
    
    /*
     * Получение текущего использования памяти (Windows)
     */
    static inline size_t test_get_memory_usage_mb(void) {
        MEMORYSTATUSEX status = {0};
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return (status.ullTotalPhys - status.ullAvailPhys) / (1024 * 1024);
    }
    
    /*
     * Принудительное освобождение памяти
     */
    static inline void test_free_memory(void) {
        // Вызов HeapCompact для сжатия кучи
        HeapCompact(GetProcessHeap(), 0);
    }
#else
    #include <sys/resource.h>
    
    /*
     * Освобождение памяти между тестами (Unix)
     * На Unix нет прямого аналога HeapCompact
     */
    static inline void test_memory_compact(void) {
        // Можно вызвать malloc_trim для glibc
        #ifdef __GLIBC__
        extern void malloc_trim(size_t pad);
        malloc_trim(0);
        #endif
    }
    
    /*
     * Получение текущего использования памяти (Unix)
     */
    static inline size_t test_get_memory_usage_mb(void) {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss / 1024;  // KB -> MB
    }
    
    /*
     * Принудительное освобождение памяти
     */
    static inline void test_free_memory(void) {
        #ifdef __GLIBC__
        extern void malloc_trim(size_t pad);
        malloc_trim(0);
        #endif
    }
#endif

/*
 * Макрос для освобождения памяти между тестами
 */
#define TEST_MEMORY_BARRIER() do { \
    test_memory_compact(); \
} while(0)

/*
 * Макрос для печати статистики памяти
 */
#define TEST_PRINT_MEMORY_USAGE(label) do { \
    size_t mem_mb = test_get_memory_usage_mb(); \
    printf("  [%s] Memory usage: %zu MB\n", label, mem_mb); \
} while(0)

/*
 * Макрос для теста с автоматическим освобождением памяти
 */
#define TEST_WITH_MEMORY_CLEANUP(test_func) do { \
    printf("\n=== Running %s ===\n", #test_func); \
    TEST_PRINT_MEMORY_USAGE("Before"); \
    test_func(); \
    TEST_PRINT_MEMORY_USAGE("After"); \
    TEST_MEMORY_BARRIER(); \
    TEST_PRINT_MEMORY_USAGE("Cleaned"); \
} while(0)

#endif /* TEST_MEMORY_UTILS_H */
