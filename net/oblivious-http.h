/*
 * Oblivious HTTP протокол для анонимизации трафика в MTProxy
 * Реализация протокола Oblivious HTTP для скрытия паттернов запросов
 */

#ifndef _OBLIVIOUS_HTTP_H_
#define _OBLIVIOUS_HTTP_H_

#include <stdint.h>

// Состояния Oblivious HTTP
typedef enum {
    OHTTP_STATUS_UNINITIALIZED = 0,
    OHTTP_STATUS_INITIALIZED = 1,
    OHTTP_STATUS_ACTIVE = 2,
    OHTTP_STATUS_ERROR = 3
} oblivious_http_status_t;

// Типы Oblivious HTTP запросов
typedef enum {
    OHTTP_REQUEST_GET = 0,
    OHTTP_REQUEST_POST = 1,
    OHTTP_REQUEST_PUT = 2,
    OHTTP_REQUEST_DELETE = 3,
    OHTTP_REQUEST_OBLIVIOUS = 4  // Специальный тип для oblivious запросов
} oblivious_http_request_type_t;

// Версии Oblivious HTTP
typedef enum {
    OHTTP_VERSION_1_0 = 1,
    OHTTP_VERSION_1_1 = 2,
    OHTTP_VERSION_2_0 = 3
} oblivious_http_version_t;

// Статистика Oblivious HTTP
typedef struct {
    long long total_requests;
    long long oblivious_requests;
    long long anonymized_bytes;
    long long successful_requests;
    long long failed_requests;
    oblivious_http_status_t current_status;
    double anonymity_effectiveness;
    int active_connections;
    long long last_activity_time;
} oblivious_http_stats_t;

// Конфигурация Oblivious HTTP
typedef struct {
    int enable_oblivious_http;
    int enable_anonymization;
    int max_request_size;
    int max_response_size;
    int enable_padding;
    int min_padding_size;
    int max_padding_size;
    int enable_multiplexing;
    int max_concurrent_streams;
    int connection_timeout_ms;
    int enable_encryption;
    int encryption_algorithm;
    int enable_request_batching;
    int batch_size_limit;
    int batch_timeout_ms;
} oblivious_http_config_t;

// Контекст Oblivious HTTP
typedef struct {
    oblivious_http_config_t config;
    oblivious_http_stats_t stats;
    oblivious_http_status_t status;
    void *request_processor;
    void *response_handler;
    void *encryption_engine;
    void *padding_generator;
    int initialized;
    long long last_request_time;
    int active_streams;
    int max_streams;
    void *stream_table;
    void *cache_manager;
} oblivious_http_context_t;

// Структура для Oblivious HTTP запроса
typedef struct {
    oblivious_http_request_type_t req_type;
    char *uri;
    char *host;
    char *headers;
    void *body;
    size_t body_size;
    int stream_id;
    int is_oblivious;
    void *padding_data;
    size_t padding_size;
    long long timestamp;
    int priority;
} oblivious_http_request_t;

// Структура для Oblivious HTTP ответа
typedef struct {
    int status_code;
    char *headers;
    void *body;
    size_t body_size;
    void *padding_data;
    size_t padding_size;
    int stream_id;
    long long timestamp;
} oblivious_http_response_t;

// Функции инициализации
int ohttp_init(oblivious_http_context_t *ctx);
int ohttp_init_with_config(oblivious_http_context_t *ctx, 
                          const oblivious_http_config_t *config);
void ohttp_cleanup(oblivious_http_context_t *ctx);

// Функции обработки запросов
int ohttp_process_request(oblivious_http_context_t *ctx, 
                         const oblivious_http_request_t *req,
                         oblivious_http_response_t *resp);
int ohttp_create_oblivious_request(const char *uri, 
                                  oblivious_http_request_t *req);
int ohttp_send_request(oblivious_http_context_t *ctx, 
                      const oblivious_http_request_t *req);
int ohttp_receive_response(oblivious_http_context_t *ctx, 
                          oblivious_http_response_t *resp);

// Функции анонимизации
int ohttp_add_padding(oblivious_http_request_t *req, size_t min_size, size_t max_size);
int ohttp_mask_traffic_patterns(oblivious_http_context_t *ctx);
int ohttp_batch_requests(oblivious_http_context_t *ctx, 
                        oblivious_http_request_t *requests, 
                        int count);

// Функции шифрования
int ohttp_encrypt_request(oblivious_http_request_t *req);
int ohttp_decrypt_response(oblivious_http_response_t *resp);

// Функции мультиплексирования
int ohttp_create_stream(oblivious_http_context_t *ctx, int *stream_id);
int ohttp_close_stream(oblivious_http_context_t *ctx, int stream_id);
int ohttp_multiplex_request(oblivious_http_context_t *ctx, 
                           const oblivious_http_request_t *req);

// Функции статистики
oblivious_http_stats_t ohttp_get_stats(oblivious_http_context_t *ctx);
void ohttp_reset_stats(oblivious_http_context_t *ctx);

// Функции конфигурации
void ohttp_get_config(oblivious_http_context_t *ctx, 
                     oblivious_http_config_t *config);
int ohttp_update_config(oblivious_http_context_t *ctx, 
                       const oblivious_http_config_t *new_config);

// Вспомогательные функции
int ohttp_is_available(void);
double ohttp_get_anonymity_effectiveness(oblivious_http_context_t *ctx);
int ohttp_validate_request(const oblivious_http_request_t *req);
int ohttp_generate_padding(size_t min_size, size_t max_size, void **padding_data, size_t *padding_size);

#endif