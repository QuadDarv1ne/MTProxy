/*
 * Certificate pinning header for MTProxy
 * Implements security through certificate validation
 */

#ifndef _CERT_PINNING_H_
#define _CERT_PINNING_H_

#include <stdint.h>

#define MAX_PINNED_CERTS 100
#define HOSTNAME_LEN 256

// Structure to hold certificate pin information
typedef struct {
    char hostname[HOSTNAME_LEN];
    unsigned char cert_hash[32];  // SHA256 hash
    int enabled;
} cert_pin_entry_t;

// Certificate pinning context
typedef struct {
    cert_pin_entry_t certs[MAX_PINNED_CERTS];
    int cert_count;
    int enabled;
} cert_pinning_context_t;

// Function prototypes
int cert_pinning_init();
int cert_pinning_add(const char *hostname, const unsigned char *cert_hash);
int cert_pinning_verify(const char *hostname, const unsigned char *received_cert_hash);
int cert_pinning_remove(const char *hostname);
void cert_pinning_cleanup();

#endif