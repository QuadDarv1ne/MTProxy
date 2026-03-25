# MTProxy Troubleshooting Guide

Руководство по диагностике и решению проблем MTProxy

---

## 📋 Содержание

1. [Проблемы сборки](#проблемы-сборки)
2. [Проблемы запуска](#проблемы-запуска)
3. [Проблемы с сетью](#проблемы-с-сетью)
4. [Проблемы производительности](#проблемы-производительности)
5. [Проблемы безопасности](#проблемы-безопасности)
6. [Частые ошибки](#частые-ошибки)
7. [Диагностические команды](#диагностические-команды)

---

## 🔧 Проблемы сборки

### Ошибка: `multiple definition of 'usage'`

**Симптомы:**
```
ld.exe: libkdb_common.a(windows-stubs.c.obj):windows-stubs.c:(.text$usage+0x0): 
multiple definition of `usage'; CMakeFiles\mtproto-proxy.dir/objects.a(mtproto-proxy.c.obj):mtproto-proxy.c:(.text$usage+0x0): first defined here
```

**Причина:** Конфликт имён функций между `windows-stubs.c` и `mtproto-proxy.c`

**Решение:**
1. Убедитесь, что `BUILD_SHARED_LIB=OFF` для основной сборки:
   ```bash
   cmake -DBUILD_SHARED_LIB=OFF ..
   ```

2. Или очистите сборку и пересоберите:
   ```bash
   rm -rf build/
   mkdir build && cd build
   cmake -DBUILD_SHARED_LIB=OFF -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --parallel
   ```

---

### Ошибка: `undefined reference to 'usage'`

**Симптомы:**
```
undefined reference to `usage'
collect2.exe: error: ld returned 1 exit status
```

**Причина:** Функция `usage()` требуется для shared library, но отсутствует

**Решение:**
1. Для сборки shared library включите `BUILD_SHARED_LIB=ON`:
   ```bash
   cmake -DBUILD_SHARED_LIB=ON ..
   ```

2. Убедитесь, что `windows-stubs.c` содержит stub-функцию:
   ```c
   #ifdef BUILD_SHARED_LIB
   void usage(void) { /* stub for shared library */ }
   #endif
   ```

---

### Ошибка: `implicit declaration of function 'Sleep'`

**Симптомы:**
```
error: implicit declaration of function 'Sleep'; did you mean '_sleep'?
```

**Причина:** Отсутствие заголовочных файлов Windows в тестовых файлах

**Решение:**
Добавьте в начало тестового файла:
```c
#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
#endif
```

---

### Ошибка: `cache_config_t has no member named 'num_partitions'`

**Симптомы:**
```
error: 'cache_config_t' has no member named 'num_partitions'
```

**Причина:** API кэша изменился, но тесты не обновлены

**Решение:**
1. Обновите тесты для использования нового API:
   - Замените `num_partitions` на `enable_partitioning`
   - Используйте `cache_get_stats(cache, &stats)` вместо `cache_get_stats(cache)`

2. Или временно отключите проблемные тесты:
   ```bash
   cmake -DBUILD_TESTS=OFF ..
   ```

---

## 🚀 Проблемы запуска

### Сервер не запускается

**Симптомы:**
- Процесс завершается сразу после запуска
- Нет сообщений в логе

**Диагностика:**
```bash
# Проверка логов
./mtproto-proxy --help

# Проверка файлов конфигурации
cat proxy-secret
cat proxy-multi.conf

# Запуск с verbose логом
./mtproto-proxy -v -p 8888 --aes-pwd proxy-secret proxy-multi.conf -M 1
```

**Возможные причины:**

1. **Неверный путь к файлам конфигурации**
   ```bash
   # Используйте абсолютные пути
   ./mtproto-proxy --aes-pwd /full/path/to/proxy-secret /full/path/to/proxy-multi.conf
   ```

2. **Неверный формат secret файла**
   ```bash
   # Secret файл должен содержать 16 байт в hex формате
   cat proxy-secret | xxd -r -p > proxy-secret.bin
   ```

3. **Порт уже занят**
   ```bash
   # Linux
   netstat -tlnp | grep 8888
   
   # Windows
   netstat -ano | findstr :8888
   ```

---

### Ошибка: "cannot open server socket"

**Симптомы:**
```
cannot open server socket at port 443: Permission denied
```

**Причины:**

1. **Порт требует root прав** (порты < 1024 на Linux)
   
   **Решение:**
   ```bash
   # Используйте порт > 1024
   ./mtproto-proxy -p 8443 ...
   
   # Или запустите от root (не рекомендуется)
   sudo ./mtproto-proxy -p 443 ...
   ```

2. **Брандмауэр блокирует порт**
   
   **Решение:**
   ```bash
   # Linux (iptables)
   iptables -A INPUT -p tcp --dport 8888 -j ACCEPT
   
   # Windows (PowerShell)
   New-NetFirewallRule -DisplayName "MTProxy" -Direction Inbound -LocalPort 8888 -Protocol TCP -Action Allow
   ```

---

### Ошибка: "Multi-worker mode not supported on Windows"

**Симптомы:**
```
WARN: Multi-worker mode not supported on Windows - using single worker mode
```

**Причина:** Windows не поддерживает `fork()`, необходимый для multi-worker режима

**Решение:**
1. Используйте `-M 1` (single worker mode):
   ```bash
   ./mtproto-proxy.exe -M 1 ...
   ```

2. Для multi-worker mode используйте WSL2 или Docker

---

## 🌐 Проблемы с сетью

### Клиенты не могут подключиться

**Диагностика:**

1. **Проверка доступности порта:**
   ```bash
   # Linux
   telnet your.server.com 443
   
   # Windows
   Test-NetConnection your.server.com -Port 443
   ```

2. **Проверка firewall:**
   ```bash
   # Linux
   iptables -L -n | grep 443
   
   # Windows
   Get-NetFirewallRule | Where-Object Enabled -Eq True
   ```

3. **Проверка NAT/маршрутизации:**
   ```bash
   traceroute your.server.com
   ```

**Решение:**

1. Откройте порт в firewall
2. Настройте проброс портов на роутере (NAT)
3. Проверьте, что сервер слушает правильный интерфейс:
   ```bash
   netstat -tlnp | grep mtproto
   ```

---

### Соединения сбрасываются

**Симптомы:**
- Клиенты подключаются, но сразу отключаются
- В логах: "connection reset by peer"

**Возможные причины:**

1. **Неверный secret ключ**
   ```bash
   # Проверьте формат ключа (32 hex символа)
   echo "your_secret" | wc -c
   
   # Пересоздайте ключ
   head -c 16 /dev/urandom | xxd -ps
   ```

2. **Проблемы с TLS обфускацией**
   ```bash
   # Отключите TLS обфускацию для теста
   ./mtproto-proxy --no-tls ...
   ```

3. **MTU проблемы**
   ```bash
   # Уменьшите MTU
   ifconfig eth0 mtu 1400
   ```

---

## ⚡ Проблемы производительности

### Высокое использование CPU

**Диагностика:**
```bash
# Linux
top -p $(pgrep mtproto)
htop

# Windows
tasklist /v | findstr mtproto
Process Explorer
```

**Решение:**

1. **Оптимизируйте количество workers:**
   ```bash
   # Количество workers = количество CPU ядер
   ./mtproto-proxy -M $(nproc) ...
   ```

2. **Включите кэширование:**
   ```c
   // В конфигурации
   cache.enabled = true
   cache.policy = LRU
   cache.max_entries = 10000
   ```

3. **Используйте оптимизированную сборку:**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_OPTIMIZATION_LEVEL=O3 ..
   ```

---

### Высокое использование памяти

**Диагностика:**
```bash
# Linux
cat /proc/$(pgrep mtproto)/status | grep Vm

# Windows
Get-Process mtproto-proxy | Select-Object WorkingSet,VirtualMemorySize
```

**Решение:**

1. **Ограничьте количество соединений:**
   ```bash
   ./mtproto-proxy --max-connections 10000 ...
   ```

2. **Уменьшите размер буферов:**
   ```bash
   ./mtproto-proxy --buffer-size 4096 ...
   ```

3. **Включите jemalloc/tcmalloc:**
   ```bash
   cmake -DSTATIC_LINKING=ON ..
   LD_PRELOAD=/usr/lib/libjemalloc.so ./mtproto-proxy ...
   ```

---

### Низкая пропускная способность

**Диагностика:**
```bash
# Проверка статистики
wget localhost:8888/stats

# Мониторинг трафика
iftop -P -p 8888
```

**Решение:**

1. **Увеличьте количество workers:**
   ```bash
   ./mtproto-proxy -M 4 ...
   ```

2. **Включите LTO оптимизацию:**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release ..
   # LTO автоматически включается для Unix
   ```

3. **Используйте AES-NI:**
   ```bash
   # Проверьте поддержку CPU
   grep -o aes /proc/cpuinfo
   ```

---

## 🔒 Проблемы безопасности

### Ошибка: "invalid secret key"

**Симптомы:**
- Клиенты не могут аутентифицироваться
- В логах: "authentication failed"

**Решение:**

1. **Проверьте формат ключа:**
   ```bash
   # Должен быть 32 hex символа
   echo -n "your_secret" | wc -c
   ```

2. **Пересоздайте ключ:**
   ```bash
   head -c 16 /dev/urandom | xxd -ps
   ```

3. **Обновите конфигурацию клиента:**
   ```
   tg://proxy?server=example.com&port=443&secret=новый_ключ
   ```

---

### DDoS атака

**Симптомы:**
- Резкий рост количества подключений
- Высокое использование CPU/памяти
- Легитимные клиенты не могут подключиться

**Решение:**

1. **Включите rate limiting:**
   ```bash
   ./mtproto-proxy --rate-limit 100 --rate-window 60 ...
   ```

2. **Используйте whitelist:**
   ```bash
   admin-cli whitelist add trusted_ip
   ```

3. **Включите circuit breaker:**
   ```c
   // В конфигурации
   error_handler.circuit_breaker = true
   error_handler.threshold = 100
   ```

---

## ❌ Частые ошибки

### Ошибка: "address already in use"

**Решение:**
```bash
# Найдите процесс, занимающий порт
lsof -i :8888
netstat -tlnp | grep 8888

# Завершите процесс
kill -9 <PID>

# Или используйте SO_REUSEADDR (уже включено)
```

---

### Ошибка: "permission denied"

**Решение:**
```bash
# Проверьте права на файлы
ls -la proxy-secret proxy-multi.conf

# Исправьте права
chmod 600 proxy-secret
chmod 644 proxy-multi.conf

# Запустите от правильного пользователя
./mtproto-proxy -u nobody ...
```

---

### Ошибка: "configuration file not found"

**Решение:**
```bash
# Используйте абсолютные пути
./mtproto-proxy --aes-pwd /opt/mtproxy/proxy-secret /opt/mtproxy/proxy-multi.conf

# Или скопируйте файлы в рабочую директорию
cp proxy-secret proxy-multi.conf ./
```

---

## 🛠️ Диагностические команды

### Проверка состояния сервера

```bash
# Статус процесса
ps aux | grep mtproto

# Статистика
curl localhost:8888/stats

# Активные подключения
netstat -an | grep :443 | wc -l

# Использование памяти
cat /proc/$(pgrep mtproto)/status | grep VmRSS
```

### Логирование

```bash
# Запуск с debug логом
./mtproto-proxy -v ...

# Просмотр логов в реальном времени
tail -f /var/log/mtproxy.log

# Фильтрация по уровню
grep "ERROR" /var/log/mtproxy.log
```

### Тестирование

```bash
# Модульные тесты
make test

# Интеграционные тесты
./build/bin/test-new-modules.exe

# Performance тесты
./build/bin/cache-performance-test-simple.exe
```

---

## 📞 Поддержка

Если проблема не решена:

1. **Соберите информацию:**
   - Версия MTProxy: `./mtproto-proxy --version`
   - ОС и версия: `uname -a` или `systeminfo`
   - Логи ошибки
   - Конфигурация запуска

2. **Проверьте известные проблемы:**
   - [GitHub Issues](https://github.com/QuadDarv1ne/MTProxy/issues)
   - [CHANGELOG.md](CHANGELOG.md)

3. **Создайте issue:**
   - Описание проблемы
   - Шаги воспроизведения
   - Ожидаемое поведение
   - Фактическое поведение
   - Логи и скриншоты

---

*Последнее обновление: 25 марта 2026 г.*
*Версия документа: 1.0*
