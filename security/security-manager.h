/*
 * Security manager header for MTProxy
 * Defines structures and functions for security enhancements
 */

#ifndef _SECURITY_MANAGER_H_
#define _SECURITY_MANAGER_H_

#include <stdint.h>

// Security statistics structure
typedef struct {
    long long total_connections_checked;
    long long blocked_connections;
    long long failed_auth_attempts;
    long long detected_attack_patterns;
} security_stats_t;

// DDoS protection settings structure
typedef struct {
    int max_connections_per_ip;
    int rate_limit_window; // in seconds
    int enable_ip_blocking;
    int block_duration; // in seconds
    int connection_timeout;
} ddos_settings_t;

// Security manager context structure
typedef struct {
    int cert_pinning_enabled;
    int ddos_protection_enabled;
    int hsm_integration_enabled;
    ddos_settings_t ddos_settings;
    security_stats_t stats;
} security_manager_context_t;

// Function prototypes
int secmgr_init();
int secmgr_add_cert_pin(const char *hostname, const unsigned char *cert_hash);
int secmgr_validate_cert_pinning(const unsigned char *received_cert_hash, const char *hostname);
int secmgr_check_ddos_protection(const char *client_ip);
int secmgr_init_hsm(const char *module_path, int slot_id);
int secmgr_hsm_encrypt(const unsigned char *plaintext, int plaintext_len, 
                      unsigned char *ciphertext, int *ciphertext_len);
int secmgr_hsm_decrypt(const unsigned char *ciphertext, int ciphertext_len, 
                      unsigned char *plaintext, int *plaintext_len);
security_stats_t secmgr_get_stats();
void secmgr_cleanup();

#endif