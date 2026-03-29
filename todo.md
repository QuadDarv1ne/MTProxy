# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** b3413c7 (dev/master)
> **Версия:** v1.0.20-utils-encoding
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] utils.c — Base64/Hex кодирование — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Тесты для utils_base64_encode/decode
- [ ] Тесты для utils_hex_encode/decode
- [ ] HTTP/3 QUIC полная реализация (nghttp3/ngtcp2)

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode для тестов
- [x] HeapCompact для Windows
- [x] Cache Memory Pool (5x быстрее calloc/free)
- [x] Интеграция: cache-manager, rate-limiter, error-handler

### Оптимизация производительности (29 марта)
- [x] network-analyzer.c — кэширование вычислений (5 сек, ~80% CPU)

### Утилиты кодирования (29 марта)
- [x] utils_base64_encode/decode — RFC 4648 совместимость
- [x] utils_hex_encode/decode — hex кодирование
- [x] ~170 строк кода добавлено

### Устранение дублирования (28 марта)
- [x] utils_int_to_string / utils_float_to_string
- [x] utils_hash_djb2
- [x] get_current_time_ms → utils_time_ms

### Инфраструктура
- [x] Windows socket API
- [x] Windows IPC (Named Pipes)
- [x] HTTP/3 QUIC stub (17 TODO)
- [x] LTO для Unix, ASAN опционально

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 421+ |
| **C/H файлов** | 392+ |
| **utils.c строк** | 764 (+172) |
| **Тестов** | 77 C + 4 Dart (100%) |
| **Оптимизация памяти** | 3 модуля (5x быстрее) |
| **Оптимизация CPU** | 1 модуль (~80%) |
| **Утилиты** | Base64, Hex |
| **Ветки** | dev = master ✅ |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (utils encoding добавлен)*
