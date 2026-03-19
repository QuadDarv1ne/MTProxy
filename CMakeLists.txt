cmake_minimum_required(VERSION 3.10)
project(MTProxy VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    message(STATUS "Detected Windows platform")
    add_definitions(-D_WIN32)
    set(PLATFORM_LIBS ws2_32 pthread)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=0x0600")
elseif(UNIX)
    message(STATUS "Detected Unix/Linux platform")
    set(PLATFORM_LIBS m rt pthread)
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
    common/common-stats.c
    common/common-stats.h
    common/platform_network.c
    common/platform_network.h
    common/platform_compat.h
    common/vlog.c
    common/vlog.h
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
    crypto/advanced-crypto-opt.c
    crypto/advanced-crypto-opt.h
    crypto/crypto-optimizer.c
    crypto/crypto-optimizer.h
    crypto/crypto-performance-optimizer.c
    crypto/crypto-performance-optimizer.h
    crypto/dh-optimized.c
    crypto/dh-optimized.h
    crypto/enhanced-crypto-optimizer.c
    crypto/enhanced-crypto-optimizer.h
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
    # Дополнительные сетевые модули
    net/adaptive-protocol-manager.c
    net/adaptive-protocol-manager.h
    net/advanced-connection-pool.c
    net/advanced-connection-pool.h
    net/advanced-network.c
    net/advanced-network.h
    net/async-network-optimizer.c
    net/async-network-optimizer.h
    net/enhanced-tls-obfuscation.c
    net/enhanced-tls-obfuscation.h
    net/multiproto-manager.c
    net/multiproto-manager.h
    net/net-buffer-manager.c
    net/net-buffer-manager.h
    net/net-connections-pool.c
    net/net-connections-pool.h
    net/net-enhanced-networking.c
    net/net-enhanced-networking.h
    net/net-event-handler.c
    net/net-event-handler.h
    net/net-performance-manager.c
    net/net-performance-manager.h
    net/net-quic-connections.c
    net/net-quic-connections.h
    net/network-connection-optimizer.c
    net/network-connection-optimizer.h
    net/protocol-reliability.c
    net/protocol-reliability.h
    net/routing-optimizer.c
    net/routing-optimizer.h
    net/shadowsocks-adapter.c
    net/shadowsocks-adapter.h
    net/simple-advanced-network.c
    net/simple-advanced-network.h
    net/tls-emulator.c
    net/tls-emulator.h
    net/websocket-support.c
    net/websocket-support.h
    net/zero-copy-optimizer.c
    net/zero-copy-optimizer.h
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
    security/ddos-protection-enhanced.c
    security/ddos-protection-enhanced.h
    security/cert-pinning.c
    security/cert-pinning.h
    security/security-utils.c
    security/security-utils.h
    security/behavioral-anomaly-detection.c
    security/behavioral-anomaly-detection.h
    security/security-enhancements.c
    security/security-enhancements.h
    security/security-patch-example.c
    security/security-patch-example.h
    security/threat-intelligence-system.c
    security/threat-intelligence-system.h
    security_enhanced/security-enhanced.c
    security_enhanced/security-enhanced.h
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
    system/adaptive-tuning.c
    system/adaptive-tuning.h
    system/advanced-memory-management.c
    system/advanced-memory-management.h
    system/advanced-optimizer.c
    system/advanced-optimizer.h
    system/advanced-predictive-analytics.c
    system/advanced-predictive-analytics.h
    system/component-integration.c
    system/component-integration.h
    system/compression-optimizer.c
    system/compression-optimizer.h
    system/dynamic-resource-allocator.c
    system/dynamic-resource-allocator.h
    system/enhanced-memory-optimizer.c
    system/enhanced-memory-optimizer.h
    system/failure-predictor.c
    system/failure-predictor.h
    system/intelligent-auto-tuning.c
    system/intelligent-auto-tuning.h
    system/intelligent-optimization-integrator.c
    system/intelligent-optimization-integrator.h
    system/intelligent-workload-distributor.c
    system/intelligent-workload-distributor.h
    system/memory-optimizer.c
    system/memory-optimizer.h
    system/memory-pool-optimizer.c
    system/memory-pool-optimizer.h
    system/memory-pool.c
    system/memory-pool.h
    system/ml-performance-predictor.c
    system/ml-performance-predictor.h
    system/modular-architecture.c
    system/modular-architecture.h
    system/numa-allocator.c
    system/numa-allocator.h
    system/numa-aware-allocator.c
    system/numa-aware-allocator.h
    system/performance-forecasting-engine.c
    system/performance-forecasting-engine.h
    system/plugin-interface.c
    system/plugin-interface.h
    system/predictive-optimizer.c
    system/predictive-optimizer.h
    system/proactive-allocator.c
    system/proactive-allocator.h
    system/self-healing.c
    system/self-healing.h
    system/unified-api.c
    system/unified-api.h
    system/config/advanced-systems-defaults.c
    system/config/advanced-systems-defaults.h
    system/config/unified-config-manager.c
    system/config/unified-config-manager.h
    system/diagnostic/advanced-diagnostic-system.c
    system/diagnostic/advanced-diagnostic-system.h
    system/integration/mtproxy-integration-layer.c
    system/integration/mtproxy-integration-layer.h
    system/integration/system-integration-coordinator.c
    system/integration/system-integration-coordinator.h
    system/monitoring/real-time-monitoring-dashboard.c
    system/monitoring/real-time-monitoring-dashboard.h
    system/monitoring/system-health-monitor.c
    system/monitoring/system-health-monitor.h
    system/optimization/performance-correlation-engine.c
    system/optimization/performance-correlation-engine.h
    system/optimization/resource-optimization-manager.c
    system/optimization/resource-optimization-manager.h
)

