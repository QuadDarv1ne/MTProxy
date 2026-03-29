# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** d32f76c (dev/master)
> **Версия:** v1.0.26-security-audit
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Security audit workflow — ✅ ВЫПОЛНЕНО
- [x] BENCHMARKS.md документация — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)
- [ ] io_uring для Linux
- [ ] Web UI мониторинг

---

## ✅ Выполнено (Март 2026)

### Security & Audit (29 марта)
- [x] Security audit CI workflow (GitHub Actions)
- [x] Проверка на unsafe функции (strcpy, strcat, sprintf, gets)
- [x] Build с AddressSanitizer
- [x] BENCHMARKS.md документация

### Security Utils (29 марта)
- [x] utils_strcpy_s/strcat_s/snprintf — безопасные версии
- [x] Тесты: 18 тестов для security utils (100% покрытие)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode, HeapCompact, Cache Memory Pool (5x быстрее)
- [x] Интеграция: cache-manager, rate-limiter, error-handler

### Memory Allocator (29 марта)
- [x] memory-allocator.h — unified API
- [x] jemalloc/tcmalloc поддержка
- [x] mt_malloc_aligned (выравнивание)
- [x] Тесты: 14 тестов
- [x] Бенчмарки: 5 тестов производительности

### Утилиты кодирования (29 марта)
- [x] utils_base64/hex encode/decode + 14 тестов

### Оптимизация CPU (29 марта)
- [x] network-analyzer.c — кэширование (~80% CPU)

### Инфраструктура
- [x] CMakeLists.txt: ENABLE_JEMALLOC/ENABLE_TCMALLOC
- [x] Windows socket API, IPC
- [x] LTO для Unix, ASAN опционально
- [x] CI/CD: security-audit workflow

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 427+ |
| **C/H файлов** | 399+ |
| **Workflow** | 6 (CI, security, docker, flutter) |
| **Тестов** | **98 C** + 4 Dart |
| **Бенчмарков** | 5 (allocator) |
| **Документов** | 40+ (BENCHMARKS.md добавлен) |
| **Security функций** | 3 + 18 тестов |
| **Ветки** | dev = master ✅ |

---

## 🔧 CI/CD Workflows

### security-audit.yml
- ✅ Проверка на unsafe функции
- ✅ Статический анализ (cppcheck)
- ✅ Build с AddressSanitizer
- ✅ Build с security флагами
- ✅ Запуск всех тестов
- ✅ Еженедельный scheduled scan

### Запуск локально
```bash
# Security check
grep -rn '[^_]strcpy(' --include='*.c' | grep -v 'utils_strcpy'

# Build with ASAN
mkdir build-asan && cd build-asan
cmake -DENABLE_ASAN=ON ..
make -j4

# Run tests
ctest --output-on-failure
```

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (security audit + BENCHMARKS.md)*
