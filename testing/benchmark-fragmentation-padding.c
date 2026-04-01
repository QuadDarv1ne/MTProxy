/**
 * @file benchmark-fragmentation-padding.c
 * @brief Бенчмарки производительности модулей Fragmentation и Padding
 *
 * Тестирует производительность:
 * - Fragmentation: фрагментация TLS сообщений
 * - Padding: добавление/удаление padding
 * - Комбинированный режим: fragmentation + padding
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../net/fragmentation.h"
#include "../net/padding.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

/* ============================================
 * Утилиты
 * ============================================ */

/**
 * @brief Получить текущее время в микросекундах
 */
static uint64_t get_time_us(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000 / freq.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000000 + tv.tv_usec);
#endif
}

/**
 * @brief Генерация тестовых данных
 */
static void generate_test_data(unsigned char *buffer, size_t size, int pattern) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (unsigned char)((pattern + i) % 256);
    }
}

/**
 * @brief Структура результатов бенчмарка
 */
typedef struct {
    const char *test_name;
    uint64_t operations;
    uint64_t duration_us;
    double ops_per_sec;
    double bytes_per_sec;
    size_t bytes_processed;
} benchmark_result_t;

/* ============================================
 * Тесты Fragmentation
 * ============================================ */

/**
 * @brief Бенчмарк: фрагментация сообщений фиксированного размера
 */
static benchmark_result_t benchmark_fragmentation_fixed(size_t message_size, 
                                                        size_t fragment_size,
                                                        int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Fragmentation Fixed";
    
    struct fragmentation_ctx ctx;
    unsigned char *message = malloc(message_size);
    struct tls_fragment *fragments = malloc(sizeof(struct tls_fragment) * FRAGMENT_MAX_COUNT);
    int fragment_count = 0;
    
    generate_test_data(message, message_size, 0x41);
    fragmentation_init(&ctx, FRAGMENT_FIXED, fragment_size);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        fragmentation_fragment_message(&ctx, message, message_size, 
                                       fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = message_size * iterations;
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    fragmentation_cleanup(&ctx);
    free(message);
    free(fragments);
    
    return result;
}

/**
 * @brief Бенчмарк: фрагментация с сборкой
 */
static benchmark_result_t benchmark_fragmentation_assemble(size_t message_size,
                                                           size_t fragment_size,
                                                           int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Fragmentation + Assemble";
    
    struct fragmentation_ctx ctx;
    unsigned char *message = malloc(message_size);
    unsigned char *assembled = malloc(message_size * 2);
    struct tls_fragment *fragments = malloc(sizeof(struct tls_fragment) * FRAGMENT_MAX_COUNT);
    int fragment_count = 0;
    size_t assembled_len = 0;
    
    generate_test_data(message, message_size, 0x42);
    fragmentation_init(&ctx, FRAGMENT_FIXED, fragment_size);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        fragmentation_fragment_message(&ctx, message, message_size,
                                       fragments, FRAGMENT_MAX_COUNT, &fragment_count);
        fragmentation_assemble_message(&ctx, fragments, fragment_count,
                                       assembled, message_size * 2, &assembled_len);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = message_size * iterations * 2;  // fragment + assemble
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    fragmentation_cleanup(&ctx);
    free(message);
    free(assembled);
    free(fragments);
    
    return result;
}

/**
 * @brief Бенчмарк: случайная фрагментация
 */
static benchmark_result_t benchmark_fragmentation_random(size_t message_size,
                                                         int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Fragmentation Random";
    
    struct fragmentation_ctx ctx;
    unsigned char *message = malloc(message_size);
    struct tls_fragment *fragments = malloc(sizeof(struct tls_fragment) * FRAGMENT_MAX_COUNT);
    int fragment_count = 0;
    
    generate_test_data(message, message_size, 0x43);
    fragmentation_init(&ctx, FRAGMENT_RANDOM, 128);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        fragmentation_fragment_message(&ctx, message, message_size,
                                       fragments, FRAGMENT_MAX_COUNT, &fragment_count);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = message_size * iterations;
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    fragmentation_cleanup(&ctx);
    free(message);
    free(fragments);
    
    return result;
}

/* ============================================
 * Тесты Padding
 * ============================================ */

/**
 * @brief Бенчмарк: добавление fixed padding
 */
static benchmark_result_t benchmark_padding_fixed(size_t data_size, 
                                                  size_t block_size,
                                                  int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Padding Fixed Add";
    
    struct padding_ctx ctx;
    unsigned char *buffer = malloc(data_size + block_size * 2);
    size_t padded_len = data_size;
    
    generate_test_data(buffer, data_size, 0x50);
    padding_init(&ctx, PADDING_FIXED, block_size, 0);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        padded_len = data_size;
        memcpy(buffer, buffer, data_size);  // Сброс данных
        padding_add(&ctx, buffer, &padded_len, data_size + block_size * 2);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = data_size * iterations;
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    padding_cleanup(&ctx);
    free(buffer);
    
    return result;
}

/**
 * @brief Бенчмарк: добавление и удаление padding
 */
static benchmark_result_t benchmark_padding_roundtrip(size_t data_size,
                                                      size_t block_size,
                                                      int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Padding Roundtrip";
    
    struct padding_ctx ctx;
    unsigned char *buffer = malloc(data_size + block_size * 2);
    size_t padded_len = data_size;
    
    generate_test_data(buffer, data_size, 0x51);
    padding_init(&ctx, PADDING_FIXED, block_size, 0);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        padded_len = data_size;
        memcpy(buffer, buffer, data_size);
        padding_add(&ctx, buffer, &padded_len, data_size + block_size * 2);
        padding_remove(&ctx, buffer, &padded_len);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = data_size * iterations * 2;  // add + remove
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    padding_cleanup(&ctx);
    free(buffer);
    
    return result;
}

/**
 * @brief Бенчмарк: random padding
 */
static benchmark_result_t benchmark_padding_random(size_t data_size,
                                                   size_t max_padding,
                                                   int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Padding Random";
    
    struct padding_ctx ctx;
    unsigned char *buffer = malloc(data_size + max_padding + 64);
    size_t padded_len = data_size;
    
    generate_test_data(buffer, data_size, 0x52);
    padding_init(&ctx, PADDING_RANDOM, 64, max_padding);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        padded_len = data_size;
        memcpy(buffer, buffer, data_size);
        padding_add(&ctx, buffer, &padded_len, data_size + max_padding + 64);
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = data_size * iterations;
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    padding_cleanup(&ctx);
    free(buffer);
    
    return result;
}

/* ============================================
 * Комбинированные тесты
 * ============================================ */

/**
 * @brief Бенчмарк: fragmentation + padding вместе
 */
static benchmark_result_t benchmark_combined(size_t message_size,
                                             size_t fragment_size,
                                             size_t block_size,
                                             int iterations)
{
    benchmark_result_t result = {0};
    result.test_name = "Combined Fragmentation + Padding";
    
    struct fragmentation_ctx frag_ctx;
    struct padding_ctx pad_ctx;
    unsigned char *message = malloc(message_size);
    unsigned char *buffer = malloc(message_size * 2);
    struct tls_fragment *fragments = malloc(sizeof(struct tls_fragment) * FRAGMENT_MAX_COUNT);
    int fragment_count = 0;
    
    generate_test_data(message, message_size, 0x60);
    fragmentation_init(&frag_ctx, FRAGMENT_FIXED, fragment_size);
    padding_init(&pad_ctx, PADDING_FIXED, block_size, 0);
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < iterations; i++) {
        // Fragment
        fragmentation_fragment_message(&frag_ctx, message, message_size,
                                       fragments, FRAGMENT_MAX_COUNT, &fragment_count);
        
        // Apply padding to each fragment
        for (int f = 0; f < fragment_count; f++) {
            memcpy(buffer, fragments[f].data, fragments[f].len);
            size_t len = fragments[f].len;
            padding_add(&pad_ctx, buffer, &len, sizeof(buffer));
        }
    }
    
    uint64_t end = get_time_us();
    
    result.operations = iterations;
    result.duration_us = end - start;
    result.ops_per_sec = (double)iterations * 1000000.0 / result.duration_us;
    result.bytes_processed = message_size * iterations;
    result.bytes_per_sec = (double)result.bytes_processed * 1000000.0 / result.duration_us;
    
    fragmentation_cleanup(&frag_ctx);
    padding_cleanup(&pad_ctx);
    free(message);
    free(buffer);
    free(fragments);
    
    return result;
}

