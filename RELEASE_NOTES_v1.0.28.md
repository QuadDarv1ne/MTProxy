# MTProxy v1.0.28 Release Notes

**Дата релиза:** 29 марта 2026 г.  
**Ветка:** `master` (0cff375)  
**Коммитов в релизе:** 16  
**Всего коммитов:** 480

---

## 🎉 Основные улучшения

### io_uring поддержка (Linux only)
Полная поддержка Linux io_uring для высокопроизводительного ввода-вывода:
- **Zero-copy операции** для уменьшения накладных расходов
- **Асинхронные I/O операции** без блокировки потоков
- **Поддержка Linux kernel 5.1+**
- **Опция сборки:** `-DENABLE_IOURING=ON`

**Файлы:**
- `net/io_uring.h` — API io_uring (263 строки)
- `net/io_uring.c` — реализация (556 строк)
- `CMakeLists.txt` — опция ENABLE_IOURING

**Использование:**
```bash
cmake -DENABLE_IOURING=ON ..
make -j4
```

---

## 🧪 Тестирование

### Новые тесты
- **test_io_uring.c** — 15 тестов для io_uring модуля
- **test_admin_cli_integration.c** — 18 интеграционных тестов admin-cli
- **test_metrics_collector.py** — 20 Python тестов для metrics_collector
- **test_docker_integration.py** — 12 Docker интеграционных тестов

### Бенчмарки
- **benchmark_io_uring.c** — 5 бенчмарков производительности io_uring
- **docker-test.sh** — Docker test runner

**Итого тестов:** 110 (100 C + 6 Python + 4 Dart)  
**Итого бенчмарков:** 7

---

## 🐳 Docker

### Готовые конфигурации
- **Dockerfile** — multi-stage сборка (runtime, debug)
- **docker-compose.yml** — production-ready с мониторингом
- **scripts/docker-test.sh** — автоматические тесты образа

**Компоненты:**
- MTProxy сервер
- Prometheus экспортёр
- Grafana дашборды
- Alertmanager уведомления

**Запуск:**
```bash
docker-compose up -d
```

---

## 📊 Статистика релиза

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| Коммитов | 480 | +16 |
| C/H файлов | 406+ | +4 |
| Тестов | 110 | +53 |
| Бенчмарков | 7 | +2 |
| Workflow | 7 | +1 |
| Документов | 24 | 0 |

---

## 🔧 Технические детали

### io_uring API
```c
// Инициализация
io_uring_ctx_t ctx;
io_uring_init(&ctx, 4096, 0);

// Submit операций
io_uring_submit_read(&ctx, fd, buffer, len, user_data);
io_uring_submit_write(&ctx, fd, buffer, len, user_data);

// Получение завершений
io_uring_wait_completions(&ctx, count, completions, timeout);

// Статистика
io_uring_get_stats(&ctx, &stats);
```

### Docker тесты
```bash
# Запуск всех тестов
./scripts/docker-test.sh all

# Только сборка
./scripts/docker-test.sh build

# Только тесты
./scripts/docker-test.sh test
```

---

## 📋 Полный список изменений

### Added
- `net/io_uring.h` — io_uring API header
- `net/io_uring.c` — io_uring implementation
- `testing/test_io_uring.c` — io_uring unit tests
- `testing/benchmark_io_uring.c` — io_uring benchmarks
- `testing/test_admin_cli_integration.c` — admin-cli integration tests
- `testing/test_metrics_collector.py` — metrics_collector tests
- `testing/test_docker_integration.py` — Docker integration tests
- `scripts/docker-test.sh` — Docker test runner

### Changed
- `CMakeLists.txt` — ENABLE_IOURING опция, test targets
- `VERSION` — 1.0.1 → 1.0.28
- `CHANGELOG.md` — v1.0.28 документация
- `todo.md` — актуализация статуса

---

## 🚀 Сборка и установка

### Linux (с io_uring)
```bash
# Установка зависимостей
apt install liburing-dev cmake build-essential

# Сборка
mkdir build && cd build
cmake -DENABLE_IOURING=ON -DCMAKE_BUILD_TYPE=Release ..
make -j4

# Запуск тестов
ctest --output-on-failure
```

### Docker
```bash
# Сборка образа
docker build -t mtproxy:latest .

# Запуск
docker run -d -p 8080:8080 mtproxy:latest

# Тесты
./scripts/docker-test.sh all
```

---

## ✅ Контрольный список релиза

- [x] VERSION обновлён
- [x] CHANGELOG.md заполнен
- [x] Все тесты проходят
- [x] Бенчмарки добавлены
- [x] Docker интеграция готова
- [x] Документация актуальна
- [x] Ветки синхронизированы

---

## 📞 Поддержка

- **Документация:** [docs/](docs/)
- **Issues:** [GitHub Issues](https://github.com/QuadDarv1ne/MTProxy/issues)
- **Обсуждения:** [GitHub Discussions](https://github.com/QuadDarv1ne/MTProxy/discussions)

---

*MTProxy Team — 29 марта 2026 г.*
