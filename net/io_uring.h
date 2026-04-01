/*
 * io_uring.h - io_uring support for MTProxy
 *
 * High-performance I/O using Linux io_uring interface.
 * Provides zero-copy operations and async I/O for improved performance.
 *
 * Requirements:
 * - Linux kernel 5.1+
 * - liburing development files
 */

#ifndef IO_URING_H
#define IO_URING_H

#ifdef __linux__

#include <stdint.h>
#include <stddef.h>

/* io_uring configuration */
#define IO_URING_QUEUE_DEPTH 4096
#define IO_URING_MAX_BATCH 128

/* io_uring context flags */
#define IO_URING_F_NONE         0x00
#define IO_URING_F_INITIALIZED  0x01
#define IO_URING_F_ENABLED      0x02
#define IO_URING_F_POLL_MODE    0x04

/* io_uring operation types */
typedef enum {
    IO_URING_OP_READ = 0,
    IO_URING_OP_WRITE,
    IO_URING_OP_ACCEPT,
    IO_URING_OP_CONNECT,
    IO_URING_OP_SENDMSG,
    IO_URING_OP_RECVMSG,
    IO_URING_OP_CLOSE,
    IO_URING_OP_TIMEOUT,
    IO_URING_OP_POLL_ADD,
    IO_URING_OP_POLL_REMOVE
} io_uring_op_t;

/* io_uring completion result */
typedef struct {
    int result;
    uint32_t flags;
    void *user_data;
} io_uring_cqe_t;

/* io_uring submission entry */
typedef struct {
    io_uring_op_t op;
    int fd;
    void *buffer;
    size_t len;
    uint64_t offset;
    void *user_data;
    uint32_t flags;
} io_uring_sqe_t;

/* io_uring context */
typedef struct {
    void *ring;                    /* io_uring ring buffer */
    int ring_fd;                   /* io_uring file descriptor */
    uint32_t queue_depth;          /* submission queue depth */
    uint32_t flags;                /* context flags */
    uint64_t submitted;            /* total submitted operations */
    uint64_t completed;            /* total completed operations */
    uint64_t bytes_read;           /* total bytes read */
    uint64_t bytes_written;        /* total bytes written */
    uint64_t zero_copy_ops;        /* zero-copy operations count */
} io_uring_ctx_t;

/* io_uring statistics */
typedef struct {
    uint64_t submissions;
    uint64_t completions;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t zero_copy_ops;
    uint64_t poll_ops;
    uint64_t timeouts;
    uint64_t errors;
} io_uring_stats_t;

/* ============================================================================
 * Initialization and cleanup
 * ============================================================================ */

/**
 * Initialize io_uring context
 * @param ctx io_uring context to initialize
 * @param queue_depth submission queue depth (recommended: 4096)
 * @param flags context flags (IO_URING_F_*)
 * @return 0 on success, negative error code on failure
 */
int io_uring_init(io_uring_ctx_t *ctx, uint32_t queue_depth, uint32_t flags);

/**
 * Cleanup io_uring context
 * @param ctx io_uring context to cleanup
 */
void io_uring_cleanup(io_uring_ctx_t *ctx);

/**
 * Check if io_uring is available on current system
 * @return 1 if available, 0 if not
 */
int io_uring_is_available(void);

/* ============================================================================
 * Submission operations
 * ============================================================================ */

/**
 * Submit read operation
 * @param ctx io_uring context
 * @param fd file descriptor
 * @param buffer destination buffer
 * @param len bytes to read
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_read(io_uring_ctx_t *ctx, int fd, void *buffer, 
                         size_t len, void *user_data);

/**
 * Submit write operation
 * @param ctx io_uring context
 * @param fd file descriptor
 * @param buffer source buffer
 * @param len bytes to write
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_write(io_uring_ctx_t *ctx, int fd, const void *buffer,
                          size_t len, void *user_data);

/**
 * Submit accept operation
 * @param ctx io_uring context
 * @param fd listening socket fd
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_accept(io_uring_ctx_t *ctx, int fd, void *user_data);

/**
 * Submit connect operation
 * @param ctx io_uring context
 * @param fd socket fd
 * @param addr socket address
 * @param addrlen address length
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_connect(io_uring_ctx_t *ctx, int fd, const void *addr,
                            socklen_t addrlen, void *user_data);

/**
 * Submit poll add operation
 * @param ctx io_uring context
 * @param fd file descriptor to poll
 * @param events poll events (POLLIN, POLLOUT, etc.)
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_poll_add(io_uring_ctx_t *ctx, int fd, short events,
                             void *user_data);

/**
 * Submit timeout operation
 * @param ctx io_uring context
 * @param timeout_ms timeout in milliseconds
 * @param user_data user-defined data for completion
 * @return 0 on success, negative error code on failure
 */
int io_uring_submit_timeout(io_uring_ctx_t *ctx, uint32_t timeout_ms,
                            void *user_data);

/**
 * Flush submission queue to kernel
 * @param ctx io_uring context
 * @param wait wait for completion if non-zero
 * @return number of submitted operations, or negative error code
 */
int io_uring_flush(io_uring_ctx_t *ctx, int wait);

/* ============================================================================
 * Completion operations
 * ============================================================================ */

/**
 * Wait for completions
 * @param ctx io_uring context
 * @param count max completions to retrieve
 * @param completions output array for completions
 * @param timeout_ms timeout in milliseconds (-1 for infinite)
 * @return number of completions, or negative error code
 */
int io_uring_wait_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions, int timeout_ms);

/**
 * Peek completions without removing them
 * @param ctx io_uring context
 * @param count max completions to peek
 * @param completions output array for completions
 * @return number of available completions
 */
int io_uring_peek_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions);

/**
 * Mark completions as seen (remove from queue)
 * @param ctx io_uring context
 * @param count number of completions to mark
 */
void io_uring_seen(io_uring_ctx_t *ctx, int count);

/* ============================================================================
 * Statistics and monitoring
 * ============================================================================ */

/**
 * Get io_uring statistics
 * @param ctx io_uring context
 * @param stats output statistics structure
 */
void io_uring_get_stats(io_uring_ctx_t *ctx, io_uring_stats_t *stats);

/**
 * Reset io_uring statistics
 * @param ctx io_uring context
 */
void io_uring_reset_stats(io_uring_ctx_t *ctx);

/**
 * Get submission queue usage
 * @param ctx io_uring context
 * @return percentage of queue usage (0-100)
 */
int io_uring_get_queue_usage(io_uring_ctx_t *ctx);

/**
 * Enable io_uring for connection
 * @param ctx io_uring context
 * @param fd file descriptor
 * @return 0 on success, negative error code on failure
 */
int io_uring_enable_connection(io_uring_ctx_t *ctx, int fd);

/**
 * Disable io_uring for connection
 * @param ctx io_uring context
 * @param fd file descriptor
 */
void io_uring_disable_connection(io_uring_ctx_t *ctx, int fd);

#endif /* __linux__ */

#endif /* IO_URING_H */