set(ADVANCED_OPTIMIZATION_SOURCES
    system/io-uring-interface.c
    system/io-uring-interface.h
    system/dpdk-interface.c
    system/dpdk-interface.h
    system/debugging/intelligent-debugging-framework.c
    system/debugging/intelligent-debugging-framework.h
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
    perf_monitor/distributed-tracing.c
    perf_monitor/distributed-tracing.h
    perf_monitor/distributed-tracing-system.c
    perf_monitor/distributed-tracing-system.h
    perf_monitor/enhanced-observability.c
    perf_monitor/enhanced-observability.h
    perf_monitor/enhanced-performance-monitor.c
    perf_monitor/enhanced-performance-monitor.h
)

set(OBFUSCATION_SOURCES
    ml/anomaly-detector.c
    ml/anomaly-detector.h
    ml/traffic-optimizer.c
    ml/traffic-optimizer.h
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

set(CONN_POOL_SOURCES
    conn_pool/adaptive-connection-pool.c
    conn_pool/adaptive-connection-pool.h
    conn_pool/advanced-connection-optimizer.c
    conn_pool/advanced-connection-optimizer.h
    conn_pool/conn-pool.c
    conn_pool/conn-pool.h
    conn_pool/simple-advanced-connection-optimizer.c
    conn_pool/simple-advanced-connection-optimizer.h
)

set(GENERAL_SOURCES
    general/config_manager.c
    general/config_manager.h
    general/protocol_manager.c
    general/protocol_manager.h
    general/mtproxy-enhanced.h
)

set(INFRASTRUCTURE_SOURCES
    infrastructure/advanced-load-balancer.c
    infrastructure/advanced-load-balancer.h
    infrastructure/auto-scaling.c
    infrastructure/auto-scaling.h
    infrastructure/load-balancer.c
    infrastructure/load-balancer.h
)

set(THREAD_SYSTEM_SOURCES
    thread_system/thread-system.c
    thread_system/thread-system.h
)

set(MTPROTO_SOURCES
    mtproto/mtproto-common.h
    mtproto/mtproto-config.c
    mtproto/mtproto-config.h
    mtproto/mtproto-proxy.c
    mtproto/mtproto-v3-adapter.c
    mtproto/mtproto-v3-adapter.h
    mtproto/mtproto-version-manager.c
    mtproto/mtproto-version-manager.h
)

# Create a static library for common functionality
add_library(kdb_common STATIC ${COMMON_SOURCES} ${JOBS_SOURCES} ${CRYPTO_SOURCES} ${VV_SOURCES} ${CONN_POOL_SOURCES} ${GENERAL_SOURCES} ${INFRASTRUCTURE_SOURCES} ${THREAD_SYSTEM_SOURCES} ${SYSTEM_SOURCES} ${SECURITY_SOURCES} ${ADVANCED_OPTIMIZATION_SOURCES} ${OBFUSCATION_SOURCES} ${PERF_MONITOR_SOURCES})

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