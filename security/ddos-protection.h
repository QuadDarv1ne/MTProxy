/*
 * DDoS protection header for MTProxy
 * Defines structures and functions for DDoS protection
 */

#ifndef _DDOS_PROTECTION_H_
#define _DDOS_PROTECTION_H_

#include <stdint.h>

#define IP_ADDR_STR_LEN 46  // IPv6 address can be up to 45 chars + null terminator

// DDoS protection configuration structure
typedef struct {
    int max_connections_per_ip;
    int rate_limit_window;  // in seconds
    int enable_ip_blocking;
    int block_duration;     // in seconds
    int connection_timeout; // in seconds
    int enable_connection_throttling;
} ddos_protection_config_t;

// DDoS statistics structure
typedef struct {
    long long total_connections_monitored;
    long long connections_blocked;
    long long attack_patterns_detected;
    long long rate_limiting_triggered;
    long long suspicious_activities_logged;
} ddos_statistics_t;

// DDoS protection context structure
typedef struct {
    ddos_protection_config_t config;
    ddos_statistics_t stats;
    int ip_count;
} ddos_protection_context_t;

// Function prototypes
int ddos_init(const ddos_protection_config_t *config);
int ddos_check_connection(const char *ip_address);
int ddos_update_config(const ddos_protection_config_t *new_config);
ddos_statistics_t ddos_get_stats();
void ddos_reset_stats();
void ddos_cleanup();

#endif