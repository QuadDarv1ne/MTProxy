#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <time.h>
    
    // Windows compatibility definitions
    #define fsync _commit
    #define PIPE_BUF 4096
    
    // pwrite stub for Windows (not fully implemented)
    static inline ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
        // Simple stub - just returns success
        return count;
    }
    
    // Time functions compatibility
    static inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
        if (localtime_s(result, timep) == 0) {
            return result;
        }
        return NULL;
    }
    
    // CPUID compatibility
    #include <intrin.h>
    static inline void __get_cpuid(unsigned int leaf, unsigned int* eax, 
                                  unsigned int* ebx, unsigned int* ecx, unsigned int* edx) {
        int cpu_info[4];
        __cpuid(cpu_info, leaf);
        *eax = cpu_info[0];
        *ebx = cpu_info[1];
        *ecx = cpu_info[2];
        *edx = cpu_info[3];
    }
    
#else
    // Unix/Linux includes
    #include <unistd.h>
    #include <time.h>
    #include <cpuid.h>
    
    #define fsync fsync
    #define pwrite pwrite
    #ifndef PIPE_BUF
        #define PIPE_BUF 4096
    #endif
    
#endif

#endif // PLATFORM_COMPAT_H