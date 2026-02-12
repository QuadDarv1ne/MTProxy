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

    Copyright 2024 Telegram Messenger Inc
*/

#pragma once

#include <time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Security enhancement constants
#define MAX_SUPPORTED_CIPHERS 16
#define MAX_SECRET_KEYS 256
#define MAX_ACCESS_CONTROLS 1024
#define NONCE_LENGTH 12
#define TAG_LENGTH 16
#define SALT_LENGTH 32

// Forward declarations for OpenSSL types to avoid direct includes
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_cipher_st EVP_CIPHER;

// Cryptographic algorithm identifiers
typedef enum {
    CRYPTO_ALG_AES_256_GCM = 0,
    CRYPTO_ALG_CHACHA20_POLY1305,
    CRYPTO_ALG_AES_128_GCM,
    CRYPTO_ALG_XCHACHA20_POLY1305,
    CRYPTO_ALG_AES_256_CTR,
    CRYPTO_ALG_COUNT
} crypto_algorithm_t;

// Access control levels
typedef enum {
    ACL_LEVEL_NONE = 0,
    ACL_LEVEL_READ_ONLY,
    ACL_LEVEL_STANDARD,
    ACL_LEVEL_ADMIN,
    ACL_LEVEL_SUPERUSER
} access_level_t;

// Client authentication status
typedef enum {
    AUTH_STATUS_UNKNOWN = 0,
    AUTH_STATUS_PENDING,
    AUTH_STATUS_GRANTED,
    AUTH_STATUS_DENIED,
    AUTH_STATUS_EXPIRED
} auth_status_t;

// Security policies
typedef struct security_policy {
    int require_strong_auth;
    int enforce_encryption;
    int rate_limit_enabled;
    int max_connections_per_ip;
    int connection_timeout_sec;
    int max_request_size;
    int enable_geo_blocking;
    char blocked_countries[100][3];  // ISO country codes
    double min_protocol_version;
    int enable_certificate_verification;
} security_policy_t;

// Enhanced key structure with additional security features
typedef struct enhanced_secret_key {
    int id;
    unsigned char key_data[64];  // Up to 512-bit key
    int key_length;
    crypto_algorithm_t algorithm;
    time_t created_at;
    time_t expires_at;
    int is_revoked;
    char description[256];
    int ref_count;
} enhanced_secret_key_t;

// Access control entry
typedef struct access_control_entry {
    unsigned int ip_address;           // IPv4 address
    unsigned int ip_mask;              // Network mask
    access_level_t level;              // Access level
    time_t valid_from;                 // Validity period start
    time_t valid_until;                // Validity period end
    auth_status_t auth_status;         // Current authentication status
    long long request_count;           // Count of requests from this IP
    time_t last_access;                // Timestamp of last access
    int is_blocked;                    // Is this IP explicitly blocked?
    char reason[128];                  // Reason for access level or blocking
} access_control_entry_t;

// Security statistics
typedef struct security_stats {
    long long total_auth_attempts;
    long long successful_auths;
    long long failed_auths;
    long long blocked_connections;
    long long encrypted_packets;
    long long decrypted_packets;
    long long invalid_packets;
    long long rate_limited_requests;
    time_t last_security_audit;
    int active_threats;
} security_stats_t;

// Enhanced crypto context with additional security features
typedef struct enhanced_crypto_context {
    EVP_CIPHER_CTX *cipher_ctx;
    unsigned char key[64];
    unsigned char iv[16];
    unsigned char salt[SALT_LENGTH];
    crypto_algorithm_t algorithm;
    time_t last_used;
    int use_count;
    unsigned char auth_tag[TAG_LENGTH];
    int tag_len;
} enhanced_crypto_context_t;

// Function declarations for security enhancements
int init_security_system(void);
int cleanup_security_system(void);
int update_crypto_library(void);
int audit_security(void);
int rotate_encryption_keys(void);

// Enhanced encryption functions
int enhanced_encrypt_data(const void *plaintext, int plaintext_len, 
                         void *ciphertext, int *ciphertext_len,
                         const unsigned char *key, int key_len,
                         const unsigned char *iv, crypto_algorithm_t alg);
int enhanced_decrypt_data(const void *ciphertext, int ciphertext_len,
                         void *plaintext, int *plaintext_len,
                         const unsigned char *key, int key_len,
                         const unsigned char *iv, crypto_algorithm_t alg);

// Key management functions
enhanced_secret_key_t* add_secret_key(const unsigned char *key_data, int key_len, 
                                     crypto_algorithm_t algorithm, const char *description);
int remove_secret_key(int key_id);
enhanced_secret_key_t* get_secret_key(int key_id);
int revoke_secret_key(int key_id);
int list_secret_keys(enhanced_secret_key_t *keys, int max_keys);

// Access control functions
int add_access_control(unsigned int ip_address, unsigned int ip_mask, 
                      access_level_t level, time_t valid_until, const char *reason);
int remove_access_control(unsigned int ip_address);
access_control_entry_t* check_access_control(unsigned int ip_address);
int update_access_level(unsigned int ip_address, access_level_t new_level);
int block_ip_address(unsigned int ip_address, const char *reason);
int unblock_ip_address(unsigned int ip_address);

// Authentication functions
auth_status_t authenticate_client(const unsigned char *auth_token, int token_len, 
                                unsigned int client_ip);
int generate_auth_token(unsigned char *token_out, int *token_len, 
                       unsigned int client_ip, access_level_t level);
int verify_auth_token(const unsigned char *token, int token_len, 
                     unsigned int client_ip, access_level_t *level);

// Security monitoring functions
void get_security_stats(security_stats_t *stats);
void reset_security_stats(void);
int check_rate_limit(unsigned int client_ip, int requests_per_second);
int detect_anomalous_activity(unsigned int client_ip, const char *activity_desc);

// Policy management functions
int set_security_policy(const security_policy_t *policy);
int get_security_policy(security_policy_t *policy);
int enforce_security_policy(void);

#ifdef __cplusplus
}
#endif