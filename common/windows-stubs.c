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

// Forward declarations
typedef struct async_job *connection_job_t;
struct free_later;
struct conn_target_info;
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

// Network stubs
int init_listening_tcpv6_connection(int sock, void *conn_type, void *rpc_methods, int flags) { (void)sock; (void)conn_type; (void)rpc_methods; (void)flags; return -1; }
int server_socket(void *addr, int addr_len, int port, int backlog, int flags) { (void)addr; (void)addr_len; (void)port; (void)backlog; (void)flags; return -1; }
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
  (void)run; (void)flags; (void)job_class; (void)size; (void)have_timer; (void)parent;
  return NULL;
}
void unlock_job(JOB_REF_ARG(job)) { (void)job; }

// POSIX regex stubs
typedef struct { int dummy; } regex_t;
int regcomp(regex_t *preg, const char *regex, int cflags) { (void)preg; (void)regex; (void)cflags; return 0; }
int regexec(const regex_t *preg, const char *string, size_t nmatch, void *pmatch, int eflags) {
  (void)preg; (void)string; (void)nmatch; (void)pmatch; (void)eflags;
  return 0;
}
void regfree(regex_t *preg) { (void)preg; }

// MTProto stubs - only if not already defined
// mtproto_version_manager_* functions are defined in mtproto-version-manager.c
// mtproto_init_connection is defined in mtproto-v3-adapter.c
// mtproto_handshake_versioned, mtproto_encrypt_packet_versioned, etc. are in mtproto-version-manager.c

// Epoll stubs (Linux-specific, not available on Windows)
int epoll_insert(int fd, int flags) { (void)fd; (void)flags; return 0; }
int epoll_work(int timeout) { (void)timeout; return 0; }
int init_epoll(void) { return 0; }

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
void job_free(JOB_REF_ARG(job)) { (void)job; }
void schedule_job(JOB_REF_ARG(job)) { (void)job; }
__thread void *this_job_thread = NULL;
int max_job_thread_id = 0;
void update_all_thread_stats(void) {}
long int drand48_j(void) { return rand(); }
void *job_timer_insert(JOB_REF_ARG(job), double expire) { (void)job; (void)expire; return NULL; }
int job_timer_check(JOB_REF_ARG(job)) { (void)job; return 0; }
void job_timer_remove(JOB_REF_ARG(job)) { (void)job; }
void *job_timer_alloc(double timeout, void (*handler)(void)) {
  (void)timeout; (void)handler; return NULL;
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
  (void)fd; (void)prio; (void)handler; (void)data; return 0;
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
void *Events = NULL;

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
