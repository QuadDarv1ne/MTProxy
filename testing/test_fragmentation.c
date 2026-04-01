/*
 * Tests for Fragmentation module
 *
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 * 
 * KNOWN ISSUES RESOLVED:
 * - test_fragmentation_fixed: теперь учитывает TLS header overhead ✅
 * - test_fragmentation_calculate_count: включён после проверки ✅
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "net/fragmentation.h"

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
#define ASSERT_GE(a, b) ASSERT((a) >= (b))
#define ASSERT_LE(a, b) ASSERT((a) <= (b))

// ============================================================================
// Test Cases
// ============================================================================

TEST(test_fragmentation_init_default)
{
    struct fragmentation_ctx ctx;
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 0);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.initialized, 1);
    ASSERT_EQ(ctx.mode, FRAGMENT_FIXED);
    ASSERT_EQ(ctx.fragment_size, FRAGMENT_DEFAULT_SIZE);
    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_init_custom_size)
{
    struct fragmentation_ctx ctx;
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 256);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.fragment_size, 256);
    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_init_null_ctx)
{
    int ret = fragmentation_init(NULL, FRAGMENT_FIXED, 128);
    ASSERT_EQ(ret, -1);
}

TEST(test_fragmentation_none)
{
    struct fragmentation_ctx ctx;
    unsigned char message[] = "Test message for fragmentation";
    struct tls_fragment fragments[FRAGMENT_MAX_COUNT];
    int fragment_count;
    
    int ret = fragmentation_init(&ctx, FRAGMENT_NONE, 0);
    ASSERT_EQ(ret, 0);
    
    // Fragment (should return as-is)
    ret = fragmentation_fragment_message(&ctx, message, strlen((char*)message),
                                        fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(fragment_count, 1);
    ASSERT_EQ(fragments[0].len, strlen((char*)message));
    ASSERT_EQ(fragments[0].is_last, 1);
    
    fragmentation_cleanup(&ctx);
}

// Test for fragmentation with TLS header overhead
// Each fragment contains: TLS header (5 bytes) + payload
TEST(test_fragmentation_fixed)
{
    struct fragmentation_ctx ctx;
    unsigned char message[500];
    struct tls_fragment fragments[FRAGMENT_MAX_COUNT];
    int fragment_count;
    size_t i;

    // Fill message
    memset(message, 'A', sizeof(message));

    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 128);
    ASSERT_EQ(ret, 0);

    // Fragment
    ret = fragmentation_fragment_message(&ctx, message, sizeof(message),
                                        fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    ASSERT_EQ(ret, 0);
    ASSERT_NEQ(fragment_count, 0);

    // Verify all fragments except last have expected size (payload + TLS header)
    // Fragment size includes TLS header, so payload = fragment_size - FRAGMENT_HEADER_SIZE
    for (i = 0; i < fragment_count - 1; i++) {
        // Each fragment len should be close to fragment_size (includes TLS header)
        ASSERT_GE(fragments[i].len, ctx.fragment_size - FRAGMENT_HEADER_SIZE);
        ASSERT_LE(fragments[i].len, ctx.fragment_size);
    }

    // Verify last fragment flag
    ASSERT_EQ(fragments[fragment_count - 1].is_last, 1);

    // Verify fragment indices
    for (i = 0; i < fragment_count; i++) {
        ASSERT_EQ(fragments[i].fragment_index, i);
        ASSERT_EQ(fragments[i].total_fragments, fragment_count);
    }

    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_assemble)
{
    struct fragmentation_ctx ctx;
    unsigned char message[] = "Test message for fragmentation and assembly";
    unsigned char assembled[1024];
    struct tls_fragment fragments[FRAGMENT_MAX_COUNT];
    int fragment_count;
    size_t output_len;
    
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 10);
    ASSERT_EQ(ret, 0);
    
    // Fragment
    ret = fragmentation_fragment_message(&ctx, message, strlen((char*)message),
                                        fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    ASSERT_EQ(ret, 0);
    
    // Assemble
    ret = fragmentation_assemble_message(&ctx, fragments, fragment_count,
                                        assembled, sizeof(assembled), &output_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(output_len, strlen((char*)message));
    
    // Verify data matches
    ASSERT_EQ(memcmp(message, assembled, strlen((char*)message)), 0);
    
    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_random)
{
    struct fragmentation_ctx ctx;
    unsigned char message[200];
    struct tls_fragment fragments1[FRAGMENT_MAX_COUNT];
    struct tls_fragment fragments2[FRAGMENT_MAX_COUNT];
    int count1, count2;
    
    memset(message, 'A', sizeof(message));
    
    int ret = fragmentation_init(&ctx, FRAGMENT_RANDOM, 50);
    ASSERT_EQ(ret, 0);
    
    // Fragment twice
    ret = fragmentation_fragment_message(&ctx, message, sizeof(message),
                                        fragments1, FRAGMENT_MAX_COUNT, &count1);
    ASSERT_EQ(ret, 0);
    
    ret = fragmentation_fragment_message(&ctx, message, sizeof(message),
                                        fragments2, FRAGMENT_MAX_COUNT, &count2);
    ASSERT_EQ(ret, 0);
    
    // Fragment counts may differ (random)
    // At least verify both succeeded
    ASSERT_GE(count1, 1);
    ASSERT_GE(count2, 1);
    
    fragmentation_cleanup(&ctx);
}

// Test for fragmentation count calculation with TLS header overhead
// Each fragment includes TLS header (5 bytes), so payload capacity = fragment_size - header
TEST(test_fragmentation_calculate_count)
{
    struct fragmentation_ctx ctx;
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 100);
    ASSERT_EQ(ret, 0);

    // Test various sizes
    // Note: fragmentation_calculate_count uses raw fragment_size without header overhead
    // because it calculates based on total message size vs fragment capacity
    ASSERT_EQ(fragmentation_calculate_count(&ctx, 50), 1);   // Less than fragment size
    ASSERT_EQ(fragmentation_calculate_count(&ctx, 100), 1);  // Exactly fragment size
    ASSERT_EQ(fragmentation_calculate_count(&ctx, 101), 2);  // Slightly over
    ASSERT_EQ(fragmentation_calculate_count(&ctx, 200), 2);  // Exactly 2 fragments
    ASSERT_EQ(fragmentation_calculate_count(&ctx, 201), 3);  // Slightly over 2

    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_needed)
{
    struct fragmentation_ctx ctx;
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 100);
    ASSERT_EQ(ret, 0);
    
    // Test fragmentation needed
    ASSERT_EQ(fragmentation_needed(&ctx, 50), 0);   // No fragmentation needed
    ASSERT_EQ(fragmentation_needed(&ctx, 100), 0);  // Exactly fragment size
    ASSERT_EQ(fragmentation_needed(&ctx, 150), 1);  // Fragmentation needed
    
    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_stats)
{
    struct fragmentation_ctx ctx;
    unsigned char message[] = "Test";
    struct tls_fragment fragments[FRAGMENT_MAX_COUNT];
    int fragment_count;
    uint64_t messages_frag, fragments_created, messages_assembled, errors;
    
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 10);
    ASSERT_EQ(ret, 0);
    
    // Fragment multiple messages
    fragmentation_fragment_message(&ctx, message, 4, fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    fragmentation_fragment_message(&ctx, message, 4, fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    fragmentation_fragment_message(&ctx, message, 4, fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    
    // Get stats
    fragmentation_get_stats(&ctx, &messages_frag, &fragments_created, &messages_assembled, &errors);
    ASSERT_EQ(messages_frag, 3);
    ASSERT_NEQ(fragments_created, 0);
    ASSERT_EQ(messages_assembled, 0);
    ASSERT_EQ(errors, 0);
    
    fragmentation_cleanup(&ctx);
}

TEST(test_fragmentation_mode_name)
{
    ASSERT_EQ(strcmp(fragmentation_mode_name(FRAGMENT_NONE), "none"), 0);
    ASSERT_EQ(strcmp(fragmentation_mode_name(FRAGMENT_FIXED), "fixed"), 0);
    ASSERT_EQ(strcmp(fragmentation_mode_name(FRAGMENT_RANDOM), "random"), 0);
    ASSERT_EQ(strcmp(fragmentation_mode_name(FRAGMENT_TLS_LIKE), "tls-like"), 0);
}

TEST(test_fragmentation_record_type_name)
{
    ASSERT_EQ(strcmp(fragmentation_record_type_name(TLS_RECORD_HANDSHAKE), "Handshake"), 0);
    ASSERT_EQ(strcmp(fragmentation_record_type_name(TLS_RECORD_ALERT), "Alert"), 0);
    ASSERT_EQ(strcmp(fragmentation_record_type_name(TLS_RECORD_APPLICATION_DATA), "ApplicationData"), 0);
}

TEST(test_fragmentation_handshake_type_name)
{
    ASSERT_EQ(strcmp(fragmentation_handshake_type_name(TLS_CLIENT_HELLO), "ClientHello"), 0);
    ASSERT_EQ(strcmp(fragmentation_handshake_type_name(TLS_SERVER_HELLO), "ServerHello"), 0);
    ASSERT_EQ(strcmp(fragmentation_handshake_type_name(TLS_CERTIFICATE), "Certificate"), 0);
}

TEST(test_fragmentation_validate_fragment)
{
    struct tls_fragment frag;
    
    // Valid fragment
    frag.len = 100;
    frag.fragment_index = 0;
    frag.total_fragments = 3;
    frag.is_last = 0;
    ASSERT_EQ(fragmentation_validate_fragment(&frag), 0);
    
    // Invalid: zero length
    frag.len = 0;
    ASSERT_NEQ(fragmentation_validate_fragment(&frag), 0);
    
    // Invalid: too large
    frag.len = FRAGMENT_MAX_SIZE + 1;
    ASSERT_NEQ(fragmentation_validate_fragment(&frag), 0);
    
    // Invalid: index out of range
    frag.len = 100;
    frag.fragment_index = 5;
    frag.total_fragments = 3;
    ASSERT_NEQ(fragmentation_validate_fragment(&frag), 0);
}

TEST(test_fragmentation_create_tls_header)
{
    unsigned char header[FRAGMENT_HEADER_SIZE];
    
    int ret = fragmentation_create_tls_header(TLS_RECORD_HANDSHAKE, 0x0303, 100, header);
    ASSERT_EQ(ret, 0);
    
    // Verify header
    ASSERT_EQ(header[0], TLS_RECORD_HANDSHAKE);  // Content type
    ASSERT_EQ(header[1], 0x03);                   // Version major
    ASSERT_EQ(header[2], 0x03);                   // Version minor
    ASSERT_EQ(header[3], 0x00);                   // Length high byte
    ASSERT_EQ(header[4], 100);                    // Length low byte
}

TEST(test_fragmentation_reset)
{
    struct fragmentation_ctx ctx;
    unsigned char message[] = "Test";
    struct tls_fragment fragments[FRAGMENT_MAX_COUNT];
    int fragment_count;
    uint64_t messages_frag;
    
    int ret = fragmentation_init(&ctx, FRAGMENT_FIXED, 10);
    ASSERT_EQ(ret, 0);
    
    // Fragment some messages
    fragmentation_fragment_message(&ctx, message, 4, fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    fragmentation_fragment_message(&ctx, message, 4, fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    
    // Reset
    ret = fragmentation_reset(&ctx);
    ASSERT_EQ(ret, 0);
    
    // Stats should be cleared
    fragmentation_get_stats(&ctx, &messages_frag, NULL, NULL, NULL);
    ASSERT_EQ(messages_frag, 0);
    
    // Context should still be initialized
    ASSERT_EQ(ctx.initialized, 1);
    
    fragmentation_cleanup(&ctx);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    printf("\n=== Fragmentation Module Tests ===\n\n");

    RUN_TEST(test_fragmentation_init_default);
    RUN_TEST(test_fragmentation_init_custom_size);
    RUN_TEST(test_fragmentation_init_null_ctx);
    RUN_TEST(test_fragmentation_none);
    RUN_TEST(test_fragmentation_fixed);
    RUN_TEST(test_fragmentation_assemble);
    RUN_TEST(test_fragmentation_random);
    RUN_TEST(test_fragmentation_calculate_count);
    RUN_TEST(test_fragmentation_needed);
    RUN_TEST(test_fragmentation_stats);
    RUN_TEST(test_fragmentation_mode_name);
    RUN_TEST(test_fragmentation_record_type_name);
    RUN_TEST(test_fragmentation_handshake_type_name);
    RUN_TEST(test_fragmentation_validate_fragment);
    RUN_TEST(test_fragmentation_create_tls_header);
    RUN_TEST(test_fragmentation_reset);

    printf("\n==================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("==================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
