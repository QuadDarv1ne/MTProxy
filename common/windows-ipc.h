/*
 * Windows IPC Header - Named Pipes for MTProxy
 * 
 * This header provides the interface for Windows IPC using Named Pipes.
 * It enables parent-worker process communication on Windows.
 */

#ifndef WINDOWS_IPC_H
#define WINDOWS_IPC_H

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>
#include <stddef.h>

// IPC message types
typedef enum {
    IPC_MSG_INIT = 0,
    IPC_MSG_STATS = 1,
    IPC_MSG_CONFIG = 2,
    IPC_MSG_COMMAND = 3,
    IPC_MSG_HEARTBEAT = 4,
    IPC_MSG_SHUTDOWN = 5
} ipc_msg_type_t;

// IPC context structure
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

// Initialization functions
int ipc_parent_init(const char *worker_id);
int ipc_parent_wait_for_connection(void);
int ipc_worker_init(const char *worker_id);

// Communication functions
int ipc_send(ipc_context_t *ctx, ipc_msg_type_t type, const void *data, size_t len);
int ipc_recv(ipc_context_t *ctx, ipc_msg_type_t *type, void *buf, size_t buf_size, size_t *out_len);

// Specialized message functions
int ipc_worker_send_stats(ipc_context_t *ctx, const void *stats, size_t len);
int ipc_parent_recv_stats(ipc_context_t *ctx, void *buf, size_t buf_size, size_t *out_len);
int ipc_parent_send_command(ipc_context_t *ctx, uint32_t cmd, const void *data, size_t len);
int ipc_worker_recv_command(ipc_context_t *ctx, uint32_t *cmd, void *buf, size_t buf_size, size_t *out_len);
int ipc_send_heartbeat(ipc_context_t *ctx);

// Cleanup functions
void ipc_close(ipc_context_t *ctx);
void ipc_cleanup(void);

// Statistics
void ipc_get_stats(ipc_context_t *ctx, uint64_t *sent, uint64_t *received, 
                   uint64_t *bytes_sent, uint64_t *bytes_received);

#endif // _WIN32

#endif // WINDOWS_IPC_H
