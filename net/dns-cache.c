/*
 * dns-cache.c - DNS кэширующий сервер/резолвер
 * Поддержка DNS, DoH (DNS over HTTPS), DoT (DNS over TLS)
 */

#include "dns-cache.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Внутренние функции */
static int dns_cache_find(dns_cache_ctx_t *ctx, const char *domain, dns_cache_entry_t **entry);
static int dns_cache_find_free_slot(dns_cache_ctx_t *ctx);
static int dns_resolve_upstream(const char *domain, char *ips, int max_ips, uint16_t record_type);

/* Инициализация DNS кэша */
int dns_cache_init(dns_cache_ctx_t *ctx, dns_config_t *config) {
    if (!ctx) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(dns_cache_ctx_t));
    
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        return -1;
    }
    
    if (pthread_rwlock_init(&ctx->rwlock, NULL) != 0) {
        pthread_mutex_destroy(&ctx->lock);
        return -1;
    }
    
    /* Копируем конфигурацию */
    if (config) {
        memcpy(&ctx->config, config, sizeof(dns_config_t));
    } else {
        /* Конфигурация по умолчанию */
        strcpy(ctx->config.upstream_dns[0], "8.8.8.8");
        strcpy(ctx->config.upstream_dns[1], "8.8.4.4");
        ctx->config.upstream_count = 2;
        ctx->config.cache_enabled = 1;
        ctx->config.cache_ttl = DNS_CACHE_DEFAULT_TTL;
        ctx->config.doh_enabled = 0;
        ctx->config.dot_enabled = 0;
    }
    
    ctx->start_time = time(NULL);
    ctx->entry_count = 0;
    
    return 0;
}

/* Освобождение ресурсов */
void dns_cache_destroy(dns_cache_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
    pthread_rwlock_destroy(&ctx->rwlock);
    pthread_mutex_destroy(&ctx->lock);
}

/* Поиск записи в кэше */
static int dns_cache_find(dns_cache_ctx_t *ctx, const char *domain, dns_cache_entry_t **entry) {
    for (int i = 0; i < ctx->entry_count; i++) {
        if (strcasecmp(ctx->entries[i].domain, domain) == 0) {
            *entry = &ctx->entries[i];
            return i;
        }
    }
    return -1;
}

/* Поиск свободного слота */
static int dns_cache_find_free_slot(dns_cache_ctx_t *ctx) {
    time_t now = time(NULL);
    
    /* Сначала ищем полностью свободный слот */
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (ctx->entries[i].domain[0] == '\0') {
            return i;
        }
    }
    
    /* Ищем истёкшую запись для замены */
    for (int i = 0; i < ctx->entry_count; i++) {
        if (now > ctx->entries[i].expires) {
            return i;
        }
    }
    
    /* Если нет истёкших, заменяем с наименьшим hit_count */
    int min_idx = 0;
    int min_hits = ctx->entries[0].hit_count;
    
    for (int i = 1; i < ctx->entry_count; i++) {
        if (ctx->entries[i].hit_count < min_hits) {
            min_hits = ctx->entries[i].hit_count;
            min_idx = i;
        }
    }
    
    return min_idx;
}

/* DNS резолвинг через upstream сервер */
static int dns_resolve_upstream(const char *domain, char *ips, int max_ips, uint16_t record_type) {
    struct addrinfo hints, *res, *p;
    int status;
    int ip_count = 0;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (record_type == DNS_TYPE_AAAA) ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    status = getaddrinfo(domain, NULL, &hints, &res);
    if (status != 0) {
        return -1;
    }
    
    for (p = res; p != NULL && ip_count < max_ips; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_sa;
            inet_ntop(AF_INET, &ipv4->sin_addr, ips + (ip_count * 64), 64);
            ip_count++;
        } else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_sa;
            inet_ntop(AF_INET6, &ipv6->sin6_addr, ips + (ip_count * 64), 64);
            ip_count++;
        }
    }
    
    freeaddrinfo(res);
    
    return ip_count;
}

