# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** e45f27f (HEAD -> dev, origin/master, origin/dev)
> **Версия:** v1.0.28 ✅
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅ (синхронизированы)
> **Тег:** v1.0.27 ✅

---

## 🎉 RELEASED: v1.0.28 (29 марта 2026)

### ✅ Выполнено (v1.0.28 релиз)
- [x] **VERSION файл** — ✅ ОБНОВЛЁН (1.0.1 → 1.0.28)
- [x] **io_uring** — ✅ Linux io_uring (819 строк, ENABLE_IOURING)
- [x] **io_uring тесты** — ✅ 15 тестов
- [x] **io_uring бенчмарки** — ✅ 5 бенчмарков
- [x] **admin-cli интеграция** — ✅ 18 тестов
- [x] **metrics_collector тесты** — ✅ 20 Python тестов
- [x] **Docker интеграция** — ✅ test_docker_integration.py + docker-test.sh
- [x] **CHANGELOG v1.0.28** — ✅ полное описание
- [x] **RELEASE_NOTES v1.0.28** — ✅ релизный документ
- [x] **README.md** — ✅ обновлён для v1.0.28

---

## 📋 Активные задачи (v1.0.29)

### 🔴 В работе
- [ ] Docker multi-arch сборка (linux/amd64, linux/arm64, linux/arm/v7)

### 🟡 Планирование
- [ ] Интеграционные тесты (metrics_collector + Docker)
- [ ] Бенчмарки производительности (100K+ операций)

### 🔵 Q2 2026
- [ ] FreeBSD поддержка
- [ ] ARM64 Linux сборка (Raspberry Pi)

---

## 📊 Статистика

| Метрика | Значение |
|---------|----------|
| **Всего коммитов** | 481 |
| **C/H файлов** | 406+ |
| **Workflow** | 7 |
| **Тестов** | 110 (100 C + 6 Python + 4 Dart) |
| **Бенчмарков** | 7 |
| **Документов** | 25 |
| **Релизов** | 2 (v1.0.27, v1.0.28) |

---

## 📚 Документация

### Основная (25 файлов)
- README.md, PROJECT_INDEX.md, TESTING.md, BENCHMARKS.md
- CHANGELOG.md, SECURITY.md, CONTRIBUTING.md, DEPLOYMENT.md
- API_REFERENCE.md, ROADMAP.md, RELEASE_NOTES_v1.0.28.md
- docs/*.md (4 файла: ADMIN_CLI_GUIDE.md, DEBUGGING.md, PERFORMANCE_TUNING.md, TROUBLESHOOTING.md)

---

## 🎯 Roadmap

| Задача | Версия | Приоритет | Статус |
|--------|--------|-----------|--------|
| Docker multi-arch | v1.0.29 | 🔴 Высокий | Ожидает |
| Интеграционные тесты | v1.0.29 | 🟡 Средний | Ожидает |
| Бенчмарки 100K+ | v1.0.29 | 🟡 Средний | Ожидает |
| FreeBSD поддержка | Q2 2026 | 🔵 Планирование | Ожидает |
| ARM64 сборка | Q2 2026 | 🔵 Планирование | Ожидает |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. — v1.0.28 релиз завершён (481 коммит)*
