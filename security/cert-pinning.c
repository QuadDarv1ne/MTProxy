/*
 * Certificate pinning implementation for MTProxy
 * Implements security through certificate validation
 */

#include "cert-pinning.h"

// Global certificate pinning context
static cert_pinning_context_t g_cert_pin_ctx = {0};

// Initialize certificate pinning
int cert_pinning_init() {
    // Initialize context
    g_cert_pin_ctx.cert_count = 0;
    g_cert_pin_ctx.enabled = 1;
    
    // Initialize all entries to zero
    for (int i = 0; i < MAX_PINNED_CERTS; i++) {
        for (int j = 0; j < HOSTNAME_LEN; j++) {
            g_cert_pin_ctx.certs[i].hostname[j] = 0;
        }
        for (int j = 0; j < 32; j++) {
            g_cert_pin_ctx.certs[i].cert_hash[j] = 0;
        }
        g_cert_pin_ctx.certs[i].enabled = 0;
    }
    
    return 0;
}

// Helper function to compare two strings
static int cert_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to copy string
static void cert_strncpy(char *dest, const char *src, int max_len) {
    int i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Add a certificate pin
int cert_pinning_add(const char *hostname, const unsigned char *cert_hash) {
    if (!hostname || !cert_hash) {
        return -1;
    }
    
    // Check if we have reached the maximum number of certificates
    if (g_cert_pin_ctx.cert_count >= MAX_PINNED_CERTS) {
        return -1;
    }
    
    // Add the new certificate pin
    cert_pin_entry_t *entry = &g_cert_pin_ctx.certs[g_cert_pin_ctx.cert_count];
    
    // Copy hostname
    cert_strncpy(entry->hostname, hostname, HOSTNAME_LEN);
    
    // Copy certificate hash
    for (int i = 0; i < 32; i++) {
        entry->cert_hash[i] = cert_hash[i];
    }
    
    entry->enabled = 1;
    g_cert_pin_ctx.cert_count++;
    
    return 0;
}

// Verify certificate against pinned values
int cert_pinning_verify(const char *hostname, const unsigned char *received_cert_hash) {
    if (!hostname || !received_cert_hash) {
        return -1;
    }
    
    // Find the pinned certificate for the given hostname
    for (int i = 0; i < g_cert_pin_ctx.cert_count; i++) {
        if (g_cert_pin_ctx.certs[i].enabled && 
            cert_strcmp(g_cert_pin_ctx.certs[i].hostname, hostname) == 0) {
            
            // Compare the received certificate hash with the pinned one
            for (int j = 0; j < 32; j++) {
                if (g_cert_pin_ctx.certs[i].cert_hash[j] != received_cert_hash[j]) {
                    // Hash mismatch - certificate validation failed
                    return 0;
                }
            }
            
            // Certificate matches the pinned one
            return 1;
        }
    }
    
    // No pinned certificate found for this hostname
    return -1;
}

// Remove a certificate pin
int cert_pinning_remove(const char *hostname) {
    if (!hostname) {
        return -1;
    }
    
    // Find the certificate pin to remove
    for (int i = 0; i < g_cert_pin_ctx.cert_count; i++) {
        if (cert_strcmp(g_cert_pin_ctx.certs[i].hostname, hostname) == 0) {
            // Mark as disabled
            g_cert_pin_ctx.certs[i].enabled = 0;
            
            // Shift remaining entries down
            for (int j = i; j < g_cert_pin_ctx.cert_count - 1; j++) {
                g_cert_pin_ctx.certs[j] = g_cert_pin_ctx.certs[j + 1];
            }
            
            g_cert_pin_ctx.cert_count--;
            return 0;
        }
    }
    
    return -1; // Certificate pin not found
}

// Cleanup certificate pinning
void cert_pinning_cleanup() {
    // Reset all entries
    for (int i = 0; i < MAX_PINNED_CERTS; i++) {
        for (int j = 0; j < HOSTNAME_LEN; j++) {
            g_cert_pin_ctx.certs[i].hostname[j] = 0;
        }
        for (int j = 0; j < 32; j++) {
            g_cert_pin_ctx.certs[i].cert_hash[j] = 0;
        }
        g_cert_pin_ctx.certs[i].enabled = 0;
    }
    
    g_cert_pin_ctx.cert_count = 0;
    g_cert_pin_ctx.enabled = 0;
}