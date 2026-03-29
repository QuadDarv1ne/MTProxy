# MTProxy на FreeBSD

## Поддержка платформ

MTProxy поддерживает запуск на FreeBSD с некоторыми ограничениями.

## Требования

- FreeBSD 13.0 или новее
- cmake 3.16+
- gcc или clang
- OpenSSL 1.1.1+ или LibreSSL
- zlib

## Установка зависимостей

```bash
# Обновление пакетов
sudo pkg update && sudo pkg upgrade

# Установка зависимостей
sudo pkg install cmake git openssl zlib
```

## Сборка

### Стандартная сборка (native)

```bash
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Сборка с оптимизациями

```bash
mkdir build && cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LTO=ON \
    -DENABLE_JEMALLOC=ON \
    ..
make -j$(sysctl -n hw.ncpu)
```

### Cross-compile (с Linux)

```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/freebsd-toolchain.cmake ..
make -j$(nproc)
```

## Известные ограничения

### Недоступные функции

| Функция | Статус | Альтернатива |
|---------|--------|--------------|
| io_uring | ❌ Не доступно | kqueue (в разработке) |
| epoll | ❌ Не доступно | kqueue |
| inotify | ❌ Не доступно | kqueue |

### Доступные функции

| Функция | Статус | Примечания |
|---------|--------|------------|
| pthread | ✅ Полная поддержка | |
| Сокеты | ✅ Полная поддержка | POSIX-совместимые |
| OpenSSL | ✅ Полная поддержка | |
| jemalloc | ✅ Полная поддержка | Рекомендуется для FreeBSD |
| tcmalloc | ✅ Поддержка | |
| Cache Memory Pool | ✅ Полная поддержка | |
| Rate Limiting | ✅ Полная поддержка | |

## Запуск

```bash
# Запуск прокси
./bin/mtproto-proxy -S <secret> -p 8888

# Запуск с админ-интерфейсом
./bin/mtproto-proxy -S <secret> -p 8888 --admin-port 8889

# Проверка статуса
./bin/mtproxy-admin status
```

## Тестирование

```bash
# Запуск тестов
cd build
ctest --output-on-failure

# Запуск конкретных тестов
./bin/test-utils-security
./bin/test-memory-allocator
./bin/benchmark-memory-allocator
```

## Производительность

### Рекомендации для FreeBSD

1. **jemalloc** — используйте jemalloc вместо стандартного аллокатора:
   ```bash
   cmake -DENABLE_JEMALLOC=ON ..
   ```

2. **Системные лимиты** — увеличьте лимиты:
   ```bash
   # В /boot/loader.conf
   kern.ipc.maxsockets=262144
   kern.ipc.somaxconn=4096
   
   # В /etc/sysctl.conf
   kern.ipc.numopenfiles=262144
   kern.ipc.nmbclusters=65536
   ```

3. **Сетевые оптимизации**:
   ```bash
   # В /etc/sysctl.conf
   net.inet.tcp.sendspace=65536
   net.inet.tcp.recvspace=65536
   net.inet.tcp.sendbuf_max=262144
   net.inet.tcp.recvbuf_max=262144
   ```

## Бенчмарки

```bash
# Memory allocator benchmark
./bin/benchmark-memory-allocator

# Highload benchmark
./bin/benchmark-highload

# Cache performance benchmark
./bin/benchmark-cache-performance
```

## Устранение проблем

### Ошибка: "kqueue not available"

Убедитесь, что используете FreeBSD 13.0 или новее.

### Ошибка: "pthread_create failed"

Увеличьте лимит потоков:
```bash
ulimit -u 4096
```

### Ошибка: "socket: too many open files"

Увеличьте лимит файловых дескрипторов:
```bash
ulimit -n 65536
```

## См. также

- [FreeBSD Performance Tuning](https://docs.freebsd.org/en/books/handbook/performance/)
- [FreeBSD Handbook](https://docs.freebsd.org/en/books/handbook/)
- [MTProxy README](../README.md)
