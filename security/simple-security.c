/*
    Реализация упрощенной системы безопасности для MTProxy
    Без зависимостей от стандартной библиотеки C
*/

#include "simple-security.h"

// Глобальная переменная
static simple_security_t *g_simple_security = 0;

// Вспомогательные функции
static client_tracker_t* find_tracker(simple_security_t *sec, uint32_t ip);
static client_tracker_t* create_tracker(uint32_t ip);
static void cleanup_expired_trackers(simple_security_t *sec);
static int is_suspicious_data(const void *data, size_t size);

// Инициализация
simple_security_t* simple_security_init(security_config_t *config) {
    // Простое выделение памяти (в реальном коде использовать malloc)
    simple_security_t *sec = (simple_security_t*)0x10000000; // Пример адреса
    if (!sec) {
        return 0;
    }
    
    // Инициализация нулями
    char *ptr = (char*)sec;
    for (int i = 0; i < sizeof(simple_security_t); i++) {
        ptr[i] = 0;
    }
    
    // Настройка конфигурации
    sec->config.rate_limit = config ? config->rate_limit : DEFAULT_RATE_LIMIT;
    sec->config.burst_limit = config ? config->burst_limit : BURST_LIMIT;
    sec->config.connection_timeout = config ? config->connection_timeout : CONNECTION_TIMEOUT;
    sec->config.max_connections = config ? config->max_connections : MAX_CONCURRENT_CONNECTIONS;
    sec->config.buffer_overflow_protection = config ? config->buffer_overflow_protection : 1;
    sec->config.protocol_validation = config ? config->protocol_validation : 1;
    sec->config.logging_level = config ? config->logging_level : 1;
    
    g_simple_security = sec;
    return sec;
}

// Очистка
void simple_security_cleanup(simple_security_t *sec) {
    if (!sec) return;
    
    // Очистка трекеров
    client_tracker_t *current = sec->client_list;
    while (current) {
        client_tracker_t *next = current->next;
        // В реальном коде: free(current);
        current = next;
    }
    
    sec->client_list = 0;
    if (g_simple_security == sec) {
        g_simple_security = 0;
    }
}

// Проверка буфера
security_status_t simple_security_check_buffer(const void *buffer, size_t size, size_t max_size) {
    if (!g_simple_security) return SECURITY_STATUS_OK;
    
    if (!buffer) {
        return SECURITY_STATUS_BLOCKED;
    }
    
    if (size > max_size) {
        g_simple_security->buffer_overflow_attempts++;
        return SECURITY_STATUS_BLOCKED;
    }
    
    // Проверка подозрительных паттернов
    if (is_suspicious_data(buffer, size)) {
        g_simple_security->invalid_requests++;
        return SECURITY_STATUS_WARNING;
    }
    
    return SECURITY_STATUS_OK;
}

// Проверка входных данных
security_status_t simple_security_check_input(const char *data, size_t length) {
    if (!g_simple_security) return SECURITY_STATUS_OK;
    
    if (!data || length == 0) {
        return SECURITY_STATUS_BLOCKED;
    }
    
    if (length > MAX_BUFFER_SIZE) {
        g_simple_security->buffer_overflow_attempts++;
        return SECURITY_STATUS_BLOCKED;
    }
    
    // Проверка ASCII символов
    for (size_t i = 0; i < length; i++) {
        if ((unsigned char)data[i] > 127) {
            g_simple_security->invalid_requests++;
            return SECURITY_STATUS_WARNING;
        }
    }
    
    return SECURITY_STATUS_OK;
}

