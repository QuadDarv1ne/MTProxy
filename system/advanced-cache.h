/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#ifndef _ADVANCED_CACHE_H_
#define _ADVANCED_CACHE_H_

// Cache entry structure
typedef struct cache_entry {
    void *key;
    void *value;
    size_t key_size;
    size_t value_size;
    long long timestamp;
    long long access_count;
    int ttl_seconds;
    struct cache_entry *next;
    struct cache_entry *prev;
} cache_entry_t;

// Cache statistics
typedef struct {
    long long hits;
    long long misses;
    long long evictions;
    long long insertions;
    long long deletions;
    size_t current_size;
    size_t max_size;
    double hit_ratio;
    int entry_count;
    int max_entries;
} cache_stats_t;

// Cache configuration
typedef struct {
    size_t max_size_bytes;
    int max_entries;
    int default_ttl_seconds;
    int enable_lru;
    int enable_ttl;
    int enable_statistics;
} cache_config_t;

// Cache structure
typedef struct {
    cache_entry_t *entries;
    cache_entry_t *head;  // LRU head (most recently used)
    cache_entry_t *tail;  // LRU tail (least recently used)
    cache_config_t config;
    cache_stats_t stats;
    size_t current_size;
    int entry_count;
    int initialized;
} advanced_cache_t;

// Function declarations
int init_advanced_cache(advanced_cache_t *cache, cache_config_t *config);
void cleanup_advanced_cache(advanced_cache_t *cache);

// Cache operations
int cache_put(advanced_cache_t *cache, void *key, size_t key_size, void *value, size_t value_size);
int cache_get(advanced_cache_t *cache, void *key, size_t key_size, void **value, size_t *value_size);
int cache_remove(advanced_cache_t *cache, void *key, size_t key_size);
int cache_clear(advanced_cache_t *cache);

// Cache management
int cache_resize(advanced_cache_t *cache, size_t new_max_size);
int cache_set_ttl(advanced_cache_t *cache, void *key, size_t key_size, int ttl_seconds);
int cache_cleanup_expired(advanced_cache_t *cache);

// Statistics
cache_stats_t cache_get_stats(advanced_cache_t *cache);
void cache_reset_stats(advanced_cache_t *cache);
void cache_print_stats(advanced_cache_t *cache);

// Utility functions
size_t cache_get_memory_usage(advanced_cache_t *cache);
int cache_get_entry_count(advanced_cache_t *cache);
double cache_get_hit_ratio(advanced_cache_t *cache);

#endif // _ADVANCED_CACHE_H_