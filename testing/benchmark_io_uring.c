/*
 * benchmark_io_uring.c — Бенчмарки производительности io_uring
 *
 * Сравнение:
 * - традиционный epoll (Linux)
 * - io_uring (Linux kernel 5.1+)
 *
 * Тесты:
 * - submit/completion производительность
 * - read/write операции
 * - многопоточный тест
 * - zero-copy операции
 *
 * Требования: Linux kernel 5.1+, liburing-dev
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#endif

#include "net/io_uring.h"

#define BENCH_ITERATIONS 10000
#define THREAD_COUNT 4
#define BUFFER_SIZE 4096

/* Статистика */
typedef struct {
    double submit_ops_per_sec;
    double complete_ops_per_sec;
    double read_ops_per_sec;
    double write_ops_per_sec;
    double total_time_ms;
    uint64_t total_bytes;
    uint64_t zero_copy_ops;
} benchmark_stats_t;

static benchmark_stats_t g_stats = {0};

#ifdef __linux__

/* Тест 1: Производительность submit операций */
static void benchmark_submit_operations(void) {
    if (!io_uring_is_available()) {
        printf("SKIP: io_uring not available\n");
        return;
    }

    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 4096, 0) != 0) {
        printf("FAIL: io_uring_init failed\n");
        return;
    }

    clock_t start = clock();

    /* Создаём pipe для тестирования */
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        io_uring_cleanup(&ctx);
        return;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', sizeof(buffer));

    /* Submit read операций */
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        io_uring_submit_read(&ctx, pipefd[0], buffer, sizeof(buffer), (void *)(uintptr_t)i);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    g_stats.submit_ops_per_sec = BENCH_ITERATIONS / elapsed * 1000;

    printf("Test 1: submit_operations (%d iterations)\n", BENCH_ITERATIONS);
    printf("  Submit:   %.2f ms (%.0f ops/sec)\n", elapsed, g_stats.submit_ops_per_sec);

    close(pipefd[0]);
    close(pipefd[1]);
    io_uring_cleanup(&ctx);
}

/* Тест 2: Производительность completion */
static void benchmark_completion_operations(void) {
    if (!io_uring_is_available()) {
        printf("SKIP: io_uring not available\n");
        return;
    }

    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 4096, 0) != 0) {
        printf("FAIL: io_uring_init failed\n");
        return;
    }

    /* Создаём pipe */
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        io_uring_cleanup(&ctx);
        return;
    }

    char buffer[BUFFER_SIZE];

    /* Submit нескольких операций */
    int submitted = 0;
    for (int i = 0; i < 100; i++) {
        if (io_uring_submit_read(&ctx, pipefd[0], buffer, sizeof(buffer), NULL) == 0) {
            submitted++;
        }
    }

    /* Flush */
    io_uring_flush(&ctx, 0);

    clock_t start = clock();

    /* Wait completions */
    io_uring_cqe_t completions[100];
    int completed = io_uring_wait_completions(&ctx, submitted, completions, 100);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    if (elapsed > 0) {
        g_stats.complete_ops_per_sec = completed / elapsed * 1000;
    }

    printf("\nTest 2: completion_operations (%d submitted, %d completed)\n", submitted, completed);
    printf("  Complete: %.2f ms (%.0f ops/sec)\n", elapsed, g_stats.complete_ops_per_sec);

    close(pipefd[0]);
    close(pipefd[1]);
    io_uring_cleanup(&ctx);
}

/* Тест 3: Read/Write производительность */
static void benchmark_read_write_operations(void) {
    if (!io_uring_is_available()) {
        printf("SKIP: io_uring not available\n");
        return;
    }

    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 4096, 0) != 0) {
        printf("FAIL: io_uring_init failed\n");
        return;
    }

    /* Создаём pipe */
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        io_uring_cleanup(&ctx);
        return;
    }

    char write_buffer[BUFFER_SIZE];
    char read_buffer[BUFFER_SIZE];
    memset(write_buffer, 'X', sizeof(write_buffer));

    clock_t start = clock();

    /* Записываем данные в pipe */
    write(pipefd[1], write_buffer, sizeof(write_buffer));

    /* Submit read */
    io_uring_submit_read(&ctx, pipefd[0], read_buffer, sizeof(read_buffer), NULL);
    io_uring_flush(&ctx, 1);

    /* Wait completion */
    io_uring_cqe_t completions[1];
    int completed = io_uring_wait_completions(&ctx, 1, completions, 1000);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    if (completed > 0 && completions[0].result > 0) {
        g_stats.read_ops_per_sec = 1 / elapsed * 1000;
        g_stats.total_bytes = completions[0].result;
    }

    printf("\nTest 3: read_write_operations\n");
    printf("  Read:     %.2f ms (%.0f ops/sec)\n", elapsed, g_stats.read_ops_per_sec);
    printf("  Bytes:    %lu\n", (unsigned long)g_stats.total_bytes);

    close(pipefd[0]);
    close(pipefd[1]);
    io_uring_cleanup(&ctx);
}

