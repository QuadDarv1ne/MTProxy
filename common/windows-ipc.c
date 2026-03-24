/*
 * Windows IPC Implementation using Named Pipes
 * 
 * This module provides inter-process communication (IPC) for MTProxy
 * on Windows using Named Pipes. It enables communication between
 * parent and worker processes in lieu of Unix fork()/pipe().
 * 
 * Features:
 * - Named pipe creation and management
 * - Parent-child process communication
 * - Message-based IPC with framing
 * - Timeout handling
 * - Error recovery
 */

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "posix-compat-windows.h"

// Debug flag for IPC tracing
#ifndef IPC_DEBUG
#define IPC_DEBUG 0
#endif

#if IPC_DEBUG
#define IPC_DEBUG(fmt, ...) fprintf(stderr, "[IPC] " fmt "\n", ##__VA_ARGS__)
#else
#define IPC_DEBUG(fmt, ...)
#endif

// Named pipe prefix for MTProxy
#define MTPIPE_PREFIX "\\\\.\\pipe\\mtproxy_"

// IPC message types
typedef enum {
    IPC_MSG_INIT = 0,
    IPC_MSG_STATS = 1,
    IPC_MSG_CONFIG = 2,
    IPC_MSG_COMMAND = 3,
    IPC_MSG_HEARTBEAT = 4,
    IPC_MSG_SHUTDOWN = 5
} ipc_msg_type_t;

// IPC message header
typedef struct {
    uint32_t magic;      // Magic number: 0xMT0001
    uint32_t type;       // Message type
    uint32_t length;     // Payload length
    uint32_t checksum;   // Simple checksum
} ipc_msg_header_t;

#define IPC_MAGIC 0x4D545001  // "MTP" + version

// IPC context for parent-child communication
typedef struct {
    HANDLE pipe_handle;
    char pipe_name[256];
    BOOL is_connected;
    DWORD last_error;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} ipc_context_t;

// Global IPC contexts
static ipc_context_t g_ipc_parent = {0};
static ipc_context_t g_ipc_worker = {0};

// Calculate simple checksum
static inline uint32_t ipc_checksum(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = ((sum << 1) | (sum >> 31)) ^ bytes[i];
    }
    return sum;
}

// Initialize IPC for parent process
int ipc_parent_init(const char *worker_id) {
    char pipe_name[256];
    snprintf(pipe_name, sizeof(pipe_name), "%sworker_%s", MTPIPE_PREFIX, worker_id);
    
    IPC_DEBUG("Creating parent pipe: %s", pipe_name);
    
    HANDLE pipe = CreateNamedPipeA(
        pipe_name,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,  // Output buffer size
        4096,  // Input buffer size
        5000,  // Timeout in ms
        NULL
    );
    
    if (pipe == INVALID_HANDLE_VALUE) {
        IPC_DEBUG("Failed to create pipe: %lu", GetLastError());
        return -1;
    }
    
    g_ipc_parent.pipe_handle = pipe;
    strncpy(g_ipc_parent.pipe_name, pipe_name, sizeof(g_ipc_parent.pipe_name) - 1);
    g_ipc_parent.pipe_name[sizeof(g_ipc_parent.pipe_name) - 1] = '\0';
    g_ipc_parent.is_connected = FALSE;
    
    return 0;
}

// Wait for worker connection (parent side)
int ipc_parent_wait_for_connection(void) {
    IPC_DEBUG("Waiting for worker connection...");
    
    if (!ConnectNamedPipe(g_ipc_parent.pipe_handle, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            IPC_DEBUG("ConnectNamedPipe failed: %lu", err);
            g_ipc_parent.last_error = err;
            return -1;
        }
    }
    
    g_ipc_parent.is_connected = TRUE;
    IPC_DEBUG("Worker connected successfully");
    
    return 0;
}

