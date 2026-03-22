/*
 * dns-cache.h - DNS кэширующий сервер/резолвер
 * Поддержка DNS, DoH (DNS over HTTPS), DoT (DNS over TLS)
 */

#ifndef __DNS_CACHE_H__
#define __DNS_CACHE_H__

#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define DNS_CACHE_SIZE 512
#define DNS_MAX_DOMAIN_LEN 256
#define DNS_MAX_IPS 8
#define DNS_CACHE_DEFAULT_TTL 300

/* Типы DNS записей */
#define DNS_TYPE_A      1
#define DNS_TYPE_AAAA   28
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_MX     15
#define DNS_TYPE_TXT    16

/* DNS запись в кэше */
typedef struct {
    char domain[DNS_MAX_DOMAIN_LEN];
    char ips[DNS_MAX_IPS][64];
    int ip_count;
    uint16_t record_type;
    time_t created;
    time_t expires;
    uint32_t ttl;
    int hit_count;
} dns_cache_entry_t;

/* Статистика DNS */
typedef struct {
    uint64_t total_queries;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t expired_entries;
    uint64_t failed_queries;
} dns_stats_t;

/* Конфигурация DNS */
typedef struct {
    char upstream_dns[8][64];
    int upstream_count;
    int cache_enabled;
    int cache_ttl;
    int doh_enabled;
    char doh_url[256];
    int dot_enabled;
    char dot_server[256];
    int dot_port;
} dns_config_t;

/* DNS кэш контекст */
typedef struct {
    dns_cache_entry_t entries[DNS_CACHE_SIZE];
    int entry_count;
    pthread_mutex_t lock;
    pthread_rwlock_t rwlock;
    dns_stats_t stats;
    dns_config_t config;
    time_t start_time;
} dns_cache_ctx_t;

/* Инициализация DNS кэша */
int dns_cache_init(dns_cache_ctx_t *ctx, dns_config_t *config);

/* Освобождение ресурсов */
void dns_cache_destroy(dns_cache_ctx_t *ctx);

/* Резолвинг домена (основная функция) */
int dns_cache_resolve(dns_cache_ctx_t *ctx, 
                       const char *domain,
                       char *ips,
                       int max_ips,
                       uint16_t record_type);

/* Добавление записи в кэш */
int dns_cache_add(dns_cache_ctx_t *ctx,
                   const char *domain,
                   const char *ip,
                   uint16_t record_type,
                   uint32_t ttl);

/* Очистка кэша */
void dns_cache_clear(dns_cache_ctx_t *ctx);

/* Очистка устаревших записей */
int dns_cache_cleanup_expired(dns_cache_ctx_t *ctx);

/* Получение статистики */
void dns_cache_get_stats(dns_cache_ctx_t *ctx, dns_stats_t *stats);

/* Экспорт статистики в JSON */
int dns_cache_export_json(dns_cache_ctx_t *ctx, char *buffer, size_t buffer_size);

/* Предзагрузка популярных доменов */
int dns_cache_prefetch(dns_cache_ctx_t *ctx, const char **domains, int count);

#endif /* __DNS_CACHE_H__ */
