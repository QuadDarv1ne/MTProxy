/*
 * Certificate pinning for MTProxy
 *
 * This file defines the interface for certificate pinning
 * to enhance security by validating upstream server certificates.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Certificate pin structure */
typedef struct cert_pin {
    char *hostname;                /* Hostname to pin */
    char *public_key_hash;         /* Hash of public key */
    char *cert_fingerprint;        /* Certificate fingerprint */
    uint8_t hash_algorithm;        /* Algorithm used for hashing */
    uint8_t pin_mode;             /* Strict or relaxed pinning */
    struct cert_pin *next;        /* Next pin in list */
} cert_pin_t;

/* Certificate validation result */
typedef enum {
    CERT_VALIDATION_SUCCESS = 0,
    CERT_PIN_MISMATCH = 1,
    CERT_NOT_PINNED = 2,
    CERT_EXPIRED = 3,
    CERT_INVALID_FORMAT = 4
} cert_validation_result_t;

/* Certificate pinning configuration */
typedef struct {
    cert_pin_t *pins;              /* List of certificate pins */
    uint32_t pin_count;           /* Number of configured pins */
    uint8_t enforce_strict;       /* Whether to enforce strict pinning */
    uint32_t max_age_days;        /* Maximum age of certificates */
    uint8_t warn_only;            /* Only warn instead of blocking */
} cert_pinning_config_t;

/* Initialize certificate pinning system */
int init_cert_pinning(const cert_pinning_config_t *config);

/* Add a certificate pin */
int add_cert_pin(const char *hostname, const char *public_key_hash, uint8_t hash_algorithm);

/* Validate certificate against pinned values */
cert_validation_result_t validate_certificate_pinning(const char *hostname, const char *server_cert);

/* Rotate pinned certificates */
int rotate_pinned_certificates(void);

/* Update certificate pinning configuration */
int update_cert_pinning_config(const cert_pinning_config_t *new_config);

/* Remove a certificate pin */
int remove_cert_pin(const char *hostname);

/* Get current certificate pinning configuration */
cert_pinning_config_t* get_cert_pinning_config(void);

/* Verify upstream server certificate */
cert_validation_result_t verify_upstream_certificate(const char *hostname, const char *cert_data, size_t cert_len);

/* Load certificate pins from file */
int load_cert_pins_from_file(const char *filename);

/* Save certificate pins to file */
int save_cert_pins_to_file(const char *filename);

/* Check if hostname has any pins configured */
int is_hostname_pinned(const char *hostname);

/* Cleanup certificate pinning resources */
void destroy_cert_pinning(void);

/* Get certificate pinning statistics */
void get_cert_pinning_stats(uint32_t *validation_attempts, uint32_t *violations);

#ifdef __cplusplus
}
#endif