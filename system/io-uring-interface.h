/*
 * Интерфейс io_uring для MTProxy
 * Поддержка асинхронного ввода-вывода для повышения производительности
 */

#ifndef _IO_URING_INTERFACE_H_
#define _IO_URING_INTERFACE_H_

#include <stdint.h>

// Типы операций io_uring
typedef enum {
    IO_URING_OP_READ = 0,
    IO_URING_OP_WRITE = 1,
    IO_URING_OP_ACCEPT = 2,
    IO_URING_OP_CONNECT = 3,
    IO_URING_OP_CLOSE = 4,
    IO_URING_OP_POLL_ADD = 5,
    IO_URING_OP_POLL_REMOVE = 6
} io_uring_operation_t;

// Статус операции
typedef enum {
    IO_URING_STATUS_PENDING = 0,
    IO_URING_STATUS_COMPLETED = 1,
    IO_URING_STATUS_ERROR = 2,
    IO_URING_STATUS_CANCELLED = 3
} io_uring_status_t;

// Статистика io_uring
typedef struct {
    long long total_operations;
    long long completed_operations;
    long long failed_operations;
    long long cancelled_operations;
    long long pending_operations;
    long long submission_queue_size;
    long long completion_queue_size;
} io_uring_stats_t;

// Конфигурация io_uring
typedef struct {
    int enable_io_uring;
    int submission_queue_entries;
    int completion_queue_entries;
    int enable_polling;
    int enable_single_issuer;
    int enable_defer_taskrun;
    int sq_thread_cpu;
    int sq_thread_idle;
} io_uring_config_t;

// Контекст io_uring
typedef struct {
    io_uring_config_t config;
    io_uring_stats_t stats;
    int ring_fd;
    int initialized;
    void *sq_ptr;  // Submission queue pointer
    void *cq_ptr;  // Completion queue pointer
    int sq_entries;
    int cq_entries;
} io_uring_context_t;

// Структура для операции
typedef struct {
    io_uring_operation_t operation;
    io_uring_status_t status;
    int fd;
    void *buffer;
    size_t buffer_size;
    size_t bytes_transferred;
    int result;
    long long user_data;
    int flags;
} io_uring_operation_entry_t;

// Функции инициализации
int io_uring_init(io_uring_context_t *ctx);
int io_uring_init_with_config(io_uring_context_t *ctx, const io_uring_config_t *config);
void io_uring_cleanup(io_uring_context_t *ctx);

// Функции работы с операциями
int io_uring_submit_read(io_uring_context_t *ctx, int fd, void *buffer, size_t size, long long user_data);
int io_uring_submit_write(io_uring_context_t *ctx, int fd, const void *buffer, size_t size, long long user_data);
// int io_uring_submit_accept(io_uring_context_t *ctx, int listen_fd, struct sockaddr *addr, socklen_t *addrlen, long long user_data);
// int io_uring_submit_connect(io_uring_context_t *ctx, int fd, const struct sockaddr *addr, socklen_t addrlen, long long user_data);
int io_uring_submit_close(io_uring_context_t *ctx, int fd, long long user_data);

// Функции управления очередями
int io_uring_submit_queue(io_uring_context_t *ctx);
int io_uring_wait_completion(io_uring_context_t *ctx, io_uring_operation_entry_t *completed_ops, int max_ops, int timeout_ms);
int io_uring_peek_completion(io_uring_context_t *ctx, io_uring_operation_entry_t *completed_ops, int max_ops);

// Функции статистики
io_uring_stats_t io_uring_get_stats(io_uring_context_t *ctx);
void io_uring_reset_stats(io_uring_context_t *ctx);

// Функции конфигурации
void io_uring_get_config(io_uring_context_t *ctx, io_uring_config_t *config);
int io_uring_update_config(io_uring_context_t *ctx, const io_uring_config_t *new_config);

// Вспомогательные функции
int io_uring_is_available(void);
int io_uring_get_ring_fd(io_uring_context_t *ctx);
int io_uring_cancel_operation(io_uring_context_t *ctx, long long user_data);

#endif