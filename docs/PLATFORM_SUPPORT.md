# MTProxy Platform Support

Руководство по запуску MTProxy на различных платформах.

---

# ARM64 Поддержка (Raspberry Pi и другие устройства)

## Поддерживаемые устройства

| Устройство | Архитектура | Статус | Примечания |
|------------|-------------|--------|------------|
| Raspberry Pi 4 | ARM64 | ✅ Полная | Рекомендуется |
| Raspberry Pi 3 | ARM64 | ✅ Поддержка | Ограниченная память |
| Raspberry Pi Zero 2 W | ARM64 | ✅ Поддержка | Low-memory режим |
| Orange Pi 3 | ARM64 | ✅ Поддержка | |
| NanoPi R4S | ARM64 | ✅ Поддержка | |
| AWS Graviton | ARM64 | ✅ Полная | |
| Apple M1/M2 | ARM64 | ✅ Полная | macOS/Docker |

## Требования

- ARM64 совместимый процессор
- Минимум 512 MB RAM (рекомендуется 1+ GB)
- Raspberry Pi OS 64-bit или другой ARM64 Linux
- cmake 3.16+
- OpenSSL 1.1.1+

## Установка зависимостей

### Raspberry Pi OS (64-bit)

```bash
sudo apt update && sudo apt upgrade

sudo apt install -y \
    cmake \
    build-essential \
    git \
    libssl-dev \
    zlib1g-dev \
    liburing-dev \
    python3 \
    python3-pip
```

### Docker (универсальный)

```bash
# Установка Docker
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
```

## Сборка

### Нативная сборка на Raspberry Pi

```bash
mkdir build && cd build

cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LTO=ON \
    -DENABLE_JEMALLOC=ON \
    -DENABLE_ARM64_CRYPTO=ON \
    ..

make -j$(nproc)
```

### Cross-compile (с x86_64 Linux)

```bash
# Установка cross-компилятора
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Сборка
mkdir build && cd build
cmake \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    ..
make -j$(nproc)
```

### Docker сборка

```bash
# Использование готового образа
docker run -d \
  --name mtproxy \
  -p 8888:8888 \
  -v ./proxy-secret:/app/proxy-secret \
  -v ./proxy-multi.conf:/app/proxy-multi.conf \
  ghcr.io/mtproxy/mtproxy:latest-arm64
```

## Оптимизация для Raspberry Pi

### Low-memory режим

```bash
cmake -DENABLE_LOW_MEMORY=ON ..
```

### Отключение тяжёлых модулей

```bash
cmake -DEXCLUDE_HEAVY_MODULES=ON ..
```

### Использование swap

```bash
# Создание swap файла
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# Постоянное включение
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

## Запуск

```bash
# Запуск прокси
./bin/mtproto-proxy \
    -S <secret> \
    -p 8888 \
    -M 2 \
    --aes-pwd proxy-secret \
    proxy-multi.conf
```

## Мониторинг

```bash
# Проверка использования CPU
top

# Проверка памяти
free -h

# Проверка температуры
vcgencmd measure_temp
```

---

# FreeBSD Поддержка

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
| pthread | ✅ Полная | |
| Сокеты | ✅ Полная | POSIX-совместимые |
| OpenSSL | ✅ Полная | |
| jemalloc | ✅ Полная | Рекомендуется для FreeBSD |
| tcmalloc | ✅ Поддержка | |
| Cache Memory Pool | ✅ Полная | |
| Rate Limiting | ✅ Полная | |

## Запуск

```bash
# Запуск прокси
./bin/mtproto-proxy \
    -S <secret> \
    -p 8888 \
    -M 1 \
    --aes-pwd proxy-secret \
    proxy-multi.conf
```

## Настройка системы

### Увеличение лимитов

```bash
# /boot/loader.conf
kern.ipc.maxsockets=262144
kern.ipc.soacceptqueue=4096

# /etc/sysctl.conf
kern.ipc.maxsockbuf=8388608
net.inet.tcp.sendspace=65536
net.inet.tcp.recvspace=65536
```

### Установка лимитов для пользователя

```bash
# /etc/login.conf
mtproxy:\
    :openfiles-cur=65536:\
    :openfiles-max=65536:\
    :maxproc-cur=256:\
    :maxproc-max=256:
