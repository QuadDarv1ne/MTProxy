/*
 * TLS Fragmentation module for MTProxy
 * Implements TLS handshake fragmentation to bypass DPI
 * 
 * Copyright 2024-2026 MTProto Proxy Enhanced Project
 *               2024-2026 Dupley Maxim Igorevich (Maestro7IT)
 */

#ifndef __FRAGMENTATION_H__
#define __FRAGMENTATION_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fragmentation constants
#define FRAGMENT_MIN_SIZE       64      // Minimum fragment size
#define FRAGMENT_MAX_SIZE       512     // Maximum fragment size
#define FRAGMENT_DEFAULT_SIZE   128     // Default fragment size
#define FRAGMENT_MAX_COUNT      32      // Maximum fragments per message
#define FRAGMENT_HEADER_SIZE    5       // TLS record header size

// TLS record types
enum tls_record_type {
    TLS_RECORD_CHANGE_CIPHER_SPEC = 20,
    TLS_RECORD_ALERT              = 21,
    TLS_RECORD_HANDSHAKE          = 22,
    TLS_RECORD_APPLICATION_DATA   = 23,
    TLS_RECORD_HEARTBEAT          = 24
};

// TLS handshake types
enum tls_handshake_type {
    TLS_HELLO_REQUEST          = 0,
    TLS_CLIENT_HELLO           = 1,
    TLS_SERVER_HELLO           = 2,
    TLS_HELLO_VERIFY_REQUEST   = 3,
    TLS_NEW_SESSION_TICKET     = 4,
    TLS_END_OF_EARLY_DATA      = 5,
    TLS_ENCRYPTED_EXTENSIONS   = 8,
    TLS_CERTIFICATE            = 11,
    TLS_SERVER_KEY_EXCHANGE    = 12,
    TLS_CERTIFICATE_REQUEST    = 13,
    TLS_SERVER_HELLO_DONE    = 14,
    TLS_CERTIFICATE_VERIFY     = 15,
    TLS_CLIENT_KEY_EXCHANGE    = 16,
    TLS_FINISHED               = 20,
    TLS_CERTIFICATE_URL        = 21,
    TLS_CERTIFICATE_STATUS     = 22,
    TLS_SUPPLEMENTAL_DATA      = 23,
    TLS_KEY_UPDATE             = 24,
    TLS_COMPRESSED_CERTIFICATE = 25,
    TLS_MESSAGE_HASH           = 254
};

// Fragmentation modes
enum fragmentation_mode {
    FRAGMENT_NONE = 0,          // No fragmentation
    FRAGMENT_FIXED = 1,         // Fixed size fragments
    FRAGMENT_RANDOM = 2,        // Random size fragments
    FRAGMENT_TLS_LIKE = 3       // TLS-like fragmentation
};

// Fragment structure
struct tls_fragment {
    unsigned char data[FRAGMENT_MAX_SIZE];  // Fragment data
    size_t len;                             // Fragment length
    int is_last;                            // Last fragment flag
    int fragment_index;                     // Fragment index (0-based)
    int total_fragments;                    // Total fragments count
};

// Fragmentation context structure
struct fragmentation_ctx {
    enum fragmentation_mode mode;   // Fragmentation mode
    size_t fragment_size;           // Target fragment size
    size_t min_fragment;            // Minimum fragment size (for random)
    size_t max_fragment;            // Maximum fragment size (for random)
    int add_header_to_each;         // Add TLS header to each fragment
    int randomize_order;            // Randomize fragment order
    int add_delay;                  // Add delay between fragments
    int delay_ms;                   // Delay in milliseconds
    
    // Statistics
    uint64_t messages_fragmented;   // Messages fragmented
    uint64_t fragments_created;     // Fragments created
    uint64_t messages_assembled;    // Messages assembled
    uint64_t fragments_processed;   // Fragments processed
    uint64_t errors;                // Errors count
    
    int initialized;                // Context initialized flag
};

// ============================================================================
// Core Functions
// ============================================================================

/**
 * Initialize fragmentation context
 * 
 * @param ctx Pointer to context structure (must be allocated by caller)
 * @param mode Fragmentation mode to use
 * @param fragment_size Target fragment size (0 for default)
 * @return 0 on success, negative value on error
 */
int fragmentation_init(struct fragmentation_ctx *ctx,
                      enum fragmentation_mode mode,
                      size_t fragment_size);

/**
 * Reset fragmentation context to initial state
 * 
 * @param ctx Pointer to context structure
 * @return 0 on success, negative value on error
 */
int fragmentation_reset(struct fragmentation_ctx *ctx);

/**
 * Cleanup fragmentation context
 * 
 * @param ctx Pointer to context structure
 */
void fragmentation_cleanup(struct fragmentation_ctx *ctx);

// ============================================================================
// Fragmentation Functions
// ============================================================================

