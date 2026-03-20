/*
 * http3-quic.h - HTTP/3 (QUIC) Protocol Support for MTProxy
 * 
 * This module provides HTTP/3 over QUIC protocol support for MTProxy
 * to enable next-generation transport with improved performance and security.
 * 
 * Features:
 * - QUIC connection establishment
 * - HTTP/3 request/response handling
 * - Stream multiplexing
 * - 0-RTT connection resumption
 * - Connection migration
 * - Forward error correction
 */

#ifndef HTTP3_QUIC_H
#define HTTP3_QUIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTTP/3 error codes
typedef enum {
    HTTP3_NO_ERROR = 0x0100,
    HTTP3_GENERAL_PROTOCOL_ERROR = 0x0101,
    HTTP3_INTERNAL_ERROR = 0x0102,
    HTTP3_STREAM_CREATION_ERROR = 0x0103,
    HTTP3_CLOSED_CRITICAL_STREAM = 0x0104,
    HTTP3_FRAME_UNEXPECTED_ERROR = 0x0105,
    HTTP3_FRAME_ERROR = 0x0106,
    HTTP3_EXCESSIVE_LOAD = 0x0107,
    HTTP3_ID_ERROR = 0x0108,
    HTTP3_SETTINGS_ERROR = 0x0109,
    HTTP3_MISSING_SETTINGS = 0x010A,
    HTTP3_REQUEST_REJECTED = 0x010B,
    HTTP3_REQUEST_CANCELLED = 0x010C,
    HTTP3_REQUEST_INCOMPLETE = 0x010D,
    HTTP3_MESSAGE_ERROR = 0x010E,
    HTTP3_CONNECT_ERROR = 0x010F,
    HTTP3_VERSION_FALLBACK = 0x0110
} http3_error_code_t;

// QUIC connection state
typedef enum {
    QUIC_STATE_IDLE = 0,
    QUIC_STATE_HANDSHAKE,
    QUIC_STATE_HANDSHAKE_COMPLETE,
    QUIC_STATE_ESTABLISHED,
    QUIC_STATE_CLOSING,
    QUIC_STATE_DRAINING,
    QUIC_STATE_CLOSED
} quic_state_t;

// HTTP/3 stream type
typedef enum {
    HTTP3_STREAM_TYPE_CONTROL = 0,
    HTTP3_STREAM_TYPE_PUSH = 1,
    HTTP3_STREAM_TYPE_QPACK_ENCODE = 2,
    HTTP3_STREAM_TYPE_QPACK_DECODE = 3,
    HTTP3_STREAM_TYPE_REQUEST = 4,
    HTTP3_STREAM_TYPE_RESPONSE = 5
} http3_stream_type_t;

// QUIC connection configuration
typedef struct {
    // Network
    const char* server_name;
    uint16_t port;
    bool ipv6_enabled;
    
    // TLS
    const char* alpn_protocols[4];  // Application-Layer Protocol Negotiation
    int alpn_count;
    bool verify_certificate;
    const char* ca_file;
    const char* cert_file;
    const char* key_file;
    
    // QUIC
    uint64_t max_idle_timeout_ms;
    uint64_t max_udp_payload_size;
    uint64_t initial_max_data;
    uint64_t initial_max_stream_data_bidi_local;
    uint64_t initial_max_stream_data_bidi_remote;
    uint64_t initial_max_stream_data_uni;
    uint64_t initial_max_streams_bidi;
    uint64_t initial_max_streams_uni;
    
    // 0-RTT
    bool enable_0rtt;
    const char* session_file;
    
    // Connection migration
    bool enable_migration;
    
    // Logging
    int log_level;  // 0=none, 1=fatal, 2=error, 3=warning, 4=info, 5=debug
} quic_config_t;

// QUIC connection context
typedef struct quic_connection {
    quic_state_t state;
    uint64_t connection_id;
    uint64_t stream_count;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t start_time;
    uint64_t last_activity;
    char peer_address[64];
    char local_address[64];
    bool is_server;
    bool is_0rtt;
    void* internal_data;  // Internal QUIC library data
} quic_connection_t;

// HTTP/3 request
typedef struct {
    const char* method;
    const char* path;
    const char* authority;
    const char* scheme;
    const char* content_type;
    const uint8_t* body;
    size_t body_length;
    int64_t content_length;
} http3_request_t;

// HTTP/3 response
typedef struct {
    int status_code;
    const char* status_text;
    const char* content_type;
    const uint8_t* body;
    size_t body_length;
    int64_t content_length;
} http3_response_t;

// Callbacks
typedef void (*quic_connection_callback_t)(quic_connection_t* conn, void* user_data);
typedef void (*quic_stream_callback_t)(quic_connection_t* conn, uint64_t stream_id, void* user_data);
typedef void (*quic_data_callback_t)(quic_connection_t* conn, uint64_t stream_id, 
                                     const uint8_t* data, size_t len, bool fin, void* user_data);
typedef void (*quic_error_callback_t)(quic_connection_t* conn, int error_code, 
                                      const char* message, void* user_data);

// HTTP/3 context
typedef struct {
    quic_config_t config;
    quic_connection_callback_t on_connection_established;
    quic_connection_callback_t on_connection_closed;
    quic_stream_callback_t on_stream_created;
    quic_data_callback_t on_data_received;
    quic_error_callback_t on_error;
    void* user_data;
} http3_context_t;

