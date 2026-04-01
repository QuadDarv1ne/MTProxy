/*
    Windows POSIX compatibility layer for MTProxy

    Provides emulation of Unix/POSIX functions for Windows compatibility.
    This allows MTProto proxy to run on Windows with minimal functionality loss.
*/

#ifndef POSIX_COMPAT_WINDOWS_H
#define POSIX_COMPAT_WINDOWS_H

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
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

// Fake sys/socket.h for Windows (winsock2.h already included above)
#ifdef _MSC_VER
#define _SYS_SOCKET_H_
#endif
#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H
// winsock2.h already provides socket functionality
#endif

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

/*
 * Windows Named Pipes IPC for multi-worker mode
 * 
 * This implementation provides IPC communication between parent and worker
 * processes using Windows Named Pipes as a substitute for Unix pipes.
 */

// Named Pipe configuration
#define MTPIPE_BUFFER_SIZE 4096
#define MTPIPE_MAX_INSTANCES 255
#define MTPIPE_TIMEOUT_MS 5000

// Pipe handle storage for IPC
typedef struct {
    HANDLE pipe_handle;
    BOOL is_server;
    BOOL connected;
    char pipe_name[256];
} win_pipe_t;

// Global pipe storage for parent-child communication
#define MAX_PIPES 64
static win_pipe_t win_pipes[MAX_PIPES];
static int win_pipes_count = 0;

