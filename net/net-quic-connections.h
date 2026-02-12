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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for types defined elsewhere
typedef struct job_base *connection_job_t;
typedef struct job_base *conn_target_job_t;

// QUIC/HTTP3 connection types and structures

typedef enum {
  QUIC_VERSION_1 = 0x00000001,
  QUIC_VERSION_2 = 0x6b3343cf,
} quic_version_t;

typedef enum {
  QUIC_STREAM_TYPE_UNIDIRECTIONAL = 0,
  QUIC_STREAM_TYPE_BIDIRECTIONAL = 1,
} quic_stream_type_t;

// QUIC connection state
typedef enum {
  QUIC_STATE_IDLE,
  QUIC_STATE_SERVER_BUSY,
  QUIC_STATE_ACTIVE,
  QUIC_STATE_CLOSING,
  QUIC_STATE_DRAINING,
  QUIC_STATE_TERMINATED
} quic_state_t;

// QUIC connection context structure
typedef struct quic_connection_ctx {
  // Generic connection info
  void *base;  // Placeholder - will be cast to struct connection_info in implementation
  
  // QUIC-specific fields
  quic_state_t state;
  uint64_t connection_id;
  uint64_t peer_connection_id;
  uint32_t version;
  
  // Encryption context
  void *encryption_ctx;
  void *decryption_ctx;
  
  // Stream management
  void *stream_table;  // Hash table or similar for stream management
  
  // Congestion control
  uint64_t congestion_window;
  uint64_t bytes_in_flight;
  uint64_t smoothed_rtt;
  uint64_t rttvar;
  
  // Flow control
  uint64_t max_data;
  uint64_t data_sent;
  uint64_t data_received;
  
  // Connection lifetime
  double creation_time;
  double last_activity_time;
} quic_connection_ctx_t;

// Function prototypes for QUIC connection handling
int init_quic_connection(void);
connection_job_t create_quic_connection(conn_target_job_t target, struct in_addr addr, int port);
int quic_connection_send_data(connection_job_t conn, const void *data, int len);
int quic_connection_receive_data(connection_job_t conn, void *data, int max_len);
int quic_handle_packet(const void *packet, int len, struct in_addr addr, int port);
void quic_cleanup_connection(connection_job_t conn);

#ifdef __cplusplus
}
#endif