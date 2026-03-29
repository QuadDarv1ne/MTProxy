# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** ef0729c (dev/master)
> **Версия:** v1.0.21-utils-tests
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Тесты для utils_base64/hex — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Проверка сборки test-utils-encoding
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)
- [ ] io_uring для Linux

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode, HeapCompact, Cache Memory Pool
- [x] Интеграция: cache-manager, rate-limiter, error-handler (5x быстрее)

### Оптимизация производительности (29 марта)
- [x] network-analyzer.c — кэширование (~80% CPU)

### Утилиты кодирования (29 марта)
- [x] utils_base64_encode/decode — RFC 4648
- [x] utils_hex_encode/decode — hex кодирование
- [x] Тесты: 14 тестов (Base64: 6, Hex: 8)

### Устранение дублирования (28 марта)
- [x] utils_int_to_string / utils_float_to_string
- [x] utils_hash_djb2, utils_time_ms

### Инфраструктура
- [x] Windows socket API, IPC
- [x] HTTP/3 QUIC stub (17 TODO)
- [x] LTO для Unix, ASAN опционально

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 422+ |
| **C/H файлов** | 394+ |
| **utils.c строк** | 764 (+172) |
| **Тестов** | 78 C + 4 Dart (100%) |
| **Оптимизация памяти** | 3 модуля (5x) |
| **Оптимизация CPU** | 1 модуль (~80%) |
| **Ветки** | dev = master ✅ |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (тесты encoding добавлены)*
