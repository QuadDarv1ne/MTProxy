# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 2d2165d (HEAD -> dev, origin/master, origin/dev)
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

### 🔴 В работе
- [x] Dockerfile multi-arch — ✅ обновлён (TARGETARCH, оптимизации)
- [x] GitHub Actions workflow — ✅ docker-multiarch.yml (4 платформы)
- [ ] Тестирование multi-arch сборки

### 🟡 Планирование
- [ ] Интеграционные тесты (metrics_collector + Docker)
- [ ] Бенчмарки производительности (100K+ операций)

### 🔵 Q2 2026
- [ ] FreeBSD поддержка
- [ ] ARM64 Linux сборка (Raspberry Pi)

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 482 | +1 |
| **C/H файлов** | 406+ | 0 |
| **Workflow** | 8 | +1 |
| **Тестов** | 110 (100 C + 6 Python + 4 Dart) | 0 |
| **Бенчмарков** | 7 | 0 |
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
| Docker multi-arch | v1.0.29 | 🔴 Высокий | В работе |
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

*Последнее обновление: 29 марта 2026 г. — Docker multi-arch в работе (482 коммита)*
