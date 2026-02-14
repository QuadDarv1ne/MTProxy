/*
    Упрощенная система безопасности для MTProxy
    Без зависимостей от стандартной библиотеки C
*/

#ifndef SIMPLE_SECURITY_H
#define SIMPLE_SECURITY_H

// Базовые типы
typedef unsigned int uint32_t;
typedef unsigned long long size_t;
typedef long long time_t;

// Конфигурация безопасности
#define MAX_BUFFER_SIZE 65536
#define DEFAULT_RATE_LIMIT 1000
#define BURST_LIMIT 5000
#define CONNECTION_TIMEOUT 300
#define MAX_CONCURRENT_CONNECTIONS 10000

// Типы атак
typedef enum {
    ATTACK_TYPE_NONE = 0,
    ATTACK_TYPE_FLOOD,
    ATTACK_TYPE_BUFFER_OVERFLOW,
    ATTACK_TYPE_INVALID_PROTOCOL,
    ATTACK_TYPE_RATE_LIMIT_EXCEEDED,
    ATTACK_TYPE_SUSPICIOUS_PATTERN
} attack_type_t;

// Статус безопасности
typedef enum {
    SECURITY_STATUS_OK = 0,
    SECURITY_STATUS_WARNING,
    SECURITY_STATUS_BLOCKED,
    SECURITY_STATUS_RATE_LIMITED
} security_status_t;

// Трекер клиента
typedef struct client_tracker {
    uint32_t ip_address;
    long long last_activity;
    int request_count;
    int connection_count;
    int violation_count;
    long long rate_limit_reset;
    security_status_t status;
    struct client_tracker *next;
} client_tracker_t;

// Конфигурация
typedef struct security_config {
    int rate_limit;
    int burst_limit;
    int connection_timeout;
    int max_connections;
    int buffer_overflow_protection;
    int protocol_validation;
    int logging_level;
} security_config_t;

// Основная структура
typedef struct simple_security {
    security_config_t config;
    client_tracker_t *client_list;
    int total_blocked;
    int total_violations;
    long long last_cleanup;
    
    // Статистика
    long long buffer_overflow_attempts;
    long long ddos_attempts;
    long long invalid_requests;
    long long rate_limit_violations;
} simple_security_t;

// Функции API
simple_security_t* simple_security_init(security_config_t *config);
void simple_security_cleanup(simple_security_t *sec);

// Проверки безопасности
security_status_t simple_security_check_buffer(const void *buffer, size_t size, size_t max_size);
security_status_t simple_security_check_input(const char *data, size_t length);
security_status_t simple_security_check_rate_limit(uint32_t client_ip);
security_status_t simple_security_check_connection(uint32_t client_ip);

// Защита от переполнения
int simple_security_safe_memcpy(void *dest, const void *src, size_t n, size_t dest_size);
int simple_security_safe_strncpy(char *dest, const char *src, size_t n);

// DDoS защита
int simple_security_detect_flood(uint32_t client_ip, int request_count);
int simple_security_block_ip(uint32_t ip);
int simple_security_unblock_ip(uint32_t ip);
int simple_security_is_blocked(uint32_t ip);

// Валидация протокола
int simple_security_validate_mtproto(const void *header, size_t header_size);
int simple_security_check_packet(const void *data, size_t size);

// Утилиты
uint32_t simple_security_ip_to_uint32(const char *ip_str);
const char* simple_security_uint32_to_ip(uint32_t ip);
const char* simple_security_status_string(security_status_t status);

// Статистика
void simple_security_get_stats(simple_security_t *sec, char *buffer, size_t buffer_size);
void simple_security_reset_stats(simple_security_t *sec);

#endif // SIMPLE_SECURITY_H