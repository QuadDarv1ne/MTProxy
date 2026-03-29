# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** fb77e16 (dev/master)
> **Версия:** v1.0.25-security-tests
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Тесты для security utils — ✅ ВЫПОЛНЕНО (18 тестов)

### 🟡 Следующие
- [ ] Аудит кода на использование unsafe функций
- [ ] Замена strcpy/strcat на utils_strcpy_s в критических местах
- [ ] HTTP/3 QUIC полная реализация

---

## ✅ Выполнено (Март 2026)

### Security (29 марта)
- [x] utils_strcpy_s/strcat_s/snprintf — безопасные версии
- [x] **Тесты: 18 тестов для security utils**
- [x] Возврат -1 при truncation (обработка ошибок)

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

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 426+ |
| **C/H файлов** | 398+ |
| **utils.c строк** | 826 (+62) |
| **Тестов** | **98 C** + 4 Dart |
| **Бенчмарков** | 5 (allocator) |
| **Security функций** | 3 + 18 тестов |
| **Ветки** | dev = master ✅ |

---

## 🔧 Security функции

### Тесты
- ✅ **utils_strcpy_s** — 6 тестов (basic, truncation, NULL, edge cases)
- ✅ **utils_strcat_s** — 5 тестов (basic, truncation, NULL, full buffer)
- ✅ **utils_snprintf** — 6 тестов (basic, truncation, NULL, complex format)
- ✅ **Edge cases** — 3 теста (empty, long string, multiple strcat)

### Использование
```c
#include "common/utils.h"

char buffer[64];

// Безопасное копирование
if (utils_strcpy_s(buffer, sizeof(buffer), source) < 0) {
    fprintf(stderr, "Warning: string truncated\n");
}

// Безопасная конкатенация
if (utils_strcat_s(buffer, sizeof(buffer), suffix) < 0) {
    fprintf(stderr, "Warning: concatenation truncated\n");
}

// Безопасный snprintf
if (utils_snprintf(buffer, sizeof(buffer), "%s: %d", name, value) < 0) {
    fprintf(stderr, "Warning: format truncated\n");
}
```

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (security tests добавлены)*