/* Резолвинг домена (основная функция) */
int dns_cache_resolve(dns_cache_ctx_t *ctx, 
                       const char *domain,
                       char *ips,
                       int max_ips,
                       uint16_t record_type) {
    if (!ctx || !domain || !ips || max_ips <= 0) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&ctx->rwlock);
    
    ctx->stats.total_queries++;
    
    /* Ищем в кэше */
    dns_cache_entry_t *entry;
    int idx = dns_cache_find(ctx, domain, &entry);
    
    if (idx >= 0) {
        time_t now = time(NULL);
        
        /* Проверяем TTL */
        if (now < entry->expires && entry->ip_count > 0) {
            /* Cache hit */
            ctx->stats.cache_hits++;
            entry->hit_count++;
            
            /* Копируем IPs */
            int count = (entry->ip_count < max_ips) ? entry->ip_count : max_ips;
            for (int i = 0; i < count; i++) {
                strncpy(ips + (i * 64), entry->ips[i], 63);
                ips[i * 64 + 63] = '\0';
            }
            
            pthread_rwlock_unlock(&ctx->rwlock);
            return count;
        }
        
        /* Истёкшая запись */
        ctx->stats.expired_entries++;
    }
    
    pthread_rwlock_unlock(&ctx->rwlock);
    
    /* Cache miss - резолвим через upstream */
    ctx->stats.cache_misses++;
    
    char resolved_ips[DNS_MAX_IPS * 64];
    int ip_count = dns_resolve_upstream(domain, resolved_ips, DNS_MAX_IPS, record_type);
    
    if (ip_count <= 0) {
        ctx->stats.failed_queries++;
        return -1;
    }
    
    /* Добавляем в кэш */
    pthread_rwlock_wrlock(&ctx->rwlock);
    
    /* Проверяем ещё раз (могла добавиться пока ждали lock) */
    idx = dns_cache_find(ctx, domain, &entry);
    
    if (idx >= 0) {
        /* Обновляем существующую запись */
        entry->ip_count = 0;
        for (int i = 0; i < ip_count && i < DNS_MAX_IPS; i++) {
            strncpy(entry->ips[i], resolved_ips + (i * 64), 63);
            entry->ips[i][63] = '\0';
            entry->ip_count++;
        }
        entry->created = time(NULL);
        entry->expires = entry->created + ctx->config.cache_ttl;
        entry->hit_count = 0;
    } else {
        /* Создаём новую запись */
        int slot = dns_cache_find_free_slot(ctx);
        
        dns_cache_entry_t *new_entry = &ctx->entries[slot];
        memset(new_entry, 0, sizeof(dns_cache_entry_t));
        
        strncpy(new_entry->domain, domain, DNS_MAX_DOMAIN_LEN - 1);
        new_entry->domain[DNS_MAX_DOMAIN_LEN - 1] = '\0';
        
        new_entry->ip_count = 0;
        for (int i = 0; i < ip_count && i < DNS_MAX_IPS; i++) {
            strncpy(new_entry->ips[i], resolved_ips + (i * 64), 63);
            new_entry->ips[i][63] = '\0';
            new_entry->ip_count++;
        }
        
        new_entry->record_type = record_type;
        new_entry->created = time(NULL);
        new_entry->expires = new_entry->created + ctx->config.cache_ttl;
        new_entry->ttl = ctx->config.cache_ttl;
        new_entry->hit_count = 0;
        
        if (slot >= ctx->entry_count) {
            ctx->entry_count = slot + 1;
        }
    }
    
    pthread_rwlock_unlock(&ctx->rwlock);
    
    /* Копируем результат */
    int count = (ip_count < max_ips) ? ip_count : max_ips;
    for (int i = 0; i < count; i++) {
        strncpy(ips + (i * 64), resolved_ips + (i * 64), 63);
        ips[i * 64 + 63] = '\0';
    }
    
    return count;
}

