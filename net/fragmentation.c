/*
 * TLS Fragmentation module for MTProxy
 * Implements TLS handshake fragmentation to bypass DPI
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

#include "fragmentation.h"

// Internal functions
static int fragment_fixed(struct fragmentation_ctx *ctx,
                         const unsigned char *message,
                         size_t message_len,
                         struct tls_fragment *fragments,
                         int max_fragments,
                         int *fragment_count);

static int fragment_random(struct fragmentation_ctx *ctx,
                          const unsigned char *message,
                          size_t message_len,
                          struct tls_fragment *fragments,
                          int max_fragments,
                          int *fragment_count);

static int fragment_tls_like(struct fragmentation_ctx *ctx,
                            const unsigned char *message,
                            size_t message_len,
                            struct tls_fragment *fragments,
                            int max_fragments,
                            int *fragment_count);

static int assemble_fixed(struct fragmentation_ctx *ctx,
                         const struct tls_fragment *fragments,
                         int fragment_count,
                         unsigned char *output,
                         size_t max_output_len,
                         size_t *output_len);

// ============================================================================
// Core Functions
// ============================================================================

int fragmentation_init(struct fragmentation_ctx *ctx,
                      enum fragmentation_mode mode,
                      size_t fragment_size)
{
    if (!ctx) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(struct fragmentation_ctx));
    
    // Set mode
    ctx->mode = mode;
    
    // Set fragment size
    if (fragment_size == 0) {
        ctx->fragment_size = FRAGMENT_DEFAULT_SIZE;
    } else if (fragment_size < FRAGMENT_MIN_SIZE) {
        ctx->fragment_size = FRAGMENT_MIN_SIZE;
    } else if (fragment_size > FRAGMENT_MAX_SIZE) {
        ctx->fragment_size = FRAGMENT_MAX_SIZE;
    } else {
        ctx->fragment_size = fragment_size;
    }
    
    // Set random mode parameters
    if (mode == FRAGMENT_RANDOM) {
        ctx->min_fragment = FRAGMENT_MIN_SIZE;
        ctx->max_fragment = ctx->fragment_size;
    }
    
    // Default settings
    ctx->add_header_to_each = 1;  // Add TLS header to each fragment
    ctx->randomize_order = 0;
    ctx->add_delay = 0;
    ctx->delay_ms = 10;
    
    // Clear statistics
    ctx->messages_fragmented = 0;
    ctx->fragments_created = 0;
    ctx->messages_assembled = 0;
    ctx->fragments_processed = 0;
    ctx->errors = 0;
    
    // Mark as initialized
    ctx->initialized = 1;
    
    return 0;
}

int fragmentation_reset(struct fragmentation_ctx *ctx)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    // Clear statistics
    ctx->messages_fragmented = 0;
    ctx->fragments_created = 0;
    ctx->messages_assembled = 0;
    ctx->fragments_processed = 0;
    ctx->errors = 0;
    
    return 0;
}

void fragmentation_cleanup(struct fragmentation_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Clear context
    memset(ctx, 0, sizeof(struct fragmentation_ctx));
}

// ============================================================================
// Fragmentation Functions
// ============================================================================

int fragmentation_fragment_message(struct fragmentation_ctx *ctx,
                                  const unsigned char *message,
                                  size_t message_len,
                                  struct tls_fragment *fragments,
                                  int max_fragments,
                                  int *fragment_count)
{
    if (!ctx || !ctx->initialized || !message || !fragments || !fragment_count) {
        return -1;
    }
    
    // No fragmentation - just return original as single fragment
    if (ctx->mode == FRAGMENT_NONE) {
        if (max_fragments < 1) {
            ctx->errors++;
            return -1;
        }
        
        fragments[0].len = message_len;
        if (message_len > FRAGMENT_MAX_SIZE) {
            ctx->errors++;
            return -1;
        }
        
        memcpy(fragments[0].data, message, message_len);
        fragments[0].is_last = 1;
        fragments[0].fragment_index = 0;
        fragments[0].total_fragments = 1;
        *fragment_count = 1;
        
        ctx->messages_fragmented++;
        return 0;
    }
    
    int result;
    
    // Apply fragmentation based on mode
    switch (ctx->mode) {
        case FRAGMENT_FIXED:
            result = fragment_fixed(ctx, message, message_len, fragments, 
                                   max_fragments, fragment_count);
            break;
            
        case FRAGMENT_RANDOM:
            result = fragment_random(ctx, message, message_len, fragments,
                                    max_fragments, fragment_count);
            break;
            
        case FRAGMENT_TLS_LIKE:
            result = fragment_tls_like(ctx, message, message_len, fragments,
                                      max_fragments, fragment_count);
            break;
            
        default:
            ctx->errors++;
            return -1;
    }
    
    if (result == 0) {
        ctx->messages_fragmented++;
        ctx->fragments_created += *fragment_count;
    } else {
        ctx->errors++;
    }
    
    return result;
}

int fragmentation_assemble_message(struct fragmentation_ctx *ctx,
                                  const struct tls_fragment *fragments,
                                  int fragment_count,
                                  unsigned char *output,
                                  size_t max_output_len,
                                  size_t *output_len)
{
    if (!ctx || !ctx->initialized || !fragments || !output || !output_len) {
        return -1;
    }
    
    if (fragment_count <= 0 || fragment_count > FRAGMENT_MAX_COUNT) {
        ctx->errors++;
        return -1;
    }
    
    // Calculate total size
    size_t total_size = 0;
    int i;
    for (i = 0; i < fragment_count; i++) {
        total_size += fragments[i].len;
    }
    
    // Check buffer size
    if (total_size > max_output_len) {
        ctx->errors++;
        return -1;
    }
    
    // Assemble fragments
    size_t offset = 0;
    for (i = 0; i < fragment_count; i++) {
        if (offset + fragments[i].len > max_output_len) {
            ctx->errors++;
            return -1;
        }
        
        memcpy(output + offset, fragments[i].data, fragments[i].len);
        offset += fragments[i].len;
        ctx->fragments_processed++;
    }
    
    *output_len = total_size;
    ctx->messages_assembled++;
    
    return 0;
}

int fragmentation_create_tls_header(enum tls_record_type record_type,
                                   uint16_t version,
                                   size_t length,
                                   unsigned char *output)
{
    if (!output) {
        return -1;
    }
    
    // TLS record header format:
    // Byte 0: Content type (1 byte)
    // Byte 1-2: Version (2 bytes)
    // Byte 3-4: Length (2 bytes, big-endian)
    
    output[0] = (unsigned char)record_type;
    output[1] = (version >> 8) & 0xFF;
    output[2] = version & 0xFF;
    output[3] = (length >> 8) & 0xFF;
    output[4] = length & 0xFF;
    
    return 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* fragmentation_mode_name(enum fragmentation_mode mode)
{
    switch (mode) {
        case FRAGMENT_NONE:
            return "none";
        case FRAGMENT_FIXED:
            return "fixed";
        case FRAGMENT_RANDOM:
            return "random";
        case FRAGMENT_TLS_LIKE:
            return "tls-like";
        default:
            return "unknown";
    }
}

const char* fragmentation_record_type_name(enum tls_record_type type)
{
    switch (type) {
        case TLS_RECORD_CHANGE_CIPHER_SPEC:
            return "ChangeCipherSpec";
        case TLS_RECORD_ALERT:
            return "Alert";
        case TLS_RECORD_HANDSHAKE:
            return "Handshake";
        case TLS_RECORD_APPLICATION_DATA:
            return "ApplicationData";
        case TLS_RECORD_HEARTBEAT:
            return "Heartbeat";
        default:
            return "Unknown";
    }
}

const char* fragmentation_handshake_type_name(enum tls_handshake_type type)
{
    switch (type) {
        case TLS_CLIENT_HELLO:
            return "ClientHello";
        case TLS_SERVER_HELLO:
            return "ServerHello";
        case TLS_CERTIFICATE:
            return "Certificate";
        case TLS_SERVER_KEY_EXCHANGE:
            return "ServerKeyExchange";
        case TLS_CLIENT_KEY_EXCHANGE:
            return "ClientKeyExchange";
        case TLS_FINISHED:
            return "Finished";
        default:
            return "Unknown";
    }
}

void fragmentation_get_stats(struct fragmentation_ctx *ctx,
                            uint64_t *messages_frag,
                            uint64_t *fragments_created,
                            uint64_t *messages_assembled,
                            uint64_t *errors)
{
    if (!ctx) {
        return;
    }
    
    if (messages_frag) {
        *messages_frag = ctx->messages_fragmented;
    }
    if (fragments_created) {
        *fragments_created = ctx->fragments_created;
    }
    if (messages_assembled) {
        *messages_assembled = ctx->messages_assembled;
    }
    if (errors) {
        *errors = ctx->errors;
    }
}

void fragmentation_print_stats(struct fragmentation_ctx *ctx)
{
    if (!ctx) {
        return;
    }
    
    printf("\n=== Fragmentation Statistics ===\n");
    printf("Mode: %s\n", fragmentation_mode_name(ctx->mode));
    printf("Fragment size: %zu bytes\n", ctx->fragment_size);
    printf("Status: %s\n", ctx->initialized ? "active" : "inactive");
    printf("Messages fragmented:  %llu\n", (unsigned long long)ctx->messages_fragmented);
    printf("Fragments created:    %llu\n", (unsigned long long)ctx->fragments_created);
    printf("Messages assembled:   %llu\n", (unsigned long long)ctx->messages_assembled);
    printf("Fragments processed:  %llu\n", (unsigned long long)ctx->fragments_processed);
    printf("Errors:               %llu\n", (unsigned long long)ctx->errors);
    
    if (ctx->messages_fragmented > 0) {
        double avg_fragments = (double)ctx->fragments_created / (double)ctx->messages_fragmented;
        printf("Avg fragments/msg:  %.2f\n", avg_fragments);
    }
    
    if (ctx->fragments_created > 0) {
        double success_rate = (double)ctx->messages_assembled / (double)ctx->messages_fragmented * 100.0;
        printf("Assembly success:   %.2f%%\n", success_rate);
    }
    printf("================================\n\n");
}

int fragmentation_calculate_count(struct fragmentation_ctx *ctx, size_t message_len)
{
    if (!ctx || !ctx->initialized || message_len == 0) {
        return -1;
    }
    
    if (ctx->mode == FRAGMENT_NONE) {
        return 1;
    }
    
    size_t frag_size;
    
    switch (ctx->mode) {
        case FRAGMENT_FIXED:
        case FRAGMENT_TLS_LIKE:
            frag_size = ctx->fragment_size;
            break;
            
        case FRAGMENT_RANDOM:
            // Use average size for estimation
            frag_size = (ctx->min_fragment + ctx->max_fragment) / 2;
            break;
            
        default:
            return -1;
    }
    
    // Calculate fragments needed (with header overhead if enabled)
    size_t effective_size = ctx->fragment_size;
    if (ctx->add_header_to_each) {
        effective_size -= FRAGMENT_HEADER_SIZE;
    }
    
    int count = (int)((message_len + effective_size - 1) / effective_size);
    return (count < 1) ? 1 : count;
}

int fragmentation_validate_fragment(const struct tls_fragment *fragment)
{
    if (!fragment) {
        return -1;
    }
    
    if (fragment->len == 0 || fragment->len > FRAGMENT_MAX_SIZE) {
        return -1;
    }
    
    if (fragment->fragment_index < 0 || fragment->total_fragments <= 0) {
        return -1;
    }
    
    if (fragment->fragment_index >= fragment->total_fragments) {
        return -1;
    }
    
    return 0;
}

// ============================================================================
// Internal Functions
// ============================================================================

static int fragment_fixed(struct fragmentation_ctx *ctx,
                         const unsigned char *message,
                         size_t message_len,
                         struct tls_fragment *fragments,
                         int max_fragments,
                         int *fragment_count)
{
    if (max_fragments < 1) {
        return -1;
    }
    
    size_t frag_size = ctx->fragment_size;
    if (ctx->add_header_to_each) {
        frag_size -= FRAGMENT_HEADER_SIZE;
    }
    
    if (frag_size == 0) {
        return -1;
    }
    
    // Calculate number of fragments
    int count = (int)((message_len + frag_size - 1) / frag_size);
    if (count > max_fragments) {
        return -1;
    }
    
    // Create fragments
    size_t offset = 0;
    int i;
    for (i = 0; i < count; i++) {
        size_t remaining = message_len - offset;
        size_t chunk_size = (remaining > frag_size) ? frag_size : remaining;
        
        // Copy data
        memcpy(fragments[i].data, message + offset, chunk_size);
        fragments[i].len = chunk_size;
        fragments[i].fragment_index = i;
        fragments[i].total_fragments = count;
        fragments[i].is_last = (i == count - 1) ? 1 : 0;
        
        offset += chunk_size;
    }
    
    *fragment_count = count;
    return 0;
}

static int fragment_random(struct fragmentation_ctx *ctx,
                          const unsigned char *message,
                          size_t message_len,
                          struct tls_fragment *fragments,
                          int max_fragments,
                          int *fragment_count)
{
    if (max_fragments < 1) {
        return -1;
    }
    
    // Initialize random seed if needed
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
        seeded = 1;
    }
    
    size_t offset = 0;
    int count = 0;
    
    while (offset < message_len && count < max_fragments) {
        // Generate random fragment size
        size_t frag_size = ctx->min_fragment + 
                          (rand() % (ctx->max_fragment - ctx->min_fragment + 1));
        
        size_t remaining = message_len - offset;
        size_t chunk_size = (remaining > frag_size) ? frag_size : remaining;
        
        // Copy data
        memcpy(fragments[count].data, message + offset, chunk_size);
        fragments[count].len = chunk_size;
        fragments[count].fragment_index = count;
        fragments[count].is_last = (offset + chunk_size >= message_len) ? 1 : 0;
        
        offset += chunk_size;
        count++;
    }
    
    // Set total fragments
    int i;
    for (i = 0; i < count; i++) {
        fragments[i].total_fragments = count;
    }
    
    *fragment_count = count;
    return 0;
}

static int fragment_tls_like(struct fragmentation_ctx *ctx,
                            const unsigned char *message,
                            size_t message_len,
                            struct tls_fragment *fragments,
                            int max_fragments,
                            int *fragment_count)
{
    // TLS-like fragmentation: each fragment has TLS header
    if (max_fragments < 1) {
        return -1;
    }
    
    size_t payload_size = ctx->fragment_size - FRAGMENT_HEADER_SIZE;
    if (payload_size == 0) {
        return -1;
    }
    
    // Calculate number of fragments
    int count = (int)((message_len + payload_size - 1) / payload_size);
    if (count > max_fragments) {
        return -1;
    }
    
    // Create fragments with TLS headers
    size_t offset = 0;
    int i;
    for (i = 0; i < count; i++) {
        size_t remaining = message_len - offset;
        size_t chunk_size = (remaining > payload_size) ? payload_size : remaining;
        
        // Create TLS header
        fragmentation_create_tls_header(TLS_RECORD_HANDSHAKE, 0x0303, chunk_size, 
                                       fragments[i].data);
        
        // Copy payload after header
        memcpy(fragments[i].data + FRAGMENT_HEADER_SIZE, message + offset, chunk_size);
        fragments[i].len = chunk_size + FRAGMENT_HEADER_SIZE;
        fragments[i].fragment_index = i;
        fragments[i].total_fragments = count;
        fragments[i].is_last = (i == count - 1) ? 1 : 0;
        
        offset += chunk_size;
    }
    
    *fragment_count = count;
    return 0;
}

static int assemble_fixed(struct fragmentation_ctx *ctx,
                         const struct tls_fragment *fragments,
                         int fragment_count,
                         unsigned char *output,
                         size_t max_output_len,
                         size_t *output_len)
{
    // Use generic assembly function
    return fragmentation_assemble_message(ctx, fragments, fragment_count,
                                         output, max_output_len, output_len);
}
