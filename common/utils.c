/*
 * utils.c - Common utility functions for MTProxy
 *
 * This module provides centralized utility functions used throughout
 * the MTProxy codebase. It eliminates code duplication and provides
 * a single source of truth for common operations.
 *
 * Features:
 * - String utilities (safe strcpy, strlen, strcmp, etc.)
 * - Memory utilities (safe memcpy, memmove, etc.)
 * - Numeric utilities (safe conversions, bounds checking)
 * - Hash utilities (CRC32, simple hash functions)
 * - Time utilities (timestamp conversions)
 * - Logging utilities (common log formats)
 */

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* ============================================================================
 * String Utilities
 * ============================================================================ */

// Safe string copy with bounds checking
size_t utils_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return 0;
    }

    size_t src_len = strlen(src);
    size_t copy_len = (src_len >= dest_size) ? (dest_size - 1) : src_len;

    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';

    return copy_len;
}

// Safe string copy with length parameter
size_t utils_strncpy(char *dest, const char *src, size_t src_len, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return 0;
    }

    size_t copy_len = (src_len >= dest_size) ? (dest_size - 1) : src_len;

    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';

    return copy_len;
}

// Safe string concatenation
size_t utils_strcat(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return 0;
    }

    size_t dest_len = strlen(dest);
    size_t remaining = dest_size - dest_len;

    if (remaining <= 1) {
        return dest_len;
    }

    size_t src_len = strlen(src);
    size_t copy_len = (src_len >= remaining) ? (remaining - 1) : src_len;

    memcpy(dest + dest_len, src, copy_len);
    dest[dest_len + copy_len] = '\0';

    return dest_len + copy_len;
}

// Secure string copy (returns 0 on success, -1 on truncation)
int utils_strcpy_s(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        // Truncation will occur
        memcpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return -1;
    }
    
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    return 0;
}

// Secure string concatenation (returns 0 on success, -1 on truncation)
int utils_strcat_s(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        // Truncation will occur
        size_t remaining = dest_size - dest_len - 1;
        memcpy(dest + dest_len, src, remaining);
        dest[dest_len + remaining] = '\0';
        return -1;
    }
    
    memcpy(dest + dest_len, src, src_len);
    dest[dest_len + src_len] = '\0';
    return 0;
}

// Secure snprintf (returns 0 on success, -1 on truncation)
int utils_snprintf(char *dest, size_t dest_size, const char *format, ...) {
    if (!dest || !format || dest_size == 0) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(dest, dest_size, format, args);
    va_end(args);
    
    if (result < 0 || (size_t)result >= dest_size) {
        // Truncation or error
        dest[dest_size - 1] = '\0';
        return -1;
    }
    
    return 0;
}

// Integer to string conversion
int utils_int_to_string(int value, char *str, size_t max_len) {
    if (max_len <= 0) return 0;

    int temp = value;
    int digits = 0;

    if (temp == 0) {
        if (max_len < 2) return 0;
        str[0] = '0';
        str[1] = '\0';
        return 1;
    }

    if (temp < 0) {
        if (max_len < 2) return 0;
        str[0] = '-';
        str++;
        max_len--;
        temp = -temp;
    }

    int temp_val = temp;
    while (temp_val > 0) {
        temp_val /= 10;
        digits++;
    }

    if (digits >= max_len) return 0;

    str[digits] = '\0';
    int pos = digits - 1;
    while (temp > 0 && pos >= 0) {
        str[pos--] = '0' + (temp % 10);
        temp /= 10;
    }

    return digits;
}

// Float to string conversion (limited precision)
int utils_float_to_string(float value, char *str, size_t max_len) {
    if (max_len <= 0) return 0;

    int int_part = (int)value;
    float frac_part = value - int_part;

    if (frac_part < 0) frac_part = -frac_part;

    int len = utils_int_to_string(int_part, str, max_len);
    if (len <= 0) return 0;

    if (max_len - len < 3) return len;

    str[len++] = '.';
    int frac = (int)(frac_part * 100);
    if (frac < 10) {
        str[len++] = '0';
        str[len++] = '0' + frac;
    } else {
        str[len++] = '0' + (frac / 10);
        str[len++] = '0' + (frac % 10);
    }
    str[len] = '\0';

    return len;
}