/**
 * Fragment a TLS message into multiple fragments
 * 
 * @param ctx Pointer to fragmentation context
 * @param message Input message buffer
 * @param message_len Length of input message
 * @param fragments Output array of fragments (must be pre-allocated)
 * @param max_fragments Maximum number of fragments
 * @param fragment_count Pointer to store actual fragment count
 * @return 0 on success, negative value on error
 */
int fragmentation_fragment_message(struct fragmentation_ctx *ctx,
                                  const unsigned char *message,
                                  size_t message_len,
                                  struct tls_fragment *fragments,
                                  int max_fragments,
                                  int *fragment_count);

/**
 * Assemble fragments back into original message
 * 
 * @param ctx Pointer to fragmentation context
 * @param fragments Input array of fragments
 * @param fragment_count Number of fragments
 * @param output Output buffer for assembled message
 * @param max_output_len Maximum output buffer size
 * @param output_len Pointer to store actual output length
 * @return 0 on success, negative value on error
 */
int fragmentation_assemble_message(struct fragmentation_ctx *ctx,
                                  const struct tls_fragment *fragments,
                                  int fragment_count,
                                  unsigned char *output,
                                  size_t max_output_len,
                                  size_t *output_len);

/**
 * Create TLS record header
 * 
 * @param record_type TLS record type
 * @param version TLS version (e.g., 0x0303 for TLS 1.2)
 * @param length Payload length
 * @param output Output buffer (must be at least FRAGMENT_HEADER_SIZE bytes)
 * @return 0 on success, negative value on error
 */
int fragmentation_create_tls_header(enum tls_record_type record_type,
                                   uint16_t version,
                                   size_t length,
                                   unsigned char *output);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get fragmentation mode name
 * 
 * @param mode Fragmentation mode
 * @return String name of the mode
 */
const char* fragmentation_mode_name(enum fragmentation_mode mode);

/**
 * Get TLS record type name
 * 
 * @param type TLS record type
 * @return String name of the type
 */
const char* fragmentation_record_type_name(enum tls_record_type type);

/**
 * Get TLS handshake type name
 * 
 * @param type TLS handshake type
 * @return String name of the type
 */
const char* fragmentation_handshake_type_name(enum tls_handshake_type type);

/**
 * Get fragmentation statistics
 * 
 * @param ctx Pointer to fragmentation context
 * @param messages_frag Pointer to store messages fragmented count (can be NULL)
 * @param fragments_created Pointer to store fragments created count (can be NULL)
 * @param messages_assembled Pointer to store messages assembled count (can be NULL)
 * @param errors Pointer to store errors count (can be NULL)
 */
void fragmentation_get_stats(struct fragmentation_ctx *ctx,
                            uint64_t *messages_frag,
                            uint64_t *fragments_created,
                            uint64_t *messages_assembled,
                            uint64_t *errors);

/**
 * Print fragmentation statistics to stdout
 * 
 * @param ctx Pointer to fragmentation context
 */
void fragmentation_print_stats(struct fragmentation_ctx *ctx);

/**
 * Calculate number of fragments needed
 * 
 * @param ctx Pointer to fragmentation context
 * @param message_len Length of message to fragment
 * @return Number of fragments needed, or negative value on error
 */
int fragmentation_calculate_count(struct fragmentation_ctx *ctx, size_t message_len);

/**
 * Validate TLS fragment
 * 
 * @param fragment Pointer to fragment to validate
 * @return 0 if valid, negative value if invalid
 */
int fragmentation_validate_fragment(const struct tls_fragment *fragment);

// ============================================================================
// Inline Functions for Performance
// ============================================================================

/**
 * Check if message needs fragmentation (inline for performance)
 * 
 * @param ctx Pointer to fragmentation context
 * @param message_len Length of message
 * @return 1 if fragmentation needed, 0 otherwise
 */
static inline int fragmentation_needed(struct fragmentation_ctx *ctx, size_t message_len)
{
    if (!ctx || !ctx->initialized || ctx->mode == FRAGMENT_NONE) {
        return 0;
    }
    
    size_t threshold = (ctx->mode == FRAGMENT_RANDOM) ? ctx->max_fragment : ctx->fragment_size;
    return (message_len > threshold) ? 1 : 0;
}

/**
 * Get optimal fragment size (inline for performance)
 * 
 * @param ctx Pointer to fragmentation context
 * @return Optimal fragment size
 */
static inline size_t fragmentation_get_size(struct fragmentation_ctx *ctx)
{
    if (!ctx || !ctx->initialized) {
        return FRAGMENT_DEFAULT_SIZE;
    }
    
    if (ctx->mode == FRAGMENT_RANDOM) {
        // Return random size between min and max
        return ctx->min_fragment + (rand() % (ctx->max_fragment - ctx->min_fragment + 1));
    }
    
    return ctx->fragment_size;
}

#ifdef __cplusplus
}
#endif

#endif // __FRAGMENTATION_H__
