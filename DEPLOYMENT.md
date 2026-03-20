# Deployment Guide для MTProxy

## 📋 Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Docker развёртывание](#docker-развёртывание)
3. [Linux/WSL развёртывание](#linuxwsl-развёртывание)
4. [Windows развёртывание](#windows-развёртывание)
5. [Конфигурация](#конфигурация)
6. [Мониторинг](#мониторинг)

---

## 🚀 Быстрый старт

### Docker (рекомендуется)

```bash
# Клонирование репозитория
git clone https://github.com/QuadDarv1ne/MTProxy.git
cd MTProxy

# Запуск через docker-compose
docker-compose up -d

# Проверка статуса
docker-compose ps

# Просмотр логов
docker-compose logs -f mtproxy
```

### Linux/WSL (сборка из исходников)

```bash
# Установка зависимостей
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake openssl libssl-dev zlib1g-dev

# Сборка
git clone https://github.com/QuadDarv1ne/MTProxy.git
cd MTProxy
make -j4

# Запуск
./objs/bin/mtproto-proxy -p 8080 --secret=your_secret
```

---

## 🐳 Docker развёртывание

### Требования

- Docker 20.10+
- Docker Compose 2.0+

### Сборка образа

```bash
# Автоматическая сборка
./scripts/build-docker.sh

# Или вручную
docker build -t mtproxy:latest .
```

### Запуск контейнера

```bash
# Простой запуск
docker run -d \
  --name mtproxy \
  -p 8080:8080 \
  -p 443:443 \
  -e MT_PROXY_SECRET=your_secret \
  mtproxy:latest

# С томами для сохранения данных
docker run -d \
  --name mtproxy \
  -p 8080:8080 \
  -v mtproxy_config:/etc/mtproxy \
  -v mtproxy_data:/var/lib/mtproxy \
  -v mtproxy_logs:/var/log/mtproxy \
  -e MT_PROXY_SECRET=your_secret \
  -e MT_PROXY_WORKERS=4 \
  mtproxy:latest
```

### Docker Compose

```bash
# Запуск
docker-compose up -d

# Остановка
docker-compose down

# Перезапуск
docker-compose restart

# Обновление
docker-compose pull
docker-compose up -d --force-recreate
```

### Переменные окружения

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `MT_PROXY_PORT` | Порт прокси | 8080 |
| `MT_PROXY_SECRET` | Секретный ключ | (требуется) |
| `MT_PROXY_WORKERS` | Количество воркеров | 4 |
| `MT_PROXY_LOG_LEVEL` | Уровень логирования | info |

---

## 🐧 Linux/WSL развёртывание

### Требования

- GCC 9+ или Clang 10+
- CMake 3.16+
- OpenSSL 1.1.1+
- ZLIB 1.2+

### Установка зависимостей

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    zlib1g-dev \
    git
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y cmake openssl-devel zlib-devel git
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake openssl zlib git
```

### Сборка

```bash
# Клонирование
git clone https://github.com/QuadDarv1ne/MTProxy.git
cd MTProxy

# CMake (рекомендуется)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cmake --install build --prefix /usr/local

# Или Make
make -j4
```

### Запуск

```bash
# Системная установка
sudo mtproto-proxy -p 8080 --secret=your_secret

# Локальный запуск
./objs/bin/mtproto-proxy -p 8080 --secret=your_secret
```

### Systemd сервис

Создайте файл `/etc/systemd/system/mtproxy.service`:

```ini
[Unit]
Description=MTProxy Server
After=network.target

[Service]
Type=simple
User=mtproxy
Group=mtproxy
ExecStart=/usr/local/bin/mtproto-proxy -p 8080 --secret=your_secret
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Активация:
```bash
sudo systemctl daemon-reload
sudo systemctl enable mtproxy
sudo systemctl start mtproxy
sudo systemctl status mtproxy
```

---

## 🪟 Windows развёртывание

### Требования

- MSYS2 или WSL
- MinGW-w64 или Visual Studio 2019+
- OpenSSL для Windows
- ZLIB для Windows

### Установка MSYS2

1. Скачайте установщик с https://www.msys2.org/
2. Установите в `C:\msys64`
3. Запустите MSYS2 UCRT64

```bash
# Установка зависимостей
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-zlib

# Сборка
cd /c/path/to/MTProxy
cmake -B build-windows-x64
cmake --build build-windows-x64 --config Release
```

### Запуск

```bash
# Из директории сборки
cd build-windows-x64/bin
./mtproto-proxy.exe -p 8080 --secret=your_secret
```

**Примечание:** На Windows поддерживается только single-worker mode.

---

## ⚙️ Конфигурация

### Базовая конфигурация

Файл: `/etc/mtproxy/proxy.json`

```json
{
  "proxy": {
    "port": 8080,
    "secret": "your_secret_here",
    "workers": 4,
    "max_connections": 10000,
    "connection_timeout_ms": 30000
  },
  "logging": {
    "level": "info",
    "file": "/var/log/mtproxy/proxy.log",
    "rotate_size_mb": 100,
    "rotate_count": 5
  },
  "cache": {
    "enabled": true,
    "max_entries": 10000,
    "ttl_seconds": 300
  },
  "rate_limit": {
    "enabled": true,
    "requests_per_second": 100,
    "burst_size": 200
  }
}
```

### Генерация секрета

```bash
# OpenSSL
openssl rand -hex 16

# Или
cat /dev/urandom | tr -dc 'a-f0-9' | fold -w 32 | head -n 1
```

---

## 📊 Мониторинг

### Проверка статуса

```bash
# Docker
docker-compose ps
docker-compose logs mtproxy

# Systemd
sudo systemctl status mtproxy
sudo journalctl -u mtproxy -f

# Admin CLI
mtproxy-admin status
mtproxy-admin stats
```

### Метрики

MTProxy предоставляет метрики через admin CLI:

```bash
# Статистика соединений
mtproxy-admin connections

# Статистика кэша
mtproxy-admin cache stats

# Статистика rate limiting
mtproxy-admin rate-limit stats
```

### Логи

```bash
# Docker логи
docker-compose logs -f mtproxy

# Файл логов
tail -f /var/log/mtproxy/proxy.log

# Systemd логи
journalctl -u mtproxy -f
```

---

## 🔧 Устранение проблем

### Контейнер не запускается

```bash
# Проверка логов
docker-compose logs mtproxy

# Проверка портов
docker port mtproxy
netstat -tlnp | grep 8080
```

### Ошибка сборки

```bash
# Очистка
make clean
rm -rf build/

# Повторная сборка
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Проблемы с производительностью

1. Увеличьте количество воркеров: `MT_PROXY_WORKERS=8`
2. Настройте кэш: увеличьте `max_entries`
3. Проверьте rate limiting: уменьшите `requests_per_second`

---

## 📝 Changelog развёртывания

- **Март 2026**: Добавлены Docker образы и docker-compose
- **Март 2026**: Обновлены скрипты развёртывания
- **Март 2026**: Добавлен Deployment Guide

---

*Последнее обновление: 20 марта 2026 г.*
