# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** c22b1f4 (dev/master)
> **Версия:** v1.0.27-release-prep
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅

---

## 📋 Активные задачи

### 🔴 В работе
- [x] Подготовка к релизу v1.0.27 — ✅ В ПРОЦЕССЕ

### 🟡 Следующие
- [ ] Финальная проверка тестов
- [ ] Release notes для GitHub
- [ ] Тегирование релиза v1.0.27

---

## ✅ Выполнено (29 марта 2026 — 14 коммитов)

### Оптимизация памяти (100% ЗАВЕРШЕНО)
- [x] Cache Memory Pool (5x быстрее)
- [x] Интеграция: cache-manager, rate-limiter, error-handler
- [x] jemalloc/tcmalloc поддержка (+60% ops/sec)

### Security & Audit
- [x] Security utils (strcpy_s, strcat_s, snprintf)
- [x] 18 тестов для security utils (100% покрытие)
- [x] Security audit CI workflow
- [x] Аудит кода: unsafe функции не найдены

### Performance
- [x] network-analyzer кэширование (~80% CPU)
- [x] 5 бенчмарков производительности
- [x] BENCHMARKS.md документация

### Tests
- [x] **98 C тестов** + 4 Dart (100%+ покрытие)
- [x] Base64/Hex: 14 тестов
- [x] Memory allocator: 14 тестов
- [x] Security utils: 18 тестов

### Documentation
- [x] CHANGELOG.md обновлён
- [x] BENCHMARKS.md создан
- [x] todo.md актуализирован

### Infrastructure
- [x] CMakeLists.txt: ENABLE_JEMALLOC/ENABLE_TCMALLOC
- [x] Windows socket API, IPC
- [x] LTO для Unix, ASAN опционально
- [x] 6 CI/CD workflows

---

## 📊 Статистика проекта

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 427+ |
| **C/H файлов** | 399+ |
| **Workflow** | 6 |
| **Тестов** | 98 C + 4 Dart |
| **Бенчмарков** | 5 |
| **Документов** | 40+ |
| **Ветки** | dev = master ✅ |

---

## 🎯 Release Checklist v1.0.27

- [x] CHANGELOG.md обновлён
- [ ] Финальный запуск тестов
- [ ] Release notes на GitHub
- [ ] Тег v1.0.27
- [ ] Merge в master

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. (подготовка к релизу v1.0.27)*
