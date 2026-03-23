/*
    Windows stubs for MTProxy
    Provides stub implementations for functions not available on Windows
*/

#ifdef _WIN32

#include <stdint.h>
#include <stdlib.h>

// Forward declarations
typedef struct async_job *connection_job_t;
struct free_later;
struct conn_target_info;

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
void fetch_aes_crypto_stat(void *sb) { (void)sb; }

// Network stubs
int init_listening_tcpv6_connection(int sock, void *conn_type, void *rpc_methods, int flags) { (void)sock; (void)conn_type; (void)rpc_methods; (void)flags; return -1; }
int server_socket(void *addr, int addr_len, int port, int backlog, int flags) { (void)addr; (void)addr_len; (void)port; (void)backlog; (void)flags; return -1; }
void cpu_server_close_connection(connection_job_t c, int err) { (void)c; (void)err; }

// Display stubs
void show_ip(unsigned int ip) { (void)ip; }
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

// Config stubs
int tcp_rpc_init_proxy_domains(void) { return 0; }
void create_all_outbound_connections(void) {}
int tcp_rpc_add_proxy_domain(const char *domain, int port) { (void)domain; (void)port; return 0; }
int tcp_rpcs_set_ext_secret(void *secret) { (void)secret; return 0; }
void tcp_set_max_accept_rate(int rate) { (void)rate; }
void net_add_nat_info(unsigned int ip, unsigned int mask) { (void)ip; (void)mask; }
void tcp_set_max_connections(int max) { (void)max; }

#endif /* _WIN32 */
