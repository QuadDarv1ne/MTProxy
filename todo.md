# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 1b878b6 (dev/master)
> **Версия:** v1.0.19-cache-pool-complete
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [ ] Оптимизация network-analyzer.c (кэширование метрик)

### 🟡 Следующие
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)
- [ ] io_uring для Linux
- [ ] jemalloc/tcmalloc интеграция

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode для тестов (флаг `--quick`)
- [x] HeapCompact для Windows
- [x] Cache Memory Pool (5x быстрее calloc/free)
- [x] test_memory_utils.h (мониторинг памяти)
- [x] Интеграция cache-memory-pool в cache-manager
- [x] Интеграция cache-memory-pool в rate-limiter
- [x] Интеграция cache-memory-pool в error-handler

### Устранение дублирования (28 марта)
- [x] utils_int_to_string / utils_float_to_string
- [x] utils_hash_djb2
- [x] get_current_time_ms → utils_time_ms

### Сетевая оптимизация
- [x] TCP_NODELAY установлен для всех сокетов
- [x] SO_KEEPALIVE с улучшенными параметрами
- [x] TCP_KEEPIDLE/TCP_KEEPINTVL/TCP_KEEPCNT

### Инфраструктура
- [x] Windows socket API
- [x] Windows IPC (Named Pipes)
- [x] HTTP/3 QUIC stub (17 TODO)
- [x] LTO для Unix, ASAN опционально
- [x] CI/CD (5 платформ)

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 419+ | +3 за день |
| **C/H файлов** | 392+ | — |
| **Тестов** | 77 C + 4 Dart (100%) | — |
| **Оптимизация памяти** | 3 модуля | cache, rate-limit, error |
| **Ускорение аллокаций** | ~5x | pool vs calloc |
| **Ветки** | dev = master ✅ | — |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (cache-memory-pool интеграция завершена)*