// Initialize IPC for worker process
int ipc_worker_init(const char *worker_id) {
    char pipe_name[256];
    snprintf(pipe_name, sizeof(pipe_name), "%sworker_%s", MTPIPE_PREFIX, worker_id);
    
    IPC_DEBUG("Connecting to worker pipe: %s", pipe_name);
    
    // Wait for pipe to be available
    for (int i = 0; i < 10; i++) {
        HANDLE pipe = CreateFileA(
            pipe_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (pipe != INVALID_HANDLE_VALUE) {
            g_ipc_worker.pipe_handle = pipe;
            strncpy(g_ipc_worker.pipe_name, pipe_name, sizeof(g_ipc_worker.pipe_name) - 1);
            g_ipc_worker.pipe_name[sizeof(g_ipc_worker.pipe_name) - 1] = '\0';
            g_ipc_worker.is_connected = TRUE;
            IPC_DEBUG("Connected to parent successfully");
            return 0;
        }
        
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_BUSY) {
            IPC_DEBUG("CreateFile failed: %lu", err);
            g_ipc_worker.last_error = err;
            return -1;
        }
        
        // Pipe is busy, wait and retry
        IPC_DEBUG("Pipe busy, waiting... (attempt %d)", i + 1);
        Sleep(100);
    }
    
    IPC_DEBUG("Failed to connect after 10 attempts");
    return -1;
}

// Send IPC message
int ipc_send(ipc_context_t *ctx, ipc_msg_type_t type, const void *data, size_t len) {
    if (!ctx->is_connected) {
        IPC_DEBUG("Not connected, cannot send");
        errno = ENOTCONN;
        return -1;
    }
    
    ipc_msg_header_t header;
    header.magic = IPC_MAGIC;
    header.type = type;
    header.length = (uint32_t)len;
    header.checksum = ipc_checksum(data, len);
    
    // Send header
    DWORD bytes_written;
    if (!WriteFile(ctx->pipe_handle, &header, sizeof(header), &bytes_written, NULL)) {
        IPC_DEBUG("Failed to send header: %lu", GetLastError());
        ctx->last_error = GetLastError();
        return -1;
    }
    
    // Send payload
    if (len > 0) {
        if (!WriteFile(ctx->pipe_handle, data, (DWORD)len, &bytes_written, NULL)) {
            IPC_DEBUG("Failed to send payload: %lu", GetLastError());
            ctx->last_error = GetLastError();
            return -1;
        }
    }
    
    ctx->messages_sent++;
    ctx->bytes_sent += sizeof(header) + len;
    
    IPC_DEBUG("Sent message type=%u, len=%u", type, (unsigned)len);
    return 0;
}

// Receive IPC message
int ipc_recv(ipc_context_t *ctx, ipc_msg_type_t *type, void *buf, size_t buf_size, size_t *out_len) {
    if (!ctx->is_connected) {
        IPC_DEBUG("Not connected, cannot receive");
        errno = ENOTCONN;
        return -1;
    }
    
    ipc_msg_header_t header;
    DWORD bytes_read;
    
    // Receive header
    if (!ReadFile(ctx->pipe_handle, &header, sizeof(header), &bytes_read, NULL)) {
        IPC_DEBUG("Failed to receive header: %lu", GetLastError());
        ctx->last_error = GetLastError();
        return -1;
    }
    
    // Validate header
    if (header.magic != IPC_MAGIC) {
        IPC_DEBUG("Invalid magic: 0x%08X", header.magic);
        errno = EPROTO;
        return -1;
    }
    
    if (header.length > buf_size) {
        IPC_DEBUG("Message too large: %u > %zu", header.length, buf_size);
        errno = EMSGSIZE;
        return -1;
    }
    
    // Receive payload
    if (header.length > 0) {
        if (!ReadFile(ctx->pipe_handle, buf, header.length, &bytes_read, NULL)) {
            IPC_DEBUG("Failed to receive payload: %lu", GetLastError());
            ctx->last_error = GetLastError();
            return -1;
        }
        
        // Validate checksum
        uint32_t calc_checksum = ipc_checksum(buf, header.length);
        if (calc_checksum != header.checksum) {
            IPC_DEBUG("Checksum mismatch: expected 0x%08X, got 0x%08X", 
                     header.checksum, calc_checksum);
            errno = EPROTO;
            return -1;
        }
    }
    
    if (type) *type = (ipc_msg_type_t)header.type;
    if (out_len) *out_len = header.length;
    
    ctx->messages_received++;
    ctx->bytes_received += sizeof(header) + header.length;
    
    IPC_DEBUG("Received message type=%u, len=%u", header.type, header.length);
    return 0;
}

