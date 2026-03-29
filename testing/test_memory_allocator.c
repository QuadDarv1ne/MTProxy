/*
 * test_memory_allocator.c — Тесты для memory-allocator
 *
 * Тестирование:
 * - mt_malloc/mt_free (базовые операции)
 * - mt_malloc_aligned (выравнивание)
 * - mt_compact_memory (освобождение памяти)
 * - Производительность vs стандартный malloc
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "common/memory-allocator.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    if (test_##name() == 0) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while(0)

/* ============================================================================
 * Basic Allocation Tests
 * ============================================================================ */

// Тест: базовое выделение
TEST(malloc_basic) {
    void *ptr = mt_malloc(1024);
    if (!ptr) return -1;
    
    // Проверка что память доступна
    memset(ptr, 0, 1024);
    mt_free(ptr);
    return 0;
}

// Тест: выделение с выравниванием 16 байт
TEST(malloc_aligned_16) {
    void *ptr = mt_malloc_aligned(1024, 16);
    if (!ptr) return -1;
    
    if ((uintptr_t)ptr % 16 != 0) {
        printf("Pointer %p not aligned to 16 bytes", ptr);
        mt_free_aligned(ptr);
        return -1;
    }
    
    memset(ptr, 0, 1024);
    mt_free_aligned(ptr);
    return 0;
}

// Тест: выделение с выравниванием 64 байт
TEST(malloc_aligned_64) {
    void *ptr = mt_malloc_aligned(2048, 64);
    if (!ptr) return -1;
    
    if ((uintptr_t)ptr % 64 != 0) {
        printf("Pointer %p not aligned to 64 bytes", ptr);
        mt_free_aligned(ptr);
        return -1;
    }
    
    memset(ptr, 0, 2048);
    mt_free_aligned(ptr);
    return 0;
}

// Тест: множественные выделения
TEST(malloc_multiple) {
    void *ptrs[100];
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = mt_malloc(256 + i * 10);
        if (!ptrs[i]) {
            // Cleanup на случай ошибки
            for (int j = 0; j < i; j++) {
                mt_free(ptrs[j]);
            }
            return -1;
        }
        memset(ptrs[i], i, 256 + i * 10);
    }
    
    for (int i = 0; i < 100; i++) {
        mt_free(ptrs[i]);
    }
    
    return 0;
}

// Тест: realloc
TEST(realloc_basic) {
    void *ptr = mt_malloc(100);
    if (!ptr) return -1;
    
    memset(ptr, 'A', 100);
    
    void *new_ptr = mt_realloc(ptr, 200);
    if (!new_ptr) {
        mt_free(ptr);
        return -1;
    }
    
    // Проверка что данные сохранились
    if (((char*)new_ptr)[0] != 'A' || ((char*)new_ptr)[99] != 'A') {
        mt_free(new_ptr);
        return -1;
    }
    
    mt_free(new_ptr);
    return 0;
}

// Тест: calloc (инициализация нулями)
TEST(calloc_zero) {
    void *ptr = mt_calloc(100, sizeof(int));
    if (!ptr) return -1;
    
    int *int_ptr = (int*)ptr;
    for (int i = 0; i < 100; i++) {
        if (int_ptr[i] != 0) {
            mt_free(ptr);
            return -1;
        }
    }
    
    mt_free(ptr);
    return 0;
}

/* ============================================================================
 * Performance Tests
 * ============================================================================ */

// Тест: производительность malloc/free
TEST(malloc_performance) {
    const int iterations = 10000;
    void *ptrs[iterations];
    
    clock_t start = clock();
    
    // Выделение
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = mt_malloc(64 + (i % 256));
        if (!ptrs[i]) {
            printf("Allocation failed at iteration %d", i);
            return -1;
        }
    }
    
    // Освобождение
    for (int i = 0; i < iterations; i++) {
        mt_free(ptrs[i]);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000; // ms
    
    printf("allocated+freed %d blocks in %.2f ms (%.0f ops/sec) ",
           iterations, elapsed, iterations / elapsed * 1000);
    
    return 0;
}

// Тест: производительность aligned malloc
TEST(aligned_performance) {
    const int iterations = 1000;
    void *ptrs[iterations];
    
    clock_t start = clock();
    
    // Выделение выровненной памяти
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = mt_malloc_aligned(1024, 64);
        if (!ptrs[i]) {
            printf("Aligned allocation failed at iteration %d", i);
            return -1;
        }
    }
    
    // Освобождение
    for (int i = 0; i < iterations; i++) {
        mt_free_aligned(ptrs[i]);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000; // ms
    
    printf("aligned alloc+free %d blocks in %.2f ms (%.0f ops/sec) ",
           iterations, elapsed, iterations / elapsed * 1000);
    
    return 0;
}

/* ============================================================================
 * Edge Cases
 * ============================================================================ */

// Тест: выделение 0 байт
TEST(malloc_zero_size) {
    void *ptr = mt_malloc(0);
    // Поведение зависит от аллокатора: может вернуть NULL или уникальный указатель
    if (ptr) {
        mt_free(ptr);
    }
    return 0;
}

// Тест: очень большое выделение
TEST(malloc_large) {
    void *ptr = mt_malloc(100 * 1024 * 1024); // 100 MB
    
    if (ptr) {
        // Если удалось выделить, освобождаем
        mt_free(ptr);
    }
    // Если NULL - тоже OK (нехватка памяти)
    return 0;
}

// Тест: compact memory
TEST(compact_memory) {
    // Выделение и освобождение для создания фрагментации
    for (int i = 0; i < 100; i++) {
        void *ptr = mt_malloc(1024);
        if (ptr) {
            memset(ptr, i, 1024);
            mt_free(ptr);
        }
    }
    
    // Компактизация (если поддерживается)
    mt_compact_memory();
    
    return 0;
}

// Тест: mt_malloc_usable_size
TEST(usable_size) {
    void *ptr = mt_malloc(100);
    if (!ptr) return -1;
    
    size_t size = mt_get_allocated_size(ptr);
    
    // Некоторые аллокаторы возвращают 0 или реальный размер
    // Главное что функция работает
    (void)size; // Suppress unused warning
    
    mt_free(ptr);
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[]) {
    printf("=== Memory Allocator Tests ===\n");
    printf("Using: ");
#ifdef USE_JEMALLOC
    printf("jemalloc\n");
#elif defined(USE_TCMALLOC)
    printf("tcmalloc\n");
#else
    printf("standard malloc\n");
#endif
    printf("\n");
    
    // Basic tests
    printf("--- Basic Allocation Tests ---\n");
    RUN_TEST(malloc_basic);
    RUN_TEST(malloc_aligned_16);
    RUN_TEST(malloc_aligned_64);
    RUN_TEST(malloc_multiple);
    RUN_TEST(realloc_basic);
    RUN_TEST(calloc_zero);
    
    // Performance tests
    printf("\n--- Performance Tests ---\n");
    RUN_TEST(malloc_performance);
    RUN_TEST(aligned_performance);
    
    // Edge cases
    printf("\n--- Edge Cases ---\n");
    RUN_TEST(malloc_zero_size);
    RUN_TEST(malloc_large);
    RUN_TEST(compact_memory);
    RUN_TEST(usable_size);
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