/* Тест 4: Многопоточный тест */
typedef struct {
    int thread_id;
    int iterations;
    double ops_per_sec;
} thread_bench_data_t;

static void* thread_bench_worker(void *arg) {
    thread_bench_data_t *data = (thread_bench_data_t *)arg;

    if (!io_uring_is_available()) {
        return NULL;
    }

    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 1024, 0) != 0) {
        return NULL;
    }

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        io_uring_cleanup(&ctx);
        return NULL;
    }

    char buffer[256];

    clock_t start = clock();

    for (int i = 0; i < data->iterations; i++) {
        io_uring_submit_read(&ctx, pipefd[0], buffer, sizeof(buffer), NULL);
        io_uring_flush(&ctx, 0);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    if (elapsed > 0) {
        data->ops_per_sec = data->iterations / elapsed * 1000;
    }

    close(pipefd[0]);
    close(pipefd[1]);
    io_uring_cleanup(&ctx);

    return NULL;
}

static void benchmark_multithreaded(void) {
    if (!io_uring_is_available()) {
        printf("SKIP: io_uring not available\n");
        return;
    }

    printf("\nTest 4: multithreaded (%d threads, %d iterations each)\n",
           THREAD_COUNT, BENCH_ITERATIONS / THREAD_COUNT);

    pthread_t threads[THREAD_COUNT];
    thread_bench_data_t thread_data[THREAD_COUNT];
    double total_ops_per_sec = 0;

    clock_t start = clock();

    /* Запуск потоков */
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = BENCH_ITERATIONS / THREAD_COUNT;
        thread_data[i].ops_per_sec = 0;

        pthread_create(&threads[i], NULL, thread_bench_worker, &thread_data[i]);
    }

    /* Ожидание завершения */
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
        total_ops_per_sec += thread_data[i].ops_per_sec;
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    printf("  Total:    %.2f ms\n", elapsed);
    printf("  Avg:      %.0f ops/sec/thread\n", total_ops_per_sec / THREAD_COUNT);
    printf("  Combined: %.0f ops/sec\n", total_ops_per_sec);
}

/* Тест 5: Статистика и мониторинг */
static void benchmark_statistics(void) {
    if (!io_uring_is_available()) {
        printf("SKIP: io_uring not available\n");
        return;
    }

    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 4096, 0) != 0) {
        printf("FAIL: io_uring_init failed\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        io_uring_cleanup(&ctx);
        return;
    }

    char buffer[BUFFER_SIZE];

    /* Выполняем несколько операций */
    for (int i = 0; i < 100; i++) {
        io_uring_submit_read(&ctx, pipefd[0], buffer, sizeof(buffer), NULL);
    }

    io_uring_flush(&ctx, 0);

    /* Получаем статистику */
    io_uring_stats_t stats;
    io_uring_get_stats(&ctx, &stats);

    printf("\nTest 5: statistics\n");
    printf("  Submissions:   %lu\n", (unsigned long)stats.submissions);
    printf("  Completions:   %lu\n", (unsigned long)stats.completions);
    printf("  Bytes read:    %lu\n", (unsigned long)stats.bytes_read);
    printf("  Bytes written: %lu\n", (unsigned long)stats.bytes_written);
    printf("  Zero-copy ops: %lu\n", (unsigned long)stats.zero_copy_ops);

    /* Тест queue usage */
    int usage = io_uring_get_queue_usage(&ctx);
    printf("  Queue usage:   %d%%\n", usage);

    close(pipefd[0]);
    close(pipefd[1]);
    io_uring_cleanup(&ctx);
}

#else /* !__linux__ */

static void benchmark_submit_operations(void) {
    printf("SKIP: io_uring only available on Linux\n");
}

static void benchmark_completion_operations(void) {
    printf("SKIP: io_uring only available on Linux\n");
}

static void benchmark_read_write_operations(void) {
    printf("SKIP: io_uring only available on Linux\n");
}

static void benchmark_multithreaded(void) {
    printf("SKIP: io_uring only available on Linux\n");
}

static void benchmark_statistics(void) {
    printf("SKIP: io_uring only available on Linux\n");
}

#endif /* __linux__ */

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("=== io_uring Performance Benchmarks ===\n");
    printf("Platform: ");
#ifdef __linux__
    printf("Linux\n");
#else
    printf("Non-Linux (benchmarks skipped)\n");
#endif
    printf("\n");

    /* Запуск бенчмарков */
    benchmark_submit_operations();
    benchmark_completion_operations();
    benchmark_read_write_operations();
    benchmark_multithreaded();
    benchmark_statistics();

    /* Summary */
    printf("\n=== Benchmark Summary ===\n");
    printf("Submit ops/sec:    %.0f\n", g_stats.submit_ops_per_sec);
    printf("Complete ops/sec:  %.0f\n", g_stats.complete_ops_per_sec);
    printf("Read ops/sec:      %.0f\n", g_stats.read_ops_per_sec);
    printf("Total bytes:       %lu\n", (unsigned long)g_stats.total_bytes);

    return 0;
}
