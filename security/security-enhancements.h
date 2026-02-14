/*
 * Security enhancements for MTProxy
 * Includes certificate pinning, DDoS protection, and HSM integration
 */

#ifndef _SECURITY_ENHANCEMENTS_H_
#define _SECURITY_ENHANCEMENTS_H_

#include <stdint.h>

// Certificate pinning structure
typedef struct {
    char *hostname;
    unsigned char pinned_cert_sha256[32];  // SHA256 hash is always 32 bytes
    int enabled;
} cert_pinning_entry_t;

// DDoS protection configuration
typedef struct {
    int enable_rate_limiting;
    int max_connections_per_ip;
    int rate_limit_window;
    int enable_connection_throttling;
    int connection_limit;
    int enable_ip_blocking;
    int block_duration;
} ddos_protection_config_t;

// HSM configuration structure (placeholder)
typedef struct {
    int hsm_enabled;
    char *pkcs11_module_path;
    int slot_id;
    char *token_label;
    char *private_key_label;
    void *hsm_session;
} hsm_config_t;

// Security context structure
typedef struct {
    cert_pinning_entry_t *cert_pins;
    int cert_pins_count;
    ddos_protection_config_t ddos_config;
    hsm_config_t hsm_config;
    int security_level;
} security_context_t;

// Function prototypes
int init_security_enhancements(security_context_t *ctx);
int add_certificate_pin(security_context_t *ctx, const char *hostname, const unsigned char *cert_hash);
int validate_certificate_pinning(const unsigned char *cert_hash, const char *hostname);
int check_ddos_protection(const char *client_ip);
int init_hsm_integration(hsm_config_t *hsm_config);
int cleanup_security_enhancements(security_context_t *ctx);

#endif