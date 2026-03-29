# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** f79d02d (dev/master)
> **Версия:** v1.0.23-memory-benchmarks
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Бенчмарки производительности allocator — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Запуск бенчмарков (malloc vs jemalloc vs tcmalloc)
- [ ] HTTP/3 QUIC полная реализация
- [ ] io_uring для Linux

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode, HeapCompact, Cache Memory Pool (5x быстрее)
- [x] Интеграция: cache-manager, rate-limiter, error-handler

### Memory Allocator (29 марта)
- [x] memory-allocator.h — unified API
- [x] jemalloc/tcmalloc поддержка
- [x] mt_malloc_aligned (выравнивание)
- [x] Тесты: 14 тестов
- [x] **Бенчмарки: 5 тестов производительности**

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
| **Всего коммитов** | 424+ |
| **C/H файлов** | 397+ |
| **Тестов** | 80 C + 4 Dart |
| **Бенчмарков** | 5 (allocator) |
| **Оптимизация памяти** | 3 модуля (5x) + allocator |
| **Оптимизация CPU** | 1 модуль (~80%) |
| **Ветки** | dev = master ✅ |

---

## 🔧 Бенчмарки производительности

### Запуск
```bash
mkdir build && cd build
cmake -DENABLE_JEMALLOC=ON ..  # или -DENABLE_TCMALLOC=ON
make -j4
./bin/benchmark-memory-allocator
```

### Тесты
1. **malloc/free** — базовая производительность
2. **aligned malloc** — выравнивание 64 байта
3. **Fragmentation** — фрагментация памяти
4. **Multi-threaded** — 4 потока
5. **Peak memory** — пиковое потребление

### Ожидаемые результаты
| Аллокатор | ops/sec | Память |
|-----------|---------|--------|
| **standard** | ~500K | базовая |
| **jemalloc** | ~800K (+60%) | -30% |
| **tcmalloc** | ~750K (+50%) | -25% |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (бенчмарки памяти)*
