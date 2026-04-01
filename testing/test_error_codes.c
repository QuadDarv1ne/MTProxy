/**
 * @file test_error_codes.c
 * @brief Тесты для системы кодов ошибок
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../common/error-codes.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  ✗ FAILED: %s\n", msg); \
    } \
} while(0)

/* ============================================
 * Тесты базовых ошибок
 * ============================================ */

static void test_error_codes_basic(void) {
    printf("\n=== Test: Basic Error Codes ===\n");
    
    TEST_ASSERT(MT_SUCCESS == 0, "MT_SUCCESS is 0");
    TEST_ASSERT(MT_ERR_GENERAL == -1, "MT_ERR_GENERAL is -1");
    TEST_ASSERT(MT_ERR_NO_MEMORY == -2, "MT_ERR_NO_MEMORY is -2");
    TEST_ASSERT(MT_ERR_INVALID_ARGS == -3, "MT_ERR_INVALID_ARGS is -3");
    TEST_ASSERT(MT_ERR_TIMEOUT == -8, "MT_ERR_TIMEOUT is -8");
}

/* ============================================
 * Тесты сетевых ошибок
 * ============================================ */

static void test_error_codes_network(void) {
    printf("\n=== Test: Network Error Codes ===\n");
    
    TEST_ASSERT(MT_ERR_SOCKET == -100, "MT_ERR_SOCKET is -100");
    TEST_ASSERT(MT_ERR_CONNECT == -101, "MT_ERR_CONNECT is -101");
    TEST_ASSERT(MT_ERR_SEND == -105, "MT_ERR_SEND is -105");
    TEST_ASSERT(MT_ERR_RECV == -106, "MT_ERR_RECV is -106");
    TEST_ASSERT(MT_ERR_DNS == -107, "MT_ERR_DNS is -107");
}

/* ============================================
 * Тесты ML ошибок
 * ============================================ */

static void test_error_codes_ml(void) {
    printf("\n=== Test: ML Error Codes ===\n");
    
    TEST_ASSERT(MT_ERR_ML_MODEL == -400, "MT_ERR_ML_MODEL is -400");
    TEST_ASSERT(MT_ERR_ML_TRAIN == -401, "MT_ERR_ML_TRAIN is -401");
    TEST_ASSERT(MT_ERR_ML_PREDICT == -402, "MT_ERR_ML_PREDICT is -402");
    TEST_ASSERT(MT_ERR_ML_INSUFFICIENT_DATA == -403, "MT_ERR_ML_INSUFFICIENT_DATA is -403");
}

/* ============================================
 * Тесты строковых описаний
 * ============================================ */

static void test_error_strings(void) {
    printf("\n=== Test: Error Strings ===\n");
    
    const char *str;
    
    str = mt_error_string(MT_SUCCESS);
    TEST_ASSERT(str != NULL, "Error string for SUCCESS is not NULL");
    TEST_ASSERT(strcmp(str, "Success") == 0, "Error string for SUCCESS is correct");
    
    str = mt_error_string(MT_ERR_NO_MEMORY);
    TEST_ASSERT(str != NULL, "Error string for NO_MEMORY is not NULL");
    TEST_ASSERT(strcmp(str, "Out of memory") == 0, "Error string for NO_MEMORY is correct");
    
    str = mt_error_string(MT_ERR_TIMEOUT);
    TEST_ASSERT(str != NULL, "Error string for TIMEOUT is not NULL");
    TEST_ASSERT(strcmp(str, "Timeout") == 0, "Error string for TIMEOUT is correct");
    
    str = mt_error_string(MT_ERR_ML_PREDICT);
    TEST_ASSERT(str != NULL, "Error string for ML_PREDICT is not NULL");
    TEST_ASSERT(strcmp(str, "ML prediction error") == 0, "Error string for ML_PREDICT is correct");
    
    str = mt_error_string(-999);
    TEST_ASSERT(str != NULL, "Error string for unknown code is not NULL");
    TEST_ASSERT(strcmp(str, "Unknown error") == 0, "Error string for unknown code is correct");
}

/* ============================================
 * Тесты категорий ошибок
 * ============================================ */

static void test_error_categories(void) {
    printf("\n=== Test: Error Categories ===\n");
    
    TEST_ASSERT(mt_error_category(MT_SUCCESS) == 0, "Category of SUCCESS is 0");
    TEST_ASSERT(mt_error_category(MT_ERR_GENERAL) == 1, "Category of GENERAL is 1");
    TEST_ASSERT(mt_error_category(MT_ERR_SOCKET) == 2, "Category of SOCKET is 2");
    TEST_ASSERT(mt_error_category(MT_ERR_FRAGMENT) == 3, "Category of FRAGMENT is 3");
    TEST_ASSERT(mt_error_category(MT_ERR_PADDING) == 4, "Category of PADDING is 4");
    TEST_ASSERT(mt_error_category(MT_ERR_ML_MODEL) == 5, "Category of ML_MODEL is 5");
    TEST_ASSERT(mt_error_category(MT_ERR_PERF_MONITOR) == 6, "Category of PERF_MONITOR is 6");
    TEST_ASSERT(mt_error_category(MT_ERR_CLI) == 7, "Category of CLI is 7");
    TEST_ASSERT(mt_error_category(-999) == 99, "Category of unknown is 99");
}

/* ============================================
 * Тесты макросов
 * ============================================ */

static int test_function_with_null(int *ptr) {
    MT_CHECK_PTR(ptr, MT_ERR_INVALID_ARGS);
    return MT_SUCCESS;
}

static int test_function_with_cond(int value) {
    MT_CHECK_COND(value > 0, MT_ERR_INVALID_ARGS);
    return MT_SUCCESS;
}

static void test_error_macros(void) {
    printf("\n=== Test: Error Macros ===\n");
    
    int ret;
    
    ret = test_function_with_null(NULL);
    TEST_ASSERT(ret == MT_ERR_INVALID_ARGS, "MT_CHECK_PTR returns error for NULL");
    
    int dummy = 42;
    ret = test_function_with_null(&dummy);
    TEST_ASSERT(ret == MT_SUCCESS, "MT_CHECK_PTR returns success for valid ptr");
    
    ret = test_function_with_cond(0);
    TEST_ASSERT(ret == MT_ERR_INVALID_ARGS, "MT_CHECK_COND returns error for false condition");
    
    ret = test_function_with_cond(1);
    TEST_ASSERT(ret == MT_SUCCESS, "MT_CHECK_COND returns success for true condition");
}

/* ============================================
 * Главная функция тестирования
 * ============================================ */

int main(void) {
    printf("\n============================================\n");
    printf("  Error Codes System Tests\n");
    printf("  Version: 1.0.33\n");
    printf("============================================\n");
    
    test_error_codes_basic();
    test_error_codes_network();
    test_error_codes_ml();
    test_error_strings();
    test_error_categories();
    test_error_macros();
    
    /* Итоговый отчет */
    printf("\n============================================\n");
    printf("  Test Summary\n");
    printf("============================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n",
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("============================================\n\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