```

## Docker на FreeBSD

```bash
# Установка Docker
pkg install docker

# Запуск демона
service docker start

# Запуск контейнера
docker run -d \
  --name mtproxy \
  -p 8888:8888 \
  ghcr.io/mtproxy/mtproxy:latest
```

---

# Linux Поддержка

## Поддерживаемые дистрибутивы

| Дистрибутив | Версии | Статус |
|-------------|--------|--------|
| Debian | 10, 11, 12 | ✅ Полная |
| Ubuntu | 20.04, 22.04, 24.04 | ✅ Полная |
| CentOS/RHEL | 8, 9 | ✅ Полная |
| Fedora | 36+ | ✅ Полная |
| Arch Linux | Rolling | ✅ Полная |

## Установка зависимостей

### Debian/Ubuntu

```bash
sudo apt update
sudo apt install -y \
    cmake \
    build-essential \
    git \
    libssl-dev \
    zlib1g-dev
```

### CentOS/RHEL/Fedora

```bash
# CentOS/RHEL
sudo yum install openssl-devel zlib-devel cmake gcc gcc-c++

# Fedora
sudo dnf install openssl-devel zlib-devel make automake gcc gcc-c++ cmake
```

### Arch Linux

```bash
sudo pacman -S cmake gcc openssl zlib
```

## Сборка с io_uring (Linux 5.1+)

```bash
sudo apt install liburing-dev  # Debian/Ubuntu

cmake -DENABLE_IOURING=ON ..
make -j$(nproc)
```

---

# macOS Поддержка

## Требования

- macOS 11.0+ (Big Sur или новее)
- Xcode Command Line Tools
- Homebrew

## Установка зависимостей

```bash
# Установка Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Установка зависимостей
brew install cmake openssl zlib
```

## Сборка

```bash
mkdir build && cd build

cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LTO=ON \
    -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
    ..

make -j$(sysctl -n hw.ncpu)
```

## Запуск

```bash
./bin/mtproto-proxy -S <secret> -p 8888
```

---

# Windows Поддержка

## Требования

- MSYS2 с UCRT64
- CMake 3.20+
- OpenSSL, ZLIB
- pthread-win32

## Установка зависимостей

```bash
# В MSYS2 UCRT64 терминале
pacman -S \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-openssl \
    mingw-w64-ucrt-x86_64-zlib
```

## Сборка

```bash
mkdir build && cd build

cmake -G "MinGW Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    ..

cmake --build . --parallel
```

## Известные ограничения

| Функция | Статус | Примечания |
|---------|--------|------------|
| fork() | ❌ Не доступно | Single-worker mode |
| epoll | ❌ Не доступно | Win32 select emulation |
| io_uring | ❌ Не доступно | Windows IOCP (в разработке) |

---

# Сравнение платформ

| Функция | Linux | FreeBSD | macOS | Windows | ARM64 |
|---------|-------|---------|-------|---------|-------|
| **io_uring** | ✅ | ❌ | ❌ | ❌ | ⚠️ Частично |
| **epoll/kqueue** | ✅ epoll | ✅ kqueue | ✅ kqueue | ❌ select | ✅ epoll |
| **jemalloc** | ✅ | ✅ | ✅ | ⚠️ Ограничено | ✅ |
| **tcmalloc** | ✅ | ✅ | ✅ | ❌ | ⚠️ Ограничено |
| **TLS offload** | ✅ | ✅ | ✅ | ⚠️ Частично | ⚠️ Частично |
| **Docker** | ✅ | ⚠️ Ограничено | ✅ | ⚠️ WSL2 | ✅ |
| **Production ready** | ✅ | ✅ | ✅ | ⚠️ Dev/Test | ✅ |

---

## Дополнительные ресурсы

- [BUILD_WINDOWS.md](../BUILD_WINDOWS.md) — Подробная сборка на Windows
- [DEPLOYMENT.md](../DEPLOYMENT.md) — Развёртывание в production
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) — Оптимизация производительности
