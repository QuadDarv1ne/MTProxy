# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 971fd10 (dev/master)
> **Версия:** v1.0.24-security-utils
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Security utils (strcpy_s, strcat_s, snprintf) — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Тесты для utils_strcpy_s/strcat_s
- [ ] Аудит кода на использование unsafe функций
- [ ] HTTP/3 QUIC полная реализация

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode, HeapCompact, Cache Memory Pool (5x быстрее)
- [x] Интеграция: cache-manager, rate-limiter, error-handler

### Security Utils (29 марта)
- [x] utils_strcpy_s — безопасное копирование
- [x] utils_strcat_s — безопасная конкатенация
- [x] utils_snprintf — безопасный snprintf
- [x] Возврат -1 при truncation

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
| **Всего коммитов** | 425+ |
| **C/H файлов** | 397+ |
| **utils.c строк** | 826 (+62) |
| **Тестов** | 80 C + 4 Dart |
| **Бенчмарков** | 5 (allocator) |
| **Security функций** | 3 (strcpy_s, strcat_s, snprintf) |
| **Ветки** | dev = master ✅ |

---

## 🔧 Security функции

### Использование
```c
#include "common/utils.h"

char buffer[64];

// Безопасное копирование
if (utils_strcpy_s(buffer, sizeof(buffer), source) < 0) {
    // Произошло усечение
}

// Безопасная конкатенация
if (utils_strcat_s(buffer, sizeof(buffer), suffix) < 0) {
    // Произошло усечение
}

// Безопасный snprintf
if (utils_snprintf(buffer, sizeof(buffer), "%s: %d", name, value) < 0) {
    // Произошло усечение
}
```

### Преимущества
- ✅ Возврат -1 при усечении (можно обработать)
- ✅ Гарантированная null-терминация
- ✅ Проверка всех параметров
- ✅ Совместимость с существующим кодом

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (security utils добавлены)*