// Case-insensitive string comparison
int utils_strcasecmp(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        return -1;
    }

#ifdef _WIN32
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

// Safe string comparison with length
int utils_strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2) {
        return -1;
    }
    return strncmp(s1, s2, n);
}

// Trim whitespace from string (in-place)
char *utils_trim(char *str) {
    if (!str) {
        return NULL;
    }

    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';

    return str;
}

// Convert string to lowercase (in-place)
char *utils_tolower_str(char *str) {
    if (!str) {
        return NULL;
    }

    for (char *p = str; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = (char)(*p - 'A' + 'a');
        }
    }

    return str;
}

// Convert string to uppercase (in-place)
char *utils_toupper_str(char *str) {
    if (!str) {
        return NULL;
    }

    for (char *p = str; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - 'a' + 'A');
        }
    }

    return str;
}

/* ============================================================================
 * Memory Utilities
 * ============================================================================ */

// Safe memory copy with overlap detection
void *utils_memcpy(void *dest, const void *src, size_t n) {
    if (!dest || !src || n == 0) {
        return dest;
    }

    // Check for overlap
    if ((dest >= src && dest < (const char *)src + n) ||
        (src >= dest && src < (const char *)dest + n)) {
        return memmove(dest, src, n);
    }

    return memcpy(dest, src, n);
}

// Safe memory move
void *utils_memmove(void *dest, const void *src, size_t n) {
    if (!dest || !src || n == 0) {
        return dest;
    }
    return memmove(dest, src, n);
}

// Memory set with validation
void *utils_memset(void *s, int c, size_t n) {
    if (!s || n == 0) {
        return s;
    }
    return memset(s, c, n);
}

// Secure memory zero (compiler won't optimize away)
void utils_memzero(volatile void *s, size_t n) {
    if (!s || n == 0) {
        return;
    }

    volatile unsigned char *p = (volatile unsigned char *)s;
    while (n--) {
        *p++ = 0;
    }

#ifdef _WIN32
    // Ensure the write is not optimized away on Windows
    MemoryBarrier();
#else
    // Ensure the write is not optimized away on Unix
    __asm__ __volatile__("" ::: "memory");
#endif
}

// Constant-time memory comparison (for security)
int utils_memcmp_const(const void *s1, const void *s2, size_t n) {
    if (!s1 || !s2) {
        return -1;
    }

    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    unsigned char result = 0;
    for (size_t i = 0; i < n; i++) {
        result |= p1[i] ^ p2[i];
    }

    return (result == 0) ? 0 : 1;
}

/* ============================================================================
 * Numeric Utilities
 * ============================================================================ */

// Safe string to integer conversion
int utils_atoi(const char *str, int *result) {
    if (!str || !result) {
        return -1;
    }

    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno != 0 || endptr == str || *endptr != '\0') {
        return -1;
    }

    if (val < INT_MIN || val > INT_MAX) {
        errno = ERANGE;
        return -1;
    }

    *result = (int)val;
    return 0;
}

// Safe string to long conversion
int utils_atol(const char *str, long *result) {
    if (!str || !result) {
        return -1;
    }

    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno != 0 || endptr == str || *endptr != '\0') {
        return -1;
    }

    *result = val;
    return 0;
}

// Safe string to long long conversion
int utils_atoll(const char *str, long long *result) {
    if (!str || !result) {
        return -1;
    }

    char *endptr;
    errno = 0;
    long long val = strtoll(str, &endptr, 10);

    if (errno != 0 || endptr == str || *endptr != '\0') {
        return -1;
    }

    *result = val;
    return 0;
}

