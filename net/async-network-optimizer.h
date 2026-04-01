/*
 * Async Network Optimizer Header for MTProxy
 * Provides asynchronous I/O operations with io_uring support
 */

#ifndef ASYNC_NETWORK_OPTIMIZER_H
#define ASYNC_NETWORK_OPTIMIZER_H

#include <stddef.h>

#ifdef _WIN32
    typedef int socklen_t;
    struct sockaddr { int sa_family; char sa_data[14]; };
#else
    #include <sys/socket.h>
#endif

// Async operation types
typedef enum {
    ASYNC_OP_READ = 0,
    ASYNC_OP_WRITE = 1,
    ASYNC_OP_ACCEPT = 2,
    ASYNC_OP_CONNECT = 3,
    ASYNC_OP_CLOSE = 4
} async_operation_type_t;

// Async operation status
typedef enum {
    ASYNC_STATUS_PENDING = 0,
    ASYNC_STATUS_COMPLETED = 1,
    ASYNC_STATUS_ERROR = 2,
    ASYNC_STATUS_CANCELLED = 3
} async_operation_status_t;

// Async operation structure
typedef struct async_operation {
    int id;
    async_operation_type_t type;
    async_operation_status_t status;
    int fd;
    void *buffer;
    size_t buffer_size;
    size_t bytes_transferred;
    int error_code;
    void *user_data;
    unsigned long long submit_time;
    unsigned long long complete_time;
    struct async_operation *next;
} async_operation_t;

// Async completion callback
typedef void (*async_completion_callback_t)(async_operation_t *op);

// Async I/O context
typedef struct {
    int max_operations;
    int current_operations;
    async_operation_t *pending_queue;
    async_operation_t *completed_queue;
    async_completion_callback_t default_callback;
    // Statistics
    long long total_submitted;
    long long total_completed;
    long long total_errors;
    long long total_cancelled;
    double avg_latency_us;
    long long peak_concurrent_ops;
} async_io_context_t;

// Async network configuration
typedef struct {
    int enable_async_io;
    int max_concurrent_operations;
    int completion_queue_size;
    int submission_queue_size;
    int enable_io_uring;
    int enable_epoll_fallback;
    size_t buffer_pool_size;
    int timeout_ms;
} async_net_config_t;

// Async network context
typedef struct {
    async_net_config_t config;
    async_io_context_t io_context;
    void *io_uring_instance; // In real implementation, this would be io_uring struct
    int epoll_fd;
    int is_initialized;
    int is_running;
    // Performance counters
    long long io_operations_submitted;
    long long io_operations_completed;
    long long io_operations_failed;
    double avg_io_latency_us;
} async_network_t;

// Function declarations
async_network_t* async_network_init(async_net_config_t *config);
int async_submit_read(async_network_t *network, int fd, void *buffer, 
                     size_t size, void *user_data);
int async_submit_write(async_network_t *network, int fd, const void *buffer, 
                      size_t size, void *user_data);
int async_submit_accept(async_network_t *network, int listen_fd, 
                       struct sockaddr *addr, socklen_t *addrlen, void *user_data);
int async_poll_completed(async_network_t *network, int timeout_ms);
async_operation_t* async_get_completed(async_network_t *network);
int async_cancel_operation(async_network_t *network, int operation_id);
void async_get_stats(async_network_t *network, long long *submitted, 
                    long long *completed, long long *errors, double *avg_latency);
void async_network_cleanup(async_network_t *network);
int init_global_async_network(void);

// Convenience macros
#define ASYNC_READ(fd, buf, size, user_data) \
    async_submit_read(g_async_network, fd, buf, size, user_data)
    
#define ASYNC_WRITE(fd, buf, size, user_data) \
    async_submit_write(g_async_network, fd, buf, size, user_data)
    
#define ASYNC_ACCEPT(listen_fd, addr, addrlen, user_data) \
    async_submit_accept(g_async_network, listen_fd, addr, addrlen, user_data)

#endif // ASYNC_NETWORK_OPTIMIZER_H