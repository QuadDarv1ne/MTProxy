/*
 * test_utils_security.c — Тесты для security utils
 *
 * Тестирование:
 * - utils_strcpy_s (безопасное копирование)
 * - utils_strcat_s (безопасная конкатенация)
 * - utils_snprintf (безопасный snprintf)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/utils.h"

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
 * utils_strcpy_s Tests
 * ============================================================================ */

// Тест: базовое копирование
TEST(strcpy_s_basic) {
    char dest[64];
    int result = utils_strcpy_s(dest, sizeof(dest), "Hello, World!");
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strcmp(dest, "Hello, World!") != 0) {
        printf("Expected 'Hello, World!', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

// Тест: копирование с усечением
TEST(strcpy_s_truncation) {
    char dest[8];
    int result = utils_strcpy_s(dest, sizeof(dest), "Hello, World!");
    
    if (result != -1) {
        printf("Expected -1 (truncation), got %d", result);
        return -1;
    }
    
    if (strlen(dest) != 7) {
        printf("Expected length 7, got %zu", strlen(dest));
        return -1;
    }
    
    // Проверка null-терминации
    if (dest[7] != '\0') {
        printf("Not null-terminated");
        return -1;
    }
    
    return 0;
}

// Тест: копирование NULL источника
TEST(strcpy_s_null_src) {
    char dest[64];
    int result = utils_strcpy_s(dest, sizeof(dest), NULL);
    
    if (result != -1) {
        printf("Expected -1 for NULL src, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: копирование в NULL dest
TEST(strcpy_s_null_dest) {
    int result = utils_strcpy_s(NULL, 64, "test");
    
    if (result != -1) {
        printf("Expected -1 for NULL dest, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: копирование с размером 0
TEST(strcpy_s_zero_size) {
    char dest[64];
    int result = utils_strcpy_s(dest, 0, "test");
    
    if (result != -1) {
        printf("Expected -1 for zero size, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: точный размер (без места для null)
TEST(strcpy_s_exact_size) {
    char dest[6];
    int result = utils_strcpy_s(dest, sizeof(dest), "Hello");
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strcmp(dest, "Hello") != 0) {
        printf("Expected 'Hello', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * utils_strcat_s Tests
 * ============================================================================ */

// Тест: базовая конкатенация
TEST(strcat_s_basic) {
    char dest[64] = "Hello";
    int result = utils_strcat_s(dest, sizeof(dest), ", World!");
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strcmp(dest, "Hello, World!") != 0) {
        printf("Expected 'Hello, World!', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

// Тест: конкатенация с усечением
TEST(strcat_s_truncation) {
    char dest[10] = "Hello";
    int result = utils_strcat_s(dest, sizeof(dest), ", World!");
    
    if (result != -1) {
        printf("Expected -1 (truncation), got %d", result);
        return -1;
    }
    
    // Должно быть "Hello, W" + null
    if (strlen(dest) != 9) {
        printf("Expected length 9, got %zu: '%s'", strlen(dest), dest);
        return -1;
    }
    
    return 0;
}

// Тест: конкатенация с NULL src
TEST(strcat_s_null_src) {
    char dest[64] = "Hello";
    int result = utils_strcat_s(dest, sizeof(dest), NULL);
    
    if (result != -1) {
        printf("Expected -1 for NULL src, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: конкатенация с NULL dest
TEST(strcat_s_null_dest) {
    int result = utils_strcat_s(NULL, 64, "test");
    
    if (result != -1) {
        printf("Expected -1 for NULL dest, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: конкатенация с полным буфером
TEST(strcat_s_full_buffer) {
    char dest[6] = "Hello";  // Уже заполнено (5 символов + null)
    int result = utils_strcat_s(dest, sizeof(dest), " World");
    
    if (result != -1) {
        printf("Expected -1 (no space), got %d", result);
        return -1;
    }
    
    // Буфер не должен измениться
    if (strcmp(dest, "Hello") != 0) {
        printf("Buffer modified: '%s'", dest);
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * utils_snprintf Tests
 * ============================================================================ */

// Тест: базовый snprintf
TEST(snprintf_basic) {
    char dest[64];
    int result = utils_snprintf(dest, sizeof(dest), "Value: %d", 42);
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strcmp(dest, "Value: 42") != 0) {
        printf("Expected 'Value: 42', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

// Тест: snprintf с усечением
TEST(snprintf_truncation) {
    char dest[8];
    int result = utils_snprintf(dest, sizeof(dest), "Value: %d", 12345);
    
    if (result != -1) {
        printf("Expected -1 (truncation), got %d", result);
        return -1;
    }
    
    if (strlen(dest) != 7) {
        printf("Expected length 7, got %zu: '%s'", strlen(dest), dest);
        return -1;
    }
    
    // Проверка null-терминации
    if (dest[7] != '\0') {
        printf("Not null-terminated");
        return -1;
    }
    
    return 0;
}

// Тест: snprintf с NULL dest
TEST(snprintf_null_dest) {
    int result = utils_snprintf(NULL, 64, "test");
    
    if (result != -1) {
        printf("Expected -1 for NULL dest, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: snprintf с NULL format
TEST(snprintf_null_format) {
    char dest[64];
    int result = utils_snprintf(dest, sizeof(dest), NULL);
    
    if (result != -1) {
        printf("Expected -1 for NULL format, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: snprintf с размером 0
TEST(snprintf_zero_size) {
    char dest[64];
    int result = utils_snprintf(dest, 0, "test");
    
    if (result != -1) {
        printf("Expected -1 for zero size, got %d", result);
        return -1;
    }
    
    return 0;
}

// Тест: snprintf со сложным форматом
TEST(snprintf_complex_format) {
    char dest[128];
    int result = utils_snprintf(dest, sizeof(dest), "%s: %d (%.2f%%)", "Test", 42, 99.5);
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strcmp(dest, "Test: 42 (99.50%)") != 0) {
        printf("Expected 'Test: 42 (99.50%%)', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * Edge Cases
 * ============================================================================ */

// Тест: пустая строка
TEST(strcpy_s_empty) {
    char dest[64] = "Original";
    int result = utils_strcpy_s(dest, sizeof(dest), "");
    
    if (result != 0) {
        printf("Expected 0, got %d", result);
        return -1;
    }
    
    if (strlen(dest) != 0) {
        printf("Expected empty string, got '%s'", dest);
        return -1;
    }
    
    return 0;
}

// Тест: длинная строка
TEST(strcpy_s_long) {
    char dest[256];
    char source[512];
    memset(source, 'A', sizeof(source) - 1);
    source[sizeof(source) - 1] = '\0';
    
    int result = utils_strcpy_s(dest, sizeof(dest), source);
    
    if (result != -1) {
        printf("Expected -1 (truncation), got %d", result);
        return -1;
    }
    
    if (strlen(dest) != 255) {
        printf("Expected length 255, got %zu", strlen(dest));
        return -1;
    }
    
    return 0;
}

// Тест: multiple strcat
TEST(strcat_s_multiple) {
    char dest[64] = "A";
    int result;
    
    result = utils_strcat_s(dest, sizeof(dest), "B");
    if (result != 0) return -1;
    
    result = utils_strcat_s(dest, sizeof(dest), "C");
    if (result != 0) return -1;
    
    result = utils_strcat_s(dest, sizeof(dest), "D");
    if (result != 0) return -1;
    
    if (strcmp(dest, "ABCD") != 0) {
        printf("Expected 'ABCD', got '%s'", dest);
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[]) {
    printf("=== Utils Security Tests ===\n\n");
    
    // utils_strcpy_s tests
    printf("--- utils_strcpy_s Tests ---\n");
    RUN_TEST(strcpy_s_basic);
    RUN_TEST(strcpy_s_truncation);
    RUN_TEST(strcpy_s_null_src);
    RUN_TEST(strcpy_s_null_dest);
    RUN_TEST(strcpy_s_zero_size);
    RUN_TEST(strcpy_s_exact_size);
    
    // utils_strcat_s tests
    printf("\n--- utils_strcat_s Tests ---\n");
    RUN_TEST(strcat_s_basic);
    RUN_TEST(strcat_s_truncation);
    RUN_TEST(strcat_s_null_src);
    RUN_TEST(strcat_s_null_dest);
    RUN_TEST(strcat_s_full_buffer);
    
    // utils_snprintf tests
    printf("\n--- utils_snprintf Tests ---\n");
    RUN_TEST(snprintf_basic);
    RUN_TEST(snprintf_truncation);
    RUN_TEST(snprintf_null_dest);
    RUN_TEST(snprintf_null_format);
    RUN_TEST(snprintf_zero_size);
    RUN_TEST(snprintf_complex_format);
    
    // Edge cases
    printf("\n--- Edge Cases ---\n");
    RUN_TEST(strcpy_s_empty);
    RUN_TEST(strcpy_s_long);
    RUN_TEST(strcat_s_multiple);
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
