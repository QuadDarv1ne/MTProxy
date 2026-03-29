/*
 * test_io_uring.c — Тесты для io_uring модуля
 *
 * Тестирование:
 * - io_uring_is_available (проверка доступности)
 * - io_uring_init/_cleanup (инициализация)
 * - io_uring_submit_* (отправка операций)
 * - io_uring_get_stats (статистика)
 *
 * Требования: Linux kernel 5.1+, liburing-dev
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "net/io_uring.h"

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
 * io_uring_is_available Tests
 * ============================================================================ */

TEST(is_available_basic) {
#ifdef __linux__
    int available = io_uring_is_available();
    
    /* На Linux может быть 0 или 1 в зависимости от ядра */
    if (available < 0 || available > 1) {
        printf("Expected 0 or 1, got %d", available);
        return -1;
    }
    
    printf("(available=%d) ", available);
#else
    /* На не-Linux платформах всегда 0 */
    int available = io_uring_is_available();
    if (available != 0) {
        printf("Expected 0 on non-Linux, got %d", available);
        return -1;
    }
    printf("(non-Linux) ");
#endif
    
    return 0;
}

/* ============================================================================
 * io_uring_init/cleanup Tests
 * ============================================================================ */

TEST(init_null_ctx) {
    int ret = io_uring_init(NULL, 4096, 0);
    
    if (ret == 0) {
        printf("Expected error for NULL ctx");
        return -1;
    }
    
    printf("(ret=%d) ", ret);
    return 0;
}

TEST(init_invalid_depth) {
#ifdef __linux__
    io_uring_ctx_t ctx;
    int ret = io_uring_init(&ctx, 0, 0);
    
    /* Должна быть ошибка при depth=0 */
    if (ret == 0) {
        printf("Expected error for depth=0");
        return -1;
    }
    
    printf("(ret=%d) ", ret);
#else
    /* На не-Linux всегда ошибка */
    io_uring_ctx_t ctx;
    int ret = io_uring_init(&ctx, 4096, 0);
    if (ret == 0) {
        printf("Expected error on non-Linux");
        return -1;
    }
    printf("(non-Linux) ");
#endif
    
    return 0;
}

TEST(init_and_cleanup) {
#ifdef __linux__
    if (!io_uring_is_available()) {
        printf("(skipped, not available) ");
        return 0;
    }
    
    io_uring_ctx_t ctx;
    int ret = io_uring_init(&ctx, 256, 0);
    
    if (ret != 0) {
        printf("init failed: ret=%d", ret);
        return -1;
    }
    
    /* Проверка флагов */
    if (!(ctx.flags & 0x01)) { /* IO_URING_F_INITIALIZED */
        printf("Not initialized flag");
        io_uring_cleanup(&ctx);
        return -1;
    }
    
    io_uring_cleanup(&ctx);
    
    /* После cleanup флаги должны быть сброшены */
    if (ctx.flags != 0) {
        printf("Flags not cleared after cleanup");
        return -1;
    }
    
    printf("(depth=256) ");
#else
    io_uring_ctx_t ctx;
    int ret = io_uring_init(&ctx, 256, 0);
    if (ret == 0) {
        printf("Expected error on non-Linux");
        return -1;
    }
    printf("(non-Linux) ");
#endif
    
    return 0;
}

/* ============================================================================
 * io_uring_submit_* Tests
 * ============================================================================ */

TEST(submit_read_null_ctx) {
    char buffer[1024];
    int ret = io_uring_submit_read(NULL, 0, buffer, sizeof(buffer), NULL);
    
    if (ret == 0) {
        printf("Expected error for NULL ctx");
        return -1;
    }
    
    printf("(ret=%d) ", ret);
    return 0;
}

TEST(submit_write_null_ctx) {
    char buffer[1024];
    int ret = io_uring_submit_write(NULL, 0, buffer, sizeof(buffer), NULL);
    
    if (ret == 0) {
        printf("Expected error for NULL ctx");
        return -1;
    }
    
    printf("(ret=%d) ", ret);
    return 0;
}

TEST(submit_read_null_buffer) {
#ifdef __linux__
    if (!io_uring_is_available()) {
        printf("(skipped, not available) ");
        return 0;
    }
    
    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 256, 0) != 0) {
        printf("init failed");
        return -1;
    }
    
    int ret = io_uring_submit_read(&ctx, 0, NULL, 1024, NULL);
    
    if (ret == 0) {
        printf("Expected error for NULL buffer");
        io_uring_cleanup(&ctx);
        return -1;
    }
    
    io_uring_cleanup(&ctx);
    printf("(ret=%d) ", ret);
#else
    int ret = io_uring_submit_read(NULL, 0, NULL, 1024, NULL);
    if (ret == 0) {
        printf("Expected error on non-Linux");
        return -1;
    }
    printf("(non-Linux) ");
#endif
    
    return 0;
}

