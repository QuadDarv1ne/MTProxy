# MTProxy на ARM64 (Raspberry Pi)

## Поддержка платформ

MTProxy поддерживает запуск на ARM64 устройствах, включая Raspberry Pi.

## Поддерживаемые устройства

| Устройство | Архитектура | Статус | Примечания |
|------------|-------------|--------|------------|
| Raspberry Pi 4 | ARM64 | ✅ Полная поддержка | Рекомендуется |
| Raspberry Pi 3 | ARM64 | ✅ Поддержка | Ограниченная память |
| Raspberry Pi Zero 2 W | ARM64 | ✅ Поддержка | Low-memory режим |
| Orange Pi 3 | ARM64 | ✅ Поддержка | |
| NanoPi R4S | ARM64 | ✅ Поддержка | |
| AWS Graviton | ARM64 | ✅ Полная поддержка | |
| Apple M1/M2 | ARM64 | ✅ Полная поддержка | macOS/Docker |

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

# Установка зависимостей
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

# Добавление пользователя в группу docker
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
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    ..

make -j$(nproc)
```

### Docker сборка (multi-arch)

```bash
# Создание buildx контекста
docker buildx create --use --name mtproxy-builder

# Сборка для ARM64
docker buildx build \
    --platform linux/arm64 \
    -t mtproxy:arm64 \
    --load \
    .

# Сборка для всех архитектур
docker buildx build \
    --platform linux/amd64,linux/arm64,linux/arm/v7 \
    -t mtproxy:latest \
    --push \
    .
```

## Оптимизации для ARM64

### NEON криптографические оптимизации

Включите аппаратные криптографические инструкции ARM NEON:

```bash
cmake -DENABLE_ARM64_CRYPTO=ON ..
```

Это ускоряет:
- AES шифрование/дешифрование
- SHA хеширование
- MTProto криптографию

### jemalloc для ARM

jemalloc рекомендуется для ARM64 устройств:

```bash
sudo apt install libjemalloc-dev
cmake -DENABLE_JEMALLOC=ON ..
```

### Low-memory режим (для Raspberry Pi с 512MB RAM)

```bash
cmake \
    -DENABLE_LOW_MEMORY=ON \
    -DCACHE_MAX_ENTRIES=10000 \
    -DCONNECTION_POOL_SIZE=100 \
    ..
```

## Запуск

### Нативный запуск

```bash
# Запуск прокси
./bin/mtproto-proxy -S <secret> -p 8888

# С админ-интерфейсом
./bin/mtproto-proxy -S <secret> -p 8888 --admin-port 8889

# С оптимизациями для Raspberry Pi
MTALLOC=jemalloc ./bin/mtproto-proxy -S <secret> -p 8888
```

### Docker запуск

```bash
docker run -d \
    --name mtproxy \
    -p 8888:8888 \
    -p 8889:8889 \
    -e PROXY_SECRET=<secret> \
    mtproxy:arm64
```

### systemd сервис

```ini
# /etc/systemd/system/mtproxy.service
[Unit]
Description=MTProxy Server
After=network.target

[Service]
Type=simple
User=mtproxy
ExecStart=/opt/mtproxy/bin/mtproto-proxy -S <secret> -p 8888
Restart=always
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

```bash
# Установка сервиса
sudo systemctl daemon-reload
sudo systemctl enable mtproxy
sudo systemctl start mtproxy
```

## Производительность

### Ожидаемая производительность

| Устройство | Соединений | Пропускная способность |
|------------|------------|------------------------|
| Raspberry Pi 4 (4GB) | 10,000+ | ~500 Mbps |
| Raspberry Pi 4 (2GB) | 5,000+ | ~300 Mbps |
| Raspberry Pi 3 | 2,000+ | ~100 Mbps |
| Raspberry Pi Zero 2 W | 500+ | ~50 Mbps |

### Бенчмарки

```bash
# Memory allocator benchmark
./bin/benchmark-memory-allocator

# Highload benchmark
./bin/benchmark-highload

# Cache performance benchmark
./bin/benchmark-cache-performance
```

### Оптимизация производительности

1. **Overclocking** (Raspberry Pi 4):
   ```bash
   # В /boot/config.txt
   over_voltage=6
   arm_freq=2147
   gpu_freq=750
   ```

2. **Охлаждение**: Установите радиатор или вентилятор

3. **USB 3.0 SSD** вместо SD карты для лучшей I/O

## Тестирование

```bash
# Запуск тестов
cd build
ctest --output-on-failure

# Запуск конкретных тестов
./bin/test-utils-security
./bin/test-memory-allocator
```

## Устранение проблем

### Ошибка: "Out of memory"

Включите low-memory режим или увеличьте swap:
```bash
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile  # CONF_SWAPSIZE=2048
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### Ошибка: "Too many open files"

Увеличьте лимит в `/etc/security/limits.conf`:
```
mtproxy soft nofile 65536
mtproxy hard nofile 65536
```

### Ошибка: "NEON instructions not available"

Убедитесь, что процессор поддерживает NEON:
```bash
cat /proc/cpuinfo | grep -i neon
```

## Мониторинг

```bash
# Использование памяти
watch -n1 free -h

# Использование CPU
top -d1

# Сетевая активность
iftop -P -n

# Температура (Raspberry Pi)
vcgencmd measure_temp
```

## См. также

- [Raspberry Pi Documentation](https://www.raspberrypi.org/documentation/)
- [ARM64 Porting Guide](https://developer.arm.com/documentation/102476/latest/)
- [MTProxy README](../README.md)
