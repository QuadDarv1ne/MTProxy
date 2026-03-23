/*
    Windows stubs for MTProxy
    Provides stub implementations for functions not available on Windows
*/

#ifdef _WIN32

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Socket mode flags from net-events.h
#define SM_UDP 1
#define SM_IPV6 2
#define SM_IPV6_ONLY 4
#define SM_LOWPRIO 8
#define SM_REUSE 16
#define SM_SPECIAL 0x10000
#define SM_NOQACK 0x20000
#define SM_RAWMSG 0x40000

// Forward declarations
typedef struct async_job *connection_job_t;
struct free_later;
struct conn_target_info;

// Job structure definition
struct job_base {
    int j_refcnt;
    int j_flags;
    int j_error;
    void *j_custom;
    int j_custom_bytes;
};

typedef struct job_base *job_t;
#define JOB_REF_ARG(__name) int __name ## _tag_int, job_t __name

// Global variables stubs
double tot_idle_time = 0.0;
double a_idle_time = 0.0;
double a_idle_quotient = 0.0;
volatile int main_thread_interrupt_status = 0;
int new_conn_generation = 0;
int tcp_maximize_buffers = 0;
int ev_heap_size = 0;
double last_epoll_wait_at = 0.0;
long long epoll_calls = 0;
long long epoll_intr = 0;
int active_special_connections = 0;
int max_special_connections = 0;
double epoll_sleep_ns = 0.0;

// Connection stubs
connection_job_t connection_get_by_fd_generation(int fd, int gen) { (void)fd; (void)gen; return NULL; }
connection_job_t connection_get_by_fd(int fd) { (void)fd; return NULL; }
void connection_write_close(connection_job_t c) { (void)c; }
void fail_connection(connection_job_t C, int err) { (void)C; (void)err; }
int set_connection_timeout(connection_job_t C, double timeout) { (void)C; (void)timeout; return 0; }
int clear_connection_timeout(connection_job_t C) { (void)C; return 0; }

// Target stubs
void *create_target(void) { return NULL; }
void destroy_target(void *target) { (void)target; }
int create_target_from_info(struct conn_target_info *info, int *generation) { (void)info; (void)generation; return 0; }

// Stats stubs
void fetch_connections_stat(void *sb) { (void)sb; }
// fetch_aes_crypto_stat is defined in net-crypto-aes.c for Windows

// Windows socket implementation
static int wsa_initialized = 0;

int init_wsa(void) {
    if (wsa_initialized) return 0;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return -1;
    }
    wsa_initialized = 1;
    return 0;
}

int new_socket_windows(int mode, int nonblock) {
    if (init_wsa() != 0) return -1;

    SOCKET socket_fd;
    int family = (mode & SM_IPV6) ? AF_INET6 : AF_INET;
    int type = (mode & SM_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int protocol = (mode & SM_UDP) ? IPPROTO_UDP : IPPROTO_TCP;

    socket_fd = WSASocket(family, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket_fd == INVALID_SOCKET) {
        printf("WSASocket failed: %d\n", WSAGetLastError());
        return -1;
    }

    if (mode & SM_IPV6) {
        DWORD ipv6only = (mode & SM_IPV6_ONLY) ? 1 : 0;
        if (setsockopt(socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only)) == SOCKET_ERROR) {
            printf("Setting IPV6_V6ONLY failed: %d\n", WSAGetLastError());
            closesocket(socket_fd);
            return -1;
        }
    }

    if (nonblock) {
        u_long mode_nonblock = 1;
        if (ioctlsocket(socket_fd, FIONBIO, &mode_nonblock) == SOCKET_ERROR) {
            printf("Setting non-blocking mode failed: %d\n", WSAGetLastError());
            closesocket(socket_fd);
            return -1;
        }
    }

    return (int)socket_fd;
}

int server_socket(int port, struct in_addr in_addr, int backlog, int mode) {
    int socket_fd = new_socket_windows(mode, 1);
    if (socket_fd == -1) return -1;

    // Set socket options
    BOOL reuse = TRUE;
    if (!(mode & SM_UDP)) {
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

        BOOL nodelay = TRUE;
        setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));

        BOOL keepalive = TRUE;
        setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepalive, sizeof(keepalive));
    }

    if (mode & SM_REUSE) {
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    }

    // Bind socket
    if (!(mode & SM_IPV6)) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr = in_addr;

        if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            printf("bind() failed: %d\n", WSAGetLastError());
            closesocket(socket_fd);
            return -1;
        }
    } else {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);
        addr.sin6_addr = in6addr_any;

        if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            printf("bind() IPv6 failed: %d\n", WSAGetLastError());
            closesocket(socket_fd);
            return -1;
        }
    }

    // Listen for TCP sockets
    if (!(mode & SM_UDP) && listen(socket_fd, backlog) == SOCKET_ERROR) {
        printf("listen() failed: %d\n", WSAGetLastError());
        closesocket(socket_fd);
        return -1;
    }

    printf("Windows server socket created: fd=%d, port=%d, mode=0x%x\n", socket_fd, port, mode);
    return socket_fd;
}

