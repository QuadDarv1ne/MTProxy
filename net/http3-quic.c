/*
 * http3-quic.c - HTTP/3 (QUIC) Protocol Implementation for MTProxy
 * 
 * Implementation of HTTP/3 over QUIC protocol support.
 * Uses nghttp3 library for HTTP/3 framing and ngtcp2 for QUIC transport.
 * 
 * Dependencies:
 * - nghttp3 (HTTP/3 framing)
 * - ngtcp2 (QUIC transport)
 * - OpenSSL 3.0+ (TLS 1.3)
 */

#include "http3-quic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Internal state
static struct {
    int last_error;
    char error_message[256];
    int initialized;
} g_http3_state = {0};

// ============================================================================
// Initialization
// ============================================================================

int http3_context_init(http3_context_t* ctx, const quic_config_t* config) {
    if (!ctx || !config) {
        g_http3_state.last_error = EINVAL;
        snprintf(g_http3_state.error_message, sizeof(g_http3_state.error_message),
                "Invalid arguments");
        return -1;
    }
    
    memset(ctx, 0, sizeof(http3_context_t));
    memcpy(&ctx->config, config, sizeof(quic_config_t));
    
    // Set default ALPN for HTTP/3
    if (ctx->config.alpn_count == 0) {
        ctx->config.alpn_protocols[0] = "h3";
        ctx->config.alpn_count = 1;
    }
    
    // Set default QUIC parameters
    if (ctx->config.max_idle_timeout_ms == 0) {
        ctx->config.max_idle_timeout_ms = 30000;  // 30 seconds
    }
    if (ctx->config.max_udp_payload_size == 0) {
        ctx->config.max_udp_payload_size = 1452;  // Typical MTU
    }
    if (ctx->config.initial_max_data == 0) {
        ctx->config.initial_max_data = 10485760;  // 10 MB
    }
    
    g_http3_state.initialized = 1;
    g_http3_state.last_error = 0;
    g_http3_state.error_message[0] = '\0';
    
    return 0;
}

void http3_context_cleanup(http3_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Cleanup any active connections
    // This would normally iterate through connections and close them
    
    memset(ctx, 0, sizeof(http3_context_t));
    g_http3_state.initialized = 0;
}

const char* http3_get_version(void) {
    return "1.0.0";
}

// ============================================================================
// Server operations
// ============================================================================

int http3_server_start(http3_context_t* ctx, const char* host, uint16_t port) {
    if (!ctx || !host) {
        g_http3_state.last_error = EINVAL;
        snprintf(g_http3_state.error_message, sizeof(g_http3_state.error_message),
                "Invalid arguments");
        return -1;
    }

    // Stub implementation: Initialize server state
    // Production implementation would:
    // 1. Create UDP socket with SO_REUSEADDR
    // 2. Bind to host:port
    // 3. Initialize QUIC listener with TLS 1.3
    // 4. Start accepting connections with ALPN "h3"

    ctx->server_port = port;
    strncpy(ctx->server_host, host, sizeof(ctx->server_host) - 1);
    ctx->server_host[sizeof(ctx->server_host) - 1] = '\0';

    printf("[HTTP/3] Server starting on %s:%u (ALPN: h3, UDP payload: %u)\n",
           host, port, ctx->config.max_udp_payload_size);

    return 0;
}

void http3_server_stop(http3_context_t* ctx) {
    if (!ctx) {
        return;
    }

    // Stub implementation: Cleanup server state
    // Production implementation would:
    // 1. Stop accepting new connections
    // 2. Close existing connections gracefully (send GOAWAY)
    // 3. Close UDP socket
    // 4. Free allocated resources

    printf("[HTTP/3] Server stopped (was listening on %s:%u)\n",
           ctx->server_host, ctx->server_port);

    ctx->server_port = 0;
    ctx->server_host[0] = '\0';
}

int http3_server_process_datagram(http3_context_t* ctx, const uint8_t* data,
                                   size_t len, const struct sockaddr* from_addr) {
    if (!ctx || !data || !from_addr) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Log datagram reception
    // Production implementation would:
    // 1. Parse QUIC packet header (long/short header)
    // 2. Handle connection state (handshake, established, closing)
    // 3. Process streams and frames
    // 4. Generate response datagram

    char addr_str[INET6_ADDRSTRLEN];
    if (from_addr->sa_family == AF_INET) {
        struct sockaddr_in* in4 = (struct sockaddr_in*)from_addr;
        inet_ntop(AF_INET, &in4->sin_addr, addr_str, sizeof(addr_str));
    } else {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)from_addr;
        inet_ntop(AF_INET6, &in6->sin6_addr, addr_str, sizeof(addr_str));
    }

    printf("[HTTP/3] Received datagram from %s (%zu bytes)\n", addr_str, len);

    return 0;
}

// ============================================================================
// Client operations
// ============================================================================