/* ============================================
 * Вывод результатов
 * ============================================ */

static void print_result(benchmark_result_t *r) {
    printf("%-35s | %10lu ops | %8.2f ms | %12.0f ops/s | %10.2f MB/s\n",
           r->test_name,
           (unsigned long)r->operations,
           r->duration_us / 1000.0,
           r->ops_per_sec,
           r->bytes_per_sec / (1024.0 * 1024.0));
}

static void print_header(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("  Fragmentation & Padding Benchmark Results\n");
    printf("================================================================================\n");
    printf("%-35s | %10s | %8s | %12s | %10s\n", 
           "Test", "Operations", "Time", "Ops/Sec", "Throughput");
    printf("================================================================================\n");
}

/* ============================================
 * Main
 * ============================================ */

int main(int argc, char *argv[]) {
    printf("\n=== Fragmentation & Padding Benchmark ===\n");
    printf("Testing TLS fragmentation and padding performance\n\n");
    
    int iterations = 10000;  // Количество итераций для каждого теста
    
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    
    print_header();
    
    // Fragmentation тесты
    benchmark_result_t r1 = benchmark_fragmentation_fixed(1024, 128, iterations);
    print_result(&r1);
    
    benchmark_result_t r2 = benchmark_fragmentation_assemble(1024, 128, iterations);
    print_result(&r2);
    
    benchmark_result_t r3 = benchmark_fragmentation_random(1024, iterations);
    print_result(&r3);
    
    // Padding тесты
    benchmark_result_t r4 = benchmark_padding_fixed(512, 64, iterations);
    print_result(&r4);
    
    benchmark_result_t r5 = benchmark_padding_roundtrip(512, 64, iterations);
    print_result(&r5);
    
    benchmark_result_t r6 = benchmark_padding_random(512, 128, iterations);
    print_result(&r6);
    
    // Комбинированный тест
    benchmark_result_t r7 = benchmark_combined(1024, 128, 64, iterations);
    print_result(&r7);
    
    printf("================================================================================\n");
    printf("\nSummary:\n");
    printf("  Total tests: 7\n");
    printf("  Iterations per test: %d\n", iterations);
    printf("  Message sizes: 512, 1024 bytes\n");
    printf("  Fragment sizes: 128 bytes\n");
    printf("  Block sizes: 64 bytes\n");
    printf("\n");
    
    return 0;
}