// ============================================================================
// Initialization and cleanup
// ============================================================================

/**
 * Initialize HTTP/3 context
 * @param ctx HTTP/3 context
 * @param config QUIC configuration
 * @return 0 on success, negative on error
 */
int http3_context_init(http3_context_t* ctx, const quic_config_t* config);

/**
 * Cleanup HTTP/3 context
 * @param ctx HTTP/3 context
 */
void http3_context_cleanup(http3_context_t* ctx);

/**
 * Get HTTP/3 library version
 * @return Version string
 */
const char* http3_get_version(void);

// ============================================================================
// Server operations
// ============================================================================

/**
 * Start HTTP/3 server
 * @param ctx HTTP/3 context
 * @param host Host to bind
 * @param port Port to bind
 * @return 0 on success, negative on error
 */
int http3_server_start(http3_context_t* ctx, const char* host, uint16_t port);

/**
 * Stop HTTP/3 server
 * @param ctx HTTP/3 context
 */
void http3_server_stop(http3_context_t* ctx);

/**
 * Process incoming UDP datagram (server)
 * @param ctx HTTP/3 context
 * @param data Datagram data
 * @param len Datagram length
 * @param from_addr Source address
 * @return 0 on success, negative on error
 */
int http3_server_process_datagram(http3_context_t* ctx, const uint8_t* data, 
                                   size_t len, const struct sockaddr* from_addr);

// ============================================================================
// Client operations
// ============================================================================

/**
 * Create QUIC connection (client)
 * @param ctx HTTP/3 context
 * @param server_name Server name
 * @param port Server port
 * @return Connection handle or NULL on error
 */
quic_connection_t* http3_client_connect(http3_context_t* ctx, const char* server_name, uint16_t port);

/**
 * Close QUIC connection
 * @param conn QUIC connection
 * @param app_error_code Application error code
 */
void http3_connection_close(quic_connection_t* conn, uint64_t app_error_code);

/**
 * Get connection state
 * @param conn QUIC connection
 * @return Connection state
 */
quic_state_t http3_connection_get_state(const quic_connection_t* conn);

/**
 * Check if connection is using 0-RTT
 * @param conn QUIC connection
 * @return true if 0-RTT was used
 */
bool http3_connection_is_0rtt(const quic_connection_t* conn);

// ============================================================================
// Stream operations
// ============================================================================

/**
 * Create new bidirectional stream
 * @param conn QUIC connection
 * @return Stream ID or negative on error
 */
int64_t http3_stream_create(quic_connection_t* conn);

/**
 * Send HTTP/3 request
 * @param conn QUIC connection
 * @param stream_id Stream ID
 * @param request HTTP/3 request
 * @return 0 on success, negative on error
 */
int http3_stream_send_request(quic_connection_t* conn, uint64_t stream_id, 
                               const http3_request_t* request);

/**
 * Send HTTP/3 response
 * @param conn QUIC connection
 * @param stream_id Stream ID
 * @param response HTTP/3 response
 * @return 0 on success, negative on error
 */
int http3_stream_send_response(quic_connection_t* conn, uint64_t stream_id,
                                const http3_response_t* response);

/**
 * Send data on stream
 * @param conn QUIC connection
 * @param stream_id Stream ID
 * @param data Data to send
 * @param len Data length
 * @param fin True if this is the last data
 * @return Bytes sent or negative on error
 */
int http3_stream_send_data(quic_connection_t* conn, uint64_t stream_id,
                            const uint8_t* data, size_t len, bool fin);

/**
 * Close stream
 * @param conn QUIC connection
 * @param stream_id Stream ID
 */
void http3_stream_close(quic_connection_t* conn, uint64_t stream_id);

// ============================================================================
// Statistics and monitoring
// ============================================================================

/**
 * Get connection statistics
 * @param conn QUIC connection
 * @param stats_out Output statistics structure
 */
void http3_connection_get_stats(const quic_connection_t* conn, 
                                 quic_connection_t* stats_out);

/**
 * Get number of active streams
 * @param conn QUIC connection
 * @return Number of streams
 */
uint64_t http3_connection_get_stream_count(const quic_connection_t* conn);

/**
 * Get RTT in microseconds
 * @param conn QUIC connection
 * @return RTT in microseconds
 */
uint64_t http3_connection_get_rtt(const quic_connection_t* conn);

// ============================================================================
// Session resumption (0-RTT)
// ============================================================================

/**
 * Save session to file for 0-RTT resumption
 * @param conn QUIC connection
 * @param filename Output file
 * @return 0 on success, negative on error
 */
int http3_session_save(const quic_connection_t* conn, const char* filename);

/**
 * Load session from file for 0-RTT resumption
 * @param ctx HTTP/3 context
 * @param filename Input file
 * @return 0 on success, negative on error
 */
int http3_session_load(http3_context_t* ctx, const char* filename);

// ============================================================================
// Error handling
// ============================================================================

/**
 * Get last error message
 * @return Error message string
 */
const char* http3_get_last_error(void);

/**
 * Convert HTTP/3 error code to string
 * @param error_code HTTP/3 error code
 * @return Error string
 */
const char* http3_error_to_string(http3_error_code_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* HTTP3_QUIC_H */
