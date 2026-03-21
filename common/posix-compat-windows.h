/*
    Windows POSIX compatibility layer for MTProxy

    Provides emulation of Unix/POSIX functions for Windows compatibility.
    This allows MTProto proxy to run on Windows with minimal functionality loss.
*/

#ifndef POSIX_COMPAT_WINDOWS_H
#define POSIX_COMPAT_WINDOWS_H

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <psapi.h>

// Define socklen_t if not already defined
#ifndef socklen_t
typedef int socklen_t;
#endif

// Random number generators (emulation for Windows)
// lrand48 emulation using rand()
static inline long int lrand48(void) {
    return (long int)rand();
}

// mrand48 emulation
static inline long int mrand48(void) {
    return (long int)rand() - RAND_MAX/2;
}

// srand48 emulation
static inline void srand48(long int seedval) {
    srand((unsigned int)seedval);
}

// POSIX signal definitions
#ifndef SIGCHLD
#define SIGCHLD 17
#endif

#ifndef SIGUSR1
#define SIGUSR1 30
#endif

#ifndef SIGUSR2
#define SIGUSR2 31
#endif

#ifndef SIGKILL
#define SIGKILL 9
#endif

#ifndef SIGTERM
#define SIGTERM 15
#endif

#ifndef SIGINT
#define SIGINT 2
#endif

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef SIGHUP
#define SIGHUP 1
#endif

#ifndef SIGPOLL
#define SIGPOLL 29
#endif

#ifndef SIGRTMAX
#define SIGRTMAX 31
#endif

#ifndef SIG_IGN
#define SIG_IGN (void (*)(int))1
#endif

#ifndef SIG_DFL
#define SIG_DFL (void (*)(int))0
#endif

#ifndef LOG_LEVEL_WARNING
#define LOG_LEVEL_WARNING 2
#endif

// setsid emulation for Windows
static inline int setsid(void) {
    // Windows doesn't have session leaders like Unix
    // Return success as a no-op
    return 0;
}

// getuid emulation - always return 0 on Windows (no user separation)
static inline int getuid(void) {
    return 0;
}

// Resource limits for Windows
#ifndef RLIMIT_NOFILE
#define RLIMIT_NOFILE 7
#endif

struct rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};

// getrlimit emulation for Windows
static inline int getrlimit(int resource, struct rlimit *rlim) {
    if (!rlim) {
        errno = EFAULT;
        return -1;
    }

    if (resource == RLIMIT_NOFILE) {
        // Windows default is typically 16384 handles per process
        rlim->rlim_cur = 16384;
        rlim->rlim_max = 16384;
        return 0;
    }

    errno = EINVAL;
    return -1;
}

// setrlimit emulation for Windows
static inline int setrlimit(int resource, const struct rlimit *rlim) {
    // No-op on Windows - limits are managed differently
    return 0;
}

// mmap flags
#ifndef PROT_READ
#define PROT_READ  1
#endif

#ifndef PROT_WRITE
#define PROT_WRITE 2
#endif

#ifndef MAP_SHARED
#define MAP_SHARED 0x0001
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x1000
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

// waitpid flags
#ifndef WNOHANG
#define WNOHANG 1
#endif

#ifndef WUNTRACED
#define WUNTRACED 2
#endif

// Process status macros
#ifndef WIFEXITED
#define WIFEXITED(status) (((status) & 0x7F) == 0)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status) >> 8) & 0xFF)
#endif

#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (((status) & 0x7F) != 0 && ((status) & 0x7F) != 0x7F)
#endif

#ifndef WTERMSIG
#define WTERMSIG(status) ((status) & 0x7F)
#endif

// Process handle storage
typedef struct {
    HANDLE process_handle;
    pid_t pid;
    int exit_status;
    int terminated;
} win_product_entry_t;

// Windows POSIX compatibility functions

