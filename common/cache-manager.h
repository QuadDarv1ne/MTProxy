/*
 * Cache manager for MTProxy
 *
 * This file defines the interface for advanced caching mechanisms
 * to improve performance by storing frequently accessed data.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* Define time_t as a long integer */
typedef long time_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Cache entry structure */
typedef struct cache_entry {
    char *key;                   /* Entry key */
    void *data;                  /* Cached data */
    size_t data_size;            /* Size of cached data */
    time_t creation_time;        /* Time when entry was created */
    time_t access_time;          /* Last access time */
    time_t expiry_time;          /* When this entry expires */
    uint32_t access_count;       /* Number of accesses */
    struct cache_entry *prev;    /* Previous entry in LRU list */
    struct cache_entry *next;    /* Next entry in LRU list */
} cache_entry_t;

/* Cache configuration */
typedef struct {
    size_t max_entries;          /* Maximum number of entries */
    size_t max_size_mb;          /* Maximum cache size in MB */
    time_t default_ttl_sec;      /* Default time-to-live in seconds */
    int enable_compression;      /* Whether to compress stored data */
    int enable_prefetch;         /* Whether to enable prefetching */
} cache_config_t;

/* Main cache structure */
typedef struct {
    cache_entry_t **buckets;     /* Hash table buckets */
    size_t num_buckets;          /* Number of buckets */
    cache_entry_t *lru_head;     /* Head of LRU list */
    cache_entry_t *lru_tail;     /* Tail of LRU list */
    size_t current_entries;      /* Current number of entries */
    size_t current_size;         /* Current cache size in bytes */
    cache_config_t config;       /* Cache configuration */
    long long hits;              /* Cache hit count */
    long long misses;            /* Cache miss count */
    long long evictions;         /* Number of evicted entries */
} lru_cache_t;

/* Initialize the cache system */
int init_lru_cache(lru_cache_t *cache, const cache_config_t *config);

/* Get data from cache */
void* get_cached_data(lru_cache_t *cache, const char *key, size_t *data_size);

/* Put data into cache */
int put_cached_data(lru_cache_t *cache, const char *key, const void *data, size_t data_size);

/* Remove entry from cache */
int remove_cached_data(lru_cache_t *cache, const char *key);

/* Check if key exists in cache */
int is_cached(lru_cache_t *cache, const char *key);

/* Get cache statistics */
void get_cache_stats(lru_cache_t *cache, long long *hits, long long *misses, long long *evictions);

/* Calculate cache hit ratio */
double get_cache_hit_ratio(lru_cache_t *cache);

/* Prefetch data into cache */
int prefetch_data(lru_cache_t *cache, const char *key);

/* Cleanup cache resources */
void destroy_lru_cache(lru_cache_t *cache);

/* Clear all cache entries */
void clear_cache(lru_cache_t *cache);

/* Resize cache */
int resize_cache(lru_cache_t *cache, size_t new_max_entries, size_t new_max_size_mb);

#ifdef __cplusplus
}
#endif