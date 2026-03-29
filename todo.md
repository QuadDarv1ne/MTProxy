# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** c06e8e5 (HEAD -> dev)
> **Версия:** v1.0.29 ✅ RELEASED | v1.0.30-dev (Q4 2026 в работе)
> **Ветки:** dev = origin/dev ✅ | master = origin/master ✅ (синхронизированы)
> **Тег:** v1.0.29 ✅

---

## 🎉 RELEASED: v1.0.29 (29 марта 2026)

### ✅ Выполнено (v1.0.29 релиз)
- [x] **gRPC API** — grpc-server.c/h (400+ строк), 10 тестов, GRPC_API.md
- [x] **Web UI** — index.html, css/style.css, js/*.js (dashboard, конфигурация, секреты, логи)
- [x] **Grafana Enhanced Dashboard** — 16 панелей (cache, rate limiting, gRPC metrics)
- [x] **CMakeLists.txt** — ENABLE_GRPC опция, генерация кода из protobuf
- [x] **CHANGELOG.md, ROADMAP.md, todo.md** — обновлены для v1.0.29
- [x] **RELEASE_NOTES_v1.0.29.md** — релизный документ
- [x] **Тесты** — +10 C тестов (test_grpc_server.c)
- [x] **Документация** — +2 файла (GRPC_API.md, webui/README.md)

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

## 📋 Активные задачи (v1.0.30-dev — Q4 2026)

### 🔴 Высокий приоритет
- [x] **CLI утилита (mtproxy-cli)** — ✅ ВЫПОЛНЕНО (mtcli.c/h, 10 тестов, CLI_GUIDE.md)
- [x] **Alert manager** — ✅ ВЫПОЛНЕНО (alert-manager.c/h, 25 тестов, Telegram/Email/Slack)
- [x] **Health checks** — ✅ ВЫПОЛНЕНО (health-check.c/h, 20 тестов, 6 типов проверок)
- [x] **Кластеризация** — ✅ ВЫПОЛНЕНО (cluster-manager.c/h, 30 тестов, leader election, failover)
- [x] **Load balancing** — ✅ ВЫПОЛНЕНО (load-balancer.c/h, 25 тестов, 6 алгоритмов)

### 🟡 Средний приоритет
- [ ] **Auto-scaling** — автоматическое масштабирование под нагрузкой

### 🟢 Низкий приоритет
- [ ] **Distributed tracing** — трассировка запросов между узлами
- [ ] **Anomaly detection** — ML-детекция аномалий в трафике
- [ ] **Predictive analytics** — прогнозирование нагрузки
- [ ] **Plugin system** — система плагинов для расширения

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 504 | +1 |
| **C/H файлов** | 426+ | +2 (load-balancer.c/h) |
| **Workflow** | 8 | 0 |
| **Тестов** | 250 (230 C + 8 Python + 4 Dart) | +25 C |
| **Бенчмарков** | 9 | 0 |
| **Документов** | 35 | 0 |
| **Релизов** | 3 (v1.0.27, v1.0.28, v1.0.29) | +1 |

---

## 📚 Документация

### Основная (35 файлов)
- README.md, PROJECT_INDEX.md, TESTING.md, BENCHMARKS.md
- CHANGELOG.md, SECURITY.md, CONTRIBUTING.md, DEPLOYMENT.md
- API_REFERENCE.md, ROADMAP.md, RELEASE_NOTES_v1.0.28.md, RELEASE_NOTES_v1.0.29.md
- docs/*.md (8 файлов: ADMIN_CLI_GUIDE.md, DEBUGGING.md, PERFORMANCE_TUNING.md, TROUBLESHOOTING.md, FREEBSD_SUPPORT.md, ARM64_SUPPORT.md, GRPC_API.md, CLI_GUIDE.md)
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
| Web UI | Q3 2026 | 🔴 Высокий | ✅ Выполнено |
| **CLI утилита** | **Q4 2026** | **🔴 Высокий** | **⏳ В работе** |
| **Alert manager** | **Q4 2026** | **🔴 Высокий** | **⏳ Планирование** |
| **Health checks** | **Q4 2026** | **🔴 Высокий** | **⏳ Планирование** |
| **Кластеризация** | **Q4 2026** | **🟡 Средний** | **⏳ Планирование** |
| **Load balancing** | **Q4 2026** | **🟡 Средний** | **⏳ Планирование** |

---

## 📝 Правила

- **Качество важнее количества**
- **Workflow:** dev → проверка → main
- **Без документации без запроса**

---

*Последнее обновление: 29 марта 2026 г. — Load Balancer v1.0.0 (504 коммита)*
