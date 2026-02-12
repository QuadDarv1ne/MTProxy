/*
 * Zero-copy operations for MTProxy
 *
 * This file defines the interface for zero-copy networking operations
 * to reduce memory copying overhead and improve performance.
 */

#pragma once

#ifdef __linux__
    #include <sys/socket.h>
    #include <sys/uio.h>
    #include <linux/fs.h>
    #include <netinet/tcp.h>
    #include <unistd.h>
    #include <stddef.h>
#else
    #include <stddef.h>
#endif

#ifndef _SSIZE_T_DEFINED
    #ifdef _WIN32
        typedef ptrdiff_t ssize_t;
    #else
        #include <sys/types.h>
    #endif
    #define _SSIZE_T_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Zero-copy message context */
typedef struct {
    int fd;                    /* File descriptor */
    void *data_ptr;           /* Pointer to data */
    size_t data_len;          /* Length of data */
    int flags;               /* Flags for operation */
    struct iovec *iov;       /* Scatter/gather vector */
    int iov_count;           /* Number of iovec elements */
} zc_message_ctx_t;

/* Zero-copy engine structure */
typedef struct {
    int enabled;             /* Whether zero-copy is enabled */
    size_t threshold;        /* Minimum size for zero-copy */
    unsigned long flags;     /* Zero-copy flags (SO_ZEROCOPY) */
    long long successes;     /* Count of successful zero-copy ops */
    long long fallbacks;     /* Count of fallbacks to normal copy */
} zc_engine_t;

/* Initialize zero-copy engine */
int init_zero_copy_engine(zc_engine_t *engine, size_t threshold);

/* Send message using zero-copy if possible */
ssize_t zerocopy_send_message(int fd, const void *buf, size_t len, int flags);

/* Receive message using zero-copy if possible */
ssize_t zerocopy_receive_message(int fd, void *buf, size_t len, int flags);

/* Setup shared buffers for zero-copy operations */
int setup_shared_buffers(void);

/* Cleanup zero-copy resources */
void cleanup_zero_copy_resources(void);

/* Check if zero-copy is supported on this platform */
int is_zero_copy_supported(void);

/* Get zero-copy statistics */
void get_zc_statistics(long long *successes, long long *fallbacks);

#ifdef __cplusplus
}
#endif