// Проверка rate limiting
security_status_t simple_security_check_rate_limit(uint32_t client_ip) {
    if (!g_simple_security) return SECURITY_STATUS_OK;
    
    client_tracker_t *tracker = find_tracker(g_simple_security, client_ip);
    if (!tracker) {
        tracker = create_tracker(client_ip);
        if (!tracker) return SECURITY_STATUS_OK;
        tracker->next = g_simple_security->client_list;
        g_simple_security->client_list = tracker;
    }
    
    // Сброс счетчика каждую секунду (упрощенно)
    long long current_time = 0; // В реальной реализации использовать системное время
    if (current_time - tracker->rate_limit_reset >= 1) {
        tracker->request_count = 0;
        tracker->rate_limit_reset = current_time;
    }
    
    tracker->request_count++;
    tracker->last_activity = current_time;
    
    // Проверка лимитов
    if (tracker->request_count > g_simple_security->config.rate_limit) {
        if (tracker->request_count > g_simple_security->config.burst_limit) {
            tracker->status = SECURITY_STATUS_BLOCKED;
            g_simple_security->rate_limit_violations++;
            return SECURITY_STATUS_BLOCKED;
        }
        tracker->status = SECURITY_STATUS_RATE_LIMITED;
        return SECURITY_STATUS_RATE_LIMITED;
    }
    
    tracker->status = SECURITY_STATUS_OK;
    return SECURITY_STATUS_OK;
}

// Проверка соединения
security_status_t simple_security_check_connection(uint32_t client_ip) {
    if (!g_simple_security) return SECURITY_STATUS_OK;
    
    client_tracker_t *tracker = find_tracker(g_simple_security, client_ip);
    if (!tracker) {
        tracker = create_tracker(client_ip);
        if (!tracker) return SECURITY_STATUS_OK;
        tracker->next = g_simple_security->client_list;
        g_simple_security->client_list = tracker;
    }
    
    if (tracker->connection_count >= g_simple_security->config.max_connections) {
        g_simple_security->ddos_attempts++;
        return SECURITY_STATUS_BLOCKED;
    }
    
    tracker->connection_count++;
    tracker->last_activity = 0; // current_time
    return SECURITY_STATUS_OK;
}

// Безопасное копирование памяти
int simple_security_safe_memcpy(void *dest, const void *src, size_t n, size_t dest_size) {
    if (!dest || !src || n > dest_size) {
        if (g_simple_security) {
            g_simple_security->buffer_overflow_attempts++;
        }
        return -1;
    }
    
    // Простое копирование байт
    char *d = (char*)dest;
    const char *s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    
    return 0;
}

// Безопасное копирование строк
int simple_security_safe_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) {
        return -1;
    }
    
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    
    return 0;
}

// Детектирование flood атак
int simple_security_detect_flood(uint32_t client_ip, int request_count) {
    if (!g_simple_security) return 0;
    
    if (request_count > g_simple_security->config.burst_limit) {
        g_simple_security->ddos_attempts++;
        return 1;
    }
    
    return 0;
}

// Блокировка IP
int simple_security_block_ip(uint32_t ip) {
    client_tracker_t *tracker = find_tracker(g_simple_security, ip);
    if (!tracker) {
        tracker = create_tracker(ip);
        if (!tracker) return -1;
        tracker->next = g_simple_security->client_list;
        g_simple_security->client_list = tracker;
    }
    
    tracker->status = SECURITY_STATUS_BLOCKED;
    g_simple_security->total_blocked++;
    return 0;
}

// Разблокировка IP
int simple_security_unblock_ip(uint32_t ip) {
    client_tracker_t *tracker = find_tracker(g_simple_security, ip);
    if (tracker) {
        tracker->status = SECURITY_STATUS_OK;
        tracker->violation_count = 0;
        return 0;
    }
    return -1;
}

// Проверка блокировки
int simple_security_is_blocked(uint32_t ip) {
    client_tracker_t *tracker = find_tracker(g_simple_security, ip);
    if (tracker && tracker->status == SECURITY_STATUS_BLOCKED) {
        return 1;
    }
    return 0;
}

// Валидация MTProto
int simple_security_validate_mtproto(const void *header, size_t header_size) {
    if (!g_simple_security || !g_simple_security->config.protocol_validation) {
        return 0;
    }
    
    if (header_size < 16) {
        g_simple_security->invalid_requests++;
        return -1;
    }
    
    // Проверка сигнатуры (пример)
    const unsigned char *hdr = (const unsigned char*)header;
    if (hdr[0] != 0xef && hdr[0] != 0xdd) {
        g_simple_security->invalid_requests++;
        return -1;
    }
    
    return 0;
}

// Проверка пакета
int simple_security_check_packet(const void *data, size_t size) {
    if (!g_simple_security) return 0;
    
    if (size == 0 || !data) {
        return 1;
    }
    
    if (size < 12) {
        g_simple_security->invalid_requests++;
        return 1;
    }
    
    return 0;
}

