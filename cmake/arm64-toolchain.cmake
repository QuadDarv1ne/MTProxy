# CMake Toolchain File для ARM64 cross-compile
# Использование:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm64-toolchain.cmake ..

# Целевая система
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Компиляторы для cross-compile
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Поиск библиотек и заголовков
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Флаги компиляции для ARM64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -march=armv8-a")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -march=armv8-a")

# ARM64 NEON оптимизации (включаются отдельно)
# add_definitions(-D__ARM_NEON__)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")

# Флаги линковки
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/aarch64-linux-gnu/lib")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/usr/aarch64-linux-gnu/lib")

# Определение ARM64 макросов
add_definitions(-D__ARM64__)
add_definitions(-D__aarch64__)

# OpenSSL для ARM64
set(OPENSSL_ROOT_DIR /usr/aarch64-linux-gnu)
set(OPENSSL_INCLUDE_DIR /usr/aarch64-linux-gnu/include)
set(OPENSSL_LIBRARIES /usr/aarch64-linux-gnu/lib/libssl.so;/usr/aarch64-linux-gnu/lib/libcrypto.so)

# zlib для ARM64
set(ZLIB_INCLUDE_DIR /usr/aarch64-linux-gnu/include)
set(ZLIB_LIBRARY /usr/aarch64-linux-gnu/lib/libz.so)

# pthread
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_USE_PTHREADS_INIT 1)

# Сетевые библиотеки (встроены в libc на Linux)
set(PLATFORM_LIBS "")

# Примечания:
# 1. io_uring доступен на ARM64 Linux (kernel 5.1+)
# 2. NEON криптографические оптимизации требуют ENABLE_ARM64_CRYPTO=ON
# 3. Для Raspberry Pi используйте -march=armv8-a -mtune=cortex-a72
