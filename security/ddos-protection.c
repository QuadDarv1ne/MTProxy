/*
 * DDoS protection implementation for MTProxy
 * Provides rate limiting and connection throttling functionality
 */

#include "ddos-protection.h"

// Maximum number of tracked IPs
#define MAX_TRACKED_IPS 10000

// Structure to track IP information
typedef struct {
    char ip_address[IP_ADDR_STR_LEN];
    int connection_count;
    long long last_connection_time;
    int is_blocked;
    long long block_expiration;
} ip_tracking_entry_t;

// Global DDoS protection context
static ddos_protection_context_t g_ddos_ctx = {0};
static ip_tracking_entry_t g_tracked_ips[MAX_TRACKED_IPS];

// Initialize DDoS protection
int ddos_init(const ddos_protection_config_t *config) {
    if (!config) {
        return -1;
    }
    
    // Copy configuration
    g_ddos_ctx.config = *config;
    g_ddos_ctx.ip_count = 0;
    
    // Initialize tracking table
    for (int i = 0; i < MAX_TRACKED_IPS; i++) {
        // Manually clear the entry
        for (int j = 0; j < IP_ADDR_STR_LEN; j++) {
            g_tracked_ips[i].ip_address[j] = 0;
        }
        g_tracked_ips[i].connection_count = 0;
        g_tracked_ips[i].last_connection_time = 0;
        g_tracked_ips[i].is_blocked = 0;
        g_tracked_ips[i].block_expiration = 0;
    }
    
    // Initialize statistics
    g_ddos_ctx.stats.total_connections_monitored = 0;
    g_ddos_ctx.stats.connections_blocked = 0;
    g_ddos_ctx.stats.attack_patterns_detected = 0;
    g_ddos_ctx.stats.rate_limiting_triggered = 0;
    g_ddos_ctx.stats.suspicious_activities_logged = 0;
    
    return 0;
}

// Helper function to compare two strings
static int ddos_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to copy string
static void ddos_strncpy(char *dest, const char *src, int max_len) {
    int i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Check if connection from IP is allowed
int ddos_check_connection(const char *ip_address) {
    if (!ip_address) {
        return 0; // Reject if no IP provided
    }
    
    // Update statistics
    g_ddos_ctx.stats.total_connections_monitored++;
    
    // Find or create IP entry
    int ip_index = -1;
    for (int i = 0; i < g_ddos_ctx.ip_count && i < MAX_TRACKED_IPS; i++) {
        if (ddos_strcmp(g_tracked_ips[i].ip_address, ip_address) == 0) {
            ip_index = i;
            break;
        }
    }
    
    // If IP not found, create new entry
    if (ip_index == -1 && g_ddos_ctx.ip_count < MAX_TRACKED_IPS) {
        ip_index = g_ddos_ctx.ip_count;
        ddos_strncpy(g_tracked_ips[ip_index].ip_address, ip_address, IP_ADDR_STR_LEN);
        g_tracked_ips[ip_index].connection_count = 0;
        g_tracked_ips[ip_index].is_blocked = 0;
        g_ddos_ctx.ip_count++;
    }
    
    if (ip_index == -1) {
        // Too many IPs being tracked, temporarily reject
        g_ddos_ctx.stats.connections_blocked++;
        return 0;
    }
    
    // Check if IP is blocked
    if (g_tracked_ips[ip_index].is_blocked) {
        // TODO: In real implementation, check if block period expired
        g_ddos_ctx.stats.connections_blocked++;
        return 0;
    }
    
    // Update connection count
    g_tracked_ips[ip_index].connection_count++;
    
    // Check if connection count exceeds limit
    if (g_tracked_ips[ip_index].connection_count > g_ddos_ctx.config.max_connections_per_ip) {
        // Block the IP
        g_tracked_ips[ip_index].is_blocked = 1;
        g_ddos_ctx.stats.connections_blocked++;
        g_ddos_ctx.stats.attack_patterns_detected++;
        return 0;
    }
    
    // Connection allowed
    return 1;
}

// Update DDoS protection configuration
int ddos_update_config(const ddos_protection_config_t *new_config) {
    if (!new_config) {
        return -1;
    }
    
    g_ddos_ctx.config = *new_config;
    return 0;
}

// Get DDoS protection statistics
ddos_statistics_t ddos_get_stats() {
    return g_ddos_ctx.stats;
}

// Reset statistics
void ddos_reset_stats() {
    g_ddos_ctx.stats.total_connections_monitored = 0;
    g_ddos_ctx.stats.connections_blocked = 0;
    g_ddos_ctx.stats.attack_patterns_detected = 0;
    g_ddos_ctx.stats.rate_limiting_triggered = 0;
    g_ddos_ctx.stats.suspicious_activities_logged = 0;
}

// Cleanup DDoS protection
void ddos_cleanup() {
    // Reset tracking table
    for (int i = 0; i < g_ddos_ctx.ip_count && i < MAX_TRACKED_IPS; i++) {
        // Manually clear the entry
        for (int j = 0; j < IP_ADDR_STR_LEN; j++) {
            g_tracked_ips[i].ip_address[j] = 0;
        }
        g_tracked_ips[i].connection_count = 0;
        g_tracked_ips[i].last_connection_time = 0;
        g_tracked_ips[i].is_blocked = 0;
        g_tracked_ips[i].block_expiration = 0;
    }
    
    g_ddos_ctx.ip_count = 0;
    
    // Reset statistics
    ddos_reset_stats();
}