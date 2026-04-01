/*
    Модульная система безопасности для MTProxy
    Содержит защиту от переполнения буфера, DDoS и валидацию данных
*/

#ifndef MODULAR_SECURITY_H
#define MODULAR_SECURITY_H

#include <stdint.h>
#include <stddef.h>

// Определение time_t для совместимости
#ifndef _TIME_T_DEFINED
typedef long long time_t;
#define _TIME_T_DEFINED
#endif

// Конфигурация безопасности
#define MAX_BUFFER_SIZE 65536
#define DEFAULT_RATE_LIMIT 1000  // запросов в секунду
#define BURST_LIMIT 5000         // максимальный burst
#define CONNECTION_TIMEOUT 300   // секунд
#define MAX_CONCURRENT_CONNECTIONS 10000

// Типы атак для детектирования
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

// Структура для отслеживания клиента
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

// Структура конфигурации безопасности
typedef struct security_config {
    int rate_limit;
    int burst_limit;
    int connection_timeout;
    int max_connections;
    int buffer_overflow_protection;
    int protocol_validation;
    int logging_level;
    char *whitelist_file;
    char *blacklist_file;
} security_config_t;

// Основная структура безопасности
typedef struct modular_security {
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
} modular_security_t;

// Инициализация системы безопасности
modular_security_t* security_init(security_config_t *config);

// Проверка безопасности данных
security_status_t security_validate_buffer(const void *buffer, size_t size, size_t max_size);
security_status_t security_validate_input(const char *data, size_t length);
security_status_t security_check_rate_limit(uint32_t client_ip);
security_status_t security_validate_connection(uint32_t client_ip);

// Защита от переполнения буфера
int security_safe_memcpy(void *dest, const void *src, size_t n, size_t dest_size);
int security_safe_strncpy(char *dest, const char *src, size_t n);
int security_validate_string(const char *str, size_t max_length);

// DDoS защита
int security_detect_flood_attack(uint32_t client_ip, int request_count);
int security_block_ip_temporarily(uint32_t ip, int duration_seconds);
int security_unblock_ip(uint32_t ip);
int security_is_ip_blocked(uint32_t ip);

// Валидация протокола
int security_validate_mtproto_header(const void *header, size_t header_size);
int security_detect_malformed_packets(const void *data, size_t size);
int security_validate_packet_sequence(uint32_t client_ip, uint32_t sequence);

// Управление списками
int security_load_whitelist(const char *filename);
int security_load_blacklist(const char *filename);
int security_add_to_whitelist(uint32_t ip);
int security_add_to_blacklist(uint32_t ip, const char *reason);

// Мониторинг и статистика
void security_get_stats(modular_security_t *sec, char *buffer, size_t buffer_size);
void security_reset_stats(modular_security_t *sec);
int security_get_active_connections(void);

// Очистка ресурсов
void security_cleanup(modular_security_t *sec);
void security_cleanup_client_trackers(modular_security_t *sec);

// Вспомогательные функции
uint32_t security_ip_to_uint32(const char *ip_str);
const char* security_uint32_to_ip(uint32_t ip);
const char* security_status_to_string(security_status_t status);
const char* security_attack_type_to_string(attack_type_t attack);

// Логирование безопасности
void security_log_event(attack_type_t attack, uint32_t client_ip, const char *details);
void security_log_blocked_request(uint32_t client_ip, const char *reason);
void security_log_violation(uint32_t client_ip, const char *violation_type);

#endif // MODULAR_SECURITY_H