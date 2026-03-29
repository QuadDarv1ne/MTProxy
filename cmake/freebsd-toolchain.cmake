# CMake Toolchain File для FreeBSD cross-compile
# Использование:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/freebsd-toolchain.cmake ..

# Целевая система
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Компиляторы
# Для cross-compile используйте:
#   pkg install mingw32-gcc mingw32-gcc-c++ (для Windows)
#   или установите x86_64-freebsd-gcc через pkg
set(CMAKE_C_COMPILER x86_64-freebsd-gcc)
set(CMAKE_CXX_COMPILER x86_64-freebsd-g++)

# Поиск библиотек и заголовков FreeBSD
set(CMAKE_FIND_ROOT_PATH /usr/local)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Флаги компиляции для FreeBSD
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Флаги линковки
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/usr/local/lib")

# Определение FreeBSD специфичных макросов
add_definitions(-D__FreeBSD__)
add_definitions(-D_FREEBSD_SOURCE)

# OpenSSL на FreeBSD обычно в /usr/local
set(OPENSSL_ROOT_DIR /usr/local)
set(OPENSSL_INCLUDE_DIR /usr/local/include)
set(OPENSSL_LIBRARIES /usr/local/lib/libssl.so;/usr/local/lib/libcrypto.so)

# zlib
set(ZLIB_INCLUDE_DIR /usr/local/include)
set(ZLIB_LIBRARY /usr/local/lib/libz.so)

# pthread на FreeBSD
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_USE_PTHREADS_INIT 1)

# Socket библиотеки (встроены в libc на FreeBSD)
set(PLATFORM_LIBS "")

# Примечания по совместимости:
# 1. io_uring НЕ доступен на FreeBSD (Linux-specific)
# 2. Используйте kqueue для event-driven I/O
# 3. Сетевые сокеты совместимы с POSIX
# 4. pthread полностью поддерживается
