/*
 * DDoS protection for MTProxy
 *
 * This file defines the interface for DDoS protection mechanisms
 * to detect and mitigate various types of distributed denial-of-service attacks.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Attack type enumeration */
typedef enum {
    ATTACK_TYPE_SYN_FLOOD = 0,
    ATTACK_TYPE_HTTP_FLOOD,
    ATTACK_TYPE_CONNECTION_EXHAUSTION,
    ATTACK_TYPE_BANDWIDTH_AMPLIFICATION,
    ATTACK_TYPE_SLOWLORIS,
    ATTACK_TYPE_UNKNOWN
} attack_type_t;

/* Rate limiter structure */
typedef struct rate_limiter {
    uint64_t request_count;        /* Number of requests */
    uint64_t window_start;         /* Window start time */
    uint64_t window_duration;      /* Window duration in seconds */
    uint64_t max_requests;         /* Max requests allowed per window */
    uint8_t is_limited;            /* Whether rate limiting is active */
} rate_limiter_t;

/* IP reputation entry */
typedef struct ip_reputation {
    char ip_address[46];          /* IP address (IPv4/v6) */
    uint32_t score;               /* Reputation score */
    uint64_t last_seen;           /* Last seen timestamp */
    uint32_t attack_count;        /* Number of attacks detected */
    uint8_t is_blocked;           /* Whether IP is blocked */
    struct ip_reputation *next;   /* Next entry in list */
} ip_reputation_t;

/* DDoS protection configuration */
typedef struct {
    uint32_t max_connections_per_ip;     /* Max connections per IP */
    uint32_t max_requests_per_minute;    /* Max requests per minute */
    uint32_t connection_timeout_sec;     /* Connection timeout */
    uint32_t detection_threshold;        /* Threshold for attack detection */
    uint8_t enable_syn_cookies;          /* Enable SYN cookies */
    uint8_t enable_rate_limiting;        /* Enable rate limiting */
    uint8_t auto_block_malicious;        /* Auto-block malicious IPs */
} ddos_protection_config_t;

/* Attack detection result */
typedef struct {
    attack_type_t attack_type;          /* Type of attack detected */
    char source_ip[46];                 /* Source IP of attack */
    uint32_t severity_level;            /* Severity level (1-10) */
    uint64_t detection_timestamp;       /* When attack was detected */
    uint8_t mitigation_applied;         /* Whether mitigation was applied */
} attack_detection_result_t;

/* DDoS protection engine */
typedef struct {
    ddos_protection_config_t config;     /* Protection configuration */
    ip_reputation_t *reputation_list;    /* List of IP reputations */
    rate_limiter_t *global_limiter;      /* Global rate limiter */
    uint64_t total_attacks_detected;     /* Total attacks detected */
    uint64_t total_mitigations;          /* Total mitigations applied */
    uint64_t blocked_connections;        /* Total blocked connections */
} ddos_protection_engine_t;

/* Initialize DDoS protection system */
int init_ddos_protection(const ddos_protection_config_t *config);

/* Detect potential DDoS attacks */
attack_detection_result_t* detect_ddos_attack(const char *source_ip, uint32_t request_count, uint64_t timeframe);

/* Apply mitigation measures for detected attacks */
int apply_mitigation(attack_detection_result_t *detection);

/* Check if IP is rate limited */
uint8_t is_ip_rate_limited(const char *ip_address);

/* Block an IP address */
int block_ip_address(const char *ip_address);

/* Update IP reputation */
int update_ip_reputation(const char *ip_address, int32_t score_delta);

/* Process incoming connection for DDoS protection */
uint8_t process_connection_for_ddos(const char *ip_address);

/* Get current DDoS protection statistics */
void get_ddos_protection_stats(uint64_t *attacks_detected, uint64_t *mitigations_applied, uint64_t *blocked_conns);

/* Update DDoS protection configuration */
int update_ddos_config(const ddos_protection_config_t *new_config);

/* Get current configuration */
ddos_protection_config_t* get_ddos_config(void);

/* Cleanup DDoS protection resources */
void destroy_ddos_protection(void);

/* Reset IP reputation scores */
int reset_ip_reputation(const char *ip_address);

/* Get list of blocked IP addresses */
ip_reputation_t* get_blocked_ips_list(void);

/* Check if IP is currently blocked */
uint8_t is_ip_blocked(const char *ip_address);

#ifdef __cplusplus
}
#endif