// Конвертация IP в uint32
uint32_t simple_security_ip_to_uint32(const char *ip_str) {
    if (!ip_str) return 0;
    
    uint32_t ip = 0;
    uint32_t part = 0;
    int shift = 24;
    
    for (int i = 0; ip_str[i] != '\0'; i++) {
        if (ip_str[i] >= '0' && ip_str[i] <= '9') {
            part = part * 10 + (ip_str[i] - '0');
        } else if (ip_str[i] == '.') {
            ip |= (part << shift);
            shift -= 8;
            part = 0;
        }
    }
    ip |= part; // Последняя часть
    
    return ip;
}

// Конвертация uint32 в IP
const char* simple_security_uint32_to_ip(uint32_t ip) {
    static char ip_str[16];
    int pos = 0;
    
    for (int i = 0; i < 4; i++) {
        uint32_t byte = (ip >> (24 - i * 8)) & 0xFF;
        if (byte >= 100) {
            ip_str[pos++] = '0' + (byte / 100);
            byte %= 100;
        }
        if (byte >= 10 || pos > 0) {
            ip_str[pos++] = '0' + (byte / 10);
            byte %= 10;
        }
        ip_str[pos++] = '0' + byte;
        if (i < 3) ip_str[pos++] = '.';
    }
    ip_str[pos] = '\0';
    
    return ip_str;
}

// Конвертация статуса в строку
const char* simple_security_status_string(security_status_t status) {
    switch (status) {
        case SECURITY_STATUS_OK: return "OK";
        case SECURITY_STATUS_WARNING: return "WARNING";
        case SECURITY_STATUS_BLOCKED: return "BLOCKED";
        case SECURITY_STATUS_RATE_LIMITED: return "RATE_LIMITED";
        default: return "UNKNOWN";
    }
}

// Получение статистики
void simple_security_get_stats(simple_security_t *sec, char *buffer, size_t buffer_size) {
    if (!sec || !buffer || buffer_size < 100) return;
    
    // Простое форматирование (в реальном коде использовать snprintf)
    int pos = 0;
    const char *stats = "Security Stats: BO="; // Buffer Overflow
    for (int i = 0; stats[i] != '\0' && pos < buffer_size - 1; i++) {
        buffer[pos++] = stats[i];
    }
    
    // Добавить числа (упрощенно)
    buffer[pos] = '\0';
}

// Сброс статистики
void simple_security_reset_stats(simple_security_t *sec) {
    if (!sec) return;
    
    sec->buffer_overflow_attempts = 0;
    sec->ddos_attempts = 0;
    sec->invalid_requests = 0;
    sec->rate_limit_violations = 0;
    sec->total_blocked = 0;
    sec->total_violations = 0;
}

// Вспомогательные функции

static client_tracker_t* find_tracker(simple_security_t *sec, uint32_t ip) {
    client_tracker_t *tracker = sec->client_list;
    while (tracker) {
        if (tracker->ip_address == ip) {
            return tracker;
        }
        tracker = tracker->next;
    }
    return 0;
}

static client_tracker_t* create_tracker(uint32_t ip) {
    // В реальном коде использовать malloc
    client_tracker_t *tracker = (client_tracker_t*)0x20000000; // Пример адреса
    if (tracker) {
        // Инициализация нулями
        char *ptr = (char*)tracker;
        for (int i = 0; i < sizeof(client_tracker_t); i++) {
            ptr[i] = 0;
        }
        tracker->ip_address = ip;
        tracker->status = SECURITY_STATUS_OK;
    }
    return tracker;
}

static void cleanup_expired_trackers(simple_security_t *sec) {
    // В реальной реализации очищать устаревшие трекеры
}

static int is_suspicious_data(const void *data, size_t size) {
    if (size < 4) return 0;
    
    const unsigned char *bytes = (const unsigned char*)data;
    
    // Проверка null bytes flood
    if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x00) {
        return 1;
    }
    
    // Проверка повторяющихся паттернов
    if (size >= 8) {
        int match = 1;
        for (int i = 0; i < 4; i++) {
            if (bytes[i] != bytes[i + 4]) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    
    return 0;
}