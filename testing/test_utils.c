/*
 * test_utils.c - Tests for common/utils.c utility functions
 *
 * This test suite validates the utility functions in common/utils.c
 * to ensure correctness and reliability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
#endif

#include "../common/utils.h"

#define TEST_PASS 1
#define TEST_FAIL 0

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("Running %s... ", #name); \
    if (test_##name()) { \
        printf("PASS\n"); \
        tests_passed++; \
    } else { \
        printf("FAIL\n"); \
        tests_failed++; \
    } \
} while (0)

#define ASSERT(cond) do { if (!(cond)) return TEST_FAIL; } while (0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) return TEST_FAIL; } while (0)
#define ASSERT_NEQ(a, b) do { if ((a) == (b)) return TEST_FAIL; } while (0)

/* ============================================================================
 * String Utility Tests
 * ============================================================================ */

TEST(utils_strcpy_basic) {
    char dest[32] = {0};
    const char *src = "Hello, World!";
    size_t copied = utils_strcpy(dest, src, sizeof(dest));
    
    ASSERT_EQ(copied, strlen(src));
    ASSERT_EQ(strcmp(dest, src), 0);
    
    return TEST_PASS;
}

TEST(utils_strcpy_truncate) {
    char dest[8] = {0};
    const char *src = "Hello, World!";
    size_t copied = utils_strcpy(dest, src, sizeof(dest));
    
    ASSERT_EQ(copied, sizeof(dest) - 1);  // Truncated
    ASSERT_EQ(strlen(dest), 7);
    ASSERT_EQ(dest[7], '\0');  // Null-terminated
    
    return TEST_PASS;
}

TEST(utils_strcat_basic) {
    char dest[32] = "Hello";
    const char *src = ", World!";
    size_t total = utils_strcat(dest, src, sizeof(dest));
    
    ASSERT_EQ(total, 12);
    ASSERT_EQ(strcmp(dest, "Hello, World!"), 0);
    
    return TEST_PASS;
}

TEST(utils_trim_basic) {
    char str[] = "  \t\nHello, World!\n\t  ";
    char *trimmed = utils_trim(str);
    
    ASSERT_EQ(strcmp(trimmed, "Hello, World!"), 0);
    
    return TEST_PASS;
}

TEST(utils_tolower_basic) {
    char str[] = "HELLO, WORLD!";
    char *lower = utils_tolower_str(str);
    
    ASSERT_EQ(strcmp(lower, "hello, world!"), 0);
    
    return TEST_PASS;
}

TEST(utils_toupper_basic) {
    char str[] = "hello, world!";
    char *upper = utils_toupper_str(str);
    
    ASSERT_EQ(strcmp(upper, "HELLO, WORLD!"), 0);
    
    return TEST_PASS;
}

/* ============================================================================
 * Memory Utility Tests
 * ============================================================================ */

TEST(utils_memcpy_basic) {
    char src[] = "Hello, World!";
    char dest[32] = {0};
    
    utils_memcpy(dest, src, strlen(src) + 1);
    ASSERT_EQ(strcmp(dest, src), 0);
    
    return TEST_PASS;
}

TEST(utils_memmove_overlap) {
    char str[] = "Hello, World!";
    
    // Overlapping copy (move forward)
    utils_memmove(str + 2, str, 5);
    ASSERT_EQ(strcmp(str, "HeHello, World!"), 0);
    
    return TEST_PASS;
}

TEST(utils_memzero_basic) {
    unsigned char buffer[32] = {0xDE, 0xAD, 0xBE, 0xEF};
    
    utils_memzero(buffer, sizeof(buffer));
    
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ASSERT_EQ(buffer[i], 0);
    }
    
    return TEST_PASS;
}

TEST(utils_memcmp_const_equal) {
    const char *s1 = "secret";
    const char *s2 = "secret";
    
    ASSERT_EQ(utils_memcmp_const(s1, s2, 6), 0);
    
    return TEST_PASS;
}

TEST(utils_memcmp_const_not_equal) {
    const char *s1 = "secret";
    const char *s2 = "secreX";
    
    ASSERT_NEQ(utils_memcmp_const(s1, s2, 6), 0);
    
    return TEST_PASS;
}

/* ============================================================================
 * Numeric Utility Tests
 * ============================================================================ */

TEST(utils_atoi_basic) {
    int result;
    
    ASSERT_EQ(utils_atoi("12345", &result), 0);
    ASSERT_EQ(result, 12345);
    
    ASSERT_EQ(utils_atoi("-42", &result), 0);
    ASSERT_EQ(result, -42);
    
    return TEST_PASS;
}

TEST(utils_atoi_invalid) {
    int result;
    
    // Invalid input
    ASSERT_EQ(utils_atoi("abc", &result), -1);
    ASSERT_EQ(utils_atoi("", &result), -1);
    ASSERT_EQ(utils_atoi("123abc", &result), -1);
    
    return TEST_PASS;
}

TEST(utils_parse_size_basic) {
    uint64_t result;
    
    ASSERT_EQ(utils_parse_size("1024", &result), 0);
    ASSERT_EQ(result, 1024);
    
    ASSERT_EQ(utils_parse_size("1K", &result), 0);
    ASSERT_EQ(result, 1024);
    
    ASSERT_EQ(utils_parse_size("1M", &result), 0);
    ASSERT_EQ(result, 1024 * 1024);
    
    ASSERT_EQ(utils_parse_size("1G", &result), 0);
    ASSERT_EQ(result, 1024 * 1024 * 1024);
    
    return TEST_PASS;
}

