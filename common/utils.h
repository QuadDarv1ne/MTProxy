/*
 * utils.h - Common utility functions header for MTProxy
 *
 * This header provides declarations for centralized utility functions
 * used throughout the MTProxy codebase.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * String Utilities
 * ============================================================================ */

// Safe string copy with bounds checking
size_t utils_strcpy(char *dest, const char *src, size_t dest_size);

// Safe string copy with length parameter
size_t utils_strncpy(char *dest, const char *src, size_t src_len, size_t dest_size);

// Safe string concatenation
size_t utils_strcat(char *dest, const char *src, size_t dest_size);

// Case-insensitive string comparison
int utils_strcasecmp(const char *s1, const char *s2);

// Safe string comparison with length
int utils_strncmp(const char *s1, const char *s2, size_t n);

// Trim whitespace from string (in-place)
char *utils_trim(char *str);

// Convert string to lowercase (in-place)
char *utils_tolower_str(char *str);

// Convert string to uppercase (in-place)
char *utils_toupper_str(char *str);

// String length with max check
size_t utils_strnlen(const char *s, size_t maxlen);

/* ============================================================================
 * Memory Utilities
 * ============================================================================ */

// Safe memory copy with overlap detection
void *utils_memcpy(void *dest, const void *src, size_t n);

// Safe memory move
void *utils_memmove(void *dest, const void *src, size_t n);

// Memory set with validation
void *utils_memset(void *s, int c, size_t n);

// Secure memory zero (compiler won't optimize away)
void utils_memzero(volatile void *s, size_t n);

// Constant-time memory comparison (for security)
int utils_memcmp_const(const void *s1, const void *s2, size_t n);

/* ============================================================================
 * Numeric Utilities
 * ============================================================================ */

// Safe string to integer conversion
int utils_atoi(const char *str, int *result);

// Safe string to long conversion
int utils_atol(const char *str, long *result);

// Safe string to long long conversion
int utils_atoll(const char *str, long long *result);

// Parse size with unit suffix (K, M, G)
int utils_parse_size(const char *str, uint64_t *result);

// Clamp integer to range
int utils_clamp_int(int value, int min, int max);

// Clamp long to range
long utils_clamp_long(long value, long min, long max);

// Integer to string conversion
int utils_int_to_string(int value, char *str, size_t max_len);

// Float to string conversion (limited precision)
int utils_float_to_string(float value, char *str, size_t max_len);

/* ============================================================================
 * Hash Utilities
 * ============================================================================ */

// Simple hash function (DJB2)
uint32_t utils_hash_djb2(const void *data, size_t len);

// FNV-1a hash function
uint32_t utils_hash_fnv1a(const void *data, size_t len);

// MurmurHash3 finalizer
uint32_t utils_hash_murmur3_finalize(uint32_t h);

/* ============================================================================
 * Time Utilities
 * ============================================================================ */

// Get current timestamp in milliseconds
uint64_t utils_time_ms(void);

// Get current timestamp in microseconds
uint64_t utils_time_us(void);

// Format timestamp to ISO 8601 string
int utils_format_timestamp(char *buf, size_t buf_size, uint64_t timestamp_ms);

/* ============================================================================
 * Byte Order Utilities
 * ============================================================================ */

// Swap 16-bit value
uint16_t utils_swap16(uint16_t val);

// Swap 32-bit value
uint32_t utils_swap32(uint32_t val);

// Swap 64-bit value
uint64_t utils_swap64(uint64_t val);

/* ============================================================================
 * Encoding Utilities (Base64, Hex)
 * ============================================================================ */

// Base64 encode
int utils_base64_encode(const uint8_t *input, size_t input_len,
                        char *output, size_t output_size);

// Base64 decode
int utils_base64_decode(const char *input, size_t input_len,
                        uint8_t *output, size_t output_size, size_t *output_len);

// Hex encode
int utils_hex_encode(const uint8_t *input, size_t input_len,
                     char *output, size_t output_size);

// Hex decode
int utils_hex_decode(const char *input, size_t input_len,
                     uint8_t *output, size_t output_size, size_t *output_len);

/* ============================================================================
 * Debug Utilities
 * ============================================================================ */

#ifdef DEBUG
// Debug print hex dump
void utils_hexdump(const void *data, size_t len, const char *prefix);
#else
#define utils_hexdump(data, len, prefix) ((void)0)
#endif

/* ============================================================================
 * Macros for common operations
 * ============================================================================ */

// Array size macro
#define UTILS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Min/Max macros
#define UTILS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define UTILS_MAX(a, b) ((a) > (b) ? (a) : (b))

// Clamp macro
#define UTILS_CLAMP(x, lo, hi) \
    (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

// Swap macro
#define UTILS_SWAP(a, b) do { \
    __typeof__(a) _tmp = (a); \
    (a) = (b); \
    (b) = _tmp; \
} while (0)

// Unused parameter macro
#define UTILS_UNUSED(x) (void)(x)

// Offset of member in struct
#define UTILS_OFFSET_OF(type, member) ((size_t)&((type *)0)->member)

// Container of macro (get struct from member pointer)
#define UTILS_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - UTILS_OFFSET_OF(type, member)))

// Alignment macros
#define UTILS_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define UTILS_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define UTILS_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Bit manipulation macros
#define UTILS_BIT_SET(x, bit) ((x) |= (1ULL << (bit)))
#define UTILS_BIT_CLEAR(x, bit) ((x) &= ~(1ULL << (bit)))
#define UTILS_BIT_TOGGLE(x, bit) ((x) ^= (1ULL << (bit)))
#define UTILS_BIT_TEST(x, bit) (((x) & (1ULL << (bit))) != 0)
#define UTILS_BIT_MASK(bits) ((1ULL << (bits)) - 1)

// Probability macro (returns true with given probability 0.0-1.0)
#define UTILS_PROBABILITY(p) (((double)rand() / RAND_MAX) < (p))

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
