#ifndef DDoS_PROTECTION_ENHANCED_H
#define DDoS_PROTECTION_ENHANCED_H

#include <stdint.h>

/*
 * Улучшенная защита от DDoS-атак - система, которая отслеживает и ограничивает
 * количество запросов от каждого IP-адреса для предотвращения перегрузки сервера.
 */

// Structure for tracking rate limits per IP
typedef struct rate_limit_entry {
    uint32_t ip_address;
    int request_count;
    long timestamp;
    struct rate_limit_entry *next;
} rate_limit_entry_t;

// Main structure for DDoS protection
typedef struct ddos_protector {
    rate_limit_entry_t *rate_limits;
    int max_requests_per_minute;
    int max_concurrent_connections;
    int challenge_threshold;
    long cleanup_interval;
} ddos_protector_t;

/*
 * Инициализирует систему защиты от DDoS-атак с заданными параметрами
 * @param max_req_per_min: максимальное количество запросов в минуту на один IP
 * @param max_concurrent: максимальное количество одновременных подключений
 * @return: указатель на новую систему защиты или NULL в случае ошибки
 */
ddos_protector_t *init_ddos_protection(int max_req_per_min, int max_concurrent);

/*
 * Проверяет, находится ли IP-адрес в пределах ограничений скорости
 * @param protector: указатель на систему защиты
 * @param client_ip: IP-адрес клиента для проверки
 * @return: 1 если в пределах нормы, 0 если превышено ограничение
 */
int check_rate_limit(ddos_protector_t *protector, uint32_t client_ip);

/*
 * Проверяет, находится ли система под DDoS-атакой
 * @param protector: указатель на систему защиты
 * @return: 1 если система под атакой, 0 если все нормально
 */
int is_under_attack(ddos_protector_t *protector);

/*
 * Очищает старые записи из системы ограничения скорости
 * @param protector: указатель на систему защиты
 */
void cleanup_old_entries(ddos_protector_t *protector);

/*
 * Уничтожает систему защиты от DDoS-атак и освобождает память
 * @param protector: указатель на систему защиты для уничтожения
 */
void destroy_ddos_protection(ddos_protector_t *protector);

#endif