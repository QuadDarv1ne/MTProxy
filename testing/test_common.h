/*
 * test_common.h — Common test framework for MTProxy
 *
 * Provides unified test macros and helpers for all C tests.
 * Usage:
 *   #include "test_common.h"
 *
 *   TEST(my_test) {
 *       // test code
 *       return 0; // success
 *   }
 *
 *   int main(void) {
 *       RUN_ALL_TESTS();
 *   }
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ============================================================================
 * Test framework state
 * ============================================================================ */

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;
static int g_assertions = 0;

/* ============================================================================
 * Test macros
 * ============================================================================ */

#define TEST(name) \
    static int test_##name(void)

#define RUN_TEST(name) \
    do { \
        g_tests_run++; \
        printf("  %-40s ... ", #name); \
        fflush(stdout); \
        int result = test_##name(); \
        if (result == 0) { \
            printf("PASSED\n"); \
            g_tests_passed++; \
        } else { \
            printf("FAILED (code %d)\n", result); \
            g_tests_failed++; \
        } \
    } while (0)

/* ============================================================================
 * Assertion macros
 * ============================================================================ */

#define ASSERT(cond) \
    do { \
        g_assertions++; \
        if (!(cond)) { \
            printf("\n    ASSERTION FAILED: %s\n", #cond); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        g_assertions++; \
        if ((a) != (b)) { \
            printf("\n    ASSERT_EQ FAILED: %s == %s\n", #a, #b); \
            printf("    values: %lld == %lld\n", (long long)(a), (long long)(b)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        g_assertions++; \
        if ((a) == (b)) { \
            printf("\n    ASSERT_NE FAILED: %s != %s\n", #a, #b); \
            printf("    value: %lld\n", (long long)(a)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        g_assertions++; \
        if ((ptr) != NULL) { \
            printf("\n    ASSERT_NULL FAILED: %s is not NULL\n", #ptr); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        g_assertions++; \
        if ((ptr) == NULL) { \
            printf("\n    ASSERT_NOT_NULL FAILED: %s is NULL\n", #ptr); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        g_assertions++; \
        if (strcmp((a), (b)) != 0) { \
            printf("\n    ASSERT_STR_EQ FAILED: %s == %s\n", #a, #b); \
            printf("    values: \"%s\" == \"%s\"\n", (a), (b)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define ASSERT_RANGE(val, min, max) \
    do { \
        g_assertions++; \
        if ((val) < (min) || (val) > (max)) { \
            printf("\n    ASSERT_RANGE FAILED: %s in [%s, %s]\n", #val, #min, #max); \
            printf("    value: %lld not in [%lld, %lld]\n", \
                   (long long)(val), (long long)(min), (long long)(max)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

/* ============================================================================
 * Test runner
 * ============================================================================ */

#define PRINT_HEADER(title) \
    do { \
        printf("\n"); \
        printf("=== %s ===\n", title); \
        printf("\n"); \
    } while (0)

#define PRINT_SUMMARY() \
    do { \
        printf("\n"); \
        printf("=== Test Summary ===\n"); \
        printf("  Tests run:      %d\n", g_tests_run); \
        printf("  Tests passed:   %d\n", g_tests_passed); \
        printf("  Tests failed:   %d\n", g_tests_failed); \
        printf("  Assertions:     %d\n", g_assertions); \
        printf("  Success rate:   %.1f%%\n", \
               g_tests_run > 0 ? (100.0 * g_tests_passed / g_tests_run) : 0.0); \
        printf("\n"); \
    } while (0)

#define RUN_ALL_TESTS() \
    do { \
        printf("=== MTProxy Test Suite ===\n"); \
        printf("  Compiled: %s %s\n", __DATE__, __TIME__); \
        printf("\n"); \
        \
        /* Run all tests */ \
        run_all_tests(); \
        \
        /* Print summary */ \
        PRINT_SUMMARY(); \
        \
        return g_tests_failed > 0 ? 1 : 0; \
    } while (0)

/* Forward declaration */
void run_all_tests(void);

/* ============================================================================
 * Utility functions
 * ============================================================================ */

static inline void test_sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static inline double get_time_ms(void) {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

#endif /* TEST_COMMON_H */
