# Performance Tuning Guide

Руководство по оптимизации производительности MTProxy.

## Содержание

1. [Быстрая настройка](#быстрая-настройка)
2. [Оптимизация сборки](#оптимизация-сборки)
3. [Настройка runtime](#настройка-runtime)
4. [Оптимизация памяти](#оптимизация-памяти)
5. [Сетевая оптимизация](#сетевая-оптимизация)
6. [Многопоточность](#многопоточность)
7. [Мониторинг и профилирование](#мониторинг-и-профилирование)
8. [Бенчмарки](#бенчмарки)

---

## Быстрая настройка

### Оптимальная конфигурация для production

```bash
# Запуск с оптимизированными параметрами
./mtproto-proxy.exe \
  -p 8888 \
  -M 4 \
  --tcp-buffer-size 262144 \
  --max-connections 65535 \
  --cache-size 256 \
  --optimize-for-throughput \
  --tcp-nodelay \
  --tcp-keepalive
```

**Параметры:**
- `-M 4` — 4 worker потока (адаптируйте под CPU)
- `--tcp-buffer-size 262144` — 256KB буфер
- `--max-connections 65535` — максимум подключений
- `--cache-size 256` — 256MB кэш
- `--optimize-for-throughput` — оптимизация пропускной способности
- `--tcp-nodelay` — отключение Nagle algorithm
- `--tcp-keepalive` — keepalive соединения

---

## Оптимизация сборки

### Compiler optimizations

**Release сборка с максимальными оптимизациями:**

```bash
cd build
rm -rf *
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-O3 -march=native -flto -ffunction-sections -fdata-sections" \
  -DCMAKE_EXE_LINKER_FLAGS="-flto -Wl,--gc-sections" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build . -j$(nproc)
```

**Флаги оптимизации:**
- `-O3` — максимальная оптимизация
- `-march=native` — оптимизация под текущий CPU
- `-flto` — Link-Time Optimization
- `-ffunction-sections` — размещение функций в отдельных секциях
- `-fdata-sections` — размещение данных в отдельных секциях
- `-Wl,--gc-sections` — удаление неиспользуемых секций

### Profile-Guided Optimization (PGO)

**Этап 1: Сборка с инструментацией**

```bash
cd build
rm -rf *
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_PGO=ON \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .
```

**Этап 2: Сбор профилировочных данных**

```bash
# Запуск с типичной нагрузкой (30-60 минут)
./bin/mtproto-proxy.exe -p 8888 -M 4

# Или через нагрузочное тестирование
ab -n 1000000 -c 100 http://localhost:8888/stats
```

**Этап 3: Пересборка с использованием профиля**

```bash
cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_PGO_PROFILE=ON \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
cmake --build .
```

**Ожидаемый прирост:** +10-20% производительности

### Cross-Platform оптимизации

**Linux (с io_uring):**

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_IO_URING=ON \
  -DCMAKE_C_FLAGS="-O3 -march=native" \
  ..
```

**Windows:**

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-O3 -march=x86-64-v3" \
  -DCMAKE_MAKE_PROGRAM=../deps/ninja.exe \
  ..
```

**ARM64 (Raspberry Pi 4+):**

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_ARM64_CRYPTO=ON \
  -DCMAKE_C_FLAGS="-O3 -march=armv8-a+crypto+neon" \
  ..
```

---

## Настройка runtime

### Выбор количества workers

**Авто-детект (по умолчанию):**
```bash
./mtproto-proxy.exe -p 8888
# workers = CPU cores (минимум 2, максимум MAX_WORKERS)
```

**Ручная настройка:**
```bash
# Формула: workers = CPU_cores * 2
./mtproto-proxy.exe -M 8 -p 8888  # для 4-ядерного CPU
```

**Рекомендации:**
- 2 ядра: `-M 2-4`
- 4 ядра: `-M 4-8`
- 8 ядер: `-M 8-16`
- 16+ ядер: `-M 16-32`

### Оптимизация TCP параметров

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --tcp-buffer-size 262144 \
  --tcp-nodelay \
  --tcp-keepalive \
  --tcp-keepidle 60 \
  --tcp-keepintvl 10 \
  --tcp-keepcnt 6
```

**Параметры:**
- `--tcp-buffer-size` — размер буфера (128KB-512KB)
- `--tcp-nodelay` — отключение алгоритма Нейгла
- `--tcp-keepalive` — включение keepalive
- `--tcp-keepidle` — время до первого keepalive (сек)
- `--tcp-keepintvl` — интервал между keepalive (сек)
- `--tcp-keepcnt` — количество keepalive попыток

### Оптимизация таймаутов

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --read-timeout 300 \
  --write-timeout 300 \
  --connect-timeout 30 \
  --accept-timeout 1000
```

---

## Оптимизация памяти

### Low-memory режим

Для систем с ограниченной памятью (< 512MB):

```bash
# Сборка в low-memory режиме
cmake -DENABLE_LOW_MEMORY=ON ..
cmake --build .

# Запуск с ограничением памяти
./mtproto-proxy.exe \
  -p 8888 \
  --cache-size 64 \
  --pool-size 32 \
  --max-connections 10000
```

### Настройка кэша

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --cache-size 256 \
  --cache-algorithm lru \
  --cache-partitions 16
```

**Алгоритмы кэширования:**
- `lru` — Least Recently Used (по умолчанию)
- `lfu` — Least Frequently Used
- `fifo` — First In First Out
- `arc` — Adaptive Replacement Cache

**Размер кэша:**
- 64MB — low-memory системы
- 128MB — стандартная конфигурация
- 256MB — high-performance
- 512MB+ — enterprise

### Настройка memory pool

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --use-memory-pool \
  --pool-size 128 \
  --pool-block-size 4096
```

---

## Сетевая оптимизация

### Оптимизация для высокой пропускной способности

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --optimize-for-throughput \
  --tcp-buffer-size 524288 \
  --max-packet-size 262144 \
  --send-buffer-size 524288 \
  --recv-buffer-size 524288
```

### Оптимизация для низкой задержки

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --optimize-for-latency \
  --tcp-nodelay \
  --tcp-buffer-size 65536 \
  --max-packet-size 16384 \
  --flush-interval 1
```

### Настройка сокетов

```bash
./mtproto-proxy.exe \
  -p 8888 \
  --socket-backlog 4096 \
  --reuse-port \
  --reuse-addr \
  --no-delay
```

---

## Многопоточность

### Конфигурация потоков

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 8 \
  --io-threads 16 \
  --cpu-threads 8 \
  --tcp-cpu-threads 4 \
  --tcp-io-threads 8
```

**Параметры:**
- `-M` — количество workers
- `--io-threads` — потоки ввода-вывода
- `--cpu-threads` — CPU потоки
- `--tcp-cpu-threads` — TCP CPU потоки
- `--tcp-io-threads` — TCP IO потоки

### Балансировка нагрузки

Для многопоточного режима:

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 4 \
  --accept-round-robin \
  --connection-balance
```

---

## Мониторинг и профилирование

### Встроенный мониторинг

```bash
# Статистика в реальном времени
watch -n 1 'curl -s http://localhost:8888/stats | jq'

# Метрики Prometheus
curl http://localhost:8888/metrics

# Health check
curl http://localhost:8888/health
```

### Профилирование с perf (Linux)

```bash
# Запись профиля
perf record -g -p $(pgrep mtproto-proxy) sleep 30

# Анализ
perf report --stdio

# Или через flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### Профилирование с VTune (Windows)

```bash
# Запуск с профилированием
vtune -collect hotspots -result-dir vtune_result \
  mtproto-proxy.exe -p 8888

# Анализ
vtune -report summary -result-dir vtune_result
```

### Профилирование памяти

```bash
# Massif (Valgrind)
valgrind --tool=massif ./mtproto-proxy.exe -p 8888

# Анализ
ms_print massif.out.PID

# Или с ASan
export ASAN_OPTIONS=detect_leaks=1:verbosity=1
./mtproto-proxy.exe -p 8888
```

---

## Бенчмарки

### Тестирование пропускной способности

```bash
# Apache Bench
ab -n 1000000 -c 100 http://localhost:8888/stats

# wrk (HTTP benchmark)
wrk -t4 -c100 -d60s http://localhost:8888/stats

# Собственный тест
./bin/rate-limiter-highload-test-simple.exe
```

### Тестирование задержки

```bash
# ping
ping -c 100 localhost

# curl timing
curl -w "@curl-format.txt" -o /dev/null -s http://localhost:8888/stats

# curl-format.txt:
# time_namelookup:  %{time_namelookup}\n
# time_connect:     %{time_connect}\n
# time_starttransfer: %{time_starttransfer}\n
# time_total:       %{time_total}\n
```

### Тестирование памяти

```bash
# Мониторинг памяти
watch -n 1 'ps -o pid,rss,vsz,comm -p $(pgrep mtproto-proxy)'

# Или через /proc (Linux)
cat /proc/$(pgrep mtproto-proxy)/status | grep -E "VmRSS|VmSize"
```

### Benchmark скрипт

```bash
#!/bin/bash
# benchmark.sh

echo "=== MTProxy Benchmark ==="

# Параметры
HOST=${1:-localhost}
PORT=${2:-8888}
DURATION=${3:-60}

echo "Target: $HOST:$PORT"
echo "Duration: ${DURATION}s"

# Тест 1: Пропускная способность
echo -e "\n=== Throughput Test ==="
ab -n 100000 -c 100 http://$HOST:$PORT/stats

# Тест 2: Задержка
echo -e "\n=== Latency Test ==="
for i in {1..100}; do
  curl -s -o /dev/null -w "%{time_total}\n" http://$HOST:$PORT/stats
done | awk '{sum+=$1} END {print "Average: " sum/NR "s"}'

# Тест 3: Память
echo -e "\n=== Memory Usage ==="
ps -o pid,rss,vsz,comm -p $(pgrep mtproto-proxy)

echo -e "\n=== Benchmark Complete ==="
```

---

## Рекомендуемые конфигурации

### Home Server (2 ядра, 1GB RAM)

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 2 \
  --cache-size 64 \
  --pool-size 32 \
  --max-connections 1000 \
  --tcp-buffer-size 65536
```

### VPS (4 ядра, 4GB RAM)

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 4 \
  --cache-size 128 \
  --pool-size 64 \
  --max-connections 10000 \
  --tcp-buffer-size 131072 \
  --tcp-nodelay
```

### Dedicated Server (8+ ядер, 16GB+ RAM)

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 8 \
  --cache-size 512 \
  --pool-size 256 \
  --max-connections 65535 \
  --tcp-buffer-size 262144 \
  --tcp-nodelay \
  --tcp-keepalive \
  --optimize-for-throughput
```

### High-Performance Cluster (16+ ядер, 32GB+ RAM)

```bash
./mtproto-proxy.exe \
  -p 8888 \
  -M 16 \
  --cache-size 1024 \
  --pool-size 512 \
  --max-connections 100000 \
  --tcp-buffer-size 524288 \
  --tcp-nodelay \
  --tcp-keepalive \
  --optimize-for-throughput \
  --reuse-port
```

---

## Чеклист оптимизации

- [ ] Сборка в Release режиме с `-O3 -flto`
- [ ] PGO профилирование выполнено
- [ ] Количество workers настроено под CPU
- [ ] TCP буферы оптимизированы
- [ ] Кэш настроен под доступную память
- [ ] TCP_NODELAY включен
- [ ] Keepalive включен
- [ ] Таймауты настроены
- [ ] Мониторинг настроен
- [ ] Бенчмарки пройдены

---

## Дополнительные ресурсы

- [Linux Performance Tuning](https://www.brendangregg.com/linuxperf.html)
- [FreeBSD Performance Tuning](https://docs.freebsd.org/en/books/handbook/performance/)
- [TCP/IP Performance Tuning](https://access.redhat.com/sites/default/files/attachments/20150129_network_performance_tuning.pdf)
