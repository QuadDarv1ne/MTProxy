/**
 * @file test_performance.c
 * @brief Performance тесты для cache-manager и rate-limiter
 * 
 * Тестирование производительности под нагрузкой
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "common/cache-manager.h"
#include "common/rate-limiter.h"

// Статистика тестов
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("\n=== Test: %s ===\n", #name)
#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  ✗ %s\n", msg); \
    } \
} while(0)

// ============================================================================
// Cache Performance Tests
// ============================================================================

static void test_cache_bulk_insert(void) {
    TEST(cache_bulk_insert);
    
    clock_t start = clock();
    
    cache_manager_t* cache = cache_init(10000, 3600);
    ASSERT(cache != NULL, "Cache initialized");
    
    // Bulk insert 10000 items
    const int ITEM_COUNT = 10000;
    for (int i = 0; i < ITEM_COUNT; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        cache_put(cache, key, value, strlen(value));
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("  Inserted %d items in %.3f seconds\n", ITEM_COUNT, time_sec);
    printf("  Throughput: %.0f items/sec\n", ITEM_COUNT / time_sec);
    
    cache_cleanup(cache);
}

static void test_cache_bulk_read(void) {
    TEST(cache_bulk_read);
    
    cache_manager_t* cache = cache_init(10000, 3600);
    
    // Pre-populate cache
    const int ITEM_COUNT = 10000;
    for (int i = 0; i < ITEM_COUNT; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        cache_put(cache, key, value, strlen(value));
    }
    
    // Bulk read
    clock_t start = clock();
    
    int hits = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        const char* result = cache_get(cache, key);
        if (result != NULL) hits++;
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    ASSERT(hits == ITEM_COUNT, "All items found");
    printf("  Read %d items in %.3f seconds\n", ITEM_COUNT, time_sec);
    printf("  Throughput: %.0f reads/sec\n", ITEM_COUNT / time_sec);
    printf("  Hit rate: 100%%\n");
    
    cache_cleanup(cache);
}

static void test_cache_hit_rate(void) {
    TEST(cache_hit_rate);
    
    cache_manager_t* cache = cache_init(1000, 3600);
    
    const int TOTAL_REQUESTS = 10000;
    const int UNIQUE_KEYS = 1000;
    int hits = 0;
    
    clock_t start = clock();
    
    for (int i = 0; i < TOTAL_REQUESTS; i++) {
        char key[64], value[64];
        int key_num = i % UNIQUE_KEYS;  // Cycle through keys
        snprintf(key, sizeof(key), "key_%d", key_num);
        snprintf(value, sizeof(value), "value_%d", i);
        
        const char* result = cache_get(cache, key);
        if (result != NULL) {
            hits++;
        } else {
            // Cache miss - add to cache
            cache_put(cache, key, value, strlen(value));
        }
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    double hit_rate = (hits * 100.0) / TOTAL_REQUESTS;
    
    printf("  Total requests: %d\n", TOTAL_REQUESTS);
    printf("  Hits: %d, Misses: %d\n", hits, TOTAL_REQUESTS - hits);
    printf("  Hit rate: %.2f%%\n", hit_rate);
    printf("  Time: %.3f seconds\n", time_sec);
    
    ASSERT(hit_rate > 80.0, "Hit rate above 80%%");
    
    cache_cleanup(cache);
}

static void test_cache_ttl_performance(void) {
    TEST(cache_ttl_performance);
    
    cache_manager_t* cache = cache_init(10000, 1);  // 1 second TTL
    
    const int ITEM_COUNT = 5000;
    
    // Insert items
    for (int i = 0; i < ITEM_COUNT; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        cache_put(cache, key, value, strlen(value));
    }
    
    printf("  Inserted %d items with 1s TTL\n", ITEM_COUNT);
    printf("  Waiting for TTL expiration (2 seconds)...\n");
    
    // Wait for TTL
    clock_t start = clock();
    while ((double)(clock() - start) / CLOCKS_PER_SEC < 2.0) {
        // Busy wait
    }
    
    // Check expiration
    start = clock();
    int expired = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        if (cache_get(cache, key) == NULL) {
            expired++;
        }
    }
    double time_sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    printf("  Expired items: %d/%d\n", expired, ITEM_COUNT);
    printf("  Check time: %.3f seconds\n", time_sec);
    
    ASSERT(expired == ITEM_COUNT, "All items expired");
    
    cache_cleanup(cache);
}

// ============================================================================
// Rate Limiter Performance Tests
// ============================================================================

static void test_rate_limiter_bulk_check(void) {
    TEST(rate_limiter_bulk_check);
    
    rate_limiter_t* limiter = rate_limiter_init(1000, 60);  // 1000 requests per 60s
    ASSERT(limiter != NULL, "Rate limiter initialized");
    
    const int REQUEST_COUNT = 5000;
    int allowed = 0;
    int blocked = 0;
    
    clock_t start = clock();
    
    // Single client making many requests
    for (int i = 0; i < REQUEST_COUNT; i++) {
        if (rate_limiter_check(limiter, 1)) {
            allowed++;
        } else {
            blocked++;
        }
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("  Total requests: %d\n", REQUEST_COUNT);
    printf("  Allowed: %d, Blocked: %d\n", allowed, blocked);
    printf("  Time: %.3f seconds\n", time_sec);
    printf("  Throughput: %.0f checks/sec\n", REQUEST_COUNT / time_sec);
    
    ASSERT(allowed == 1000, "Exactly 1000 requests allowed");
    ASSERT(blocked == 4000, "4000 requests blocked");
    
    rate_limiter_cleanup(limiter);
}

static void test_rate_limiter_multi_client(void) {
    TEST(rate_limiter_multi_client);
    
    rate_limiter_t* limiter = rate_limiter_init(100, 60);  // 100 requests per 60s per client
    
    const int CLIENT_COUNT = 100;
    const int REQUESTS_PER_CLIENT = 150;
    int total_allowed = 0;
    int total_blocked = 0;
    
    clock_t start = clock();
    
    for (int client_id = 0; client_id < CLIENT_COUNT; client_id++) {
        for (int req = 0; req < REQUESTS_PER_CLIENT; req++) {
            if (rate_limiter_check(limiter, client_id)) {
                total_allowed++;
            } else {
                total_blocked++;
            }
        }
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("  Clients: %d\n", CLIENT_COUNT);
    printf("  Requests per client: %d\n", REQUESTS_PER_CLIENT);
    printf("  Total requests: %d\n", CLIENT_COUNT * REQUESTS_PER_CLIENT);
    printf("  Allowed: %d, Blocked: %d\n", total_allowed, total_blocked);
    printf("  Time: %.3f seconds\n", time_sec);
    
    // Each client should have 100 allowed, 50 blocked
    ASSERT(total_allowed == CLIENT_COUNT * 100, "100 requests per client allowed");
    ASSERT(total_blocked == CLIENT_COUNT * 50, "50 requests per client blocked");
    
    rate_limiter_cleanup(limiter);
}

static void test_rate_limiter_whitelist_performance(void) {
    TEST(rate_limiter_whitelist_performance);
    
    rate_limiter_t* limiter = rate_limiter_init(10, 60);  // Low limit
    
    // Add 10 whitelisted clients
    for (int i = 0; i < 10; i++) {
        rate_limiter_add_to_whitelist(limiter, i);
    }
    
    const int WHITELISTED_CLIENTS = 10;
    const int NORMAL_CLIENTS = 10;
    const int REQUESTS = 100;
    
    int whitelist_allowed = 0;
    int normal_allowed = 0;
    
    clock_t start = clock();
    
    // Test whitelisted clients
    for (int client_id = 0; client_id < WHITELISTED_CLIENTS; client_id++) {
        for (int req = 0; req < REQUESTS; req++) {
            if (rate_limiter_check(limiter, client_id)) {
                whitelist_allowed++;
            }
        }
    }
    
    // Test normal clients
    for (int client_id = 100; client_id < 100 + NORMAL_CLIENTS; client_id++) {
        for (int req = 0; req < REQUESTS; req++) {
            if (rate_limiter_check(limiter, client_id)) {
                normal_allowed++;
            }
        }
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("  Whitelisted clients: %d\n", WHITELISTED_CLIENTS);
    printf("  Normal clients: %d\n", NORMAL_CLIENTS);
    printf("  Requests each: %d\n", REQUESTS);
    printf("  Whitelist allowed: %d\n", whitelist_allowed);
    printf("  Normal allowed: %d\n", normal_allowed);
    printf("  Time: %.3f seconds\n", time_sec);
    
    // Whitelisted should always be allowed
    ASSERT(whitelist_allowed == WHITELISTED_CLIENTS * REQUESTS, "Whitelist always allowed");
    // Normal clients limited to 10 per client
    ASSERT(normal_allowed == NORMAL_CLIENTS * 10, "Normal clients limited");
    
    rate_limiter_cleanup(limiter);
}

// ============================================================================
// Mixed Workload Test
// ============================================================================

static void test_mixed_cache_ratelimit_workload(void) {
    TEST(mixed_cache_ratelimit_workload);
    
    cache_manager_t* cache = cache_init(5000, 300);
    rate_limiter_t* limiter = rate_limiter_init(100, 60);
    
    const int OPERATIONS = 10000;
    int cache_hits = 0;
    int cache_misses = 0;
    int rate_allowed = 0;
    int rate_blocked = 0;
    
    clock_t start = clock();
    
    for (int i = 0; i < OPERATIONS; i++) {
        int client_id = i % 100;
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i % 1000);
        snprintf(value, sizeof(value), "value_%d", i);
        
        // Check rate limit
        if (!rate_limiter_check(limiter, client_id)) {
            rate_blocked++;
            continue;
        }
        rate_allowed++;
        
        // Access cache
        const char* result = cache_get(cache, key);
        if (result != NULL) {
            cache_hits++;
        } else {
            cache_misses++;
            cache_put(cache, key, value, strlen(value));
        }
    }
    
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("  Total operations: %d\n", OPERATIONS);
    printf("  Rate allowed: %d, Blocked: %d\n", rate_allowed, rate_blocked);
    printf("  Cache hits: %d, Misses: %d\n", cache_hits, cache_misses);
    printf("  Time: %.3f seconds\n", time_sec);
    printf("  Throughput: %.0f ops/sec\n", OPERATIONS / time_sec);
    
    double hit_rate = (cache_hits * 100.0) / (cache_hits + cache_misses);
    printf("  Cache hit rate: %.2f%%\n", hit_rate);
    
    ASSERT(hit_rate > 50.0, "Cache hit rate above 50%%");
    
    cache_cleanup(cache);
    rate_limiter_cleanup(limiter);
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("===========================================\n");
    printf("  MTProxy Performance Test Suite\n");
    printf("===========================================\n");
    
    // Cache performance tests
    test_cache_bulk_insert();
    test_cache_bulk_read();
    test_cache_hit_rate();
    test_cache_ttl_performance();
    
    // Rate limiter performance tests
    test_rate_limiter_bulk_check();
    test_rate_limiter_multi_client();
    test_rate_limiter_whitelist_performance();
    
    // Mixed workload
    test_mixed_cache_ratelimit_workload();
    
    // Summary
    printf("\n===========================================\n");
    printf("  Test Summary\n");
    printf("===========================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
