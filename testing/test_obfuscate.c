/*
 * Tests for Obfuscation module
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "crypto/obfuscate.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test macros
#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("Running %s... ", #name); \
    name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAILED\n  Assertion failed: %s\n  At line %d\n", #cond, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))

// ============================================================================
// Test Cases
// ============================================================================

TEST(test_obfuscate_init_default)
{
    struct obfuscate_ctx ctx;
    int ret = obfuscate_init(&ctx, NULL, 0, OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.initialized, 1);
    ASSERT_EQ(ctx.method, OBFUSCATE_XOR_BASIC);
    ASSERT_NEQ(ctx.key_len, 0);
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_init_custom_key)
{
    struct obfuscate_ctx ctx;
    const unsigned char key[] = "test-key-12345";
    int ret = obfuscate_init(&ctx, key, strlen((char*)key), OBFUSCATE_XOR_ROTATING);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.initialized, 1);
    ASSERT_EQ(ctx.method, OBFUSCATE_XOR_ROTATING);
    ASSERT_EQ(ctx.key_len, strlen((char*)key));
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_init_null_ctx)
{
    int ret = obfuscate_init(NULL, NULL, 0, OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, -1);
}

TEST(test_obfuscate_init_key_too_short)
{
    struct obfuscate_ctx ctx;
    const unsigned char key[] = "short";  // Less than OBFUSCATE_KEY_MIN_SIZE
    int ret = obfuscate_init(&ctx, key, 3, OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, -1);
}

TEST(test_obfuscate_xor_basic_reversible)
{
    struct obfuscate_ctx ctx_enc, ctx_dec;
    const unsigned char key[] = "my-secret-key-12345";
    unsigned char data[] = "Hello, World! This is a test message.";
    unsigned char original[sizeof(data)];
    
    // Инициализируем два контекста с одинаковым ключом
    int ret = obfuscate_init(&ctx_enc, key, strlen((char*)key), OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    
    ret = obfuscate_init(&ctx_dec, key, strlen((char*)key), OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    
    // Save original
    memcpy(original, data, sizeof(data));
    
    // Obfuscate
    ret = obfuscate_packet(&ctx_enc, data, strlen((char*)data));
    ASSERT_NEQ(ret, -1);
    
    // Data should be changed
    ASSERT_NEQ(memcmp(data, original, strlen((char*)data)), 0);
    
    // Deobfuscate (используем отдельный контекст)
    ret = deobfuscate_packet(&ctx_dec, data, strlen((char*)data));
    ASSERT_NEQ(ret, -1);
    
    // Data should be restored
    ASSERT_EQ(memcmp(data, original, strlen((char*)data)), 0);
    
    obfuscate_cleanup(&ctx_enc);
    obfuscate_cleanup(&ctx_dec);
}

TEST(test_obfuscate_xor_rotating)
{
    struct obfuscate_ctx ctx_enc, ctx_dec;
    const unsigned char key[] = "rotating-key-test";
    unsigned char data[] = "Test data for rotating XOR";
    unsigned char original[sizeof(data)];
    
    int ret = obfuscate_init(&ctx_enc, key, strlen((char*)key), OBFUSCATE_XOR_ROTATING);
    ASSERT_EQ(ret, 0);
    
    ret = obfuscate_init(&ctx_dec, key, strlen((char*)key), OBFUSCATE_XOR_ROTATING);
    ASSERT_EQ(ret, 0);
    
    memcpy(original, data, sizeof(data));
    
    // Obfuscate
    ret = obfuscate_packet(&ctx_enc, data, strlen((char*)data));
    ASSERT_NEQ(ret, -1);
    
    // Deobfuscate (используем отдельный контекст)
    ret = deobfuscate_packet(&ctx_dec, data, strlen((char*)data));
    ASSERT_NEQ(ret, -1);
    
    // Data should be restored
    ASSERT_EQ(memcmp(data, original, strlen((char*)data)), 0);
    
    obfuscate_cleanup(&ctx_enc);
    obfuscate_cleanup(&ctx_dec);
}

TEST(test_obfuscate_none)
{
    struct obfuscate_ctx ctx;
    unsigned char data[] = "Test data";
    unsigned char original[sizeof(data)];
    
    int ret = obfuscate_init(&ctx, NULL, 0, OBFUSCATE_NONE);
    ASSERT_EQ(ret, 0);
    
    memcpy(original, data, sizeof(data));
    
    // Obfuscate with NONE method - should not change data
    ret = obfuscate_packet(&ctx, data, strlen((char*)data));
    ASSERT_NEQ(ret, -1);
    
    // Data should NOT be changed
    ASSERT_EQ(memcmp(data, original, sizeof(data)), 0);
    
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_stats)
{
    struct obfuscate_ctx ctx;
    const unsigned char key[] = "stats-test-key";
    unsigned char data[] = "Test";
    uint64_t packets_obf, packets_deobf, bytes_proc;
    
    int ret = obfuscate_init(&ctx, key, strlen((char*)key), OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    
    // Obfuscate multiple packets
    obfuscate_packet(&ctx, data, 4);
    obfuscate_packet(&ctx, data, 4);
    obfuscate_packet(&ctx, data, 4);
    
    // Deobfuscate
    deobfuscate_packet(&ctx, data, 4);
    deobfuscate_packet(&ctx, data, 4);
    
    // Check stats
    obfuscate_get_stats(&ctx, &packets_obf, &packets_deobf, &bytes_proc);
    ASSERT_EQ(packets_obf, 3);
    ASSERT_EQ(packets_deobf, 2);
    ASSERT_EQ(bytes_proc, 12);  // 3 * 4 bytes
    
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_reset)
{
    struct obfuscate_ctx ctx;
    const unsigned char key[] = "reset-test";
    unsigned char data[] = "Test";
    uint64_t packets_obf;
    
    int ret = obfuscate_init(&ctx, key, strlen((char*)key), OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    
    // Obfuscate some packets
    obfuscate_packet(&ctx, data, 4);
    obfuscate_packet(&ctx, data, 4);
    
    // Reset
    ret = obfuscate_reset(&ctx);
    ASSERT_EQ(ret, 0);
    
    // Stats should be cleared
    obfuscate_get_stats(&ctx, &packets_obf, NULL, NULL);
    ASSERT_EQ(packets_obf, 0);
    
    // Context should still be initialized
    ASSERT_EQ(ctx.initialized, 1);
    
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_generate_key)
{
    unsigned char key1[OBFUSCATE_KEY_SIZE];
    unsigned char key2[OBFUSCATE_KEY_SIZE];
    
    int ret = obfuscate_generate_key(key1, OBFUSCATE_KEY_SIZE);
    ASSERT_EQ(ret, 0);
    
    ret = obfuscate_generate_key(key2, OBFUSCATE_KEY_SIZE);
    ASSERT_EQ(ret, 0);
    
    // Keys should be different (random)
    ASSERT_NEQ(memcmp(key1, key2, OBFUSCATE_KEY_SIZE), 0);
}

TEST(test_obfuscate_hash_key)
{
    unsigned char key[OBFUSCATE_KEY_SIZE];
    size_t key_len = OBFUSCATE_KEY_SIZE;
    
    const char *input = "my-secret-password";
    int ret = obfuscate_hash_key(input, strlen(input), key, &key_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NEQ(key_len, 0);
    
    // Hash same input again - should produce same result
    unsigned char key2[OBFUSCATE_KEY_SIZE];
    size_t key_len2 = OBFUSCATE_KEY_SIZE;
    ret = obfuscate_hash_key(input, strlen(input), key2, &key_len2);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(key_len, key_len2);
    ASSERT_EQ(memcmp(key, key2, key_len), 0);
}

TEST(test_obfuscate_method_name)
{
    ASSERT_EQ(strcmp(obfuscate_method_name(OBFUSCATE_NONE), "none"), 0);
    ASSERT_EQ(strcmp(obfuscate_method_name(OBFUSCATE_XOR_BASIC), "xor-basic"), 0);
    ASSERT_EQ(strcmp(obfuscate_method_name(OBFUSCATE_XOR_ROTATING), "xor-rotating"), 0);
    ASSERT_EQ(strcmp(obfuscate_method_name(OBFUSCATE_XOR_HASH), "xor-hash"), 0);
}

TEST(test_obfuscate_packet_ex)
{
    struct obfuscate_ctx ctx;
    const unsigned char key[] = "test-key";
    const unsigned char input[] = "Test input data";
    unsigned char output[sizeof(input)];
    size_t output_len = sizeof(output);
    
    int ret = obfuscate_init(&ctx, key, strlen((char*)key), OBFUSCATE_XOR_BASIC);
    ASSERT_EQ(ret, 0);
    
    // Obfuscate with separate output buffer
    ret = obfuscate_packet_ex(&ctx, input, strlen((char*)input), output, &output_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(output_len, strlen((char*)input));
    
    // Output should be different from input
    ASSERT_NEQ(memcmp(input, output, strlen((char*)input)), 0);
    
    obfuscate_cleanup(&ctx);
}

TEST(test_obfuscate_inline)
{
    unsigned char data[] = "Inline test";
    const unsigned char key[] = "key";
    size_t key_pos = 0;
    
    // Save original
    unsigned char original[sizeof(data)];
    memcpy(original, data, sizeof(data));
    
    // Obfuscate
    obfuscate_xor_inline(data, strlen((char*)data), key, strlen((char*)key), &key_pos);
    
    // Data should be changed
    ASSERT_NEQ(memcmp(data, original, sizeof(data)), 0);
    
    // Deobfuscate (reset key_pos)
    key_pos = 0;
    obfuscate_xor_inline(data, strlen((char*)data), key, strlen((char*)key), &key_pos);
    
    // Data should be restored
    ASSERT_EQ(memcmp(data, original, sizeof(data)), 0);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    printf("\n=== Obfuscation Module Tests ===\n\n");
    
    RUN_TEST(test_obfuscate_init_default);
    RUN_TEST(test_obfuscate_init_custom_key);
    RUN_TEST(test_obfuscate_init_null_ctx);
    RUN_TEST(test_obfuscate_init_key_too_short);
    RUN_TEST(test_obfuscate_xor_basic_reversible);
    RUN_TEST(test_obfuscate_xor_rotating);
    RUN_TEST(test_obfuscate_none);
    RUN_TEST(test_obfuscate_stats);
    RUN_TEST(test_obfuscate_reset);
    RUN_TEST(test_obfuscate_generate_key);
    RUN_TEST(test_obfuscate_hash_key);
    RUN_TEST(test_obfuscate_method_name);
    RUN_TEST(test_obfuscate_packet_ex);
    RUN_TEST(test_obfuscate_inline);
    
    printf("\n=================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("=================================\n\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
