# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 278576e (dev/master)
> **Версия:** v1.0.19-cache-no-copy
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Интеграция cache-memory-pool в error-handler — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Сетевая оптимизация (zero-copy, TCP_NODELAY)
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)
- [ ] Оптимизация network-analyzer.c

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта)
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

### Инфраструктура
- [x] Windows socket API
- [x] Windows IPC (Named Pipes)
- [x] HTTP/3 QUIC stub (17 TODO)
- [x] LTO для Unix, ASAN опционально
- [x] CI/CD (5 платформ)

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 418+ |
| **C/H файлов** | 392+ |
| **Тестов** | 77 C + 4 Dart (100%) |
| **Ветки** | dev = master ✅ |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г.*