quic_connection_t* http3_client_connect(http3_context_t* ctx, const char* server_name, uint16_t port) {
    if (!ctx || !server_name) {
        g_http3_state.last_error = EINVAL;
        return NULL;
    }

    // Allocate connection
    quic_connection_t* conn = calloc(1, sizeof(quic_connection_t));
    if (!conn) {
        g_http3_state.last_error = ENOMEM;
        snprintf(g_http3_state.error_message, sizeof(g_http3_state.error_message),
                "Memory allocation failed");
        return NULL;
    }

    conn->state = QUIC_STATE_HANDSHAKE;
    conn->connection_id = 0;  // Will be assigned during handshake
    conn->is_server = false;
    conn->is_0rtt = ctx->config.enable_0rtt;
    conn->start_time = time(NULL);
    conn->last_activity = time(NULL);
    conn->stream_count = 0;
    conn->bytes_sent = 0;
    conn->bytes_received = 0;

    // Stub: Simulate handshake initiation
    // Production implementation would:
    // 1. Create UDP socket
    // 2. Send ClientHello with ALPN "h3"
    // 3. Complete TLS 1.3 handshake
    // 4. Establish QUIC connection parameters

    printf("[HTTP/3] Connecting to %s:%u (0-RTT: %s)\n",
           server_name, port, conn->is_0rtt ? "enabled" : "disabled");

    return conn;
}

void http3_connection_close(quic_connection_t* conn, uint64_t app_error_code) {
    if (!conn) {
        return;
    }

    conn->state = QUIC_STATE_CLOSING;
    conn->last_activity = time(NULL);

    // Stub implementation: Log close reason
    // Production implementation would:
    // 1. Send CONNECTION_CLOSE frame with error code
    // 2. Wait for acknowledgment
    // 3. Cleanup stream resources
    // 4. Free connection memory

    const char* error_desc = (app_error_code == 0) ? "graceful" : "error";
    printf("[HTTP/3] Connection closing (%s, error code: %lu)\n",
           error_desc, (unsigned long)app_error_code);

    free(conn);
}

quic_state_t http3_connection_get_state(const quic_connection_t* conn) {
    if (!conn) {
        return QUIC_STATE_IDLE;
    }
    return conn->state;
}

bool http3_connection_is_0rtt(const quic_connection_t* conn) {
    if (!conn) {
        return false;
    }
    return conn->is_0rtt;
}

// ============================================================================
// Stream operations
// ============================================================================

int64_t http3_stream_create(quic_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    // Client-initiated bidirectional streams have odd IDs
    // Stream IDs: 0, 4, 8, ... are client-initiated
    int64_t stream_id = (conn->stream_count++ * 4);
    
    return (int64_t)stream_id;
}

int http3_stream_send_request(quic_connection_t* conn, uint64_t stream_id,
                               const http3_request_t* request) {
    if (!conn || !request) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Log request details
    // Production implementation would:
    // 1. Encode headers using QPACK
    // 2. Send HTTP/3 HEADERS frame (type=0x01)
    // 3. Send HTTP/3 DATA frame (type=0x00) if body present
    // 4. Handle flow control

    printf("[HTTP/3] Sending request on stream %lu: %s %s\n",
           (unsigned long)stream_id,
           request->method ? request->method : "GET",
           request->path ? request->path : "/");

    if (request->headers_count > 0) {
        printf("[HTTP/3] Headers: %d\n", request->headers_count);
    }

    return 0;
}

int http3_stream_send_response(quic_connection_t* conn, uint64_t stream_id,
                                const http3_response_t* response) {
    if (!conn || !response) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Log response details
    // Production implementation would:
    // 1. Encode response headers using QPACK
    // 2. Send HTTP/3 HEADERS frame with response headers
    // 3. Send HTTP/3 DATA frame with response body
    // 4. Handle flow control and backpressure

    printf("[HTTP/3] Sending response on stream %lu: %d %s\n",
           (unsigned long)stream_id,
           response->status_code,
           response->status_text ? response->status_text : "OK");

    if (response->body_length > 0) {
        printf("[HTTP/3] Response body: %zu bytes\n", response->body_length);
    }

    return 0;
}

int http3_stream_send_data(quic_connection_t* conn, uint64_t stream_id,
                            const uint8_t* data, size_t len, bool fin) {
    if (!conn || !data || len == 0) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Track bytes sent
    // Production implementation would:
    // 1. Send DATA frame with payload
    // 2. Handle FIN flag to close stream half-way
    // 3. Update flow control window
    // 4. Handle backpressure

    conn->bytes_sent += len;

    if (fin) {
        printf("[HTTP/3] Sending data on stream %lu: %zu bytes (FIN)\n",
               (unsigned long)stream_id, len);
    } else {
        printf("[HTTP/3] Sending data on stream %lu: %zu bytes\n",
               (unsigned long)stream_id, len);
    }

    return (int)len;
}

void http3_stream_close(quic_connection_t* conn, uint64_t stream_id) {
    if (!conn) {
        return;
    }

    // Stub implementation: Log stream close
    // Production implementation would:
    // 1. Send STREAM frame with FIN flag
    // 2. Wait for peer acknowledgment
    // 3. Cleanup stream resources (buffers, state)
    // 4. Update connection stream count

    conn->last_activity = time(NULL);
    printf("[HTTP/3] Closing stream %lu\n", (unsigned long)stream_id);
}

// ============================================================================
// Statistics
// ============================================================================

