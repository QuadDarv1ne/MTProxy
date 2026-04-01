/*
 * io_uring.c - io_uring support for MTProxy
 *
 * High-performance I/O using Linux io_uring interface.
 * Implementation based on liburing library.
 */

#include "io_uring.h"

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* liburing header */
#include <liburing.h>

/* Internal state */
static struct {
    int initialized;
    int available;
} g_io_uring_state = {0, -1};

/* Check if io_uring is available */
int io_uring_is_available(void) {
    if (g_io_uring_state.available >= 0) {
        return g_io_uring_state.available;
    }

    struct io_uring_probe *probe;
    probe = io_uring_get_probe();
    if (!probe) {
        g_io_uring_state.available = 0;
        return 0;
    }

    /* Check for required operations */
    if (!io_uring_opcode_supported(probe, IORING_OP_READV) ||
        !io_uring_opcode_supported(probe, IORING_OP_WRITEV)) {
        io_uring_free_probe(probe);
        g_io_uring_state.available = 0;
        return 0;
    }

    io_uring_free_probe(probe);
    g_io_uring_state.available = 1;
    return 1;
}

/* Initialize io_uring context */
int io_uring_init(io_uring_ctx_t *ctx, uint32_t queue_depth, uint32_t flags) {
    if (!ctx) {
        return -EINVAL;
    }

    if (!io_uring_is_available()) {
        return -EOPNOTSUPP;
    }

    memset(ctx, 0, sizeof(io_uring_ctx_t));

    struct io_uring *ring = malloc(sizeof(struct io_uring));
    if (!ring) {
        return -ENOMEM;
    }

    /* Create io_uring instance */
    int ret = io_uring_queue_init(queue_depth, ring, 0);
    if (ret < 0) {
        free(ring);
        return ret;
    }

    ctx->ring = ring;
    ctx->ring_fd = ring->ring_fd;
    ctx->queue_depth = queue_depth;
    ctx->flags = flags | IO_URING_F_INITIALIZED;
    ctx->submitted = 0;
    ctx->completed = 0;
    ctx->bytes_read = 0;
    ctx->bytes_written = 0;
    ctx->zero_copy_ops = 0;

    g_io_uring_state.initialized = 1;
    return 0;
}