// Create a named pipe for IPC
static inline int win_pipe_create(char *name, BOOL is_server) {
    if (win_pipes_count >= MAX_PIPES) {
        errno = EMFILE;
        return -1;
    }

    win_pipe_t *pipe = &win_pipes[win_pipes_count];
    pipe->is_server = is_server;
    pipe->connected = FALSE;
    strncpy(pipe->pipe_name, name, sizeof(pipe->pipe_name) - 1);
    pipe->pipe_name[sizeof(pipe->pipe_name) - 1] = '\0';

    if (is_server) {
        pipe->pipe_handle = CreateNamedPipeA(
            name,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            MTPIPE_BUFFER_SIZE,
            MTPIPE_BUFFER_SIZE,
            MTPIPE_TIMEOUT_MS,
            NULL
        );
    } else {
        pipe->pipe_handle = CreateFileA(
            name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (pipe->pipe_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    return win_pipes_count++;
}

// Connect to a named pipe (server side)
static inline int win_pipe_connect(HANDLE pipe_handle) {
    if (!ConnectNamedPipe(pipe_handle, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            return -1;
        }
    }
    return 0;
}

// Read from named pipe
static inline int win_pipe_read(HANDLE pipe_handle, void *buf, size_t len) {
    DWORD bytes_read;
    if (!ReadFile(pipe_handle, buf, (DWORD)len, &bytes_read, NULL)) {
        return -1;
    }
    return (int)bytes_read;
}

// Write to named pipe
static inline int win_pipe_write(HANDLE pipe_handle, const void *buf, size_t len) {
    DWORD bytes_written;
    if (!WriteFile(pipe_handle, buf, (DWORD)len, &bytes_written, NULL)) {
        return -1;
    }
    return (int)bytes_written;
}

// Close named pipe
static inline int win_pipe_close(HANDLE pipe_handle) {
    if (pipe_handle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(pipe_handle);
        CloseHandle(pipe_handle);
        return 0;
    }
    return -1;
}

// fork emulation using CreateProcess with Named Pipes for IPC
// Note: This is a simplified emulation - true fork is not possible on Windows
// We use a helper process model with Named Pipes for parent-child communication
static inline pid_t fork(void) {
    // Windows doesn't support fork() - return -1 to indicate unsupported
    // The proxy will use alternative multiprocessing model
    // 
    // For multi-worker mode on Windows, we use CreateProcess with Named Pipes
    // for IPC communication between parent and worker processes.
    // 
    // However, MTProto proxy's fork usage is primarily for creating worker
    // processes that share memory via mmap. On Windows, we can't truly share
    // memory between processes without using Windows-specific APIs.
    // 
    // Current approach: Use single-worker mode on Windows (workers = 0)
    // See mtproto-proxy.c line 2394 for the implementation.
    
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

// inet_pton and inet_ntop are provided by ws2tcpip.h on Windows
// We use InetPtonA and InetNtopA which are available in modern Windows
#define inet_pton InetPtonA
#define inet_ntop(af, src, dst, size) InetNtopA(af, src, dst, size)

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
// Memory alignment functions
#ifndef _STDLIB_H
#include <malloc.h>
#endif

static inline int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (!memptr || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return EINVAL;
    }
    *memptr = _aligned_malloc(size, alignment);
    return (*memptr == NULL) ? ENOMEM : 0;
}

#define aligned_free _aligned_free

// Only provide stubs for functions NOT declared in other headers
// Note: show_ip, show_ipv6, assert_engine_thread, etc. are in windows-stubs.c
// Note: client_socket and client_socket_ipv6 are declared in net-events.h

// /dev/random and /dev/urandom emulation for Windows
// Uses CryptGenRandom for cryptographically secure random numbers
#ifndef _CRYPTUIAPI_H_
#include <wincrypt.h>
#endif

static inline int windows_get_random(void *buf, size_t len) {
    HCRYPTPROV hCryptProv = 0;
    BOOL result = FALSE;

    if (len == 0) return 0;

    // Acquire cryptographic context
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        return -1;
    }

    // Generate random bytes
    result = CryptGenRandom(hCryptProv, (DWORD)len, (BYTE *)buf);

    // Release cryptographic context
    CryptReleaseContext(hCryptProv, 0);

    return result ? (int)len : -1;
}

// Stub for /dev/random file descriptor
#define DEV_RANDOM_FD_MAGIC 0xDEADBEEF
#define DEV_URANDOM_FD_MAGIC 0xCAFEBABE

static inline int open_dev_random(const char *pathname, int flags) {
    if (strcmp(pathname, "/dev/random") == 0 ||
        strcmp(pathname, "/dev/urandom") == 0) {
        // Return magic fd for random device
        return strcmp(pathname, "/dev/random") == 0 ?
               DEV_RANDOM_FD_MAGIC : DEV_URANDOM_FD_MAGIC;
    }
    errno = ENOENT;
    return -1;
}

static inline int read_dev_random(int fd, void *buf, size_t count) {
    if (fd == DEV_RANDOM_FD_MAGIC || fd == DEV_URANDOM_FD_MAGIC) {
        return windows_get_random(buf, count);
    }
    errno = EBADF;
    return -1;
}

static inline int close_dev_random(int fd) {
    if (fd == DEV_RANDOM_FD_MAGIC || fd == DEV_URANDOM_FD_MAGIC) {
        return 0;
    }
    errno = EBADF;
    return -1;
}

// Redefine open/read/close for /dev/random on Windows
#ifdef open
#undef open
#endif
#ifdef read
#undef read
#endif
#ifdef close
#undef close
#endif

// Helper macros for open with variable arguments
#define _OPEN_2_ARGS(pathname, flags) \
    (strcmp((pathname), "/dev/random") == 0 || strcmp((pathname), "/dev/urandom") == 0 ? \
     open_dev_random((pathname), (flags)) : _open((pathname), (flags), 0))
#define _OPEN_3_ARGS(pathname, flags, mode) \
    (strcmp((pathname), "/dev/random") == 0 || strcmp((pathname), "/dev/urandom") == 0 ? \
     open_dev_random((pathname), (flags)) : _open((pathname), (flags), (mode)))
#define _GET_OPEN_MACRO(_1, _2, _3, NAME, ...) NAME

#define open(...) _GET_OPEN_MACRO(__VA_ARGS__, _OPEN_3_ARGS, _OPEN_2_ARGS)(__VA_ARGS__)

#define read(fd, buf, count) \
    ((fd) == DEV_RANDOM_FD_MAGIC || (fd) == DEV_URANDOM_FD_MAGIC ? \
     read_dev_random((fd), (buf), (count)) : \
     _read((fd), (buf), (count)))

#define close(fd) \
    ((fd) == DEV_RANDOM_FD_MAGIC || (fd) == DEV_URANDOM_FD_MAGIC ? \
     close_dev_random(fd) : \
     _close(fd))

// O_NONBLOCK for Windows (already defined in winsock2.h for sockets)
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x0004
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif

// Stub for connections_prepare_stat - correct signature
typedef struct stats_buffer stats_buffer_t;
static inline int connections_prepare_stat(stats_buffer_t *sb) { return 0; }

// crypto_aes_prepare_stat is defined in net-crypto-aes.c via MODULE_STAT_FUNCTION
// Don't define stub here to avoid redefinition error

// Note: assert_engine_thread, assert_net_cpu_thread, incr_active_dh_connections,
//       nat_translate_ip are defined in windows-stubs.c

// Additional Windows stubs for missing functions
// Only stub functions that are truly missing on Windows
// Don't stub functions declared in net-connections.h, net-events.h etc. - they have real implementations

// Use crc32c_fast on Windows (crc32c_partial is in crc32c.c)
#ifndef crc32c_partial
#define crc32c_partial crc32c_fast
#endif

// Global variables and stub functions are now defined in windows-stubs.c

#endif // _WIN32

#endif // POSIX_COMPAT_WINDOWS_H