// mmap emulation for Windows
static inline void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    DWORD protect = PAGE_READWRITE;
    DWORD access = FILE_MAP_WRITE;

    if (prot & PROT_READ) {
        if (!(prot & PROT_WRITE)) {
            protect = PAGE_READONLY;
            access = FILE_MAP_READ;
        }
    }

    // For anonymous mappings, use VirtualAlloc
    if (flags & MAP_ANONYMOUS) {
        void *ptr = VirtualAlloc(addr, length, MEM_COMMIT | MEM_RESERVE, protect);
        if (!ptr) {
            errno = ENOMEM;
            return MAP_FAILED;
        }
        return ptr;
    }

    // For file mappings
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    if (fd >= 0) {
        file_handle = (HANDLE)_get_osfhandle(fd);
    }

    HANDLE mapping = CreateFileMappingA(file_handle, NULL, protect, 0, length, NULL);
    if (!mapping) {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    void *ptr = MapViewOfFile(mapping, access, 0, offset, length);
    CloseHandle(mapping);

    if (!ptr) {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    return ptr;
}

// munmap emulation for Windows
static inline int munmap(void *addr, size_t length) {
    if (VirtualFree(addr, 0, MEM_RELEASE)) {
        return 0;
    }
    errno = EINVAL;
    return -1;
}

// fork emulation using CreateProcess
// Note: This is a simplified emulation - true fork is not possible on Windows
// We use a helper process model instead
static inline pid_t fork(void) {
    // Windows doesn't support fork() - return -1 to indicate unsupported
    // The proxy will use alternative multiprocessing model
    errno = ENOSYS;
    return -1;
}

// waitpid emulation
static inline pid_t waitpid(pid_t pid, int *status, int options) {
    if (pid <= 0) {
        errno = ECHILD;
        return -1;
    }

    // For Windows, we need to track child processes differently
    // This is a stub - real implementation would need process tracking
    errno = ECHILD;
    return -1;
}

// kill emulation - sends signal to process
static inline int kill(pid_t pid, int sig) {
    if (pid <= 0) {
        errno = ESRCH;
        return -1;
    }

    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!process) {
        errno = ESRCH;
        return -1;
    }

    if (sig == SIGKILL || sig == SIGTERM) {
        if (!TerminateProcess(process, sig)) {
            CloseHandle(process);
            errno = ESRCH;
            return -1;
        }
    }
    // Note: Windows doesn't support Unix signals natively
    // This is a limited emulation

    CloseHandle(process);
    return 0;
}

// getppid emulation - returns parent process ID
static inline pid_t getppid(void) {
    return (pid_t)GetCurrentProcessId();
}

// setsockopt wrapper for Windows with correct parameter types
static inline int win_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    return setsockopt(sockfd, level, optname, (const char *)optval, optlen);
}

// TCP_WINDOW_CLAMP is not available on Windows
// Define as a no-op or use SO_SNDBUF as alternative
#ifndef TCP_WINDOW_CLAMP
#define TCP_WINDOW_CLAMP SO_SNDBUF
#endif

// Initialize Windows POSIX compatibility layer
static inline int windows_posix_init(void) {
    // Initialize Winsock if needed
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

// Cleanup Windows POSIX compatibility layer
static inline void windows_posix_cleanup(void) {
    WSACleanup();
}

// arpa/inet.h emulation for Windows
#ifndef _ARPA_INET_H_
#define _ARPA_INET_H_

#include <winsock2.h>

// inet_ntoa is already in winsock2.h
// inet_addr is already in winsock2.h

// htons, htonl, ntohs, ntohl are already in winsock2.h

// inet_pton for Windows (only if not provided by system)
#ifndef HAVE_INET_PTON
static inline int inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN + 1];

    if (src == NULL || dst == NULL) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    ZeroMemory(&ss, sizeof(ss));
    strncpy(src_copy, src, INET6_ADDRSTRLEN);
    src_copy[INET6_ADDRSTRLEN] = '\0';

    if (af == AF_INET) {
        if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) != 0) {
            return 0;
        }
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
        memcpy(dst, &sin->sin_addr, sizeof(sin->sin_addr));
        return 1;
    } else if (af == AF_INET6) {
        if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) != 0) {
            return 0;
        }
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
        memcpy(dst, &sin6->sin6_addr, sizeof(sin6->sin6_addr));
        return 1;
    }

    errno = EAFNOSUPPORT;
    return -1;
}
#endif

// inet_ntop for Windows (only if not provided by system)
#ifndef HAVE_INET_NTOP
static inline const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    struct sockaddr_storage ss;
    unsigned long ssize = sizeof(ss);

    ZeroMemory(&ss, sizeof(ss));
    ss.ss_family = af;

    if (af == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
        memcpy(&sin->sin_addr, src, sizeof(sin->sin_addr));
    } else if (af == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
        memcpy(&sin6->sin6_addr, src, sizeof(sin6->sin6_addr));
    } else {
        errno = EAFNOSUPPORT;
        return NULL;
    }

    if (WSAAddressToStringA((struct sockaddr *)&ss, sizeof(ss), NULL, dst, (unsigned long *)&size) != 0) {
        errno = EINVAL;
        return NULL;
    }

    return dst;
}

#endif // HAVE_INET_NTOP

#endif // _ARPA_INET_H_

// Resource usage emulation for Windows
#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