/* Добавление записи в кэш */
int dns_cache_add(dns_cache_ctx_t *ctx,
                   const char *domain,
                   const char *ip,
                   uint16_t record_type,
                   uint32_t ttl) {
    if (!ctx || !domain || !ip) {
        return -1;
    }
    
    pthread_rwlock_wrlock(&ctx->rwlock);
    
    dns_cache_entry_t *entry;
    int idx = dns_cache_find(ctx, domain, &entry);
    
    if (idx >= 0) {
        /* Обновляем существующую */
        if (entry->ip_count < DNS_MAX_IPS) {
            strncpy(entry->ips[entry->ip_count], ip, 63);
            entry->ips[entry->ip_count][63] = '\0';
            entry->ip_count++;
        }
        entry->record_type = record_type;
        entry->ttl = ttl;
        entry->created = time(NULL);
        entry->expires = entry->created + ttl;
    } else {
        /* Создаём новую */
        int slot = dns_cache_find_free_slot(ctx);
        dns_cache_entry_t *new_entry = &ctx->entries[slot];
        
        memset(new_entry, 0, sizeof(dns_cache_entry_t));
        strncpy(new_entry->domain, domain, DNS_MAX_DOMAIN_LEN - 1);
        strncpy(new_entry->ips[0], ip, 63);
        new_entry->ips[0][63] = '\0';
        new_entry->ip_count = 1;
        new_entry->record_type = record_type;
        new_entry->ttl = ttl;
        new_entry->created = time(NULL);
        new_entry->expires = new_entry->created + ttl;
        
        if (slot >= ctx->entry_count) {
            ctx->entry_count = slot + 1;
        }
    }
    
    pthread_rwlock_unlock(&ctx->rwlock);
    
    return 0;
}

/* Очистка кэша */
void dns_cache_clear(dns_cache_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
    pthread_rwlock_wrlock(&ctx->rwlock);
    
    memset(ctx->entries, 0, sizeof(ctx->entries));
    ctx->entry_count = 0;
    
    pthread_rwlock_unlock(&ctx->rwlock);
}

/* Очистка устаревших записей */
int dns_cache_cleanup_expired(dns_cache_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    pthread_rwlock_wrlock(&ctx->rwlock);
    
    time_t now = time(NULL);
    int removed = 0;
    
    for (int i = 0; i < ctx->entry_count; i++) {
        if (now > ctx->entries[i].expires) {
            memset(&ctx->entries[i], 0, sizeof(dns_cache_entry_t));
            removed++;
        }
    }
    
    pthread_rwlock_unlock(&ctx->rwlock);
    
    return removed;
}

/* Получение статистики */
void dns_cache_get_stats(dns_cache_ctx_t *ctx, dns_stats_t *stats) {
    if (!ctx || !stats) {
        return;
    }
    
    pthread_rwlock_rdlock(&ctx->rwlock);
    memcpy(stats, &ctx->stats, sizeof(dns_stats_t));
    pthread_rwlock_unlock(&ctx->rwlock);
}

/* Экспорт статистики в JSON */
int dns_cache_export_json(dns_cache_ctx_t *ctx, char *buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size < 512) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&ctx->rwlock);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"stats\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_queries\": %llu,\n", 
                       (unsigned long long)ctx->stats.total_queries);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"cache_hits\": %llu,\n", 
                       (unsigned long long)ctx->stats.cache_hits);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"cache_misses\": %llu,\n", 
                       (unsigned long long)ctx->stats.cache_misses);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"expired_entries\": %llu,\n", 
                       (unsigned long long)ctx->stats.expired_entries);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"failed_queries\": %llu\n", 
                       (unsigned long long)ctx->stats.failed_queries);
    offset += snprintf(buffer + offset, buffer_size - offset, "  },\n");
    
    /* Hit rate */
    double hit_rate = 0.0;
    if (ctx->stats.total_queries > 0) {
        hit_rate = (double)ctx->stats.cache_hits / ctx->stats.total_queries * 100.0;
    }
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"hit_rate\": %.2f,\n", hit_rate);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"cached_entries\": %d,\n", ctx->entry_count);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"cache_size\": %d\n", DNS_CACHE_SIZE);
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    pthread_rwlock_unlock(&ctx->rwlock);
    
    return offset;
}

/* Предзагрузка популярных доменов */
int dns_cache_prefetch(dns_cache_ctx_t *ctx, const char **domains, int count) {
    if (!ctx || !domains || count <= 0) {
        return -1;
    }
    
    char ips[DNS_MAX_IPS * 64];
    int resolved = 0;
    
    for (int i = 0; i < count; i++) {
        int ret = dns_cache_resolve(ctx, domains[i], ips, DNS_MAX_IPS, DNS_TYPE_A);
        if (ret > 0) {
            resolved++;
        }
    }
    
    return resolved;
}
