/*
 * Obfuscation module for MTProxy
 * Implements XOR-based packet obfuscation to bypass DPI
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 *               2024-2026 Dupley Maxim Igorevich (Maestro7IT)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/rand.h>

#include "obfuscate.h"

// Internal functions
static int obfuscate_init_key(struct obfuscate_ctx *ctx, 
                              const unsigned char *key, 
                              size_t key_len);
static int obfuscate_xor_basic(struct obfuscate_ctx *ctx, 
                               unsigned char *data, 
                               size_t len);
static int obfuscate_xor_rotating(struct obfuscate_ctx *ctx, 
                                  unsigned char *data, 
                                  size_t len);
static unsigned char obfuscate_byte(unsigned char byte, 
                                    const unsigned char *key,
                                    size_t key_len,
                                    size_t pos);

// ============================================================================
// Core Functions
// ============================================================================

int obfuscate_init(struct obfuscate_ctx *ctx, 
                   const unsigned char *key, 
                   size_t key_len,
                   enum obfuscate_method method)
{
    if (!ctx) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(struct obfuscate_ctx));
    
    // Set method
    ctx->method = method;
    
    // Initialize key
    if (obfuscate_init_key(ctx, key, key_len) != 0) {
        return -1;
    }
    
    // Reset position
    ctx->key_pos = 0;
    
    // Clear statistics
    ctx->packets_obfuscated = 0;
    ctx->packets_deobfuscated = 0;
    ctx->bytes_processed = 0;
    
    // Mark as initialized
    ctx->initialized = 1;
    
    return 0;
}

int obfuscate_reset(struct obfuscate_ctx *ctx)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    // Reset key position
    ctx->key_pos = 0;
    
    // Clear statistics
    ctx->packets_obfuscated = 0;
    ctx->packets_deobfuscated = 0;
    ctx->bytes_processed = 0;
    
    return 0;
}

void obfuscate_cleanup(struct obfuscate_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Securely erase sensitive data
    if (ctx->key_len > 0) {
        memset(ctx->key, 0, OBFUSCATE_KEY_SIZE);
    }
    
    // Clear context
    memset(ctx, 0, sizeof(struct obfuscate_ctx));
}

// ============================================================================
// Obfuscation Functions
// ============================================================================

int obfuscate_packet(struct obfuscate_ctx *ctx, 
                     unsigned char *data, 
                     size_t len)
{
    if (!ctx || !ctx->initialized || !data || len == 0) {
        return -1;
    }
    
    // No obfuscation - just return
    if (ctx->method == OBFUSCATE_NONE) {
        return (int)len;
    }
    
    int result;
    
    // Apply obfuscation based on method
    switch (ctx->method) {
        case OBFUSCATE_XOR_BASIC:
            result = obfuscate_xor_basic(ctx, data, len);
            break;
            
        case OBFUSCATE_XOR_ROTATING:
            result = obfuscate_xor_rotating(ctx, data, len);
            break;
            
        case OBFUSCATE_XOR_HASH:
            // Hash method uses rotating internally
            result = obfuscate_xor_rotating(ctx, data, len);
            break;
            
        default:
            return -1;
    }
    
    if (result > 0) {
        ctx->packets_obfuscated++;
        ctx->bytes_processed += (uint64_t)result;
    }
    
    return result;
}

int deobfuscate_packet(struct obfuscate_ctx *ctx, 
                       unsigned char *data, 
                       size_t len)
{
    if (!ctx || !ctx->initialized || !data || len == 0) {
        return -1;
    }
    
    // No obfuscation - just return
    if (ctx->method == OBFUSCATE_NONE) {
        return (int)len;
    }
    
    int result;
    
    // Apply deobfuscation (same as obfuscation for XOR)
    switch (ctx->method) {
        case OBFUSCATE_XOR_BASIC:
            result = obfuscate_xor_basic(ctx, data, len);
            break;
            
        case OBFUSCATE_XOR_ROTATING:
            result = obfuscate_xor_rotating(ctx, data, len);
            break;
            
        case OBFUSCATE_XOR_HASH:
            result = obfuscate_xor_rotating(ctx, data, len);
            break;
            
        default:
            return -1;
    }
    
    if (result > 0) {
        ctx->packets_deobfuscated++;
    }
    
    return result;
}

int obfuscate_packet_ex(struct obfuscate_ctx *ctx,
                        const unsigned char *input,
                        size_t input_len,
                        unsigned char *output,
                        size_t *output_len)
{
    if (!ctx || !ctx->initialized || !input || !output || !output_len) {
        return -1;
    }
    
    if (*output_len < input_len) {
        return -1; // Output buffer too small
    }
    
    // Copy input to output
    memcpy(output, input, input_len);
    
    // Obfuscate in-place
    int result = obfuscate_packet(ctx, output, input_len);
    if (result < 0) {
        return result;
    }
    
    *output_len = (size_t)result;
    return 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

int obfuscate_generate_key(unsigned char *key, size_t key_len)
{
    if (!key || key_len == 0 || key_len > OBFUSCATE_KEY_SIZE) {
        return -1;
    }

    // Используем криптографически стойкий генератор случайных чисел OpenSSL
    if (RAND_bytes(key, (int)key_len) != 1) {
        return -1; // Ошибка генерации случайных данных
    }

    return 0;
}

int obfuscate_hash_key(const char *input, 
                       size_t input_len,
                       unsigned char *key, 
                       size_t *key_len)
{
    if (!input || !key || !key_len) {
        return -1;
    }
    
    if (*key_len < OBFUSCATE_KEY_MIN_SIZE) {
        return -1; // Key buffer too small
    }
    
    // Simple hash function (djb2 variant)
    unsigned long hash = 5381;
    size_t i;
    for (i = 0; i < input_len; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)input[i];
    }
    
    // Generate key from hash
    size_t actual_len = (*key_len < OBFUSCATE_KEY_SIZE) ? *key_len : OBFUSCATE_KEY_SIZE;
    for (i = 0; i < actual_len; i++) {
        key[i] = (unsigned char)((hash >> (i % 8)) ^ (input[i % input_len]));
    }
    
    *key_len = actual_len;
    return 0;
}

const char* obfuscate_method_name(enum obfuscate_method method)
{
    switch (method) {
        case OBFUSCATE_NONE:
            return "none";
        case OBFUSCATE_XOR_BASIC:
            return "xor-basic";
        case OBFUSCATE_XOR_ROTATING:
            return "xor-rotating";
        case OBFUSCATE_XOR_HASH:
            return "xor-hash";
        default:
            return "unknown";
    }
}

void obfuscate_get_stats(struct obfuscate_ctx *ctx,
                         uint64_t *packets_obf,
                         uint64_t *packets_deobf,
                         uint64_t *bytes_proc)
{
    if (!ctx) {
        return;
    }
    
    if (packets_obf) {
        *packets_obf = ctx->packets_obfuscated;
    }
    if (packets_deobf) {
        *packets_deobf = ctx->packets_deobfuscated;
    }
    if (bytes_proc) {
        *bytes_proc = ctx->bytes_processed;
    }
}

void obfuscate_print_stats(struct obfuscate_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    printf("\n=== Obfuscation Statistics ===\n");
    printf("Method: %s\n", obfuscate_method_name(ctx->method));
    printf("Status: %s\n", ctx->initialized ? "active" : "inactive");
    printf("Packets obfuscated:  %llu\n", (unsigned long long)ctx->packets_obfuscated);
    printf("Packets deobfuscated: %llu\n", (unsigned long long)ctx->packets_deobfuscated);
    printf("Bytes processed:     %llu\n", (unsigned long long)ctx->bytes_processed);
    
    if (ctx->packets_obfuscated > 0) {
        double success_rate = (double)ctx->packets_deobfuscated / 
                             (double)ctx->packets_obfuscated * 100.0;
        printf("Success rate:        %.2f%%\n", success_rate);
    }
    printf("==============================\n\n");
}

// ============================================================================
// Internal Functions
// ============================================================================

static int obfuscate_init_key(struct obfuscate_ctx *ctx, 
                              const unsigned char *key, 
                              size_t key_len)
{
    if (!key || key_len == 0) {
        // Use default key
        key = (const unsigned char *)OBFUSCATE_DEFAULT_KEY;
        key_len = strlen(OBFUSCATE_DEFAULT_KEY);
    }
    
    // Validate key length
    if (key_len < OBFUSCATE_KEY_MIN_SIZE) {
        return -1; // Key too short
    }
    
    // Store key
    ctx->key_len = (key_len > OBFUSCATE_KEY_SIZE) ? OBFUSCATE_KEY_SIZE : key_len;
    memcpy(ctx->key, key, ctx->key_len);
    
    return 0;
}

static int obfuscate_xor_basic(struct obfuscate_ctx *ctx, 
                               unsigned char *data, 
                               size_t len)
{
    // Basic XOR with static key position (always starts from beginning)
    size_t temp_pos = 0;
    obfuscate_xor_inline(data, len, ctx->key, ctx->key_len, &temp_pos);
    return (int)len;
}

static int obfuscate_xor_rotating(struct obfuscate_ctx *ctx, 
                                  unsigned char *data, 
                                  size_t len)
{
    // Rotating XOR with continuous key position
    obfuscate_xor_inline(data, len, ctx->key, ctx->key_len, &ctx->key_pos);
    return (int)len;
}

static unsigned char obfuscate_byte(unsigned char byte, 
                                    const unsigned char *key,
                                    size_t key_len,
                                    size_t pos)
{
    return byte ^ key[pos % key_len];
}
