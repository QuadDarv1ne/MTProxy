cmake_minimum_required(VERSION 3.10)
project(MTProxy VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    message(STATUS "Detected Windows platform")
    add_definitions(-D_WIN32)
    set(PLATFORM_LIBS ws2_32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=0x0600")
elseif(UNIX)
    message(STATUS "Detected Unix/Linux platform")
    set(PLATFORM_LIBS m rt)
endif()

# Compiler-specific options
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Base optimization flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wno-array-bounds -fno-strict-aliasing -fno-strict-overflow -fwrapv -DAES=1 -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64")
    
    # Architecture-specific optimizations
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpclmul -march=native -mfpmath=sse -mssse3")
    
    # Advanced optimization flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffast-math -funroll-loops -ftree-vectorize")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native")
    
    # Profile-guided optimization support
    option(ENABLE_PGO "Enable Profile-Guided Optimization" OFF)
    if(ENABLE_PGO)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-generate")
        message(STATUS "PGO enabled - building with profiling instrumentation")
    endif()
    
    # Link-time optimization
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto=auto")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto=auto")
    
    # Security-specific flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -D_FORTIFY_SOURCE=2")
    
    # Debug flags for development
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG=1")
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -DNDEBUG")
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DNDEBUG")
    else()
        # Release build - maximum optimization
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native")
    endif()
    
    # Additional optimization flags for specific architectures
    include(CheckCCompilerFlag)
    CHECK_C_COMPILER_FLAG("-mavx2" HAS_AVX2)
    if(HAS_AVX2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
    endif()
    
    CHECK_C_COMPILER_FLAG("-mavx512f" HAS_AVX512)
    if(HAS_AVX512)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx512f")
    endif()
    
    # Enable link-time garbage collection
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
    
    # Static linking options
    option(STATIC_LINKING "Enable static linking" OFF)
    if(STATIC_LINKING)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
    
    # Memory optimization flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
    
    # Vectorization hints
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
    
    # Cache optimization
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -freorder-blocks-algorithm=stc")
    
    # Branch prediction optimization
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-correction")
    
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall /D_WIN32_WINNT=0x0600")
    
    # MSVC optimization flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /GL /Gy /Gw")
    
    # Security-specific flags for MSVC
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS /guard:cf")
    
    # Whole program optimization
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    
    # Debug flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi /Od /DDEBUG=1")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /DNDEBUG")
    endif()
endif()

# Define COMMIT hash (similar to Makefile)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(NOT GIT_COMMIT_HASH)
        set(GIT_COMMIT_HASH "unknown")
    endif()
else()
    set(GIT_COMMIT_HASH "unknown")
endif()

add_definitions(-DCOMMIT="${GIT_COMMIT_HASH}")

# Include directories
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/common)
include_directories(${PROJECT_SOURCE_DIR}/security)

# Define source files
set(COMMON_SOURCES
    common/crc32.c
    common/crc32.h
    common/cpuid.c
    common/cpuid.h
    common/kprintf.c
    common/kprintf.h
    common/md5.c
    common/md5.h
    common/mp-queue.c
    common/mp-queue.h
    common/parse-config.c
    common/parse-config.h
    common/pid.c
    common/pid.h
    common/precise-time.c
    common/precise-time.h
    common/proc-stat.c
    common/proc-stat.h
    common/resolver.c
    common/resolver.h
    common/rpc-const.h
    common/server-functions.c
    common/server-functions.h
    common/sha1.c
    common/sha1.h
    common/sha256.c
    common/sha256.h
    common/tl-parse.c
    common/tl-parse.h
    common/config-manager.c
    common/config-manager.h
    common/runtime-tuner.c
    common/runtime-tuner.h
    common/structured-logger.c
    common/structured-logger.h
    common/log-aggregator.c
    common/log-aggregator.h
    common/advanced-logger.c
    common/advanced-logger.h
    common/platform_network.c
    common/platform_network.h
    common/platform_compat.h
)

set(JOBS_SOURCES
    jobs/jobs.c
    jobs/jobs.h
)

set(CRYPTO_SOURCES
    crypto/aesni256.c
    crypto/aesni256.h
    crypto/aes-optimized.c
    crypto/aes-optimized.h
    crypto/dh-optimized.c
    crypto/dh-optimized.h
    crypto/crypto-optimizer.c
    crypto/crypto-optimizer.h
    crypto/vectorized-crypto.c
    crypto/vectorized-crypto.h
)

# NET_SOURCES - все сетевые модули (объединено)
set(NET_SOURCES
    # Расширенные функции
    net/shadowsocks-advanced.c
    net/shadowsocks-advanced.h
    net/pluggable-transports.c
    net/pluggable-transports.h
    net/network-profiler.c
    net/network-profiler.h
    net/network-analyzer.c
    net/network-analyzer.h
    # Основные сетевые модули
    net/net-config.c
    net/net-config.h
    net/net-connections.c
    net/net-connections.h
    net/net-crypto-aes.c
    net/net-crypto-aes.h
    net/net-crypto-dh.c
    net/net-crypto-dh.h
    net/net-events.c
    net/net-events.h
    net/net-http-server.c
    net/net-http-server.h
    net/net-msg-buffers.c
    net/net-msg-buffers.h
    net/net-msg.c
    net/net-msg.h
    net/net-rpc-targets.c
    net/net-rpc-targets.h
    net/net-stats.c
    net/net-tcp-connections.c
    net/net-tcp-connections.h
    net/net-tcp-rpc-client.c
    net/net-tcp-rpc-client.h
    net/net-tcp-rpc-common.c
    net/net-tcp-rpc-common.h
    net/net-tcp-rpc-ext-server.c
    net/net-tcp-rpc-ext-server.h
    net/net-tcp-rpc-server.c
    net/net-tcp-rpc-server.h
    net/net-thread.c
    net/net-thread.h
    net/net-timers.c
    net/net-timers.h
)

# SECURITY_SOURCES - все модули безопасности (объединено)
set(SECURITY_SOURCES
    security/modular-security.c
    security/modular-security.h
    security/simple-security.c
    security/simple-security.h
    security/security-manager.c
    security/security-manager.h
    security/ddos-protection.c
    security/ddos-protection.h
    security/cert-pinning.c
    security/cert-pinning.h
    security/security-utils.c
    security/security-utils.h
    common/mtproxy-fixes-simple.h
)

set(ENGINE_SOURCES
    engine/engine.c
    engine/engine.h
    engine/engine-net.c
    engine/engine-net.h
    engine/engine-rpc.c
    engine/engine-rpc.h
    engine/engine-rpc-common.c
    engine/engine-rpc-common.h
    engine/engine-signals.c
    engine/engine-signals.h
)

set(SYSTEM_SOURCES
    system/performance-optimizer.c
    system/performance-optimizer.h
    system/optimizer-integration.c
    system/optimizer-integration.h
    system/simple-performance-optimizer.c
    system/simple-performance-optimizer.h
    system/memory-optimization.c
    system/memory-optimization.h
    system/connection-optimizer.c
    system/connection-optimizer.h
    system/advanced-cache.c
    system/advanced-cache.h
    system/memory-manager.c
    system/memory-manager.h
    system/distributed-monitor.c
    system/distributed-monitor.h
    system/threat-detector.c
    system/threat-detector.h
    system/auto-scaler.c
    system/auto-scaler.h
)

set(ADVANCED_OPTIMIZATION_SOURCES
    system/numa-allocator.c
    system/numa-allocator.h
    system/io-uring-interface.c
    system/io-uring-interface.h
    system/dpdk-interface.c
    system/dpdk-interface.h
    system/advanced-optimizer.c
    system/advanced-optimizer.h
)

set(PERF_MONITOR_SOURCES
    perf_monitor/advanced-metrics.c
    perf_monitor/advanced-metrics.h
    perf_monitor/advanced-monitoring.c
    perf_monitor/advanced-monitoring.h
    perf_monitor/simple-monitoring.c
    perf_monitor/simple-monitoring.h
    perf_monitor/extended-monitoring.c
    perf_monitor/extended-monitoring.h
    perf_monitor/perf-monitor.c
    perf_monitor/perf-monitor.h
)

set(OBFUSCATION_SOURCES
    ml/anomaly-detector.c
    ml/anomaly-detector.h
    net/tls-emulator.c
    net/tls-emulator.h
    shadowsocks/shadowsocks-obfuscator.c
    shadowsocks/shadowsocks-obfuscator.h
)

set(VV_SOURCES
    vv/vv-tree.c
    vv/vv-tree.h
    vv/vv-io.h
)

set(MTPROTO_SOURCES
    mtproto/mtproto-common.h
    mtproto/mtproto-config.c
    mtproto/mtproto-config.h
    mtproto/mtproto-proxy.c
)

# Create a static library for common functionality
add_library(kdb_common STATIC ${COMMON_SOURCES} ${JOBS_SOURCES} ${CRYPTO_SOURCES} ${VV_SOURCES} ${SYSTEM_SOURCES} ${SECURITY_SOURCES} ${ADVANCED_OPTIMIZATION_SOURCES} ${OBFUSCATION_SOURCES} ${PERF_MONITOR_SOURCES})

# Target for the main mtproto-proxy executable
add_executable(mtproto-proxy
    ${MTPROTO_SOURCES}
    ${ENGINE_SOURCES}
    ${NET_SOURCES}
)

# Link libraries
target_link_libraries(mtproto-proxy kdb_common ${PLATFORM_LIBS} crypto z pthread)

# Set output directory
set_target_properties(mtproto-proxy PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation rules
install(TARGETS mtproto-proxy
    RUNTIME DESTINATION bin
)

# Print build info
message(STATUS "Building MTProxy version ${PROJECT_VERSION}")
message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")