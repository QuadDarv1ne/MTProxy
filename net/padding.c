/*
 * Packet Padding module for MTProxy
 * Implements packet size padding to hide traffic patterns
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 *               2024-2026 Dupley Maxim Igorevich (Maestro7IT)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>  // for getpid() on Windows
#endif

#include "padding.h"

// Internal functions
static int padding_add_fixed(struct padding_ctx *ctx,
                             unsigned char *packet,
                             size_t *packet_len,
                             size_t max_len);
static int padding_add_random(struct padding_ctx *ctx,
                              unsigned char *packet,
                              size_t *packet_len,
                              size_t max_len);
static int padding_add_tls_like(struct padding_ctx *ctx,
                                unsigned char *packet,
                                size_t *packet_len,
                                size_t max_len);
static int padding_remove_fixed(struct padding_ctx *ctx,
                                unsigned char *packet,
                                size_t *packet_len);
static int padding_remove_random(struct padding_ctx *ctx,
                                 unsigned char *packet,
                                 size_t *packet_len);
static int padding_remove_tls_like(struct padding_ctx *ctx,
                                   unsigned char *packet,
                                   size_t *packet_len);

// ============================================================================
// Core Functions
// ============================================================================

int padding_init(struct padding_ctx *ctx,
                 enum padding_method method,
                 size_t block_size,
                 size_t max_padding)
{
    if (!ctx) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(struct padding_ctx));
    
    // Set method
    ctx->method = method;
    
    // Set block size
    if (block_size == 0) {
        ctx->block_size = PADDING_BLOCK_SIZE_DEFAULT;
    } else if (block_size < PADDING_BLOCK_SIZE_MIN) {
        ctx->block_size = PADDING_BLOCK_SIZE_MIN;
    } else if (block_size > PADDING_BLOCK_SIZE_MAX) {
        ctx->block_size = PADDING_BLOCK_SIZE_MAX;
    } else {
        ctx->block_size = block_size;
    }
    
    // Set max padding
    ctx->max_padding = (max_padding > PADDING_MAX_SIZE) ? PADDING_MAX_SIZE : max_padding;
    
    // Default settings
    ctx->use_length_prefix = 1;  // Use length prefix by default
    ctx->use_random_fill = 0;
    
    // Clear statistics
    ctx->packets_padded = 0;
    ctx->packets_unpadded = 0;
    ctx->bytes_added = 0;
    
    // Mark as initialized
    ctx->initialized = 1;
    
    return 0;
}

int padding_reset(struct padding_ctx *ctx)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    // Clear statistics
    ctx->packets_padded = 0;
    ctx->packets_unpadded = 0;
    ctx->bytes_added = 0;
    
    return 0;
}

void padding_cleanup(struct padding_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Clear context
    memset(ctx, 0, sizeof(struct padding_ctx));
}

// ============================================================================
// Padding Functions
// ============================================================================

int padding_add(struct padding_ctx *ctx,
                unsigned char *packet,
                size_t *packet_len,
                size_t max_len)
{
    if (!ctx || !ctx->initialized || !packet || !packet_len) {
        return -1;
    }
    
    // No padding - just return
    if (ctx->method == PADDING_NONE) {
        return 0;
    }
    
    int result;
    
    // Apply padding based on method
    switch (ctx->method) {
        case PADDING_FIXED:
            result = padding_add_fixed(ctx, packet, packet_len, max_len);
            break;
            
        case PADDING_RANDOM:
            result = padding_add_random(ctx, packet, packet_len, max_len);
            break;
            
        case PADDING_TLS_LIKE:
            result = padding_add_tls_like(ctx, packet, packet_len, max_len);
            break;
            
        default:
            return -1;
    }
    
    if (result == 0) {
        ctx->packets_padded++;
    }
    
    return result;
}

int padding_remove(struct padding_ctx *ctx,
                   unsigned char *packet,
                   size_t *packet_len)
{
    if (!ctx || !ctx->initialized || !packet || !packet_len) {
        return -1;
    }
    
    // No padding - just return
    if (ctx->method == PADDING_NONE) {
        return 0;
    }
    
    int result;
    
    // Remove padding based on method
    switch (ctx->method) {
        case PADDING_FIXED:
            result = padding_remove_fixed(ctx, packet, packet_len);
            break;
            
        case PADDING_RANDOM:
            result = padding_remove_random(ctx, packet, packet_len);
            break;
            
        case PADDING_TLS_LIKE:
            result = padding_remove_tls_like(ctx, packet, packet_len);
            break;
            
        default:
            return -1;
    }
    
    if (result == 0) {
        ctx->packets_unpadded++;
    }
    
    return result;
}

int padding_calculate_size(struct padding_ctx *ctx, size_t original_len)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    size_t result;
    
    switch (ctx->method) {
        case PADDING_NONE:
            return (int)original_len;
            
        case PADDING_FIXED:
            result = padding_fixed_calc(original_len, ctx->block_size, 
                                       ctx->use_length_prefix ? PADDING_LENGTH_PREFIX_SIZE : 0);
            return (int)result;
            
        case PADDING_RANDOM:
            // Random padding: original + random(0 to max_padding)
            if (ctx->use_length_prefix) {
                return (int)(original_len + PADDING_LENGTH_PREFIX_SIZE + 
                           (rand() % (ctx->max_padding + 1)));
            }
            return (int)(original_len + (rand() % (ctx->max_padding + 1)));
            
        case PADDING_TLS_LIKE:
            // TLS-like: 256-byte blocks
            result = padding_fixed_calc(original_len, 256, 
                                       ctx->use_length_prefix ? PADDING_LENGTH_PREFIX_SIZE : 0);
            return (int)result;
            
        default:
            return -1;
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* padding_method_name(enum padding_method method)
{
    switch (method) {
        case PADDING_NONE:
            return "none";
        case PADDING_FIXED:
            return "fixed";
        case PADDING_RANDOM:
            return "random";
        case PADDING_TLS_LIKE:
            return "tls-like";
        default:
            return "unknown";
    }
}

void padding_get_stats(struct padding_ctx *ctx,
                       uint64_t *packets_pad,
                       uint64_t *packets_unpad,
                       uint64_t *bytes_added)
{
    if (!ctx) {
        return;
    }
    
    if (packets_pad) {
        *packets_pad = ctx->packets_padded;
    }
    if (packets_unpad) {
        *packets_unpad = ctx->packets_unpadded;
    }
    if (bytes_added) {
        *bytes_added = ctx->bytes_added;
    }
}

void padding_print_stats(struct padding_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    printf("\n=== Padding Statistics ===\n");
    printf("Method: %s\n", padding_method_name(ctx->method));
    printf("Block size: %zu bytes\n", ctx->block_size);
    printf("Status: %s\n", ctx->initialized ? "active" : "inactive");
    printf("Packets padded:    %llu\n", (unsigned long long)ctx->packets_padded);
    printf("Packets unpadded:  %llu\n", (unsigned long long)ctx->packets_unpadded);
    printf("Bytes added:       %llu\n", (unsigned long long)ctx->bytes_added);
    
    if (ctx->packets_padded > 0) {
        double avg_padding = (double)ctx->bytes_added / (double)ctx->packets_padded;
        printf("Average padding:   %.2f bytes/packet\n", avg_padding);
    }
    printf("=========================\n\n");
}

int padding_generate_random(unsigned char *buffer, size_t len)
{
    if (!buffer || len == 0) {
        return -1;
    }
    
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
        seeded = 1;
    }
    
    size_t i;
    for (i = 0; i < len; i++) {
        buffer[i] = (unsigned char)(rand() % 256);
    }
    
    return 0;
}

int padding_generate_pattern(unsigned char *buffer, size_t len, unsigned char start)
{
    if (!buffer || len == 0) {
        return -1;
    }
    
    size_t i;
    for (i = 0; i < len; i++) {
        buffer[i] = start + (unsigned char)i;
    }
    
    // Last byte contains padding length
    if (len > 0) {
        buffer[len - 1] = (unsigned char)len;
    }
    
    return 0;
}

// ============================================================================
// Internal Functions
// ============================================================================

static int padding_add_fixed(struct padding_ctx *ctx,
                             unsigned char *packet,
                             size_t *packet_len,
                             size_t max_len)
{
    size_t original_len = *packet_len;

    // Calculate padded size
    size_t padded_len = padding_fixed_calc(original_len, ctx->block_size,
                                          ctx->use_length_prefix ? PADDING_LENGTH_PREFIX_SIZE : 0);

    // Check buffer size
    if (padded_len > max_len) {
        return -1; // Buffer too small
    }

    // Add length prefix if enabled - shift data first
    size_t offset = 0;
    if (ctx->use_length_prefix) {
        // Shift original data to make room for length prefix
        memmove(packet + PADDING_LENGTH_PREFIX_SIZE, packet, original_len);
        
        // Store original length in big-endian format at the beginning
        packet[0] = (original_len >> 24) & 0xFF;
        packet[1] = (original_len >> 16) & 0xFF;
        packet[2] = (original_len >> 8) & 0xFF;
        packet[3] = original_len & 0xFF;
        offset = PADDING_LENGTH_PREFIX_SIZE;
    }

    // Generate pattern padding after original data
    size_t padding_len = padded_len - original_len - offset;
    if (padding_len > 0) {
        padding_generate_pattern(packet + offset + original_len, padding_len, 0x01);
    }

    *packet_len = padded_len;
    ctx->bytes_added += (padded_len - original_len);

    return 0;
}

static int padding_add_random(struct padding_ctx *ctx,
                              unsigned char *packet,
                              size_t *packet_len,
                              size_t max_len)
{
    size_t original_len = *packet_len;

    // Generate random padding size
    size_t padding_size = rand() % (ctx->max_padding + 1);
    size_t total_len = original_len + padding_size;

    if (ctx->use_length_prefix) {
        total_len += PADDING_LENGTH_PREFIX_SIZE;
    }

    // Check buffer size
    if (total_len > max_len) {
        return -1; // Buffer too small
    }

    // Add length prefix if enabled - shift data first
    size_t offset = 0;
    if (ctx->use_length_prefix) {
        // Shift original data to make room for length prefix
        memmove(packet + PADDING_LENGTH_PREFIX_SIZE, packet, original_len);
        
        // Store original length in big-endian format
        packet[0] = (original_len >> 24) & 0xFF;
        packet[1] = (original_len >> 16) & 0xFF;
        packet[2] = (original_len >> 8) & 0xFF;
        packet[3] = original_len & 0xFF;
        offset = PADDING_LENGTH_PREFIX_SIZE;
    }

    // Generate random padding after original data
    if (padding_size > 0) {
        padding_generate_random(packet + offset + original_len, padding_size);
    }

    *packet_len = total_len;
    ctx->bytes_added += (total_len - original_len);

    return 0;
}

static int padding_add_tls_like(struct padding_ctx *ctx,
                                unsigned char *packet,
                                size_t *packet_len,
                                size_t max_len)
{
    // TLS-like uses 256-byte blocks
    size_t original_len = *packet_len;
    size_t block_size = 256;

    size_t total_len = original_len;
    if (ctx->use_length_prefix) {
        total_len += PADDING_LENGTH_PREFIX_SIZE;
    }

    // Round up to next 256-byte block
    size_t remainder = total_len % block_size;
    if (remainder != 0) {
        total_len += (block_size - remainder);
    }

    // Check buffer size
    if (total_len > max_len) {
        return -1; // Buffer too small
    }

    // Add length prefix if enabled - shift data first
    size_t offset = 0;
    if (ctx->use_length_prefix) {
        // Shift original data to make room for length prefix
        memmove(packet + PADDING_LENGTH_PREFIX_SIZE, packet, original_len);
        
        // Store original length in big-endian format
        packet[0] = (original_len >> 24) & 0xFF;
        packet[1] = (original_len >> 16) & 0xFF;
        packet[2] = (original_len >> 8) & 0xFF;
        packet[3] = original_len & 0xFF;
        offset = PADDING_LENGTH_PREFIX_SIZE;
    }

    // Generate TLS-like padding (PKCS#7 style) after original data
    size_t padding_len = total_len - original_len - offset;
    if (padding_len > 0) {
        memset(packet + offset + original_len, (unsigned char)padding_len, padding_len);
    }

    *packet_len = total_len;
    ctx->bytes_added += (total_len - original_len);

    return 0;
}

static int padding_remove_fixed(struct padding_ctx *ctx,
                                unsigned char *packet,
                                size_t *packet_len)
{
    if (*packet_len == 0) {
        return -1;
    }
    
    size_t original_len;
    
    // Read length prefix if enabled
    if (ctx->use_length_prefix) {
        if (*packet_len < PADDING_LENGTH_PREFIX_SIZE) {
            return -1; // Packet too short
        }
        original_len = ((size_t)packet[0] << 24) |
                      ((size_t)packet[1] << 16) |
                      ((size_t)packet[2] << 8) |
                      (size_t)packet[3];
        
        // Validate length
        if (original_len > *packet_len - PADDING_LENGTH_PREFIX_SIZE) {
            return -1; // Invalid length
        }
        
        // Shift data to remove prefix
        memmove(packet, packet + PADDING_LENGTH_PREFIX_SIZE, original_len);
    } else {
        // Calculate original length from block alignment
        size_t remainder = *packet_len % ctx->block_size;
        if (remainder == 0) {
            original_len = *packet_len;
        } else {
            original_len = *packet_len - (ctx->block_size - remainder);
        }
    }
    
    *packet_len = original_len;
    return 0;
}

static int padding_remove_random(struct padding_ctx *ctx,
                                 unsigned char *packet,
                                 size_t *packet_len)
{
    // For random padding, we need the length prefix
    if (!ctx->use_length_prefix || *packet_len < PADDING_LENGTH_PREFIX_SIZE) {
        return -1; // Cannot determine original length
    }
    
    size_t original_len = ((size_t)packet[0] << 24) |
                         ((size_t)packet[1] << 16) |
                         ((size_t)packet[2] << 8) |
                         (size_t)packet[3];
    
    // Validate length
    if (original_len > *packet_len - PADDING_LENGTH_PREFIX_SIZE) {
        return -1; // Invalid length
    }
    
    // Shift data to remove prefix
    memmove(packet, packet + PADDING_LENGTH_PREFIX_SIZE, original_len);
    *packet_len = original_len;
    
    return 0;
}

static int padding_remove_tls_like(struct padding_ctx *ctx,
                                   unsigned char *packet,
                                   size_t *packet_len)
{
    if (*packet_len == 0) {
        return -1;
    }
    
    size_t original_len;
    
    // Read length prefix if enabled
    if (ctx->use_length_prefix) {
        if (*packet_len < PADDING_LENGTH_PREFIX_SIZE) {
            return -1; // Packet too short
        }
        original_len = ((size_t)packet[0] << 24) |
                      ((size_t)packet[1] << 16) |
                      ((size_t)packet[2] << 8) |
                      (size_t)packet[3];
        
        // Validate length
        if (original_len > *packet_len - PADDING_LENGTH_PREFIX_SIZE) {
            return -1; // Invalid length
        }
        
        // Shift data to remove prefix
        memmove(packet, packet + PADDING_LENGTH_PREFIX_SIZE, original_len);
    } else {
        // PKCS#7 style: last byte contains padding length
        unsigned char padding_len = packet[*packet_len - 1];
        
        if (padding_len == 0 || padding_len > *packet_len) {
            return -1; // Invalid padding
        }
        
        original_len = *packet_len - padding_len;
    }
    
    *packet_len = original_len;
    return 0;
}