// Parse size with unit suffix (K, M, G)
int utils_parse_size(const char *str, uint64_t *result) {
    if (!str || !result) {
        return -1;
    }

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno != 0 || endptr == str) {
        return -1;
    }

    // Handle unit suffix
    if (*endptr != '\0') {
        switch (*endptr) {
            case 'K':
            case 'k':
                val *= 1024;
                break;
            case 'M':
            case 'm':
                val *= 1024 * 1024;
                break;
            case 'G':
            case 'g':
                val *= 1024 * 1024 * 1024;
                break;
            default:
                return -1;
        }
    }

    if (val < 0 || val > (double)UINT64_MAX) {
        errno = ERANGE;
        return -1;
    }

    *result = (uint64_t)val;
    return 0;
}

// Clamp integer to range
int utils_clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Clamp long to range
long utils_clamp_long(long value, long min, long max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/* ============================================================================
 * Hash Utilities
 * ============================================================================ */

// Simple hash function (DJB2)
uint32_t utils_hash_djb2(const void *data, size_t len) {
    if (!data || len == 0) {
        return 0;
    }

    const unsigned char *bytes = (const unsigned char *)data;
    uint32_t hash = 5381;

    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }

    return hash;
}

// FNV-1a hash function
uint32_t utils_hash_fnv1a(const void *data, size_t len) {
    if (!data || len == 0) {
        return 0;
    }

    const unsigned char *bytes = (const unsigned char *)data;
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }

    return hash;
}

// Rotate left helper
static inline uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

// MurmurHash3 finalizer
uint32_t utils_hash_murmur3_finalize(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/* ============================================================================
 * Time Utilities
 * ============================================================================ */

// Get current timestamp in milliseconds
uint64_t utils_time_ms(void) {
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif
}

// Get current timestamp in microseconds
uint64_t utils_time_us(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (uint64_t)(count.QuadPart * 1000000 / freq.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
#endif
}

// Format timestamp to ISO 8601 string
int utils_format_timestamp(char *buf, size_t buf_size, uint64_t timestamp_ms) {
    if (!buf || buf_size < 21) {
        return -1;
    }

    time_t seconds = (time_t)(timestamp_ms / 1000);
    int millis = (int)(timestamp_ms % 1000);

#ifdef _WIN32
    struct tm tm_buf;
    struct tm *tm_info = gmtime_s(&tm_buf, &seconds) ? NULL : &tm_buf;
#else
    struct tm tm_buf;
    struct tm *tm_info = gmtime_r(&seconds, &tm_buf);
#endif

    if (!tm_info) {
        buf[0] = '\0';
        return -1;
    }

    snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             millis);

    return 0;
}

/* ============================================================================
 * Byte Order Utilities
 * ============================================================================ */

// Swap 16-bit value
uint16_t utils_swap16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

// Swap 32-bit value
uint32_t utils_swap32(uint32_t val) {
    return ((val << 24) | ((val << 8) & 0x00FF0000) |
            ((val >> 8) & 0x0000FF00) | (val >> 24));
}

// Swap 64-bit value
uint64_t utils_swap64(uint64_t val) {
    return ((val << 56) | ((val << 40) & 0x00FF000000000000ULL) |
            ((val << 24) & 0x0000FF0000000000ULL) | ((val << 8) & 0x000000FF00000000ULL) |
            ((val >> 8) & 0x00000000FF000000ULL) | ((val >> 24) & 0x0000000000FF0000ULL) |
            ((val >> 40) & 0x000000000000FF00ULL) | (val >> 56));
}

/* ============================================================================
 * Debug Utilities
 * ============================================================================ */

#ifdef DEBUG

// Debug print hex dump
void utils_hexdump(const void *data, size_t len, const char *prefix) {
    if (!data || len == 0) {
        return;
    }

    const unsigned char *bytes = (const unsigned char *)data;
    char prefix_buf[64];

    if (prefix) {
        snprintf(prefix_buf, sizeof(prefix_buf), "%s: ", prefix);
    } else {
        prefix_buf[0] = '\0';
    }

    fprintf(stderr, "%s", prefix_buf);
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) {
            fprintf(stderr, "\n%s", prefix_buf);
        }
        fprintf(stderr, "%02X ", bytes[i]);
    }
    fprintf(stderr, "\n");
}

