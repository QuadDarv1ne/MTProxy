# Troubleshooting Guide

Руководство по диагностике и решению проблем MTProxy.

## Содержание

1. [Проблемы сборки](#проблемы-сборки)
2. [Проблемы запуска](#проблемы-запуска)
3. [Проблемы производительности](#проблемы-производительности)
4. [Сетевые проблемы](#сетевые-проблемы)
5. [Проблемы с памятью](#проблемы-с-памятью)
6. [Логирование и диагностика](#логирование-и-диагностика)

---

## Проблемы сборки

### Ошибка: CMake не находит компилятор

**Симптомы:**
```
CMake Error: CMAKE_C_COMPILER not set
```

**Решение:**

**Windows (MSYS2):**
```bash
# Проверка наличия компилятора
where gcc g++

# Если не найден — установка MSYS2
# Скачать с https://www.msys2.org/
# Установить пакеты:
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

**Linux:**
```bash
# Установка компилятора
sudo apt-get install build-essential cmake ninja-build  # Debian/Ubuntu
sudo dnf install gcc gcc-c++ cmake ninja-build         # Fedora/RHEL
```

### Ошибка: CMake не находит OpenSSL

**Симптомы:**
```
CMake Error at CMakeLists.txt:XX (find_package):
  Could not find a package configuration file provided by "OpenSSL"
```

**Решение:**

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-ucrt-x86_64-openssl
```

**Linux:**
```bash
sudo apt-get install libssl-dev              # Debian/Ubuntu
sudo dnf install openssl-devel               # Fedora/RHEL
sudo pacman -S openssl                       # Arch Linux
```

**macOS:**
```bash
brew install openssl
```

### Ошибка: нехватка памяти при сборке

**Симптомы:**
```
v8::base::FatalOOM
Out of memory
```

**Решение:**

1. **Уменьшить параллелизм:**
```bash
cd build
cmake -DCMAKE_BUILD_PARALLEL_LEVEL=2 ..
cmake --build . -j2
```

2. **Использовать Ninja с ограничением:**
```bash
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe -DCMAKE_BUILD_PARALLEL_LEVEL=2 ..
```

3. **Включить low-memory режим:**
```bash
cmake -DENABLE_LOW_MEMORY=ON ..
```

### Ошибка: undefined reference

**Симптомы:**
```
undefined reference to `pthread_create'
undefined reference to `WSAStartup'
```

**Решение:**

Проверить наличие библиотек в CMakeLists.txt:
```bash
# Для pthread (Linux)
sudo apt-get install libpthread-stubs0-dev

# Для Windows socket API
# Убедиться, что в CMakeLists.txt есть ws2_32
```

---

## Проблемы запуска

### Ошибка: cannot open secret file

**Симптомы:**
```
cannot open password file proxy-secret: No such file or directory
```

**Решение:**

1. **Создать файл секрета:**
```bash
# Генерация случайного секрета
openssl rand -hex 16 > proxy-secret

# Или использовать утилиту:
./mtproto-proxy.exe --generate-secret
```

2. **Проверить права доступа:**
```bash
chmod 600 proxy-secret
```

3. **Указать полный путь:**
```bash
./mtproto-proxy.exe -p 8888 -S /full/path/to/proxy-secret
```

### Ошибка: cannot bind to port

**Симптомы:**
```
cannot bind to port 8888: Address already in use
```

**Решение:**

1. **Проверить занятость порта:**
```bash
# Windows
netstat -ano | findstr :8888

# Linux/macOS
lsof -i :8888
ss -tlnp | grep 8888
```

2. **Освободить порт:**
```bash
# Windows
taskkill /PID <PID> /F

# Linux/macOS
kill -9 <PID>
```

3. **Использовать другой порт:**
```bash
./mtproto-proxy.exe -p 8889
```

### Ошибка: fork failed (Windows)

**Симптомы:**
```
fork: Function not implemented
```

**Решение:**

Windows не поддерживает fork(). Используйте single-worker режим:

```bash
# Запуск с одним worker'ом
./mtproto-proxy.exe -M 1 -p 8888

# Или с опцией single-thread
./mtproto-proxy.exe --single-thread
```

### Ошибка: too many open files

**Симптомы:**
```
socket: Too many open files
```

**Решение:**

**Linux/macOS:**
```bash
# Проверить текущий лимит
ulimit -n

# Увеличить лимит
ulimit -n 65536

# Постоянное изменение (/etc/security/limits.conf)
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf
```

**Windows:**
```bash
# Увеличить лимит через реестр
# HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters
# Создать DWORD: MaxUserPort = 65534
```

---

## Проблемы производительности

### Низкая пропускная способность

**Симптомы:**
- Скорость < 100 Мбит/с
- Высокая задержка

**Решение:**

1. **Включить многопоточность:**
```bash
# Авто-детект CPU ядер (по умолчанию)
./mtproto-proxy.exe -p 8888

# Или явно указать количество workers
./mtproto-proxy.exe -M 4 -p 8888
```

2. **Проверить использование CPU:**
```bash
# Windows
tasklist | findstr mtproto-proxy

# Linux
top -p $(pgrep mtproto-proxy)
```

3. **Оптимизировать параметры:**
```bash
# Увеличить размер буфера
./mtproto-proxy.exe --tcp-buffer-size 262144 -p 8888

# Включить оптимизации
./mtproto-proxy.exe --optimize-for-throughput -p 8888
```

### Высокое использование памяти

**Симптомы:**
- Потребление > 512 MB
- OOM killer убивает процесс

**Решение:**

1. **Включить low-memory режим:**
```bash
cmake -DENABLE_LOW_MEMORY=ON ..
cmake --build .
```

2. **Ограничить размер кэша:**
```bash
./mtproto-proxy.exe --cache-size 64 -p 8888
```

3. **Уменьшить количество connections:**
```bash
./mtproto-proxy.exe --max-connections 10000 -p 8888
```

### Высокая задержка (latency)

**Симптомы:**
- Ping > 100ms
- Задержки при передаче данных

**Решение:**

1. **Проверить сетевую задержку:**
```bash
ping -c 10 <server-ip>
```

2. **Включить TCP_NODELAY:**
```bash
./mtproto-proxy.exe --tcp-nodelay -p 8888
```

3. **Оптимизировать размер пакетов:**
```bash
./mtproto-proxy.exe --max-packet-size 131072 -p 8888
```

---

## Сетевые проблемы

### Клиенты не могут подключиться

**Симптомы:**
- Connection refused
- Timeout при подключении

**Решение:**

1. **Проверить брандмауэр:**
```bash
# Windows
netsh advfirewall firewall add rule name="MTProxy" dir=in action=allow protocol=TCP localport=8888

# Linux (iptables)
sudo iptables -A INPUT -p tcp --dport 8888 -j ACCEPT

# Linux (firewalld)
sudo firewall-cmd --add-port=8888/tcp --permanent
sudo firewall-cmd --reload
```

2. **Проверить прослушивание:**
```bash
# Windows
netstat -ano | findstr :8888

# Linux
ss -tlnp | grep 8888
```

3. **Проверить конфигурацию:**
```bash
# Убедиться, что сервер слушает 0.0.0.0
./mtproto-proxy.exe --bind 0.0.0.0 -p 8888
```

### Обрыв соединения

**Симптомы:**
- Connection reset by peer
- Unexpected EOF

**Решение:**

1. **Увеличить timeout:**
```bash
./mtproto-proxy.exe --read-timeout 600 --write-timeout 600 -p 8888
```

2. **Включить keepalive:**
```bash
./mtproto-proxy.exe --tcp-keepalive -p 8888
```

3. **Проверить логи:**
```bash
./mtproto-proxy.exe --log-level 2 --log-file proxy.log -p 8888
```

---

## Проблемы с памятью

### Утечки памяти

**Симптомы:**
- Постепенный рост потребления памяти
- OOM через несколько часов работы

**Решение:**

1. **Сборка с ASan для диагностики:**
```bash
cd build
cmake -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Запуск с детекцией утечек
export ASAN_OPTIONS=detect_leaks=1
./bin/mtproto-proxy.exe -p 8888
```

2. **Использовать Valgrind:**
```bash
valgrind --leak-check=full --show-leak-kinds=all \
  ./bin/mtproto-proxy.exe -p 8888
```

3. **Включить периодический GC:**
```bash
./mtproto-proxy.exe --gc-interval 3600 -p 8888
```

### Фрагментация памяти

**Симптомы:**
- Высокое потребление памяти при низкой нагрузке
- Частые аллокации

**Решение:**

1. **Включить memory pool:**
```bash
./mtproto-proxy.exe --use-memory-pool -p 8888
```

2. **Оптимизировать размер пула:**
```bash
./mtproto-proxy.exe --pool-size 64 -p 8888
```

---

## Логирование и диагностика

### Включение подробного логирования

```bash
# Уровень логирования: 0=quiet, 1=error, 2=warning, 3=info, 4=debug
./mtproto-proxy.exe --log-level 3 --log-file proxy.log -p 8888
```

### Просмотр статистики

```bash
# Статистика через REST API
curl http://localhost:8888/stats

# Расширенная статистика
curl http://localhost:8888/metrics

# Статус сервера
curl http://localhost:8888/status
```

### Проверка здоровья

```bash
# Health check
curl http://localhost:8888/health

# Проверка конфигурации
curl http://localhost:8888/config
```

### Динамическая перезагрузка

```bash
# Перезагрузка конфигурации
curl -X POST http://localhost:8888/reload

# Обновление секретов
curl -X POST http://localhost:8888/secrets/reload
```

---

## Диагностика через admin-cli

```bash
# Запуск admin-cli
./mtproxy-admin.exe

# Команды диагностики
> status          # Статус сервера
> stats           # Расширенная статистика
> connections     # Список подключений
> config          # Текущая конфигурация
> cache stats     # Статистика кэша
> ratelimit status # Статус rate limiter'ов
> health          # Проверка здоровья
```

---

## Часто задаваемые вопросы

### Q: Как узнать версию MTProxy?

```bash
./mtproto-proxy.exe --version
# или
curl http://localhost:8888/status | jq .version
```

### Q: Как сбросить статистику?

```bash
curl -X POST http://localhost:8888/stats/reset
```

### Q: Как экспортировать конфигурацию?

```bash
curl http://localhost:8888/config > config-backup.json
```

### Q: Как импортировать конфигурацию?

```bash
curl -X POST -H "Content-Type: application/json" \
  -d @config-backup.json \
  http://localhost:8888/config/reload
```

### Q: Где найти логи?

```bash
# По умолчанию
./proxy.log

# Или через опцию
./mtproto-proxy.exe --log-file /var/log/mtproxy/proxy.log -p 8888
```

---

## Обратная связь

Если проблема не решена:

1. **Соберите информацию:**
   - Версия MTProxy
   - Версия ОС
   - Логи (с уровнем 3+)
   - Вывод `--version` и `--help`

2. **Проверьте известные проблемы:**
   - [GitHub Issues](https://github.com/mtproxy/MTProxy/issues)
   - [CHANGELOG.md](CHANGELOG.md)

3. **Создайте issue:**
   - Описание проблемы
   - Шаги воспроизведения
   - Ожидаемое поведение
   - Фактическое поведение
   - Логи и скриншоты
