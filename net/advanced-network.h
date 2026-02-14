/*
    Система современных сетевых технологий MTProxy
    io_uring, zero-copy, epoll - все в одном модуле
*/

#ifndef ADVANCED_NETWORK_H
#define ADVANCED_NETWORK_H

#include <stdint.h>
#include <stddef.h>

// Базовые типы для совместимости
#ifndef _OFF_T_DEFINED
typedef long long off_t;
#define _OFF_T_DEFINED
#endif

// Конфигурация io_uring
#define MAX_IO_URING_ENTRIES 4096
#define MAX_BATCH_SUBMIT 128
#define IO_URING_QUEUE_DEPTH 1024

// Типы I/O операций
typedef enum {
    IO_OP_READ = 0,
    IO_OP_WRITE = 1,
    IO_OP_RECV = 2,
    IO_OP_SEND = 3,
    IO_OP_ACCEPT = 4,
    IO_OP_CONNECT = 5,
    IO_OP_CLOSE = 6,
    IO_OP_SPLICE = 7
} io_operation_t;

// Статус операции
typedef enum {
    IO_STATUS_PENDING = 0,
    IO_STATUS_IN_PROGRESS = 1,
    IO_STATUS_COMPLETED = 2,
    IO_STATUS_ERROR = 3,
    IO_STATUS_CANCELLED = 4
} io_status_t;

// Тип бэкенда
typedef enum {
    IO_BACKEND_EPOLL = 0,
    IO_BACKEND_IO_URING = 1,
    IO_BACKEND_KQUEUE = 2,
    IO_BACKEND_AUTO = 3
} io_backend_t;

// Zero-copy буфер
typedef struct zc_buffer {
    void *buffer;
    size_t length;
    int fd;
    off_t offset;
    int is_mapped;
} zc_buffer_t;

// I/O запрос
typedef struct io_request {
    uint64_t request_id;
    int fd;
    io_operation_t operation;
    io_status_t status;
    void *buffer;
    size_t length;
    size_t offset;
    void *user_data;
    int result;
    void (*callback)(struct io_request *req);
} io_request_t;

// io_uring контекст
typedef struct io_uring_context {
    int ring_fd;
    void *submission_queue;
    void *completion_queue;
    unsigned int sq_head;
    unsigned int sq_tail;
    unsigned int cq_head;
    unsigned int cq_tail;
    unsigned int ring_mask;
    io_request_t *requests;
    int request_count;
    int is_initialized;
} io_uring_context_t;

// Оптимизированный event loop
typedef struct advanced_event_loop {
    // Бэкенд
    io_backend_t backend;
    int epoll_fd;
    io_uring_context_t *uring;
    
    // Мониторинг
    int registered_fds[MAX_IO_URING_ENTRIES];
    int fd_count;
    
    // Статистика
    long long total_io_operations;
    long long completed_operations;
    long long failed_operations;
    long long zero_copy_operations;
    double avg_latency_us;
    
    // Конфигурация
    int enable_zero_copy;
    int enable_kernel_bypass;
    int batch_size;
    
    // Статус
    int is_running;
    int is_initialized;
} advanced_event_loop_t;

// Network buffer optimization
typedef struct net_buffer_opt {
    char *data;
    size_t capacity;
    size_t length;
    size_t read_pos;
    size_t write_pos;
    int ref_count;
    int is_zerocopy;
} net_buffer_opt_t;

// API функции

// Инициализация
advanced_event_loop_t* adv_net_init(io_backend_t backend);
int adv_net_configure(advanced_event_loop_t *loop, int enable_zc, int batch_size);
void adv_net_cleanup(advanced_event_loop_t *loop);

// Регистрация файловых дескрипторов
int adv_net_register_fd(advanced_event_loop_t *loop, int fd, void *user_data);
int adv_net_unregister_fd(advanced_event_loop_t *loop, int fd);

// I/O операции
int adv_net_submit_read(advanced_event_loop_t *loop, int fd, void *buf, size_t len, off_t offset, void *user_data);
int adv_net_submit_write(advanced_event_loop_t *loop, int fd, const void *buf, size_t len, off_t offset, void *user_data);
int adv_net_submit_recv(advanced_event_loop_t *loop, int fd, void *buf, size_t len, void *user_data);
int adv_net_submit_send(advanced_event_loop_t *loop, int fd, const void *buf, size_t len, void *user_data);

// Zero-copy операции
int adv_net_submit_zc_read(advanced_event_loop_t *loop, int fd, zc_buffer_t *buf, size_t len, off_t offset, void *user_data);
int adv_net_submit_zc_write(advanced_event_loop_t *loop, int fd, zc_buffer_t *buf, size_t len, off_t offset, void *user_data);
int adv_net_register_zc_buffer(advanced_event_loop_t *loop, zc_buffer_t *buf);

// Event loop
int adv_net_poll(advanced_event_loop_t *loop, int timeout_ms);
int adv_net_process_completions(advanced_event_loop_t *loop, int max_events);

// Утилиты
int adv_net_detect_backend(void);
int adv_net_supports_zero_copy(void);
int adv_net_supports_io_uring(void);
uint64_t adv_net_get_request_id(void);

// Статистика
void adv_net_get_stats(advanced_event_loop_t *loop, char *buffer, size_t buffer_size);
void adv_net_reset_stats(advanced_event_loop_t *loop);

// Batch операции
int adv_net_submit_batch(advanced_event_loop_t *loop, io_request_t **requests, int count);
int adv_net_process_batch(advanced_event_loop_t *loop, int max_completions);

#endif // ADVANCED_NETWORK_H