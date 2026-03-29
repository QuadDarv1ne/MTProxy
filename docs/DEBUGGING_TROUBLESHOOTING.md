# Debugging and Troubleshooting Guide

Полное руководство по отладке и диагностике проблем MTProxy.

---

# Часть 1: Отладка (Debugging)

## AddressSanitizer (ASan)

**AddressSanitizer** — инструмент для обнаружения ошибок работы с памятью:
- Выход за границы массива (buffer overflow)
- Использование после освобождения (use-after-free)
- Двойное освобождение (double-free)
- Утечки памяти (memory leaks)

### Сборка с ASan

```bash
# Очистка build директории
rm -rf build && mkdir build
cd build

# Конфигурация с ASan (Debug режим)
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=ON \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..

# Сборка
cmake --build . -j2
```

### Запуск с ASan

```bash
./bin/mtproto-proxy.exe -p 8888 -S proxy-secret
# ASan автоматически обнаружит ошибки и выведет отчёт
```

### Пример отчёта ASan

```
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000001234
READ of size 4 at 0x602000001234 thread T0
    #0 0x7ff012345678 in tcp_server_free_connection net/net-connections.c:123
    #1 0x7ff012345679 in connection_decref net/net-connections.c:456
    #2 0x7ff012345680 in main_loop engine/engine.c:789

0x602000001234 is located 4 bytes inside of 64-byte region [0x602000001230,0x602000001270)
freed by thread T0 here:
    #0 0x7ff012345681 in free (/lib64/libasan.so.5+0x12345)
    #1 0x7ff012345682 in connection_free net/net-connections.c:234

previously allocated by thread T0 here:
    #0 0x7ff012345683 in malloc (/lib64/libasan.so.5+0x12346)
    #1 0x7ff012345684 in connection_alloc net/net-connections.c:100
```

### Настройки ASan

```bash
# Детальная отчётность
export ASAN_OPTIONS=detect_leaks=1:log_path=asan_report.txt:halt_on_error=0

# Запуск с проверкой утечек
./bin/mtproto-proxy.exe -p 8888
```

**Полезные опции:**
- `detect_leaks=1` — включение детектора утечек
- `log_path=asan_report.txt` — запись отчёта в файл
- `halt_on_error=0` — продолжение работы после ошибки
- `verbosity=1` — подробный вывод

---

## UndefinedBehaviorSanitizer (UBSan)

**UBSan** обнаруживает неопределённое поведение:
- Выход за границы целочисленных типов (integer overflow)
- Невыровненное обращение к памяти (misaligned access)
- Null pointer dereference

### Сборка с UBSan

UBSan включен по умолчанию вместе с ASan через `-fsanitize=undefined`.

### Пример отчёта UBSan

```
==12345==ERROR: UndefinedBehaviorSanitizer: undefined-behavior net/net-crypto.c:123:4
runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

---

## Отладка утечек памяти

### LeakSanitizer (LSan)

LSan автоматически включается вместе с ASan.

```bash
export ASAN_OPTIONS=detect_leaks=1:verbosity=1
./bin/mtproto-proxy.exe -p 8888

# Остановка прокси и проверка отчёта (Ctrl+C)
```

### Пример отчёта об утечках

```
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 64 byte(s) in 1 object(s) allocated from:
    #0 0x7ff012345678 in malloc (/lib64/libasan.so.5+0x12345)
    #1 0x7ff012345679 in config_alloc config/config.c:45
    #2 0x7ff012345680 in init_config config/config.c:78

SUMMARY: AddressSanitizer: 192 byte(s) leaked in 3 allocation(s)
```

---

## Отладка многопоточных ошибок

### ThreadSanitizer (TSan)

**ThreadSanitizer** обнаруживает гонки данных (data races).

### Сборка с TSan

```bash
rm -rf build && mkdir build
cd build

cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=thread -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..

cmake --build . -j2
```

### Пример отчёта TSan

```
==12345==WARNING: ThreadSanitizer: data race (pid=12345)
  Read of size 4 at 0x7b0c00001234 by thread T1:
    #0 0x7ff012345678 in connection_get_refcnt net/net-connections.c:234

  Previous write of size 4 at 0x7b0c00001234 by thread T0:
    #0 0x7ff012345680 in connection_set_refcnt net/net-connections.c:245
```

---

## GDB отладка

### Запуск в GDB

```bash
# Сборка без ASan (конфликтует с GDB)
cmake -DENABLE_ASAN=OFF -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Запуск в GDB
gdb ./bin/mtproto-proxy.exe
(gdb) break main
(gdb) run -p 8888 -S proxy-secret
```

### Полезные команды GDB

```
break <function>      # Точка останова
break <file>:<line>   # Точка останова по строке
continue              # Продолжение выполнения
next                  # Следующая строка (без захода в функцию)
step                  # Следующая строка (с заходом в функцию)
print <variable>      # Вывод переменной
backtrace             # Трассировка стека
info threads          # Список потоков
thread <id>           # Переключение на поток
```

---

## Valgrind

**Valgrind** — инструмент для обнаружения утечек и ошибок памяти.

### Запуск Valgrind

```bash
# Сборка без ASan
cmake -DENABLE_ASAN=OFF -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Запуск Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  --track-origins=yes --verbose \
  ./bin/mtproto-proxy.exe -p 8888 -S proxy-secret
```

---

## Флаги компиляции

### Debug сборка

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=ON \
  -DCMAKE_C_FLAGS="-g3 -O0 -Wall -Wextra -Wpedantic" \
  ..
```