// Network stubs
int init_listening_tcpv6_connection(int sock, void *conn_type, void *rpc_methods, int flags) {
    printf("init_listening_tcpv6_connection: sock=%d, flags=0x%x\n", sock, flags);
    // For now, just return success - the socket is already created and listening
    return 0;
}
void cpu_server_close_connection(connection_job_t c, int err) { (void)c; (void)err; }

// Display stubs
const char *show_ip(unsigned int ip) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (ip >> 0) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
  return buf;
}
void show_ip_print(unsigned int ip) {
  // Print IP address for debugging
  printf("%u.%u.%u.%u", (ip >> 0) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}
const char *show_ipv6(const unsigned char ipv6[16]) { (void)ipv6; return ""; }

// Utility stubs
void assert_engine_thread(void) {}
void assert_net_cpu_thread(void) {}
void incr_active_dh_connections(void) {}
unsigned int nat_translate_ip(unsigned int ip) { return ip; }
void insert_free_later_struct(struct free_later *ptr) { (void)ptr; }
void free_later_act(void) {}

// Server stubs
void server_failed(connection_job_t c) { (void)c; }
void server_noop(connection_job_t c) { (void)c; }
int server_check_ready(connection_job_t c) { (void)c; return 0; }
int net_accept_new_connections(void) { return 0; }

// Jobs stubs
void job_signal(JOB_REF_ARG(job), int signo) { (void)job; (void)signo; }
job_t job_incref(job_t job) { return job; }
void job_decref(JOB_REF_ARG(job)) { (void)job; }
void jobs_prepare_stat(void *st) { (void)st; }
long int lrand48_j(void) { return rand(); }
job_t create_async_job(int (*run)(job_t, int, void*), int flags, int job_class, int size, int have_timer, job_t parent) {
  (void)run; (void)flags; (void)job_class; (void)have_timer; (void)parent;

  // Allocate job structure with custom data
  int total_size = sizeof(struct job_base) + size;
  struct job_base *job = (struct job_base *)calloc(1, total_size);
  if (!job) {
    return NULL;
  }

  job->j_refcnt = 1;
  job->j_flags = flags;
  job->j_error = 0;
  job->j_custom = (size > 0) ? (void *)(job + 1) : NULL;
  job->j_custom_bytes = size;

  return (job_t)job;
}
void unlock_job(JOB_REF_ARG(job)) { (void)job; }

// POSIX regex stubs
typedef struct { int dummy; } regex_t;
int regcomp(regex_t *preg, const char *regex, int cflags) {
    (void)preg; (void)regex; (void)cflags;
    return 0;
}
int regexec(const regex_t *preg, const char *string, size_t nmatch, void *pmatch, int eflags) {
    (void)preg; (void)string; (void)nmatch; (void)pmatch; (void)eflags;
    return 0;
}
void regfree(regex_t *preg) {
    (void)preg;
}

// Network socket stubs
int client_socket(unsigned int ip, int port, int use_ipv6) {
    (void)ip; (void)port; (void)use_ipv6;
    return -1;
}

int client_socket_ipv6(const unsigned char ipv6[16], int port, int flags) {
    (void)ipv6; (void)port; (void)flags;
    return -1;
}

// MTProto stubs - only if not already defined
// mtproto_version_manager_* functions are defined in mtproto-version-manager.c
// mtproto_init_connection is defined in mtproto-v3-adapter.c
// mtproto_handshake_versioned, mtproto_encrypt_packet_versioned, etc. are in mtproto-version-manager.c

// Windows epoll emulation using select()
// This provides basic event loop functionality for Windows

#include <winsock2.h>
#include <time.h>

// Event system for Windows
#define MAX_EVENTS 4096

typedef struct event_descr event_t;
typedef int (*event_handler_t)(int fd, void *data, event_t *ev);

struct event_descr {
  int fd;
  int state;
  int ready;
  int epoll_state;
  int epoll_ready;
  int timeout;
  int priority;
  int in_queue;
  long long timestamp;
  long long refcnt;
  event_handler_t work;
  void *data;
};

event_t Events[MAX_EVENTS];

static fd_set win_read_fds, win_write_fds, win_except_fds;
static int win_max_fd = 0;
static int win_epoll_initialized = 0;

// Event flags from net-events.h
#define EVT_READ    4
#define EVT_WRITE   2
#define EVT_SPEC    1

int init_epoll(void) {
    if (win_epoll_initialized) {
        return 1;
    }

    // Initialize Events array - mark all as unused
    for (int i = 0; i < MAX_EVENTS; i++) {
        Events[i].fd = -1;
        Events[i].work = NULL;
        Events[i].data = NULL;
    }

    FD_ZERO(&win_read_fds);
    FD_ZERO(&win_write_fds);
    FD_ZERO(&win_except_fds);
    win_max_fd = 0;
    win_epoll_initialized = 1;
    return 1;
}

int epoll_insert(int fd, int flags) {
    if (!win_epoll_initialized) {
        init_epoll();
    }

    if (fd < 0) return -1;

    // Clear fd from all sets first
    FD_CLR(fd, &win_read_fds);
    FD_CLR(fd, &win_write_fds);
    FD_CLR(fd, &win_except_fds);

    // Add to appropriate sets based on flags
    if (flags & EVT_READ) {
        FD_SET(fd, &win_read_fds);
    }
    if (flags & EVT_WRITE) {
        FD_SET(fd, &win_write_fds);
    }
    if (flags & EVT_SPEC) {
        FD_SET(fd, &win_except_fds);
    }

    // Update max fd
    if (fd > win_max_fd) {
        win_max_fd = fd;
    }

    return 0;
}

int epoll_remove(int fd) {
    if (fd < 0) return -1;

    FD_CLR(fd, &win_read_fds);
    FD_CLR(fd, &win_write_fds);
    FD_CLR(fd, &win_except_fds);

    return 0;
}

int epoll_work(int timeout) {
    if (!win_epoll_initialized) {
        init_epoll();
    }

    // Check if any file descriptors are registered
    if (win_read_fds.fd_count == 0 && win_write_fds.fd_count == 0 && win_except_fds.fd_count == 0) {
        // No fds to monitor, just sleep
        if (timeout > 0) {
            Sleep(timeout < 100 ? timeout : 100);
        }
        return 0;
    }

    // Windows select() implementation
    fd_set read_fds = win_read_fds;
    fd_set write_fds = win_write_fds;
    fd_set except_fds = win_except_fds;

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    int result = select(0, &read_fds, &write_fds, &except_fds,
                       timeout >= 0 ? &tv : NULL);

    if (result == SOCKET_ERROR) {
        printf("select() failed: %d\n", WSAGetLastError());
        return -1;
    }

    if (result == 0) {
        // Timeout, no events
        return 0;
    }

    // Process ready file descriptors and call handlers
    int events_processed = 0;

    for (int i = 0; i < MAX_EVENTS && events_processed < result; i++) {
        event_t *ev = &Events[i];

        // Skip uninitialized entries
        if (ev->fd < 0 || ev->work == NULL) {
            continue;
        }

        int fd = ev->fd;
        int ready_flags = 0;

        // Check if this fd is ready
        if (FD_ISSET(fd, &read_fds)) {
            ready_flags |= EVT_READ;
        }
        if (FD_ISSET(fd, &write_fds)) {
            ready_flags |= EVT_WRITE;
        }
        if (FD_ISSET(fd, &except_fds)) {
            ready_flags |= EVT_SPEC;
        }

        if (ready_flags == 0) {
            continue;
        }

        events_processed++;
        ev->ready = ready_flags;

        // Call the event handler
        event_handler_t handler = ev->work;
        void *handler_data = ev->data;

        if (handler != NULL) {
            int handler_result = handler(fd, handler_data, ev);

            // Handle return values
            if (Events[i].fd == fd) {
                if (handler_result == -3) { // EVA_REMOVE
                    epoll_remove(fd);
                    Events[i].fd = -1;
                    Events[i].work = NULL;
                } else if (handler_result < 0 && handler_result != -2) { // Error (not EVA_RERUN)
                    epoll_remove(fd);
                    Events[i].fd = -1;
                    Events[i].work = NULL;
                }
            }
        }
    }

    return result;
}

// Notification event stubs
void notification_event_insert_tcp_conn_close(void *c) { (void)c; }
void notification_event_insert_tcp_conn_wakeup(void *c) { (void)c; }
void notification_event_insert_tcp_conn_alarm(void *c) { (void)c; }
void notification_event_insert_tcp_conn_ready(void *c) { (void)c; }

// CRC32C stub
unsigned crc32c_partial(const void *data, long len, unsigned crc) {
  (void)data; (void)len; (void)crc;
  return 0;
}

// Additional jobs stubs
void *register_thread_callback(const char *name, void (*callback)(void)) {
  (void)name; (void)callback; return NULL;
}
void job_free(JOB_REF_ARG(job)) {
  (void)job_tag_int;
  if (job) {
    free(job);
  }
}
void schedule_job(JOB_REF_ARG(job)) { (void)job; }
__thread void *this_job_thread = NULL;
int max_job_thread_id = 0;
void update_all_thread_stats(void) {}
long int drand48_j(void) { return rand(); }
void *job_timer_insert(JOB_REF_ARG(job), double expire) { (void)job; (void)expire; return NULL; }
int job_timer_check(JOB_REF_ARG(job)) { (void)job; return 0; }
void job_timer_remove(JOB_REF_ARG(job)) { (void)job; }
void *job_timer_alloc(double timeout, void (*handler)(void)) {
  (void)timeout; (void)handler;

  // Allocate a minimal timer structure
  struct job_base *timer = (struct job_base *)calloc(1, sizeof(struct job_base));
  if (timer) {
    timer->j_refcnt = 1;
  }
  return timer;
}
void run_pending_main_jobs(void) {}
void *alloc_timer_manager(void) { return NULL; }
void notification_event_job_create(void *event) { (void)event; }
void *create_new_job_class(int class_id, const char *name, int size) {
  (void)class_id; (void)name; (void)size; return NULL;
}
void init_async_jobs(void) {}
void insert_job_into_job_list(void *job, int flags) { (void)job; (void)flags; }

// Network stubs
unsigned int get_my_ipv4(void) { return 0x7F000001; } // 127.0.0.1
int get_my_ipv6(unsigned char ipv6[16]) {
  if (ipv6) memset(ipv6, 0, 16);
  return 0;
}
int epoll_sethandler(int fd, int prio, void *handler, void *data) {
  if (fd < 0) {
    return -1;
  }

  // On Windows, socket handles can be large values, not small integers
  // So we can't use fd as array index directly. Instead, find a free slot.
  event_t *ev = NULL;

  // First, check if this fd is already registered
  for (int i = 0; i < MAX_EVENTS; i++) {
    if (Events[i].fd == fd) {
      ev = &Events[i];
      break;
    }
  }

  // If not found, find a free slot
  if (ev == NULL) {
    for (int i = 0; i < MAX_EVENTS; i++) {
      if (Events[i].fd < 0) {
        ev = &Events[i];
        break;
      }
    }
  }

  if (ev == NULL) {
    // No free slots
    return -1;
  }

  // Initialize event if needed
  if (ev->fd != fd) {
    memset(ev, 0, sizeof(*ev));
    ev->fd = fd;
  }

  ev->priority = prio;
  ev->work = (event_handler_t)handler;
  ev->data = data;
  ev->refcnt = 1;

  return 0;
}

// Usage stub - defined in mtproto-proxy.c
// void usage(void) {}

// Admin/Engine stats stubs
int net_get_active_connections(void) { return 0; }
int net_get_total_connections(void) { return 0; }
long long net_get_bytes_read(void) { return 0; }
long long net_get_bytes_written(void) { return 0; }
long long net_get_rejected_connections(void) { return 0; }
size_t get_memory_used(void) { return 0; }
size_t get_memory_allocated(void) { return 0; }
time_t engine_get_start_time(void) { return 0; }
int engine_get_workers_count(void) { return 1; }
const char *get_commit_hash(void) { return COMMIT; }
void *rate_limiter_get_stats_json(void *limiter, char *buf, int size) {
  (void)limiter; if (buf && size > 0) buf[0] = '\0'; return buf;
}

// FFI API stubs - only if src/mtproxy.c is not included
// Most FFI functions are defined in src/mtproxy.c
// These are stubs for functions that might be missing
int mtproxy_init(void);  // Defined in src/mtproxy.c
int mtproxy_start(void); // Defined in src/mtproxy.c

// Global variable stubs
void *ct_tcp_rpc_ext_server = NULL;

// Usage stub - required for libmtproxy.dll (weak symbol doesn't work in DLL)
#ifdef BUILD_SHARED_LIB
void usage(void) {}
#endif

// Shadowsocks stubs
int shadowsocks_set_config(void *config) { (void)config; return 0; }
int shadowsocks_detect_protocol(void *data, int len) { (void)data; (void)len; return 0; }
int shadowsocks_init_connection(void *conn) { (void)conn; return 0; }
int shadowsocks_encrypt_data(void *conn, void *in, void *out, int len) { (void)conn; (void)in; (void)out; (void)len; return len; }
int shadowsocks_decrypt_data(void *conn, void *in, void *out, int len) { (void)conn; (void)in; (void)out; (void)len; return len; }

// Config stubs
int tcp_rpc_init_proxy_domains(void) { return 0; }
void create_all_outbound_connections(void) {}
int tcp_rpc_add_proxy_domain(const char *domain, int port) { (void)domain; (void)port; return 0; }
int tcp_rpcs_set_ext_secret(void *secret) { (void)secret; return 0; }
void tcp_set_max_accept_rate(int rate) { (void)rate; }
void net_add_nat_info(unsigned int ip, unsigned int mask) { (void)ip; (void)mask; }
void tcp_set_max_connections(int max) { (void)max; }

#endif /* _WIN32 */
