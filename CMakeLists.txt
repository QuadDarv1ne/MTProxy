cmake_minimum_required(VERSION 3.10)
project(MTProxy VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler-specific options
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wno-array-bounds -fno-strict-aliasing -fno-strict-overflow -fwrapv -DAES=1 -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpclmul -march=core2 -mfpmath=sse -mssse3")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lm -lrt -lcrypto -lz -lpthread")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall /D_WIN32_WINNT=0x0600")
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
)

set(NET_SOURCES
    ${NET_SOURCES}
    net/shadowsocks-advanced.c
    net/shadowsocks-advanced.h
    net/pluggable-transports.c
    net/pluggable-transports.h
    net/network-profiler.c
    net/network-profiler.h
    net/network-analyzer.c
    net/network-analyzer.h
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

set(NET_SOURCES
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

# Create a static library for common functionality
add_library(kdb_common STATIC ${COMMON_SOURCES} ${JOBS_SOURCES} ${CRYPTO_SOURCES} ${VV_SOURCES})

# Target for the main mtproto-proxy executable
add_executable(mtproto-proxy
    ${MTPROTO_SOURCES}
    ${ENGINE_SOURCES}
    ${NET_SOURCES}
)

# Link libraries
target_link_libraries(mtproto-proxy kdb_common m rt crypto z pthread)

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