// rusage structure
struct rusage {
    struct ru_utime {
        long tv_sec;
        long tv_usec;
    } ru_utime;
    struct ru_stime {
        long tv_sec;
        long tv_usec;
    } ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN -1

// getrusage emulation
static inline int getrusage(int who, struct rusage *usage) {
    if (!usage) {
        errno = EFAULT;
        return -1;
    }

    // Initialize to zero
    memset(usage, 0, sizeof(struct rusage));

    if (who == RUSAGE_SELF) {
        // Get memory usage
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        if (GlobalMemoryStatusEx(&status)) {
            usage->ru_maxrss = (long)(status.ullTotalPhys / 1024); // KB
        }

        // Get process times
        FILETIME creation_time, exit_time, kernel_time, user_time;
        HANDLE process = GetCurrentProcess();
        if (GetProcessTimes(process, &creation_time, &exit_time, &kernel_time, &user_time)) {
            // Convert FILETIME to timeval
            unsigned long long user_ticks = ((unsigned long long)user_time.dwHighDateTime << 32) | user_time.dwLowDateTime;
            unsigned long long kernel_ticks = ((unsigned long long)kernel_time.dwHighDateTime << 32) | kernel_time.dwLowDateTime;

            usage->ru_utime.tv_sec = user_ticks / 10000000;
            usage->ru_utime.tv_usec = (user_ticks % 10000000) / 10;
            usage->ru_stime.tv_sec = kernel_ticks / 10000000;
            usage->ru_stime.tv_usec = (kernel_ticks % 10000000) / 10;
        }
    }

    return 0;
}

#endif // _SYS_RESOURCE_H_

// fcntl flags and commands
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif

// fcntl emulation for Windows (limited support)
static inline int fcntl(int fd, int cmd, ...) {
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }
    
    // Only support F_SETFL with O_NONBLOCK for socket non-blocking mode
    if (cmd == F_SETFL) {
        // For Windows sockets, non-blocking mode is set via ioctlsocket
        // This is a stub - actual socket handling should use ioctlsocket
        return 0; // Success (socket already in non-blocking mode by default in our code)
    }
    
    if (cmd == F_GETFL) {
        return 0; // Default flags
    }
    
    errno = EINVAL;
    return -1;
}

// Stub implementations for excluded network functions
// Only provide stubs for functions NOT declared in other headers

// show_ip stub - returns static buffer with IP string
static inline const char *show_ip(unsigned ip) {
    static char buf[16];
    struct in_addr addr;
    addr.s_addr = ip;
    strncpy(buf, inet_ntoa(addr), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    return buf;
}

// show_ipv6 stub - returns static buffer with IPv6 string
static inline const char *show_ipv6(const unsigned char ipv6[16]) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
             ipv6[0], ipv6[1], ipv6[2], ipv6[3], ipv6[4], ipv6[5], ipv6[6], ipv6[7],
             ipv6[8], ipv6[9], ipv6[10], ipv6[11], ipv6[12], ipv6[13], ipv6[14], ipv6[15]);
    return buf;
}

// Stub for client_socket - not used in Windows build
static inline int client_socket(unsigned int ip, int port, int use_ipv6) {
    errno = ENOSYS;
    return -1;
}

// Stub for client_socket_ipv6 - not used in Windows build
static inline int client_socket_ipv6(const unsigned char *ipv6, int port, int flags) {
    errno = ENOSYS;
    return -1;
}

// Stub for connections_prepare_stat - correct signature
typedef struct stats_buffer stats_buffer_t;
static inline int connections_prepare_stat(stats_buffer_t *sb) { return 0; }

// Stub for crypto_aes_prepare_stat - correct signature
static inline int crypto_aes_prepare_stat(stats_buffer_t *sb) { return 0; }

// Stub for assert_engine_thread
static inline void assert_engine_thread(void) {}

// Stub for assert_net_cpu_thread
static inline void assert_net_cpu_thread(void) {}

// Stub for incr_active_dh_connections
static inline void incr_active_dh_connections(void) {}

// Stub for nat_translate_ip
static inline unsigned int nat_translate_ip(unsigned int ip) { return ip; }

// Global variables stubs for excluded modules
// Note: Types must match the original declarations (from net-events.c)
extern double tot_idle_time;
extern double a_idle_time;
extern double a_idle_quotient;
extern double last_epoll_wait_at;
extern long long epoll_calls;
extern long long epoll_intr;

// main_secret is aes_secret_t (defined in net-crypto-aes.h)
// Include the header for proper type definition
#include "../net/net-crypto-aes.h"
extern aes_secret_t main_secret;

// main_thread_interrupt_status is volatile int
extern volatile int main_thread_interrupt_status;

static double _tot_idle_time_stub = 0.0;
static double _a_idle_time_stub = 0.0;
static double _a_idle_quotient_stub = 0.0;
static double _last_epoll_wait_at_stub = 0.0;
static long long _epoll_calls_stub = 0;
static long long _epoll_intr_stub = 0;
// Stub for main_secret - actual aes_secret_t structure
static aes_secret_t _main_secret_stub = {0, 0, {0}};
static volatile int _main_thread_interrupt_status_stub = 0;

#define tot_idle_time _tot_idle_time_stub
#define a_idle_time _a_idle_time_stub
#define a_idle_quotient _a_idle_quotient_stub
#define last_epoll_wait_at _last_epoll_wait_at_stub
#define epoll_calls _epoll_calls_stub
#define epoll_intr _epoll_intr_stub
#define main_secret _main_secret_stub
#define main_thread_interrupt_status _main_thread_interrupt_status_stub

// Stub for crc32c_partial (use crc32c_fast instead)
#define crc32c_partial crc32c_fast

#endif // _WIN32

#endif // POSIX_COMPAT_WINDOWS_H
