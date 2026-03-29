# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 8647416 (HEAD -> dev, origin/master, origin/dev)
> **Версия:** v1.0.29-dev (в разработке)
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅ (синхронизированы)
> **Тег:** v1.0.28 ✅

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

### 🔴 Выполнено
- [x] Dockerfile multi-arch — ✅ обновлён (TARGETARCH, оптимизации)
- [x] GitHub Actions workflow — ✅ docker-multiarch.yml (4 платформы)
- [x] CTest интеграция — ✅ 11 тестов зарегистрировано
- [x] test_common.h — ✅ единый тестовый фреймворк (195 строк)
- [x] test_io_uring.c — ✅ переведён на test_common.h (-57 строк, рефакторинг)
- [x] test_metrics_docker_integration.py — ✅ объединение metrics + Docker тестов (507 строк)
- [x] benchmark_highload.c — ✅ бенчмарки 100K-1M операций (413 строк, 6 тестов)
- [x] BENCHMARKS.md — ✅ обновлён (highload бенчмарки + io_uring)
- [x] TESTING.md — ✅ обновлён (100 C + 8 Python + 4 Dart тестов)
- [x] README.md — ✅ обновлён (112 тестов, 8 бенчмарков)

### 🟡 В работе
- [ ] Интеграционные тесты (metrics_collector + Docker) — ✅ ВЫПОЛНЕНО
- [ ] Бенчмарки производительности (100K+ операций) — ✅ ВЫПОЛНЕНО

### 🔵 Q2 2026
- [ ] FreeBSD поддержка
- [ ] ARM64 Linux сборка (Raspberry Pi)

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 489 | +1 |
| **C/H файлов** | 408+ | 0 |
| **Workflow** | 8 | 0 |
| **Тестов** | 112 (100 C + 8 Python + 4 Dart) | 0 |
| **Бенчмарков** | 8 | 0 |
| **Документов** | 25 | 0 |
| **Релизов** | 2 (v1.0.27, v1.0.28) | 0 |

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
| Интеграционные тесты | v1.0.29 | 🔴 Высокий | ✅ Выполнено |
| Бенчмарки 100K+ | v1.0.29 | 🟡 Средний | ✅ Выполнено |
| FreeBSD поддержка | Q2 2026 | 🔵 Планирование | Ожидает |
| ARM64 сборка | Q2 2026 | 🔵 Планирование | Ожидает |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. — документация обновлена (489 коммитов)*
