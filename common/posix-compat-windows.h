/*
    Windows POSIX compatibility layer for MTProxy
    
    Provides emulation of Unix/POSIX functions for Windows compatibility.
    This allows MTProto proxy to run on Windows with minimal functionality loss.
*/

#ifndef POSIX_COMPAT_WINDOWS_H
#define POSIX_COMPAT_WINDOWS_H

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

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
} win_process_entry_t;

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

// usleep emulation
static inline int usleep(useconds_t useconds) {
    if (useconds >= 1000) {
        Sleep(useconds / 1000);
    } else if (useconds > 0) {
        Sleep(1);
    }
    return 0;
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

// getppid emulation
static inline pid_t getppid(void) {
    return (pid_t)_getppid();
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

#endif // _WIN32

#endif // POSIX_COMPAT_WINDOWS_H