TEST(utils_clamp_basic) {
    ASSERT_EQ(utils_clamp_int(5, 0, 10), 5);
    ASSERT_EQ(utils_clamp_int(-5, 0, 10), 0);
    ASSERT_EQ(utils_clamp_int(15, 0, 10), 10);
    
    return TEST_PASS;
}

/* ============================================================================
 * Hash Utility Tests
 * ============================================================================ */

TEST(utils_hash_djb2_basic) {
    const char *data = "Hello, World!";
    uint32_t hash = utils_hash_djb2(data, strlen(data));
    
    ASSERT_NEQ(hash, 0);
    
    // Same data should produce same hash
    uint32_t hash2 = utils_hash_djb2(data, strlen(data));
    ASSERT_EQ(hash, hash2);
    
    return TEST_PASS;
}

TEST(utils_hash_fnv1a_basic) {
    const char *data = "Hello, World!";
    uint32_t hash = utils_hash_fnv1a(data, strlen(data));
    
    ASSERT_NEQ(hash, 0);
    
    // Same data should produce same hash
    uint32_t hash2 = utils_hash_fnv1a(data, strlen(data));
    ASSERT_EQ(hash, hash2);
    
    return TEST_PASS;
}

/* ============================================================================
 * Time Utility Tests
 * ============================================================================ */

TEST(utils_time_ms_basic) {
    uint64_t time1 = utils_time_ms();
    
    // Sleep for a bit (platform-independent)
#ifdef _WIN32
    Sleep(10);
#else
    usleep(10000);
#endif
    
    uint64_t time2 = utils_time_ms();
    
    ASSERT(time2 >= time1);
    ASSERT(time2 - time1 >= 5);  // At least 5ms passed
    
    return TEST_PASS;
}

TEST(utils_format_timestamp_basic) {
    char buf[64];
    uint64_t timestamp = utils_time_ms();
    
    int result = utils_format_timestamp(buf, sizeof(buf), timestamp);
    
    ASSERT_EQ(result, 0);
    ASSERT(strlen(buf) > 0);
    ASSERT(strlen(buf) >= 20);  // ISO 8601 format
    
    return TEST_PASS;
}

/* ============================================================================
 * Byte Order Utility Tests
 * ============================================================================ */

TEST(utils_swap16_basic) {
    uint16_t val = 0x1234;
    uint16_t swapped = utils_swap16(val);
    
    ASSERT_EQ(swapped, 0x3412);
    
    // Double swap should return original
    ASSERT_EQ(utils_swap16(swapped), val);
    
    return TEST_PASS;
}

TEST(utils_swap32_basic) {
    uint32_t val = 0x12345678;
    uint32_t swapped = utils_swap32(val);
    
    ASSERT_EQ(swapped, 0x78563412);
    
    // Double swap should return original
    ASSERT_EQ(utils_swap32(swapped), val);
    
    return TEST_PASS;
}

TEST(utils_swap64_basic) {
    uint64_t val = 0x123456789ABCDEF0ULL;
    uint64_t swapped = utils_swap64(val);
    
    ASSERT_EQ(swapped, 0xF0DEBC9A78563412ULL);
    
    // Double swap should return original
    ASSERT_EQ(utils_swap64(swapped), val);
    
    return TEST_PASS;
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("=== Utils Module Tests ===\n\n");
    
    // String tests
    printf("--- String Utilities ---\n");
    RUN_TEST(utils_strcpy_basic);
    RUN_TEST(utils_strcpy_truncate);
    RUN_TEST(utils_strcat_basic);
    RUN_TEST(utils_trim_basic);
    RUN_TEST(utils_tolower_basic);
    RUN_TEST(utils_toupper_basic);
    
    // Memory tests
    printf("\n--- Memory Utilities ---\n");
    RUN_TEST(utils_memcpy_basic);
    RUN_TEST(utils_memmove_overlap);
    RUN_TEST(utils_memzero_basic);
    RUN_TEST(utils_memcmp_const_equal);
    RUN_TEST(utils_memcmp_const_not_equal);
    
    // Numeric tests
    printf("\n--- Numeric Utilities ---\n");
    RUN_TEST(utils_atoi_basic);
    RUN_TEST(utils_atoi_invalid);
    RUN_TEST(utils_parse_size_basic);
    RUN_TEST(utils_clamp_basic);
    
    // Hash tests
    printf("\n--- Hash Utilities ---\n");
    RUN_TEST(utils_hash_djb2_basic);
    RUN_TEST(utils_hash_fnv1a_basic);
    
    // Time tests
    printf("\n--- Time Utilities ---\n");
    RUN_TEST(utils_time_ms_basic);
    RUN_TEST(utils_format_timestamp_basic);
    
    // Byte order tests
    printf("\n--- Byte Order Utilities ---\n");
    RUN_TEST(utils_swap16_basic);
    RUN_TEST(utils_swap32_basic);
    RUN_TEST(utils_swap64_basic);
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
