/*
 * Obfuscation module for MTProxy
 * Implements XOR-based packet obfuscation to bypass DPI
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 *               2024-2026 Dupley Maxim Igorevich (Maestro7IT)
 */

#ifndef __OBFUSCATE_H__
#define __OBFUSCATE_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Obfuscation constants
#define OBFUSCATE_KEY_SIZE      32      // Maximum key size in bytes
#define OBFUSCATE_KEY_MIN_SIZE  8       // Minimum key size
#define OBFUSCATE_DEFAULT_KEY   "mtproto-obfuscation-key-2024"

// Obfuscation methods
enum obfuscate_method {
    OBFUSCATE_NONE = 0,         // No obfuscation
    OBFUSCATE_XOR_BASIC = 1,    // Basic XOR with static key
    OBFUSCATE_XOR_ROTATING = 2, // XOR with rotating key
    OBFUSCATE_XOR_HASH = 3      // XOR with hashed key
};

// Obfuscation context structure
struct obfuscate_ctx {
    unsigned char key[OBFUSCATE_KEY_SIZE];  // Obfuscation key
    size_t key_len;                         // Actual key length
    size_t key_pos;                         // Current key position (for rotating)
    enum obfuscate_method method;           // Obfuscation method
    uint64_t packets_obfuscated;            // Statistics: packets obfuscated
    uint64_t packets_deobfuscated;          // Statistics: packets deobfuscated
    uint64_t bytes_processed;               // Statistics: bytes processed
    int initialized;                        // Context initialized flag
};

// ============================================================================
// Core Functions
// ============================================================================

/**
 * Initialize obfuscation context
 * 
 * @param ctx Pointer to context structure (must be allocated by caller)
 * @param key Obfuscation key (NULL for default key)
 * @param key_len Length of the key (0 for default key)
 * @param method Obfuscation method to use
 * @return 0 on success, negative value on error
 */
int obfuscate_init(struct obfuscate_ctx *ctx, 
                   const unsigned char *key, 
                   size_t key_len,
                   enum obfuscate_method method);

/**
 * Reset obfuscation context to initial state
 * 
 * @param ctx Pointer to context structure
 * @return 0 on success, negative value on error
 */
int obfuscate_reset(struct obfuscate_ctx *ctx);

/**
 * Cleanup obfuscation context (securely erase sensitive data)
 * 
 * @param ctx Pointer to context structure
 */
void obfuscate_cleanup(struct obfuscate_ctx *ctx);

// ============================================================================
// Obfuscation Functions
// ============================================================================

/**
 * Obfuscate packet data in-place
 * 
 * @param ctx Pointer to obfuscation context
 * @param data Pointer to data buffer (modified in-place)
 * @param len Length of data
 * @return Number of bytes processed, negative value on error
 */
int obfuscate_packet(struct obfuscate_ctx *ctx, 
                     unsigned char *data, 
                     size_t len);

/**
 * Deobfuscate packet data in-place (XOR is reversible)
 * 
 * @param ctx Pointer to obfuscation context
 * @param data Pointer to data buffer (modified in-place)
 * @param len Length of data
 * @return Number of bytes processed, negative value on error
 */
int deobfuscate_packet(struct obfuscate_ctx *ctx, 
                       unsigned char *data, 
                       size_t len);

/**
 * Obfuscate packet data with output buffer
 * 
 * @param ctx Pointer to obfuscation context
 * @param input Input data buffer
 * @param input_len Length of input data
 * @param output Output buffer (must be at least input_len bytes)
 * @param output_len Pointer to output length (set to actual length)
 * @return 0 on success, negative value on error
 */
int obfuscate_packet_ex(struct obfuscate_ctx *ctx,
                        const unsigned char *input,
                        size_t input_len,
                        unsigned char *output,
                        size_t *output_len);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Generate random obfuscation key
 * 
 * @param key Buffer for key (must be at least OBFUSCATE_KEY_SIZE bytes)
 * @param key_len Desired key length
 * @return 0 on success, negative value on error
 */
int obfuscate_generate_key(unsigned char *key, size_t key_len);

/**
 * Hash a string to create an obfuscation key
 * 
 * @param input Input string
 * @param input_len Length of input string
 * @param key Output key buffer (must be at least OBFUSCATE_KEY_SIZE bytes)
 * @param key_len Pointer to key length (set to actual length)
 * @return 0 on success, negative value on error
 */
int obfuscate_hash_key(const char *input, 
                       size_t input_len,
                       unsigned char *key, 
                       size_t *key_len);

/**
 * Get obfuscation method name
 * 
 * @param method Obfuscation method
 * @return String name of the method
 */
const char* obfuscate_method_name(enum obfuscate_method method);

/**
 * Get obfuscation statistics
 * 
 * @param ctx Pointer to obfuscation context
 * @param packets_obf Pointer to store packets obfuscated count (can be NULL)
 * @param packets_deobf Pointer to store packets deobfuscated count (can be NULL)
 * @param bytes_proc Pointer to store bytes processed count (can be NULL)
 */
void obfuscate_get_stats(struct obfuscate_ctx *ctx,
                         uint64_t *packets_obf,
                         uint64_t *packets_deobf,
                         uint64_t *bytes_proc);

/**
 * Print obfuscation statistics to stdout
 * 
 * @param ctx Pointer to obfuscation context
 */
void obfuscate_print_stats(struct obfuscate_ctx *ctx);

// ============================================================================
// Inline Functions for Performance
// ============================================================================

/**
 * Quick XOR obfuscation (inline for performance)
 * 
 * @param data Data buffer
 * @param len Data length
 * @param key Key buffer
 * @param key_len Key length
 * @param key_pos Pointer to current key position (updated)
 */
static inline void obfuscate_xor_inline(unsigned char *data, 
                                        size_t len,
                                        const unsigned char *key,
                                        size_t key_len,
                                        size_t *key_pos)
{
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= key[*key_pos];
        *key_pos = (*key_pos + 1) % key_len;
    }
}

/**
 * Quick XOR deobfuscation (inline for performance)
 * Same as obfuscation since XOR is reversible
 */
#define deobfuscate_xor_inline obfuscate_xor_inline

#ifdef __cplusplus
}
#endif

#endif // __OBFUSCATE_H__
