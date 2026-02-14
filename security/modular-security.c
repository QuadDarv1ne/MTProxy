/*
    Реализация модульной системы безопасности для MTProxy
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "modular-security.h"

// Глобальная переменная для системы безопасности
static modular_security_t *g_security = NULL;

// Вспомогательные функции
static client_tracker_t* find_client_tracker(modular_security_t *sec, uint32_t ip);
static client_tracker_t* create_client_tracker(uint32_t ip);
static void cleanup_expired_trackers(modular_security_t *sec);
static int is_suspicious_pattern(const void *data, size_t size);

// Инициализация системы безопасности
modular_security_t* security_init(security_config_t *config) {
    modular_security_t *sec = calloc(1, sizeof(modular_security_t));
    if (!sec) {
        return NULL;
    }
    
    // Установка конфигурации по умолчанию
    sec->config.rate_limit = config ? config->rate_limit : DEFAULT_RATE_LIMIT;
    sec->config.burst_limit = config ? config->burst_limit : BURST_LIMIT;
    sec->config.connection_timeout = config ? config->connection_timeout : CONNECTION_TIMEOUT;
    sec->config.max_connections = config ? config->max_connections : MAX_CONCURRENT_CONNECTIONS;
    sec->config.buffer_overflow_protection = config ? config->buffer_overflow_protection : 1;
    sec->config.protocol_validation = config ? config->protocol_validation : 1;
    sec->config.logging_level = config ? config->logging_level : 1;
    
    if (config && config->whitelist_file) {
        sec->config.whitelist_file = strdup(config->whitelist_file);
    }
    if (config && config->blacklist_file) {
        sec->config.blacklist_file = strdup(config->blacklist_file);
    }
    
    sec->last_cleanup = 0;
    g_security = sec;
    
    // Загрузка списков если указаны
    if (sec->config.whitelist_file) {
        security_load_whitelist(sec->config.whitelist_file);
    }
    if (sec->config.blacklist_file) {
        security_load_blacklist(sec->config.blacklist_file);
    }
    
    return sec;
}

// Проверка безопасности буфера
security_status_t security_validate_buffer(const void *buffer, size_t size, size_t max_size) {
    if (!g_security) return SECURITY_STATUS_OK;
    
    if (!buffer) {
        return SECURITY_STATUS_BLOCKED;
    }
    
    if (size > max_size) {
        g_security->buffer_overflow_attempts++;
        security_log_event(ATTACK_TYPE_BUFFER_OVERFLOW, 0, "Buffer size exceeds maximum allowed");
        return SECURITY_STATUS_BLOCKED;
    }
    
    // Проверка на подозрительные паттерны
    if (is_suspicious_pattern(buffer, size)) {
        g_security->invalid_requests++;
        security_log_event(ATTACK_TYPE_INVALID_PROTOCOL, 0, "Suspicious data pattern detected");
        return SECURITY_STATUS_WARNING;
    }
    
    return SECURITY_STATUS_OK;
}

// Проверка входных данных
security_status_t security_validate_input(const char *data, size_t length) {
    if (!g_security) return SECURITY_STATUS_OK;
    
    if (!data || length == 0) {
        return SECURITY_STATUS_BLOCKED;
    }
    
    // Проверка длины
    if (length > MAX_BUFFER_SIZE) {
        g_security->buffer_overflow_attempts++;
        return SECURITY_STATUS_BLOCKED;
    }
    
    // Проверка на недопустимые символы
    for (size_t i = 0; i < length; i++) {
        if ((unsigned char)data[i] > 127) {
            g_security->invalid_requests++;
            security_log_event(ATTACK_TYPE_INVALID_PROTOCOL, 0, "Non-ASCII character detected");
            return SECURITY_STATUS_WARNING;
        }
    }
    
    return SECURITY_STATUS_OK;
}

// Проверка rate limiting
security_status_t security_check_rate_limit(uint32_t client_ip) {
    if (!g_security) return SECURITY_STATUS_OK;
    
    client_tracker_t *tracker = find_client_tracker(g_security, client_ip);
    if (!tracker) {
        tracker = create_client_tracker(client_ip);
        if (!tracker) return SECURITY_STATUS_OK;
        tracker->next = g_security->client_list;
        g_security->client_list = tracker;
    }
    
    long long current_time = 0; // В реальной реализации использовать gettimeofday
    
    // Сброс счетчика если прошла секунда
    if (current_time - tracker->rate_limit_reset >= 1) {
        tracker->request_count = 0;
        tracker->rate_limit_reset = current_time;
    }
    
    tracker->request_count++;
    tracker->last_activity = current_time;
    
    // Проверка rate limit
    if (tracker->request_count > g_security->config.rate_limit) {
        if (tracker->request_count > g_security->config.burst_limit) {
            tracker->status = SECURITY_STATUS_BLOCKED;
            g_security->rate_limit_violations++;
            security_log_event(ATTACK_TYPE_RATE_LIMIT_EXCEEDED, client_ip, "Rate limit exceeded");
            return SECURITY_STATUS_BLOCKED;
        }
        tracker->status = SECURITY_STATUS_RATE_LIMITED;
        return SECURITY_STATUS_RATE_LIMITED;
    }
    
    tracker->status = SECURITY_STATUS_OK;
    return SECURITY_STATUS_OK;
}

// Проверка соединения
security_status_t security_validate_connection(uint32_t client_ip) {
    if (!g_security) return SECURITY_STATUS_OK;
    
    client_tracker_t *tracker = find_client_tracker(g_security, client_ip);
    if (!tracker) {
        tracker = create_client_tracker(client_ip);
        if (!tracker) return SECURITY_STATUS_OK;
        tracker->next = g_security->client_list;
        g_security->client_list = tracker;
    }
    
    // Проверка максимального количества соединений
    if (tracker->connection_count >= g_security->config.max_connections) {
        g_security->ddos_attempts++;
        security_log_event(ATTACK_TYPE_FLOOD, client_ip, "Too many concurrent connections");
        return SECURITY_STATUS_BLOCKED;
    }
    
    tracker->connection_count++;
    tracker->last_activity = 0; // current_time
    
    return SECURITY_STATUS_OK;
}

// Безопасное копирование памяти
int security_safe_memcpy(void *dest, const void *src, size_t n, size_t dest_size) {
    if (!dest || !src || n > dest_size) {
        if (g_security) {
            g_security->buffer_overflow_attempts++;
        }
        return -1;
    }
    
    memcpy(dest, src, n);
    return 0;
}

// Безопасное копирование строк
int security_safe_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) {
        return -1;
    }
    
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
    return 0;
}

// Валидация строк
int security_validate_string(const char *str, size_t max_length) {
    if (!str) return -1;
    
    size_t len = strlen(str);
    if (len >= max_length) {
        return -1;
    }
    
    // Проверка на недопустимые символы
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)str[i] < 32 && str[i] != '\n' && str[i] != '\r' && str[i] != '\t') {
            return -1;
        }
    }
    
    return 0;
}

// Детектирование flood атак
int security_detect_flood_attack(uint32_t client_ip, int request_count) {
    if (!g_security) return 0;
    
    if (request_count > g_security->config.burst_limit) {
        g_security->ddos_attempts++;
        security_log_event(ATTACK_TYPE_FLOOD, client_ip, "Flood attack detected");
        return 1;
    }
    
    return 0;
}

// Временная блокировка IP
int security_block_ip_temporarily(uint32_t ip, int duration_seconds) {
    client_tracker_t *tracker = find_client_tracker(g_security, ip);
    if (!tracker) {
        tracker = create_client_tracker(ip);
        if (!tracker) return -1;
        tracker->next = g_security->client_list;
        g_security->client_list = tracker;
    }
    
    tracker->status = SECURITY_STATUS_BLOCKED;
    tracker->rate_limit_reset = 0; // current_time + duration_seconds
    g_security->total_blocked++;
    
    char ip_str[16];
    security_uint32_to_ip(ip);
    security_log_blocked_request(ip, "Temporary block for security violation");
    
    return 0;
}

// Разблокировка IP
int security_unblock_ip(uint32_t ip) {
    client_tracker_t *tracker = find_client_tracker(g_security, ip);
    if (tracker) {
        tracker->status = SECURITY_STATUS_OK;
        tracker->violation_count = 0;
        return 0;
    }
    return -1;
}

// Проверка блокировки IP
int security_is_ip_blocked(uint32_t ip) {
    client_tracker_t *tracker = find_client_tracker(g_security, ip);
    if (tracker && tracker->status == SECURITY_STATUS_BLOCKED) {
        return 1;
    }
    return 0;
}

// Валидация заголовка MTProto
int security_validate_mtproto_header(const void *header, size_t header_size) {
    if (!g_security || !g_security->config.protocol_validation) {
        return 0;
    }
    
    if (header_size < 16) {
        g_security->invalid_requests++;
        security_log_event(ATTACK_TYPE_INVALID_PROTOCOL, 0, "MTProto header too short");
        return -1;
    }
    
    // Базовая проверка сигнатуры MTProto (в реальной реализации более сложная)
    const unsigned char *hdr = (const unsigned char*)header;
    if (hdr[0] != 0xef && hdr[0] != 0xdd) { // Пример проверки
        g_security->invalid_requests++;
        security_log_event(ATTACK_TYPE_INVALID_PROTOCOL, 0, "Invalid MTProto signature");
        return -1;
    }
    
    return 0;
}

// Детектирование malformed пакетов
int security_detect_malformed_packets(const void *data, size_t size) {
    if (!g_security) return 0;
    
    if (size == 0 || !data) {
        return 1;
    }
    
    // Проверка на минимальный размер MTProto пакета
    if (size < 12) {
        g_security->invalid_requests++;
        return 1;
    }
    
    return 0;
}

// Валидация последовательности пакетов
int security_validate_packet_sequence(uint32_t client_ip, uint32_t sequence) {
    // В реальной реализации отслеживать последовательность пакетов от клиента
    // и детектировать аномалии
    return 0;
}

// Загрузка whitelist
int security_load_whitelist(const char *filename) {
    if (!filename) return -1;
    
    FILE *file = fopen(filename, "r");
    if (!file) return -1;
    
    char line[64];
    while (fgets(line, sizeof(line), file)) {
        // Удаление символа новой строки
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            uint32_t ip = security_ip_to_uint32(line);
            security_add_to_whitelist(ip);
        }
    }
    
    fclose(file);
    return 0;
}

// Загрузка blacklist
int security_load_blacklist(const char *filename) {
    if (!filename) return -1;
    
    FILE *file = fopen(filename, "r");
    if (!file) return -1;
    
    char line[64];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            uint32_t ip = security_ip_to_uint32(line);
            security_add_to_blacklist(ip, "From blacklist file");
        }
    }
    
    fclose(file);
    return 0;
}

// Добавление в whitelist
int security_add_to_whitelist(uint32_t ip) {
    client_tracker_t *tracker = find_client_tracker(g_security, ip);
    if (!tracker) {
        tracker = create_client_tracker(ip);
        if (!tracker) return -1;
        tracker->next = g_security->client_list;
        g_security->client_list = tracker;
    }
    
    tracker->status = SECURITY_STATUS_OK;
    tracker->violation_count = 0;
    return 0;
}

// Добавление в blacklist
int security_add_to_blacklist(uint32_t ip, const char *reason) {
    client_tracker_t *tracker = find_client_tracker(g_security, ip);
    if (!tracker) {
        tracker = create_client_tracker(ip);
        if (!tracker) return -1;
        tracker->next = g_security->client_list;
        g_security->client_list = tracker;
    }
    
    tracker->status = SECURITY_STATUS_BLOCKED;
    g_security->total_blocked++;
    security_log_event(ATTACK_TYPE_NONE, ip, reason ? reason : "Added to blacklist");
    return 0;
}

// Получение статистики
void security_get_stats(modular_security_t *sec, char *buffer, size_t buffer_size) {
    if (!sec || !buffer) return;
    
    snprintf(buffer, buffer_size,
        "Security Stats:\n"
        "Buffer Overflow Attempts: %lld\n"
        "DDoS Attempts: %lld\n"
        "Invalid Requests: %lld\n"
        "Rate Limit Violations: %lld\n"
        "Total Blocked IPs: %d\n"
        "Total Violations: %d\n",
        sec->buffer_overflow_attempts,
        sec->ddos_attempts,
        sec->invalid_requests,
        sec->rate_limit_violations,
        sec->total_blocked,
        sec->total_violations);
}

// Сброс статистики
void security_reset_stats(modular_security_t *sec) {
    if (!sec) return;
    
    sec->buffer_overflow_attempts = 0;
    sec->ddos_attempts = 0;
    sec->invalid_requests = 0;
    sec->rate_limit_violations = 0;
    sec->total_blocked = 0;
    sec->total_violations = 0;
}

// Получение активных соединений
int security_get_active_connections(void) {
    if (!g_security) return 0;
    
    int count = 0;
    client_tracker_t *tracker = g_security->client_list;
    while (tracker) {
        if (tracker->connection_count > 0) {
            count += tracker->connection_count;
        }
        tracker = tracker->next;
    }
    return count;
}

// Очистка ресурсов
void security_cleanup(modular_security_t *sec) {
    if (!sec) return;
    
    security_cleanup_client_trackers(sec);
    
    if (sec->config.whitelist_file) {
        free(sec->config.whitelist_file);
    }
    if (sec->config.blacklist_file) {
        free(sec->config.blacklist_file);
    }
    
    free(sec);
    if (g_security == sec) {
        g_security = NULL;
    }
}

// Очистка клиентских трекеров
void security_cleanup_client_trackers(modular_security_t *sec) {
    if (!sec) return;
    
    client_tracker_t *current = sec->client_list;
    while (current) {
        client_tracker_t *next = current->next;
        free(current);
        current = next;
    }
    sec->client_list = NULL;
}

// Вспомогательные функции
static client_tracker_t* find_client_tracker(modular_security_t *sec, uint32_t ip) {
    client_tracker_t *tracker = sec->client_list;
    while (tracker) {
        if (tracker->ip_address == ip) {
            return tracker;
        }
        tracker = tracker->next;
    }
    return NULL;
}

static client_tracker_t* create_client_tracker(uint32_t ip) {
    client_tracker_t *tracker = calloc(1, sizeof(client_tracker_t));
    if (tracker) {
        tracker->ip_address = ip;
        tracker->status = SECURITY_STATUS_OK;
    }
    return tracker;
}

static void cleanup_expired_trackers(modular_security_t *sec) {
    long long current_time = 0; // В реальной реализации использовать gettimeofday
    long long timeout = sec->config.connection_timeout;
    
    client_tracker_t *prev = NULL;
    client_tracker_t *current = sec->client_list;
    
    while (current) {
        if (current_time - current->last_activity > timeout) {
            if (prev) {
                prev->next = current->next;
            } else {
                sec->client_list = current->next;
            }
            client_tracker_t *next = current->next;
            free(current);
            current = next;
        } else {
            prev = current;
            current = current->next;
        }
    }
}

static int is_suspicious_pattern(const void *data, size_t size) {
    // Простая проверка на подозрительные паттерны
    // В реальной реализации более сложные алгоритмы
    if (size < 4) return 0;
    
    const unsigned char *bytes = (const unsigned char*)data;
    
    // Проверка на известные сигнатуры атак
    if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x00) {
        return 1; // Null bytes flood
    }
    
    // Проверка на повторяющиеся паттерны
    if (size >= 8) {
        if (memcmp(bytes, bytes + 4, 4) == 0) {
            // Повторяющийся 4-байтный паттерн
            return 1;
        }
    }
    
    return 0;
}

// Конвертация IP в uint32
uint32_t security_ip_to_uint32(const char *ip_str) {
    if (!ip_str) return 0;
    
    struct in_addr addr;
    if (inet_aton(ip_str, &addr)) {
        return ntohl(addr.s_addr);
    }
    return 0;
}

// Конвертация uint32 в IP
const char* security_uint32_to_ip(uint32_t ip) {
    static char ip_str[16];
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    strncpy(ip_str, inet_ntoa(addr), sizeof(ip_str) - 1);
    ip_str[sizeof(ip_str) - 1] = '\0';
    return ip_str;
}

// Конвертация статуса в строку
const char* security_status_to_string(security_status_t status) {
    switch (status) {
        case SECURITY_STATUS_OK: return "OK";
        case SECURITY_STATUS_WARNING: return "WARNING";
        case SECURITY_STATUS_BLOCKED: return "BLOCKED";
        case SECURITY_STATUS_RATE_LIMITED: return "RATE_LIMITED";
        default: return "UNKNOWN";
    }
}

// Конвертация типа атаки в строку
const char* security_attack_type_to_string(attack_type_t attack) {
    switch (attack) {
        case ATTACK_TYPE_NONE: return "NONE";
        case ATTACK_TYPE_FLOOD: return "FLOOD";
        case ATTACK_TYPE_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case ATTACK_TYPE_INVALID_PROTOCOL: return "INVALID_PROTOCOL";
        case ATTACK_TYPE_RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case ATTACK_TYPE_SUSPICIOUS_PATTERN: return "SUSPICIOUS_PATTERN";
        default: return "UNKNOWN";
    }
}

// Логирование событий безопасности
void security_log_event(attack_type_t attack, uint32_t client_ip, const char *details) {
    if (!g_security || g_security->config.logging_level < 1) return;
    
    FILE *log_file = fopen("security.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] ATTACK: %s from %s - %s\n",
                "CURRENT_TIME",  // В реальной реализации использовать реальное время
                security_attack_type_to_string(attack),
                security_uint32_to_ip(client_ip),
                details ? details : "No details");
        fclose(log_file);
    }
}

// Логирование заблокированных запросов
void security_log_blocked_request(uint32_t client_ip, const char *reason) {
    if (!g_security || g_security->config.logging_level < 2) return;
    
    FILE *log_file = fopen("blocked.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] BLOCKED: %s - %s\n",
                "CURRENT_TIME",
                security_uint32_to_ip(client_ip),
                reason ? reason : "No reason");
        fclose(log_file);
    }
}

// Логирование нарушений
void security_log_violation(uint32_t client_ip, const char *violation_type) {
    if (!g_security || g_security->config.logging_level < 1) return;
    
    FILE *log_file = fopen("violations.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] VIOLATION: %s from %s\n",
                "CURRENT_TIME",
                violation_type ? violation_type : "Unknown violation",
                security_uint32_to_ip(client_ip));
        fclose(log_file);
    }
}