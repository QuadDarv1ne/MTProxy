/*
 * Enhanced TLS Obfuscation for MTProxy
 * Implements advanced fake TLS functionality to bypass censorship
 */

#pragma once

// Forward declaration of types used in this header
typedef struct job_base *connection_job_t;

// Define in_addr if not already defined
#ifndef _STRUCT_IN_ADDR
#define _STRUCT_IN_ADDR
struct in_addr {
    unsigned long s_addr;  // Address in network byte order
};
#endif

// Define time-related types if not already defined
#ifndef _TIME_T_DEFINED
typedef long long time_t;
#define _TIME_T_DEFINED
#endif

#define MAX_TLS_EXTENSIONS 32
#define MAX_CIPHER_SUITES 64
#define MAX_HOSTNAME_LEN 256
#define TLS_RECORD_HEADER_SIZE 5

// TLS version constants
#define TLS_VERSION_1_2 0x0303
#define TLS_VERSION_1_3 0x0304

// Protocol mimicking modes
typedef enum {
    PROTO_MIMIC_BROWSER_HTTPS = 0,  // Look like HTTPS to popular sites
    PROTO_MIMIC_VIDCONFERENCE,      // Look like video conference
    PROTO_MIMIC_STREAMING,          // Look like streaming service
    PROTO_MIMIC_GENERIC_TLS,        // Generic TLS traffic
    PROTO_MIMIC_MOBILE_APP         // Look like mobile app traffic
} protocol_mimic_mode_t;

// TLS fingerprint template structure
typedef struct {
    char *user_agent;                    // Browser-specific user agent
    int tls_version;                     // TLS version to emulate
    int supported_ciphers[MAX_CIPHER_SUITES];  // Browser-specific cipher suites
    int cipher_count;
    int extensions_order[MAX_TLS_EXTENSIONS];    // Order of extensions
    int extension_count;
    int record_size_distribution;        // Size of TLS records
    int handshake_timing;                // Timing between packets (ms)
    char *alpn_protocols;               // ALPN protocols to advertise
    int grease_support;                 // Whether to include GREASE extensions
    int point_formats;                  // Supported point formats
    int elliptic_curves[MAX_TLS_EXTENSIONS]; // Elliptic curves
    int curve_count;
} tls_fingerprint_template_t;

// Timing obfuscation configuration
typedef struct {
    double base_delay_ms;
    double jitter_percentage;  // Percentage of base delay to vary
    int enable_random_padding; // Add random delays occasionally
    int packet_size_variation; // Variation in packet sizes
    int connection_fingerprint; // Unique fingerprint for this connection
} timing_obfuscation_config_t;

// Enhanced domain info with TLS parameters
struct enhanced_domain_info {
    const char *domain;
    struct in_addr target;
    unsigned char target_ipv6[16];
    short server_hello_encrypted_size;
    char use_random_encrypted_size;
    char is_reversed_extension_order;
    
    // Enhanced TLS parameters
    tls_fingerprint_template_t *tls_template;
    timing_obfuscation_config_t *timing_config;
    protocol_mimic_mode_t mimic_mode;
    
    struct enhanced_domain_info *next;
};

// Function prototypes for enhanced TLS functionality
int enhanced_generate_tls_handshake(unsigned char *output, int *output_len, 
                                  const char *domain, protocol_mimic_mode_t mode);

int configure_for_protocol_mimic(connection_job_t C, protocol_mimic_mode_t mode);

int apply_timing_obfuscation(double *delay_ms, timing_obfuscation_config_t *config);

int generate_dynamic_cert(unsigned char *output, int *output_len, 
                         const char *sni_hostname);

int register_enhanced_domain(const char *domain, protocol_mimic_mode_t mode);

int update_enhanced_domain_info(struct enhanced_domain_info *info);

// Enhanced initialization functions
void init_tls_obfuscation_templates(void);

void cleanup_tls_obfuscation_resources(void);

// Browser-specific TLS templates
extern tls_fingerprint_template_t chrome_90_template;
extern tls_fingerprint_template_t firefox_88_template;
extern tls_fingerprint_template_t safari_14_template;
extern tls_fingerprint_template_t edge_90_template;
extern tls_fingerprint_template_t mobile_chrome_template;
extern tls_fingerprint_template_t mobile_safari_template;