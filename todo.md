# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 80d1022 (HEAD -> dev, origin/master, origin/dev)
> **Версия:** v1.0.29-dev (Q3 2026 ✅ завершено, RELEASE_NOTES подготовлены)
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
- [x] CTest интеграция — ✅ 14 тестов зарегистрировано
- [x] test_common.h — ✅ единый тестовый фреймворк (195 строк)
- [x] test_io_uring.c — ✅ переведён на test_common.h (-57 строк, рефакторинг)
- [x] test_metrics_docker_integration.py — ✅ объединение metrics + Docker тестов (507 строк)
- [x] benchmark_highload.c — ✅ бенчмарки 100K-1M операций (413 строк, 6 тестов)
- [x] benchmark_cache_performance.c — ✅ кэш performance тесты (560 строк, 6 тестов)
- [x] BENCHMARKS.md — ✅ обновлён (highload бенчмарки + io_uring)
- [x] TESTING.md — ✅ обновлён (100 C + 8 Python + 4 Dart тестов)
- [x] README.md — ✅ обновлён (112 тестов, 8 бенчмарков)

### 🟡 Q2 2026 (в работе)
- [x] admin-cli интеграционные тесты — ✅ ВЫПОЛНЕНО (17 тестов, 699 строк)
- [x] Кэш performance тесты — нагрузка 100K+ операций — ✅ ВЫПОЛНЕНО
- [x] FreeBSD поддержка — toolchain + документация — ✅ ВЫПОЛНЕНО
- [x] ARM64 Linux — toolchain + документация — ✅ ВЫПОЛНЕНО

### 🟢 Q3 2026 (в работе)
- [x] Prometheus экспортёр — расширенные метрики — ✅ ВЫПОЛНЕНО
- [x] Alerting rules — 20+ правил для Prometheus — ✅ ВЫПОЛНЕНО
- [x] Grafana README — обновлённая документация — ✅ ВЫПОЛНЕНО
- [x] REST API тесты — 18 тестов для REST API endpoints — ✅ ВЫПОЛНЕНО
- [x] gRPC интерфейс — управление через gRPC — ✅ ВЫПОЛНЕНО (grpc-server.c/h, 10 тестов, документация)
- [x] Grafana дашборд — расширение панелей — ✅ ВЫПОЛНЕНО (16 панелей, enhanced dashboard)

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 497 | +2 |
| **C/H файлов** | 415+ | +1 |
| **Workflow** | 8 | 0 |
| **Тестов** | 140 (120 C + 8 Python + 4 Dart) | 0 |
| **Бенчмарков** | 9 | 0 |
| **Документов** | 32 | +2 (RELEASE_NOTES_v1.0.29.md, webui/README.md) |
| **Релизов** | 2 (v1.0.27, v1.0.28) | 0 |

---

## 📚 Документация

### Основная (32 файлов)
- README.md, PROJECT_INDEX.md, TESTING.md, BENCHMARKS.md
- CHANGELOG.md, SECURITY.md, CONTRIBUTING.md, DEPLOYMENT.md
- API_REFERENCE.md, ROADMAP.md, RELEASE_NOTES_v1.0.28.md, RELEASE_NOTES_v1.0.29.md
- docs/*.md (7 файлов: ADMIN_CLI_GUIDE.md, DEBUGGING.md, PERFORMANCE_TUNING.md, TROUBLESHOOTING.md, FREEBSD_SUPPORT.md, ARM64_SUPPORT.md, GRPC_API.md)
- grafana/README.md, prometheus/mtproxy-alerts.yml
- webui/README.md

---

## 🎯 Roadmap

| Задача | Версия | Приоритет | Статус |
|--------|--------|-----------|--------|
| Интеграционные тесты | v1.0.29 | 🔴 Высокий | ✅ Выполнено |
| Бенчмарки 100K+ | v1.0.29 | 🟡 Средний | ✅ Выполнено |
| admin-cli тесты | Q2 2026 | 🔴 Высокий | ✅ Выполнено |
| Кэш performance | Q2 2026 | 🔴 Высокий | ✅ Выполнено |
| FreeBSD поддержка | Q2 2026 | 🔵 Планирование | ✅ Выполнено |
| ARM64 сборка | Q2 2026 | 🔵 Планирование | ✅ Выполнено |
| Prometheus/Grafana | Q3 2026 | 🟢 Средний | ✅ Выполнено |
| REST API тесты | Q3 2026 | 🟢 Средний | ✅ Выполнено |
| gRPC интерфейс | Q3 2026 | 🟢 Низкий | ✅ Выполнено |
| Grafana дашборд | Q3 2026 | 🟢 Низкий | ✅ Выполнено |
| Web UI | Q4 2026 | 🔴 Высокий | ✅ Выполнено |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. — REST API тесты (497 коммитов)*
