/*
    Упрощенная система современных сетевых технологий
    Без зависимостей от стандартной библиотеки
*/

#ifndef SIMPLE_ADVANCED_NETWORK_H
#define SIMPLE_ADVANCED_NETWORK_H

// Базовые типы
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long size_t;
typedef long long off_t;

// Конфигурация
#define MAX_NET_ENTRIES 2048
#define MAX_BATCH_SIZE 64

// Типы
typedef enum {
    IO_BACKEND_SELECT = 0,
    IO_BACKEND_EPOLL = 1,
    IO_BACKEND_IO_URING = 2
} io_backend_type_t;

typedef enum {
    NET_STATUS_IDLE = 0,
    NET_STATUS_ACTIVE = 1,
    NET_STATUS_ERROR = 2
} net_status_t;

// Структуры
typedef struct simple_net_buffer {
    char *data;
    size_t capacity;
    size_t length;
    size_t read_pos;
    size_t write_pos;
    int ref_count;
} simple_net_buffer_t;

typedef struct simple_io_request {
    uint64_t request_id;
    int fd;
    int operation;
    net_status_t status;
    void *buffer;
    size_t length;
    void *user_data;
    int result;
} simple_io_request_t;

typedef struct simple_adv_network {
    // Конфигурация
    io_backend_type_t backend;
    int enable_zero_copy;
    int enable_batch;
    int batch_size;
    
    // Статус
    net_status_t status;
    int is_initialized;
    
    // Статистика
    uint64_t total_operations;
    uint64_t completed_operations;
    uint64_t zero_copy_ops;
    double avg_latency_ms;
    
    // Буферы
    simple_net_buffer_t *buffers;
    int buffer_count;
} simple_adv_network_t;

// API
simple_adv_network_t* simple_net_init(io_backend_type_t backend);
int simple_net_configure(simple_adv_network_t *net, int enable_zc, int batch_size);
void simple_net_cleanup(simple_adv_network_t *net);

// Операции
int simple_net_read(simple_adv_network_t *net, int fd, void *buf, size_t len);
int simple_net_write(simple_adv_network_t *net, int fd, const void *buf, size_t len);
int simple_net_recv(simple_adv_network_t *net, int fd, void *buf, size_t len);
int simple_net_send(simple_adv_network_t *net, int fd, const void *buf, size_t len);

// Zero-copy
int simple_net_zc_read(simple_adv_network_t *net, int fd, simple_net_buffer_t *buf, size_t len);
int simple_net_zc_write(simple_adv_network_t *net, int fd, simple_net_buffer_t *buf, size_t len);

// Batch операции
int simple_net_submit_batch(simple_adv_network_t *net, simple_io_request_t **reqs, int count);
int simple_net_process_completions(simple_adv_network_t *net, int max_count);

// Утилиты
int simple_net_detect_backend(void);
int simple_net_supports_zero_copy(void);
uint64_t simple_net_get_request_id(void);

// Статистика
void simple_net_get_stats(simple_adv_network_t *net, char *buffer, size_t buffer_size);
void simple_net_reset_stats(simple_adv_network_t *net);

// Буферы
simple_net_buffer_t* simple_net_create_buffer(size_t capacity);
void simple_net_destroy_buffer(simple_net_buffer_t *buf);
int simple_net_buffer_write(simple_net_buffer_t *buf, const void *data, size_t len);
int simple_net_buffer_read(simple_net_buffer_t *buf, void *data, size_t len);

#endif // SIMPLE_ADVANCED_NETWORK_H