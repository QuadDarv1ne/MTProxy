# Debugging Guide

Руководство по отладке MTProxy с использованием AddressSanitizer и других инструментов.

## Содержание

1. [AddressSanitizer](#addresssanitizer)
2. [UndefinedBehaviorSanitizer](#undefinedbehaviorsanitizer)
3. [Отладка утечек памяти](#отладка-утечек-памяти)
4. [Отладка многопоточных ошибок](#отладка-многопоточных-ошибок)
5. [Полезные флаги компиляции](#полезные-флаги-компиляции)

---

## AddressSanitizer

**AddressSanitizer (ASan)** — инструмент для обнаружения ошибок работы с памятью:
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
# Запуск прокси с ASan
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

Переменные окружения для настройки ASan:

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

## UndefinedBehaviorSanitizer

**UndefinedBehaviorSanitizer (UBSan)** — обнаружение неопределённого поведения:
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
# Запуск с детекцией утечек
export ASAN_OPTIONS=detect_leaks=1:verbosity=1
./bin/mtproto-proxy.exe -p 8888

# Остановка прокси и проверка отчёта
# Ctrl+C — ASan выведет все обнаруженные утечки
```

### Пример отчёта об утечках

```
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 64 byte(s) in 1 object(s) allocated from:
    #0 0x7ff012345678 in malloc (/lib64/libasan.so.5+0x12345)
    #1 0x7ff012345679 in config_alloc config/config.c:45
    #2 0x7ff012345680 in init_config config/config.c:78

Indirect leak of 128 byte(s) in 2 object(s) allocated from:
    #0 0x7ff012345678 in malloc (/lib64/libasan.so.5+0x12345)
    #1 0x7ff012345679 in connection_alloc net/net-connections.c:100

SUMMARY: AddressSanitizer: 192 byte(s) leaked in 3 allocation(s)
```

---

## Отладка многопоточных ошибок

### ThreadSanitizer (TSan)

**ThreadSanitizer** — обнаружение гонок данных (data races).

### Сборка с TSan

```bash
# Очистка
rm -rf build && mkdir build
cd build

# Конфигурация с TSan
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=thread -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..

# Сборка
cmake --build . -j2
```

### Пример отчёта TSan

```
==12345==WARNING: ThreadSanitizer: data race (pid=12345)
  Read of size 4 at 0x7b0c00001234 by thread T1:
    #0 0x7ff012345678 in connection_get_refcnt net/net-connections.c:234
    #1 0x7ff012345679 in worker_thread engine/engine.c:456

  Previous write of size 4 at 0x7b0c00001234 by thread T0:
    #0 0x7ff012345680 in connection_set_refcnt net/net-connections.c:245
    #1 0x7ff012345681 in connection_decref net/net-connections.c:267

  Location is heap block of size 64 at 0x7b0c00001230 allocated by thread T0:
    #0 0x7ff012345682 in malloc (/lib64/libtsan.so.5+0x12345)
    #1 0x7ff012345683 in connection_alloc net/net-connections.c:100
```

---

## Полезные флаги компиляции

### Debug сборка (максимальная отладка)

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=ON \
  -DCMAKE_C_FLAGS="-g3 -O0 -Wall -Wextra -Wpedantic" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
```

### Release сборка (максимальная производительность)

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-O3 -march=native -flto" \
  -DCMAKE_EXE_LINKER_FLAGS="-flto" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
```

### Профилирование (PGO)

```bash
# Этап 1: Сборка с инструментацией
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_PGO=ON \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .

# Этап 2: Запуск с типичной нагрузкой
./bin/mtproto-proxy.exe -p 8888 --run-for 60

# Этап 3: Пересборка с использованием профиля
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_PGO_PROFILE=ON \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .
```

---

## GDB отладка

### Запуск в GDB

```bash
# Сборка без ASan (конфликтует с GDB)
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=OFF \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .

# Запуск в GDB
gdb ./bin/mtproto-proxy.exe
(gdb) break main
(gdb) run -p 8888 -S proxy-secret
```

### Полезные команды GDB

```
break <function>      # Установка точки останова
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
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=OFF \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .

# Запуск Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  --track-origins=yes --verbose \
  ./bin/mtproto-proxy.exe -p 8888 -S proxy-secret
```

### Пример отчёта Valgrind

```
==12345== 48 bytes in 1 blocks are definitely lost in loss record 1 of 1
==12345==    at 0x4C2FB55: malloc (vg_replace_malloc.c:299)
==12345==    by 0x412345: config_alloc (config.c:45)
==12345==    by 0x412346: init_config (config.c:78)
==12345==    by 0x412347: main (main.c:100)
```

---

## Быстрая диагностика проблем

### Проблема: Segmentation fault

```bash
# Сборка с ASan
cmake -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Запуск — ASan покажет точное место ошибки
./bin/mtproto-proxy.exe
```

### Проблема: Утечка памяти

```bash
# Запуск с детекцией утечек
export ASAN_OPTIONS=detect_leaks=1
./bin/mtproto-proxy.exe

# Или Valgrind
valgrind --leak-check=full ./bin/mtproto-proxy.exe
```

### Проблема: Гонка данных

```bash
# Сборка с TSan
cmake -DCMAKE_C_FLAGS="-fsanitize=thread" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" ..
cmake --build .

# Запуск — TSan обнаружит гонки
./bin/mtproto-proxy.exe
```

---

## Интеграция с CI/CD

Автоматическая проверка в CI:

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
      
      - name: Run tests with ASan
        run: |
          cd build
          ctest --output-on-failure
```

---

## Дополнительные ресурсы

- [AddressSanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [ThreadSanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [Valgrind User Guide](https://valgrind.org/docs/manual/manual.html)
- [GDB Documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