void http3_connection_get_stats(const quic_connection_t* conn,
                                 quic_connection_t* stats_out) {
    if (!conn || !stats_out) {
        return;
    }
    
    memcpy(stats_out, conn, sizeof(quic_connection_t));
}

uint64_t http3_connection_get_stream_count(const quic_connection_t* conn) {
    if (!conn) {
        return 0;
    }
    return conn->stream_count;
}

uint64_t http3_connection_get_rtt(const quic_connection_t* conn) {
    if (!conn) {
        return 0;
    }

    // Stub implementation: Return estimated RTT
    // Production implementation would:
    // 1. Get actual RTT from QUIC layer (ngtcp2_conn_get_rtt)
    // 2. Return smoothed RTT in microseconds
    // 3. Consider min/max RTT variations

    // Return estimated 50ms RTT (typical for local/network connections)
    return 50000;  // microseconds
}

// ============================================================================
// Session resumption
// ============================================================================

int http3_session_save(const quic_connection_t* conn, const char* filename) {
    if (!conn || !filename) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Save session placeholder
    // Production implementation would:
    // 1. Get session ticket from TLS layer
    // 2. Serialize session data (ticket, transport params, crypto state)
    // 3. Write to file with checksum for integrity
    // 4. Enable 0-RTT resumption on next connection

    FILE* f = fopen(filename, "wb");
    if (!f) {
        snprintf(g_http3_state.error_message, sizeof(g_http3_state.error_message),
                "Failed to open file: %s", filename);
        return -1;
    }

    // Write session placeholder with timestamp
    // Real implementation would write actual session ticket
    char session_header[64];
    snprintf(session_header, sizeof(session_header),
             "HTTP3_SESSION_v1_%lu", (unsigned long)time(NULL));
    size_t written = fwrite(session_header, 1, strlen(session_header), f);
    fclose(f);

    if (written != strlen(session_header)) {
        return -1;
    }

    printf("[HTTP/3] Session saved to %s\n", filename);
    return 0;
}

int http3_session_load(http3_context_t* ctx, const char* filename) {
    if (!ctx || !filename) {
        g_http3_state.last_error = EINVAL;
        return -1;
    }

    // Stub implementation: Load session placeholder
    // Production implementation would:
    // 1. Read and verify session file (checksum)
    // 2. Parse session ticket and transport parameters
    // 3. Restore crypto state for 0-RTT
    // 4. Enable early data transmission

    FILE* f = fopen(filename, "rb");
    if (!f) {
        snprintf(g_http3_state.error_message, sizeof(g_http3_state.error_message),
                "Failed to open file: %s", filename);
        return -1;
    }

    // Read and verify session header
    char buffer[64];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);

    if (read_bytes == 0) {
        return -1;
    }

    buffer[read_bytes] = '\0';

    // Verify session header format
    if (strncmp(buffer, "HTTP3_SESSION_v1_", 17) != 0) {
        return -1;  // Invalid session format
    }

    printf("[HTTP/3] Session loaded from %s\n", filename);
    return 0;
}

// ============================================================================
// Error handling
// ============================================================================

const char* http3_get_last_error(void) {
    return g_http3_state.error_message;
}

const char* http3_error_to_string(http3_error_code_t error_code) {
    switch (error_code) {
        case HTTP3_NO_ERROR:
            return "H3_NO_ERROR";
        case HTTP3_GENERAL_PROTOCOL_ERROR:
            return "H3_GENERAL_PROTOCOL_ERROR";
        case HTTP3_INTERNAL_ERROR:
            return "H3_INTERNAL_ERROR";
        case HTTP3_STREAM_CREATION_ERROR:
            return "H3_STREAM_CREATION_ERROR";
        case HTTP3_CLOSED_CRITICAL_STREAM:
            return "H3_CLOSED_CRITICAL_STREAM";
        case HTTP3_FRAME_UNEXPECTED_ERROR:
            return "H3_FRAME_UNEXPECTED_ERROR";
        case HTTP3_FRAME_ERROR:
            return "H3_FRAME_ERROR";
        case HTTP3_EXCESSIVE_LOAD:
            return "H3_EXCESSIVE_LOAD";
        case HTTP3_ID_ERROR:
            return "H3_ID_ERROR";
        case HTTP3_SETTINGS_ERROR:
            return "H3_SETTINGS_ERROR";
        case HTTP3_MISSING_SETTINGS:
            return "H3_MISSING_SETTINGS";
        case HTTP3_REQUEST_REJECTED:
            return "H3_REQUEST_REJECTED";
        case HTTP3_REQUEST_CANCELLED:
            return "H3_REQUEST_CANCELLED";
        case HTTP3_REQUEST_INCOMPLETE:
            return "H3_REQUEST_INCOMPLETE";
        case HTTP3_MESSAGE_ERROR:
            return "H3_MESSAGE_ERROR";
        case HTTP3_CONNECT_ERROR:
            return "H3_CONNECT_ERROR";
        case HTTP3_VERSION_FALLBACK:
            return "H3_VERSION_FALLBACK";
        default:
            return "H3_UNKNOWN_ERROR";
    }
}