#endif

/* ============================================================================
 * Encoding Utilities (Base64, Hex)
 * ============================================================================ */

// Base64 encoding table
static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encode
int utils_base64_encode(const uint8_t *input, size_t input_len,
                        char *output, size_t output_size) {
    if (!input || !output || input_len == 0) {
        return -1;
    }

    size_t encoded_len = 4 * ((input_len + 2) / 3);
    if (output_size <= encoded_len) {
        return -1;
    }

    size_t i, j;
    for (i = 0, j = 0; i < input_len;) {
        uint32_t octet_a = i < input_len ? input[i++] : 0;
        uint32_t octet_b = i < input_len ? input[i++] : 0;
        uint32_t octet_c = i < input_len ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        output[j++] = base64_table[(triple >> 18) & 0x3F];
        output[j++] = base64_table[(triple >> 12) & 0x3F];
        output[j++] = base64_table[(triple >> 6) & 0x3F];
        output[j++] = base64_table[triple & 0x3F];
    }

    // Add padding
    size_t mod = input_len % 3;
    if (mod > 0) {
        for (size_t k = 0; k < 3 - mod; k++) {
            output[encoded_len - 1 - k] = '=';
        }
    }

    output[encoded_len] = '\0';
    return (int)encoded_len;
}

// Base64 decoding table
static const int base64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

// Base64 decode
int utils_base64_decode(const char *input, size_t input_len,
                        uint8_t *output, size_t output_size, size_t *output_len) {
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }

    if (input_len % 4 != 0) {
        return -1;
    }

    size_t decoded_len = (input_len / 4) * 3;
    if (output_size < decoded_len) {
        return -1;
    }

    // Count padding
    int padding = 0;
    if (input[input_len - 1] == '=') padding++;
    if (input[input_len - 2] == '=') padding++;

    size_t i, j;
    for (i = 0, j = 0; i < input_len;) {
        int a = (i < input_len) ? base64_decode_table[(unsigned char)input[i++]] : -1;
        int b = (i < input_len) ? base64_decode_table[(unsigned char)input[i++]] : -1;
        int c = (i < input_len) ? base64_decode_table[(unsigned char)input[i++]] : -1;
        int d = (i < input_len) ? base64_decode_table[(unsigned char)input[i++]] : -1;

        if (a == -1 || b == -1) {
            return -1;
        }

        uint32_t triple = (a << 18) | (b << 12) | ((c == -1 ? 0 : c) << 6) | (d == -1 ? 0 : d);

        if (j < decoded_len - padding) output[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_len - padding) output[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_len - padding) output[j++] = triple & 0xFF;
    }

    *output_len = decoded_len - padding;
    return (int)*output_len;
}

// Hex encode
int utils_hex_encode(const uint8_t *input, size_t input_len,
                     char *output, size_t output_size) {
    if (!input || !output || input_len == 0) {
        return -1;
    }

    if (output_size < input_len * 2 + 1) {
        return -1;
    }

    static const char hex_chars[] = "0123456789abcdef";
    size_t i, j;
    for (i = 0, j = 0; i < input_len; i++) {
        output[j++] = hex_chars[(input[i] >> 4) & 0x0F];
        output[j++] = hex_chars[input[i] & 0x0F];
    }
    output[j] = '\0';

    return (int)(input_len * 2);
}

// Hex decode value
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Hex decode
int utils_hex_decode(const char *input, size_t input_len,
                     uint8_t *output, size_t output_size, size_t *output_len) {
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }

    if (input_len % 2 != 0) {
        return -1;
    }

    size_t decoded_len = input_len / 2;
    if (output_size < decoded_len) {
        return -1;
    }

    size_t i, j;
    for (i = 0, j = 0; i < input_len; i += 2) {
        int high = hex_char_to_int(input[i]);
        int low = hex_char_to_int(input[i + 1]);

        if (high == -1 || low == -1) {
            return -1;
        }

        output[j++] = (uint8_t)((high << 4) | low);
    }

    *output_len = decoded_len;
    return (int)*output_len;
}
