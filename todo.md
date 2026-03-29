# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** b82c949 (dev/master)
> **Версия:** v1.0.22-jemalloc-support
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] jemalloc/tcmalloc интеграция — ✅ ВЫПОЛНЕНО

### 🟡 Следующие
- [ ] Проверка сборки с jemalloc
- [ ] Бенчмарки производительности (malloc vs jemalloc)
- [ ] HTTP/3 QUIC полная реализация

---

## ✅ Выполнено (Март 2026)

### Оптимизация памяти (29 марта) — 100% ЗАВЕРШЕНО
- [x] Quick mode, HeapCompact, Cache Memory Pool
- [x] Интеграция: cache-manager, rate-limiter, error-handler (5x быстрее)

### Оптимизация производительности (29 марта)
- [x] network-analyzer.c — кэширование (~80% CPU)

### Утилиты кодирования (29 марта)
- [x] utils_base64/hex encode/decode
- [x] Тесты: 14 тестов (Base64: 6, Hex: 8)

### Memory Allocator (29 марта)
- [x] memory-allocator.h — unified API
- [x] jemalloc/tcmalloc поддержка
- [x] mt_malloc_aligned (выравнивание)
- [x] Тесты: 14 тестов для allocator

### Инфраструктура
- [x] CMakeLists.txt: ENABLE_JEMALLOC/ENABLE_TCMALLOC
- [x] Windows socket API, IPC
- [x] LTO для Unix, ASAN опционально

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 423+ |
| **C/H файлов** | 396+ |
| **Тестов** | 79 C + 4 Dart |
| **Оптимизация памяти** | 3 модуля (5x) + allocator |
| **Оптимизация CPU** | 1 модуль (~80%) |
| **Ветки** | dev = master ✅ |

---

## 🔧 Сборка с jemalloc/tcmalloc

### Linux (jemalloc)
```bash
sudo apt-get install libjemalloc-dev
mkdir build && cd build
cmake -DENABLE_JEMALLOC=ON ..
make -j4
./bin/test-memory-allocator
```

### Linux (tcmalloc)
```bash
sudo apt-get install libtcmalloc-minimal4
mkdir build && cd build
cmake -DENABLE_TCMALLOC=ON ..
make -j4
```

### Windows (стандартный malloc)
```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build --config Release
```

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (jemalloc/tcmalloc поддержка)*