### Release сборка

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-O3 -march=native -flto" \
  ..
```

### Профилирование (PGO)

```bash
# Этап 1: Сборка с инструментацией
cmake -DENABLE_PGO=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Этап 2: Запуск с типичной нагрузкой
./bin/mtproto-proxy.exe -p 8888 --run-for 60

# Этап 3: Пересборка с профилем
cmake -DUSE_PGO_PROFILE=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

---

# Часть 2: Диагностика проблем (Troubleshooting)

## Проблемы сборки

### CMake не находит компилятор

**Симптомы:** `CMake Error: CMAKE_C_COMPILER not set`

**Решение:**

**Windows (MSYS2):**
```bash
where gcc g++
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
```

**Linux:**
```bash
sudo apt-get install build-essential cmake ninja-build
```

### CMake не находит OpenSSL

**Решение:**
```bash
# Windows
pacman -S mingw-w64-ucrt-x86_64-openssl

# Linux
sudo apt-get install libssl-dev
```

### Нехватка памяти при сборке

**Решение:**
```bash
cd build
cmake -DCMAKE_BUILD_PARALLEL_LEVEL=2 ..
cmake --build . -j2

# Или включить low-memory режим
cmake -DENABLE_LOW_MEMORY=ON ..
```

---

## Проблемы запуска

### Cannot open secret file

**Симптомы:** `cannot open password file proxy-secret`

**Решение:**
```bash
# Генерация секрета
openssl rand -hex 16 > proxy-secret

# Проверка прав
chmod 600 proxy-secret
```

### Cannot bind to port

**Симптомы:** `cannot bind to port 8888: Address already in use`

**Решение:**
```bash
# Проверка порта
netstat -ano | findstr :8888

# Освободить порт
taskkill /PID <PID> /F

# Или использовать другой порт
./mtproto-proxy.exe -p 8889
```

### Fork failed (Windows)

**Решение:**
```bash
# Windows не поддерживает fork - используйте single-worker
./mtproto-proxy.exe -M 1 -p 8888
# или
./mtproto-proxy.exe --single-thread
```

### Too many open files

**Решение (Linux):**
```bash
ulimit -n 65536

# Постоянное изменение
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
```

---

## Проблемы производительности

### Низкая пропускная способность

**Решение:**
```bash
# Включить многопоточность
./mtproto-proxy.exe -M 4 -p 8888

# Увеличить буфер
./mtproto-proxy.exe --tcp-buffer-size 262144 -p 8888
```

### Высокое использование памяти

**Решение:**
```bash
# Low-memory режим сборки
cmake -DENABLE_LOW_MEMORY=ON ..

# Ограничить кэш
./mtproto-proxy.exe --cache-size 64 -p 8888

# Уменьшить connections
./mtproto-proxy.exe --max-connections 10000 -p 8888
```

### Высокая задержка

**Решение:**
```bash
# TCP_NODELAY
./mtproto-proxy.exe --tcp-nodelay -p 8888

# Keepalive
./mtproto-proxy.exe --tcp-keepalive -p 8888
```

---

## Сетевые проблемы

### Клиенты не могут подключиться

**Решение:**
```bash
# Проверить брандмауэр
# Windows:
netsh advfirewall firewall add rule name="MTProxy" dir=in action=allow protocol=TCP localport=8888

# Linux:
sudo iptables -A INPUT -p tcp --dport 8888 -j ACCEPT

# Проверить прослушивание
netstat -ano | findstr :8888
```

### Обрыв соединения

**Решение:**
```bash
# Увеличить timeout
./mtproto-proxy.exe --read-timeout 600 --write-timeout 600 -p 8888

# Включить keepalive
./mtproto-proxy.exe --tcp-keepalive -p 8888
```

---

## Логирование и диагностика

### Включение подробного логирования

```bash
./mtproto-proxy.exe --log-level 3 --log-file proxy.log -p 8888
```

### Проверка через REST API

```bash
# Статистика
curl http://localhost:8888/stats

# Метрики
curl http://localhost:8888/metrics

# Статус
curl http://localhost:8888/status

# Health check
curl http://localhost:8888/health
```

### Диагностика через admin-cli

```bash
./mtproxy-admin.exe

# Команды:
> status          # Статус сервера
> stats           # Расширенная статистика
> connections     # Список подключений
> config          # Текущая конфигурация
> cache stats     # Статистика кэша
> health          # Проверка здоровья
```

---

## Быстрая диагностика

| Проблема | Решение |
|----------|---------|
| Segmentation fault | Сборка с ASan: `-DENABLE_ASAN=ON` |
| Утечка памяти | `export ASAN_OPTIONS=detect_leaks=1` |
| Гонка данных | Сборка с TSan: `-fsanitize=thread` |
| Высокое CPU | Уменьшить workers: `-M 1` |
| Timeout | Увеличить timeout: `--read-timeout 600` |

---

## Интеграция с CI/CD

```yaml
# .github/workflows/sanitizers.yml
name: Sanitizers
on: [push, pull_request]
jobs:
  asan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build with ASan
        run: |
          mkdir build && cd build
          cmake -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug ..
          cmake --build .
      - name: Run tests
        run: ctest --output-on-failure
```

---

## Дополнительные ресурсы

- [AddressSanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [Valgrind User Guide](https://valgrind.org/docs/manual/manual.html)
- [GDB Documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [API_REFERENCE.md](../API_REFERENCE.md)
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md)
