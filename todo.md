# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 73bb2f0 (HEAD → dev/master)
> **Версия:** v1.0.19-cache-no-copy
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅ (синхронизированы)

---

## 📋 Активные задачи

### 🔴 В работе
- [ ] Предварительное выделение пула записей (cache-manager)
- [ ] Сетевая оптимизация

### 🟡 Следующие
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)
- [ ] io_uring для Linux
- [ ] jemalloc/tcmalloc интеграция
- [ ] FreeBSD поддержка
- [ ] ARM64 Linux (Raspberry Pi)

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта)
- [x] Quick mode для тестов (флаг `--quick`)
- [x] HeapCompact для Windows
- [x] Cache Memory Pool (5x быстрее calloc/free)
- [x] test_memory_utils.h (мониторинг памяти)

### Устранение дублирования (28 марта)
- [x] utils_int_to_string / utils_float_to_string
- [x] utils_hash_djb2 (замена cache_hash_key/rate_limit_hash)
- [x] get_current_time_ms → utils_time_ms
- [x] **Итого:** 8 файлов, 128+ строк, utils.c/h (593 строки)

### Web UI, плагины, тесты (28 марта)
- [x] Web UI для управления прокси
- [x] Улучшена система плагинов
- [x] Тесты для admin-cli
- [x] Скрипты автоматизации тестирования

### Многопоточность (28 марта)
- [x] Многопоточный режим по умолчанию
- [x] Auto-detect CPU ядер
- [x] Опция --single-thread

### Безопасность (27 марта)
- [x] Проверки strdup с cleanup
- [x] ARM64 поддержка (CMakeLists.txt)
- [x] Проверки fopen на ошибки
- [x] Проверки pthread_mutex_init
- [x] Замена unsafe string функций (utils_strncpy)

### Оптимизация памяти (27 марта)
- [x] memory-limits модуль (OOM protection)
- [x] CMakeLists.txt: ENABLE_LOW_MEMORY, EXCLUDE_HEAVY_MODULES
- [x] Makefile: MEMORY_LIMIT_FLAGS

### Критические исправления (25-26 марта)
- [x] NULL pointer dereference (7 исправлений)
- [x] Бесконечные циклы (timeout + лимит итераций)
- [x] Use-after-free (hazard pointers)
- [x] Buffer overflow (aes_create_keys)
- [x] Missing socket timeout
- [x] Проверки malloc/calloc

### Новые модули (25 марта)
- [x] Docker Multi-Arch образы
- [x] CodeQL Security Analysis
- [x] Audit Logging System
- [x] REST API (13 endpoints)
- [x] Plugin System

### Инфраструктура
- [x] Windows socket API (WSASocket, bind, listen, accept)
- [x] Windows event loop (select вместо epoll)
- [x] Windows IPC (Named Pipes)
- [x] 5 Windows stub файлов
- [x] HTTP/3 QUIC stub (17 TODO реализовано)
- [x] LTO для Unix, ASAN опционально
- [x] CI/CD (5 платформ: Linux/Windows/macOS/Android/iOS)
- [x] FFI + Mobile app (Flutter/Dart)

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 416+ |
| **C/H файлов** | 392+ |
| **Тестов** | 77 C + 4 Dart (100%) |
| **Документов** | 38+ |
| **Сборка Windows** | ✅ (mtproto-proxy.exe) |
| **Ветки** | dev = master ✅ |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (коммит 73bb2f0)*