// Send stats from worker to parent
int ipc_worker_send_stats(ipc_context_t *ctx, const void *stats, size_t len) {
    return ipc_send(ctx, IPC_MSG_STATS, stats, len);
}

// Receive stats from worker (parent side)
int ipc_parent_recv_stats(ipc_context_t *ctx, void *buf, size_t buf_size, size_t *out_len) {
    ipc_msg_type_t type;
    return ipc_recv(ctx, &type, buf, buf_size, out_len);
}

// Send command from parent to worker
int ipc_parent_send_command(ipc_context_t *ctx, uint32_t cmd, const void *data, size_t len) {
    // For simplicity, pack cmd into the data
    uint8_t *buffer = (uint8_t *)malloc(len + sizeof(cmd));
    if (!buffer) {
        errno = ENOMEM;
        return -1;
    }
    
    memcpy(buffer, &cmd, sizeof(cmd));
    if (len > 0) {
        memcpy(buffer + sizeof(cmd), data, len);
    }
    
    int ret = ipc_send(ctx, IPC_MSG_COMMAND, buffer, len + sizeof(cmd));
    free(buffer);
    
    return ret;
}

// Receive command from parent (worker side)
int ipc_worker_recv_command(ipc_context_t *ctx, uint32_t *cmd, void *buf, size_t buf_size, size_t *out_len) {
    ipc_msg_type_t type;
    size_t len;
    
    int ret = ipc_recv(ctx, &type, buf, buf_size, &len);
    if (ret < 0) return ret;
    
    if (type != IPC_MSG_COMMAND) {
        IPC_DEBUG("Expected COMMAND, got type=%u", type);
        errno = EPROTO;
        return -1;
    }
    
    if (len < sizeof(uint32_t)) {
        IPC_DEBUG("Command too short: %zu", len);
        errno = EPROTO;
        return -1;
    }
    
    if (cmd) *cmd = *(uint32_t *)buf;
    if (out_len) *out_len = len - sizeof(uint32_t);
    
    // Shift buffer to exclude cmd
    if (out_len && len > sizeof(uint32_t)) {
        memmove(buf, (uint8_t *)buf + sizeof(uint32_t), len - sizeof(uint32_t));
    }
    
    return 0;
}

// Send heartbeat
int ipc_send_heartbeat(ipc_context_t *ctx) {
    uint64_t timestamp = GetTickCount64();
    return ipc_send(ctx, IPC_MSG_HEARTBEAT, &timestamp, sizeof(timestamp));
}

// Close IPC connection
void ipc_close(ipc_context_t *ctx) {
    if (ctx->pipe_handle != INVALID_HANDLE_VALUE) {
        IPC_DEBUG("Closing pipe: %s", ctx->pipe_name);
        DisconnectNamedPipe(ctx->pipe_handle);
        CloseHandle(ctx->pipe_handle);
        ctx->pipe_handle = INVALID_HANDLE_VALUE;
        ctx->is_connected = FALSE;
    }
}

// Cleanup IPC resources
void ipc_cleanup(void) {
    ipc_close(&g_ipc_parent);
    ipc_close(&g_ipc_worker);
    IPC_DEBUG("IPC cleanup completed");
}

// Get IPC statistics
void ipc_get_stats(ipc_context_t *ctx, uint64_t *sent, uint64_t *received, 
                   uint64_t *bytes_sent, uint64_t *bytes_received) {
    if (sent) *sent = ctx->messages_sent;
    if (received) *received = ctx->messages_received;
    if (bytes_sent) *bytes_sent = ctx->bytes_sent;
    if (bytes_received) *bytes_received = ctx->bytes_received;
}

#endif // _WIN32
