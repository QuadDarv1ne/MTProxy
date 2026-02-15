/*
 * Zero-Copy Buffer Optimizer Header for MTProxy
 * Provides zero-copy operations with memory mapping and shared buffers
 */

#ifndef ZERO_COPY_OPTIMIZER_H
#define ZERO_COPY_OPTIMIZER_H

#include <stddef.h>

// Zero-copy buffer types
typedef enum {
    ZC_BUFFER_REGULAR = 0,    // Regular mapped buffer
    ZC_BUFFER_SHARED = 1,     // Shared between processes
    ZC_BUFFER_RING = 2,       // Ring buffer for streaming
    ZC_BUFFER_PACKET = 3      // Packet-oriented buffer
} zc_buffer_type_t;

// Zero-copy buffer flags
#define ZC_FLAG_READ_ONLY     0x01
#define ZC_FLAG_WRITE_ONLY    0x02
#define ZC_FLAG_READ_WRITE    0x03
#define ZC_FLAG_USER_MAPPED   0x04
#define ZC_FLAG_KERNEL_MAPPED 0x08
#define ZC_FLAG_PINNED        0x10

// Zero-copy buffer structure
typedef struct zc_buffer {
    void *virtual_addr;       // Virtual address of buffer
    size_t physical_addr;     // Physical address (for DMA)
    size_t size;              // Size of buffer
    size_t used_size;         // Currently used size
    zc_buffer_type_t type;    // Type of buffer
    unsigned int flags;       // Buffer flags
    int fd;                   // Associated file descriptor
    void *user_data;          // User-defined data
    // Metadata
    unsigned long long creation_time;
    unsigned long long last_access;
    int ref_count;
    int is_locked;
    struct zc_buffer *next;
} zc_buffer_t;

// Zero-copy ring buffer structure
typedef struct zc_ring_buffer {
    zc_buffer_t *buffers;     // Array of buffers
    int buffer_count;         // Number of buffers
    int read_index;           // Read position
    int write_index;          // Write position
    int available_count;      // Available buffers
    int occupied_count;       // Occupied buffers
    size_t buffer_size;       // Size of each buffer
    int is_blocking;          // Blocking mode
} zc_ring_buffer_t;

// Zero-copy operation types
typedef enum {
    ZC_OP_SEND = 0,
    ZC_OP_RECV = 1,
    ZC_OP_COPY = 2,
    ZC_OP_TRANSFER = 3
} zc_operation_type_t;

// Zero-copy operation context
typedef struct zc_operation {
    zc_operation_type_t type;
    zc_buffer_t *buffer;
    int fd;                   // Socket or file descriptor
    size_t offset;            // Offset in buffer
    size_t length;            // Length of operation
    int flags;                // Operation flags
    void *user_data;          // User data
    unsigned long long start_time;
    unsigned long long complete_time;
    int result;               // Operation result
} zc_operation_t;

// Zero-copy manager configuration
typedef struct {
    int enable_zero_copy;
    size_t min_zc_size;       // Minimum size for zero-copy
    size_t max_zc_size;       // Maximum size for zero-copy
    int max_concurrent_ops;   // Maximum concurrent operations
    int enable_kernel_bypass; // Enable kernel bypass
    int enable_shared_buffers; // Enable shared buffers
    size_t shared_pool_size;  // Size of shared buffer pool
    int enable_dma;           // Enable DMA support
    int enable_pin_memory;    // Enable memory pinning
} zc_config_t;

// Zero-copy manager context
typedef struct {
    zc_config_t config;
    zc_buffer_t *free_list;
    zc_buffer_t *active_list;
    zc_ring_buffer_t *ring_buffers;
    int ring_buffer_count;
    // Statistics
    long long total_zc_operations;
    long long total_regular_copies;
    long long saved_copy_ops;
    long long saved_bytes;
    long long pinned_memory_ops;
    long long unpinned_memory_ops;
    double efficiency_ratio;
    // Status
    int is_initialized;
    int is_active;
} zc_manager_t;

// Function declarations
zc_manager_t* zc_manager_init(zc_config_t *config);
zc_buffer_t* zc_create_buffer(size_t size, zc_buffer_type_t type, unsigned int flags);
zc_ring_buffer_t* zc_create_ring_buffer(int buffer_count, size_t buffer_size, int is_blocking);
int zc_pin_memory(void *addr, size_t size);
int zc_unpin_memory(void *addr, size_t size);
int zc_send_buffer(int sockfd, zc_buffer_t *buffer, size_t offset, size_t length);
int zc_receive_buffer(int sockfd, zc_buffer_t *buffer, size_t offset, size_t length);
zc_buffer_t* zc_ring_acquire_write(zc_ring_buffer_t *ring);
int zc_ring_release_write(zc_ring_buffer_t *ring, zc_buffer_t *buffer);
zc_buffer_t* zc_ring_acquire_read(zc_ring_buffer_t *ring);
int zc_ring_release_read(zc_ring_buffer_t *ring, zc_buffer_t *buffer);
void zc_get_stats(long long *total_zc_ops, long long *total_reg_copies, 
                 long long *saved_bytes, double *efficiency);
void zc_destroy_buffer(zc_buffer_t *buffer);
void zc_destroy_ring_buffer(zc_ring_buffer_t *ring);
void zc_manager_cleanup(zc_manager_t *manager);
int init_global_zc_manager(void);

// Convenience macros
#define ZC_CREATE_BUFFER(size, type) zc_create_buffer(size, type, ZC_FLAG_READ_WRITE)
#define ZC_SEND(sockfd, buffer, offset, length) zc_send_buffer(sockfd, buffer, offset, length)
#define ZC_RECV(sockfd, buffer, offset, length) zc_receive_buffer(sockfd, buffer, offset, length)

#endif // ZERO_COPY_OPTIMIZER_H