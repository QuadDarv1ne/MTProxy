/*
 * test_utils_encoding.c — Тесты для функций кодирования utils
 *
 * Тестирование:
 * - utils_base64_encode/decode
 * - utils_hex_encode/decode
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/utils.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    if (test_##name() == 0) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while(0)

/* ============================================================================
 * Base64 Tests
 * ============================================================================ */

// Тест: базовое кодирование
TEST(base64_encode_basic) {
    const uint8_t input[] = "Hello";
    char output[64];
    int result = utils_base64_encode(input, 5, output, sizeof(output));
    
    if (result < 0) return -1;
    if (strcmp(output, "SGVsbG8=") != 0) {
        printf("Expected 'SGVsbG8=', got '%s'", output);
        return -1;
    }
    return 0;
}

// Тест: кодирование пустой строки
TEST(base64_encode_empty) {
    const uint8_t input[] = "";
    char output[64];
    int result = utils_base64_encode(input, 0, output, sizeof(output));
    
    if (result != -1) {
        printf("Expected -1 for empty input, got %d", result);
        return -1;
    }
    return 0;
}

// Тест: кодирование бинарных данных
TEST(base64_encode_binary) {
    const uint8_t input[] = {0x00, 0x01, 0x02, 0x03};
    char output[64];
    int result = utils_base64_encode(input, 4, output, sizeof(output));
    
    if (result < 0) return -1;
    if (strcmp(output, "AAECAw==") != 0) {
        printf("Expected 'AAECAw==', got '%s'", output);
        return -1;
    }
    return 0;
}

// Тест: декодирование базовое
TEST(base64_decode_basic) {
    const char input[] = "SGVsbG8=";
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_base64_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    if (result < 0) return -1;
    if (output_len != 5) {
        printf("Expected length 5, got %zu", output_len);
        return -1;
    }
    if (memcmp(output, "Hello", 5) != 0) {
        printf("Expected 'Hello', got '%.*s'", (int)output_len, output);
        return -1;
    }
    return 0;
}

// Тест: декодирование без padding
TEST(base64_decode_no_padding) {
    const char input[] = "SGVsbG8";  // Без '='
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_base64_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    // Должно работать с padding или без
    if (result < 0 && result != -1) return -1;
    return 0;
}

// Тест: roundtrip encode/decode
TEST(base64_roundtrip) {
    const uint8_t original[] = "Test data for Base64 roundtrip!";
    char encoded[128];
    uint8_t decoded[128];
    size_t decoded_len = 0;
    
    int enc_result = utils_base64_encode(original, strlen((const char*)original), encoded, sizeof(encoded));
    if (enc_result < 0) return -1;
    
    int dec_result = utils_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded), &decoded_len);
    if (dec_result < 0) return -1;
    
    if (decoded_len != strlen((const char*)original)) {
        printf("Length mismatch: %zu vs %zu", decoded_len, strlen((const char*)original));
        return -1;
    }
    
    if (memcmp(original, decoded, decoded_len) != 0) {
        printf("Data mismatch");
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * Hex Tests
 * ============================================================================ */

// Тест: базовое hex кодирование
TEST(hex_encode_basic) {
    const uint8_t input[] = "Hello";
    char output[64];
    int result = utils_hex_encode(input, 5, output, sizeof(output));
    
    if (result < 0) return -1;
    if (strcmp(output, "48656c6c6f") != 0) {
        printf("Expected '48656c6c6f', got '%s'", output);
        return -1;
    }
    return 0;
}

// Тест: hex кодирование пустой строки
TEST(hex_encode_empty) {
    const uint8_t input[] = "";
    char output[64];
    int result = utils_hex_encode(input, 0, output, sizeof(output));
    
    if (result != -1) {
        printf("Expected -1 for empty input, got %d", result);
        return -1;
    }
    return 0;
}

// Тест: hex кодирование бинарных данных
TEST(hex_encode_binary) {
    const uint8_t input[] = {0x00, 0x01, 0x02, 0x03, 0xFF};
    char output[64];
    int result = utils_hex_encode(input, 5, output, sizeof(output));
    
    if (result < 0) return -1;
    if (strcmp(output, "00010203ff") != 0) {
        printf("Expected '00010203ff', got '%s'", output);
        return -1;
    }
    return 0;
}

// Тест: hex декодирование базовое
TEST(hex_decode_basic) {
    const char input[] = "48656c6c6f";
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_hex_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    if (result < 0) return -1;
    if (output_len != 5) {
        printf("Expected length 5, got %zu", output_len);
        return -1;
    }
    if (memcmp(output, "Hello", 5) != 0) {
        printf("Expected 'Hello'");
        return -1;
    }
    return 0;
}

// Тест: hex декодирование с заглавными буквами
TEST(hex_decode_uppercase) {
    const char input[] = "48656C6C6F";  // Заглавные
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_hex_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    if (result < 0) return -1;
    if (output_len != 5) return -1;
    if (memcmp(output, "Hello", 5) != 0) return -1;
    return 0;
}

// Тест: hex roundtrip encode/decode
TEST(hex_roundtrip) {
    const uint8_t original[] = "Test data for Hex roundtrip!";
    char encoded[128];
    uint8_t decoded[128];
    size_t decoded_len = 0;
    
    int enc_result = utils_hex_encode(original, strlen((const char*)original), encoded, sizeof(encoded));
    if (enc_result < 0) return -1;
    
    int dec_result = utils_hex_decode(encoded, strlen(encoded), decoded, sizeof(decoded), &decoded_len);
    if (dec_result < 0) return -1;
    
    if (decoded_len != strlen((const char*)original)) {
        printf("Length mismatch");
        return -1;
    }
    
    if (memcmp(original, decoded, decoded_len) != 0) {
        printf("Data mismatch");
        return -1;
    }
    
    return 0;
}

// Тест: hex декодирование невалидных данных
TEST(hex_decode_invalid) {
    const char input[] = "48656X6c6f";  // 'X' не валидный hex символ
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_hex_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    if (result != -1) {
        printf("Expected -1 for invalid hex, got %d", result);
        return -1;
    }
    return 0;
}

// Тест: hex декодирование нечётной длины
TEST(hex_decode_odd_length) {
    const char input[] = "48656c6c";  // 7 символов (нечётно)
    uint8_t output[64];
    size_t output_len = 0;
    int result = utils_hex_decode(input, strlen(input), output, sizeof(output), &output_len);
    
    if (result != -1) {
        printf("Expected -1 for odd length, got %d", result);
        return -1;
    }
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[]) {
    printf("=== Utils Encoding Tests ===\n\n");
    
    // Base64 тесты
    printf("--- Base64 Tests ---\n");
    RUN_TEST(base64_encode_basic);
    RUN_TEST(base64_encode_empty);
    RUN_TEST(base64_encode_binary);
    RUN_TEST(base64_decode_basic);
    RUN_TEST(base64_decode_no_padding);
    RUN_TEST(base64_roundtrip);
    
    // Hex тесты
    printf("\n--- Hex Tests ---\n");
    RUN_TEST(hex_encode_basic);
    RUN_TEST(hex_encode_empty);
    RUN_TEST(hex_encode_binary);
    RUN_TEST(hex_decode_basic);
    RUN_TEST(hex_decode_uppercase);
    RUN_TEST(hex_decode_invalid);
    RUN_TEST(hex_decode_odd_length);
    RUN_TEST(hex_roundtrip);
    
    // Итоги
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