TEST(submit_invalid_fd) {
#ifdef __linux__
    if (!io_uring_is_available()) {
        printf("(skipped, not available) ");
        return 0;
    }
    
    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 256, 0) != 0) {
        printf("init failed");
        return -1;
    }
    
    char buffer[1024];
    int ret = io_uring_submit_read(&ctx, -1, buffer, sizeof(buffer), NULL);
    
    /* Должна быть ошибка для invalid fd */
    if (ret == 0) {
        printf("Expected error for invalid fd");
        io_uring_cleanup(&ctx);
        return -1;
    }
    
    io_uring_cleanup(&ctx);
    printf("(ret=%d) ", ret);
#else
    char buffer[1024];
    int ret = io_uring_submit_read(NULL, -1, buffer, sizeof(buffer), NULL);
    if (ret == 0) {
        printf("Expected error on non-Linux");
        return -1;
    }
    printf("(non-Linux) ");
#endif
    
    return 0;
}

/* ============================================================================
 * io_uring_stats Tests
 * ============================================================================ */

TEST(get_stats_null_ctx) {
    io_uring_stats_t stats;
    io_uring_get_stats(NULL, &stats);
    
    /* Должно быть no-op для NULL ctx */
    printf("(no-op) ");
    return 0;
}

TEST(get_stats_null_stats) {
#ifdef __linux__
    if (!io_uring_is_available()) {
        printf("(skipped, not available) ");
        return 0;
    }
    
    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 256, 0) != 0) {
        printf("init failed");
        return -1;
    }
    
    io_uring_get_stats(&ctx, NULL);
    
    io_uring_cleanup(&ctx);
    printf("(no-op) ");
#else
    io_uring_get_stats(NULL, NULL);
    printf("(non-Linux) ");
#endif
    
    return 0;
}

TEST(reset_stats) {
#ifdef __linux__
    if (!io_uring_is_available()) {
        printf("(skipped, not available) ");
        return 0;
    }
    
    io_uring_ctx_t ctx;
    if (io_uring_init(&ctx, 256, 0) != 0) {
        printf("init failed");
        return -1;
    }
    
    io_uring_reset_stats(&ctx);
    
    /* После reset статистика должна быть нулевой */
    if (ctx.submitted != 0 || ctx.completed != 0) {
        printf("Stats not reset");
        io_uring_cleanup(&ctx);
        return -1;
    }
    
    io_uring_cleanup(&ctx);
    printf("(reset ok) ");
#else
    io_uring_ctx_t ctx;
    io_uring_reset_stats(&ctx);
    printf("(non-Linux) ");
#endif
    
    return 0;
}

/* ============================================================================
 * io_uring_queue_usage Tests
 * ============================================================================ */

TEST(get_queue_usage_null_ctx) {
    int usage = io_uring_get_queue_usage(NULL);
    
    if (usage != 0) {
        printf("Expected 0 for NULL ctx, got %d", usage);
        return -1;
    }
    
    printf("(usage=%d) ", usage);
    return 0;
}

/* ============================================================================
 * io_uring_enable/disable_connection Tests
 * ============================================================================ */

TEST(enable_connection_null_ctx) {
    int ret = io_uring_enable_connection(NULL, 0);
    
    if (ret == 0) {
        printf("Expected error for NULL ctx");
        return -1;
    }
    
    printf("(ret=%d) ", ret);
    return 0;
}

TEST(disable_connection_null_ctx) {
    /* Должно быть no-op */
    io_uring_disable_connection(NULL, 0);
    printf("(no-op) ");
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("=== io_uring Module Tests ===\n\n");
    
    /* io_uring_is_available Tests */
    printf("--- io_uring_is_available ---\n");
    RUN_TEST(is_available_basic);
    printf("\n");
    
    /* io_uring_init/cleanup Tests */
    printf("--- io_uring_init/cleanup ---\n");
    RUN_TEST(init_null_ctx);
    RUN_TEST(init_invalid_depth);
    RUN_TEST(init_and_cleanup);
    printf("\n");
    
    /* io_uring_submit_* Tests */
    printf("--- io_uring_submit_* ---\n");
    RUN_TEST(submit_read_null_ctx);
    RUN_TEST(submit_write_null_ctx);
    RUN_TEST(submit_read_null_buffer);
    RUN_TEST(submit_invalid_fd);
    printf("\n");
    
    /* io_uring_stats Tests */
    printf("--- io_uring_stats ---\n");
    RUN_TEST(get_stats_null_ctx);
    RUN_TEST(get_stats_null_stats);
    RUN_TEST(reset_stats);
    printf("\n");
    
    /* io_uring_queue_usage Tests */
    printf("--- io_uring_queue_usage ---\n");
    RUN_TEST(get_queue_usage_null_ctx);
    printf("\n");
    
    /* io_uring_enable/disable_connection Tests */
    printf("--- io_uring_enable/disable_connection ---\n");
    RUN_TEST(enable_connection_null_ctx);
    RUN_TEST(disable_connection_null_ctx);
    printf("\n");
    
    /* Summary */
    printf("=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
