/*
 * Packet Padding module for MTProxy
 * Implements packet size padding to hide traffic patterns
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 *               2024-2026 Dupley Maxim Igorevich (Maestro7IT)
 */

#ifndef __PADDING_H__
#define __PADDING_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Padding constants
#define PADDING_BLOCK_SIZE_DEFAULT  64      // Default padding block size
#define PADDING_BLOCK_SIZE_MIN      16      // Minimum block size
#define PADDING_BLOCK_SIZE_MAX      1024    // Maximum block size
#define PADDING_MAX_SIZE            2048    // Maximum padding size
#define PADDING_LENGTH_PREFIX_SIZE  4       // 4-byte length prefix

// Padding methods
enum padding_method {
    PADDING_NONE = 0,             // No padding
    PADDING_FIXED = 1,            // Fixed block size padding
    PADDING_RANDOM = 2,           // Random padding up to max
    PADDING_TLS_LIKE = 3          // TLS-like padding (256-byte blocks)
};

// Padding context structure
struct padding_ctx {
    enum padding_method method;   // Padding method
    size_t block_size;            // Block size for fixed padding
    size_t max_padding;           // Maximum padding size
    int use_length_prefix;        // Use 4-byte length prefix
    int use_random_fill;          // Add random fill before padding
    uint64_t packets_padded;      // Statistics: packets padded
    uint64_t packets_unpadded;    // Statistics: packets unpadded
    uint64_t bytes_added;         // Statistics: bytes added
    int initialized;              // Context initialized flag
};

// ============================================================================
// Core Functions
// ============================================================================

/**
 * Initialize padding context
 * 
 * @param ctx Pointer to context structure (must be allocated by caller)
 * @param method Padding method to use
 * @param block_size Block size for fixed padding (0 for default)
 * @param max_padding Maximum padding size (0 for no limit)
 * @return 0 on success, negative value on error
 */
int padding_init(struct padding_ctx *ctx,
                 enum padding_method method,
                 size_t block_size,
                 size_t max_padding);

/**
 * Reset padding context to initial state
 * 
 * @param ctx Pointer to context structure
 * @return 0 on success, negative value on error
 */
int padding_reset(struct padding_ctx *ctx);

/**
 * Cleanup padding context
 * 
 * @param ctx Pointer to context structure
 */
void padding_cleanup(struct padding_ctx *ctx);

// ============================================================================
// Padding Functions
// ============================================================================

/**
 * Add padding to packet
 * 
 * @param ctx Pointer to padding context
 * @param packet Input/output packet buffer (must have enough space)
 * @param packet_len Pointer to packet length (updated with padded length)
 * @param max_len Maximum buffer size
 * @return 0 on success, negative value on error
 */
int padding_add(struct padding_ctx *ctx,
                unsigned char *packet,
                size_t *packet_len,
                size_t max_len);

/**
 * Remove padding from packet
 * 
 * @param ctx Pointer to padding context
 * @param packet Input/output packet buffer
 * @param packet_len Pointer to packet length (updated with unpadded length)
 * @return 0 on success, negative value on error
 */
int padding_remove(struct padding_ctx *ctx,
                   unsigned char *packet,
                   size_t *packet_len);

/**
 * Calculate padded size
 * 
 * @param ctx Pointer to padding context
 * @param original_len Original packet length
 * @return Padded packet length, or negative value on error
 */
int padding_calculate_size(struct padding_ctx *ctx, size_t original_len);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get padding method name
 * 
 * @param method Padding method
 * @return String name of the method
 */
const char* padding_method_name(enum padding_method method);

/**
 * Get padding statistics
 * 
 * @param ctx Pointer to padding context
 * @param packets_pad Pointer to store packets padded count (can be NULL)
 * @param packets_unpad Pointer to store packets unpadded count (can be NULL)
 * @param bytes_added Pointer to store bytes added count (can be NULL)
 */
void padding_get_stats(struct padding_ctx *ctx,
                       uint64_t *packets_pad,
                       uint64_t *packets_unpad,
                       uint64_t *bytes_added);

/**
 * Print padding statistics to stdout
 * 
 * @param ctx Pointer to padding context
 */
void padding_print_stats(struct padding_ctx *ctx);

/**
 * Generate random padding data
 * 
 * @param buffer Output buffer
 * @param len Length of padding to generate
 * @return 0 on success, negative value on error
 */
int padding_generate_random(unsigned char *buffer, size_t len);

/**
 * Generate pattern padding (0x01, 0x02, ...)
 * 
 * @param buffer Output buffer
 * @param len Length of padding to generate
 * @param start Starting value (default: 1)
 * @return 0 on success, negative value on error
 */
int padding_generate_pattern(unsigned char *buffer, size_t len, unsigned char start);

// ============================================================================
// Inline Functions for Performance
// ============================================================================

/**
 * Quick fixed padding calculation (inline for performance)
 * 
 * @param len Original length
 * @param block_size Block size
 * @param prefix_size Length prefix size (0 or 4)
 * @return Padded length
 */
static inline size_t padding_fixed_calc(size_t len, size_t block_size, int prefix_size)
{
    size_t total_len = len + (prefix_size > 0 ? prefix_size : 0);
    size_t remainder = total_len % block_size;
    
    if (remainder == 0) {
        return total_len;
    }
    
    return total_len + (block_size - remainder);
}

#ifdef __cplusplus
}
#endif

#endif // __PADDING_H__