/* Cleanup io_uring context */
void io_uring_cleanup(io_uring_ctx_t *ctx) {
    if (!ctx || !(ctx->flags & IO_URING_F_INITIALIZED)) {
        return;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    if (ring) {
        io_uring_queue_exit(ring);
        free(ring);
    }

    memset(ctx, 0, sizeof(io_uring_ctx_t));
}

/* Submit read operation */
int io_uring_submit_read(io_uring_ctx_t *ctx, int fd, void *buffer,
                         size_t len, void *user_data) {
    if (!ctx || !buffer || fd < 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    struct iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = len;

    io_uring_prep_readv(sqe, fd, &iov, 1, 0);
    io_uring_sqe_set_data(sqe, user_data);

    ctx->submitted++;
    return 0;
}

/* Submit write operation */
int io_uring_submit_write(io_uring_ctx_t *ctx, int fd, const void *buffer,
                          size_t len, void *user_data) {
    if (!ctx || !buffer || fd < 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    struct iovec iov;
    iov.iov_base = (void *)buffer;
    iov.iov_len = len;

    io_uring_prep_writev(sqe, fd, &iov, 1, 0);
    io_uring_sqe_set_data(sqe, user_data);

    ctx->submitted++;
    ctx->bytes_written += len;
    return 0;
}

/* Submit accept operation */
int io_uring_submit_accept(io_uring_ctx_t *ctx, int fd, void *user_data) {
    if (!ctx || fd < 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    if (!addr) {
        return -ENOMEM;
    }
    socklen_t *addrlen = malloc(sizeof(socklen_t));
    if (!addrlen) {
        free(addr);
        return -ENOMEM;
    }
    *addrlen = sizeof(struct sockaddr_in);

    io_uring_prep_accept(sqe, fd, (struct sockaddr *)addr, addrlen, 0);
    io_uring_sqe_set_data(sqe, user_data);

    /* Store addr/addrlen for cleanup in completion handler */
    /* This is simplified - real implementation needs better tracking */

    ctx->submitted++;
    return 0;
}

/* Submit connect operation */
int io_uring_submit_connect(io_uring_ctx_t *ctx, int fd, const void *addr,
                            socklen_t addrlen, void *user_data) {
    if (!ctx || !addr || fd < 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    io_uring_prep_connect(sqe, fd, (struct sockaddr *)addr, addrlen);
    io_uring_sqe_set_data(sqe, user_data);

    ctx->submitted++;
    return 0;
}

/* Submit poll add operation */
int io_uring_submit_poll_add(io_uring_ctx_t *ctx, int fd, short events,
                             void *user_data) {
    if (!ctx || fd < 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    io_uring_prep_poll_add(sqe, fd, events);
    io_uring_sqe_set_data(sqe, user_data);

    ctx->submitted++;
    return 0;
}

/* Submit timeout operation */
int io_uring_submit_timeout(io_uring_ctx_t *ctx, uint32_t timeout_ms,
                            void *user_data) {
    if (!ctx) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        return -ENOBUFS;
    }

    struct __kernel_timespec ts;
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1000000;

    io_uring_prep_timeout(sqe, &ts, 0, 0);
    io_uring_sqe_set_data(sqe, user_data);

    ctx->submitted++;
    return 0;
}

/* Flush submission queue */
int io_uring_flush(io_uring_ctx_t *ctx, int wait) {
    if (!ctx || !(ctx->flags & IO_URING_F_INITIALIZED)) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    return io_uring_submit(ring);
}

/* Wait for completions */
int io_uring_wait_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions, int timeout_ms) {
    if (!ctx || !completions || count <= 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_cqe *cqe;
    int i = 0;

    unsigned head;
    unsigned available = io_uring_cq_ready(ring);
    int to_fetch = (count > (int)available) ? count - available : 0;

    if (to_fetch > 0 && timeout_ms != 0) {
        struct __kernel_timespec ts;
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        
        int ret = io_uring_wait_cqe_timeout(ring, &cqe, &ts);
        if (ret < 0 && ret != -ETIME) {
            return ret;
        }
        available = io_uring_cq_ready(ring);
    }

    io_uring_for_each_cqe(ring, head, cqe) {
        if (i >= count) break;

        completions[i].result = cqe->res;
        completions[i].flags = cqe->flags;
        completions[i].user_data = (void *)(uintptr_t)cqe->user_data;

        ctx->completed++;
        if (cqe->res > 0) {
            ctx->bytes_read += cqe->res;
        }

        i++;
    }

    return i;
}

/* Peek completions */
int io_uring_peek_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions) {
    if (!ctx || !completions || count <= 0) {
        return -EINVAL;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    struct io_uring_cqe *cqe;
    int i = 0;

    unsigned head;
    unsigned available = io_uring_cq_ready(ring);

    io_uring_for_each_cqe(ring, head, cqe) {
        if (i >= count) break;

        completions[i].result = cqe->res;
        completions[i].flags = cqe->flags;
        completions[i].user_data = (void *)(uintptr_t)cqe->user_data;

        i++;
    }

    return i;
}

/* Mark completions as seen */
void io_uring_seen(io_uring_ctx_t *ctx, int count) {
    if (!ctx || count <= 0) {
        return;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    io_uring_cq_advance(ring, count);
}

/* Get statistics */
void io_uring_get_stats(io_uring_ctx_t *ctx, io_uring_stats_t *stats) {
    if (!ctx || !stats) {
        return;
    }

    memset(stats, 0, sizeof(io_uring_stats_t));
    stats->submissions = ctx->submitted;
    stats->completions = ctx->completed;
    stats->bytes_read = ctx->bytes_read;
    stats->bytes_written = ctx->bytes_written;
    stats->zero_copy_ops = ctx->zero_copy_ops;
}

/* Reset statistics */
void io_uring_reset_stats(io_uring_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    ctx->submitted = 0;
    ctx->completed = 0;
    ctx->bytes_read = 0;
    ctx->bytes_written = 0;
    ctx->zero_copy_ops = 0;
}

/* Get queue usage percentage */
int io_uring_get_queue_usage(io_uring_ctx_t *ctx) {
    if (!ctx || !(ctx->flags & IO_URING_F_INITIALIZED)) {
        return 0;
    }

    struct io_uring *ring = (struct io_uring *)ctx->ring;
    unsigned sq_head = *ring->sq.khead;
    unsigned sq_tail = *ring->sq.ktail;
    unsigned sq_entries = ring->sq.ring_entries;

    unsigned used = sq_tail - sq_head;
    return (used * 100) / sq_entries;
}

/* Enable io_uring for connection */
int io_uring_enable_connection(io_uring_ctx_t *ctx, int fd) {
    if (!ctx || fd < 0) {
        return -EINVAL;
    }

    /* Set socket to non-blocking mode */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -errno;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -errno;
    }

    ctx->flags |= IO_URING_F_ENABLED;
    return 0;
}

/* Disable io_uring for connection */
void io_uring_disable_connection(io_uring_ctx_t *ctx, int fd) {
    if (!ctx || fd < 0) {
        return;
    }

    /* Reset to blocking mode if needed */
    ctx->flags &= ~IO_URING_F_ENABLED;
}

#else /* !__linux__ */

/* Stub implementations for non-Linux platforms */

int io_uring_is_available(void) {
    return 0;
}

int io_uring_init(io_uring_ctx_t *ctx, uint32_t queue_depth, uint32_t flags) {
    (void)ctx;
    (void)queue_depth;
    (void)flags;
    return -EOPNOTSUPP;
}

void io_uring_cleanup(io_uring_ctx_t *ctx) {
    (void)ctx;
}

int io_uring_submit_read(io_uring_ctx_t *ctx, int fd, void *buffer,
                         size_t len, void *user_data) {
    (void)ctx;
    (void)fd;
    (void)buffer;
    (void)len;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_submit_write(io_uring_ctx_t *ctx, int fd, const void *buffer,
                          size_t len, void *user_data) {
    (void)ctx;
    (void)fd;
    (void)buffer;
    (void)len;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_submit_accept(io_uring_ctx_t *ctx, int fd, void *user_data) {
    (void)ctx;
    (void)fd;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_submit_connect(io_uring_ctx_t *ctx, int fd, const void *addr,
                            socklen_t addrlen, void *user_data) {
    (void)ctx;
    (void)fd;
    (void)addr;
    (void)addrlen;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_submit_poll_add(io_uring_ctx_t *ctx, int fd, short events,
                             void *user_data) {
    (void)ctx;
    (void)fd;
    (void)events;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_submit_timeout(io_uring_ctx_t *ctx, uint32_t timeout_ms,
                            void *user_data) {
    (void)ctx;
    (void)timeout_ms;
    (void)user_data;
    return -EOPNOTSUPP;
}

int io_uring_flush(io_uring_ctx_t *ctx, int wait) {
    (void)ctx;
    (void)wait;
    return -EOPNOTSUPP;
}

int io_uring_wait_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions, int timeout_ms) {
    (void)ctx;
    (void)count;
    (void)completions;
    (void)timeout_ms;
    return -EOPNOTSUPP;
}

int io_uring_peek_completions(io_uring_ctx_t *ctx, int count,
                              io_uring_cqe_t *completions) {
    (void)ctx;
    (void)count;
    (void)completions;
    return -EOPNOTSUPP;
}

void io_uring_seen(io_uring_ctx_t *ctx, int count) {
    (void)ctx;
    (void)count;
}

void io_uring_get_stats(io_uring_ctx_t *ctx, io_uring_stats_t *stats) {
    (void)ctx;
    (void)stats;
}

void io_uring_reset_stats(io_uring_ctx_t *ctx) {
    (void)ctx;
}

int io_uring_get_queue_usage(io_uring_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

int io_uring_enable_connection(io_uring_ctx_t *ctx, int fd) {
    (void)ctx;
    (void)fd;
    return -EOPNOTSUPP;
}

void io_uring_disable_connection(io_uring_ctx_t *ctx, int fd) {
    (void)ctx;
    (void)fd;
}

#endif /* __linux__ */
