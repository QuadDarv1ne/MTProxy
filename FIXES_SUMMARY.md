# MTProxy - Исправления ошибок (23 марта 2026)

## Критические исправления

### 1. Use-after-free в net/net-msg-buffers.c
**Проблема**: Использование указателя после free()
**Исправление**: Изменен порядок операций - free_mp_queue() вызывается до free(C)

### 2. Неиспользуемая константа в crypto/aes-optimized.c
**Проблема**: Warning о неиспользуемой переменной precomputed_round_keys
**Исправление**: Добавлен атрибут __attribute__((unused))

### 3. Truncation warning в net/pluggable-transports.c
**Проблема**: strncpy может обрезать строку
**Исправление**: Использована локальная переменная name_len для ясности

### 4. Несовместимость типов в common/crc32c.c
**Проблема**: Несоответствие типов функции crc32c_partial_stub
**Исправление**: Изменены типы параметров (int → long, unsigned int → unsigned)

### 5. Конфликты inet_pton/inet_ntop на Windows
**Проблема**: Дублирование определений с ws2tcpip.h
**Исправление**: Использованы макросы для InetPtonA/InetNtopA

### 6. Отсутствие posix_memalign на Windows
**Проблема**: Функция не существует в Windows API
**Исправление**: Добавлена реализация через _aligned_malloc

### 7. Отсутствие client_socket функций
**Проблема**: Функции не определены для Windows
**Исправление**: Добавлены stub-реализации в windows-stubs.c

### 8. Неправильный include path в net/net-events.c
**Проблема**: #include "vv-io.h" вместо "vv/vv-io.h"
**Исправление**: Исправлен путь к заголовочному файлу

### 9. Отсутствие ws2tcpip.h в posix-compat-windows.h
**Проблема**: inet_pton не объявлен, ошибка компиляции engine-net.c
**Исправление**: Добавлен #include <ws2tcpip.h> после winsock2.h

### 10. Illegal instruction при запуске на Windows
**Проблема**: mtproto-proxy.exe падает с "Illegal instruction" из-за AVX2/AVX512
**Исправление**: Использовать SSE4.2 вместо AVX на Windows для совместимости
**Файл**: CMakeLists.txt - добавлена условная компиляция для Windows

### 11. Windows epoll emulation
**Проблема**: Proxy завершался сразу после запуска (epoll_work возвращал 0)
**Исправление**: Реализован event loop через select() в windows-stubs.c
**Функции**: init_epoll(), epoll_insert(), epoll_remove(), epoll_work()

## Оставшиеся проблемы

### Критическая проблема: Segmentation fault при запуске
**Проблема**: Proxy падает сразу после "Server started"
**Причина**: Отсутствие реализации критических функций:
- `server_socket()` - создание listening socket (возвращает -1)
- `init_listening_tcpv6_connection()` - инициализация соединений (возвращает -1)
- Event loop не обрабатывает события (epoll emulation неполная)
- Pipes и IPC механизмы не реализованы

**Требуется для исправления:**
1. Реализовать Windows socket API (WSASocket, bind, listen, accept)
2. Реализовать полноценный event loop (IOCP или select())
3. Реализовать Windows pipes для IPC
4. Переработать connection management для Windows

**Статус**: Требуется значительная доработка. Рекомендуется WSL2/Docker.

### net/net-events.c - Linux-специфичный код
**Проблема**: Файл содержит 65+ вызовов Linux API (epoll, getifaddrs, ifaddrs)
**Статус**: Уже исключен из Windows сборки в CMakeLists.txt (строки 415-419)
**Решение**: Использовать CMake вместо Makefile для сборки на Windows

## Рекомендации

### Для сборки на Windows:
```bash
# Использовать CMake (рекомендуется)
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel

# Makefile не поддерживает условную компиляцию для Windows
```

### Для сборки на Linux/WSL:
```bash
make -j4
```

## Статистика исправлений
- Исправлено файлов: 10
- Исправлено критических ошибок: 11
- Добавлено Windows-совместимых функций: 7 (epoll emulation, inet_pton/ntop)
- Улучшено предупреждений компилятора: 5
- Размер бинарника: 84 MB (mtproto-proxy.exe)
- Статус: ⚠️ Proxy компилируется, но падает при запуске (требуется доработка сетевого слоя)

