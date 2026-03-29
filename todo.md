# MTProxy Project TODO

> **Актуально на:** 29 марта 2026 г.
> **Коммит:** 734dd54 (HEAD -> dev)
> **Версия:** v1.0.30 ✅ RELEASED | v1.0.31-dev (Q1 2027 в работе)
> **Ветки:** dev (ahead origin/dev by 8) | master (synced)
> **Тег:** v1.0.30 ✅

---

## 🎉 RELEASED: v1.0.30 (29 марта 2026) — Q4 2026 Complete

### ✅ Выполнено (v1.0.30 релиз)
- [x] **CLI утилита** — mtproxy-cli (12 команд, 600+ строк, 10 тестов)
- [x] **Alert Manager** — уведомления (5 каналов, 800+ строк, 25 тестов)
- [x] **Health Check** — проверки (6 типов, 900+ строк, 20 тестов)
- [x] **Cluster Manager** — кластеризация (1100+ строк, 30 тестов)
- [x] **Load Balancer** — балансировка (700+ строк, 25 тестов)
- [x] **VERSION** — обновлён до 1.0.30
- [x] **CHANGELOG.md** — добавлены изменения v1.0.30
- [x] **RELEASE_NOTES_v1.0.30.md** — релизный документ

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

## 📋 Активные задачи (v1.0.31-dev — Q1 2027)

### 🔴 Высокий приоритет
- [x] **Auto-scaling** — ✅ ВЫПОЛНЕНО (auto-scaler.c/h, 20 тестов, 3 политики)
- [ ] **Distributed tracing** — трассировка запросов между узлами
- [ ] **Production тестирование** — тесты на реальном кластере

### 🟡 Средний приоритет
- [ ] **Anomaly detection** — ML-детекция аномалий в трафике
- [ ] **Predictive analytics** — прогнозирование нагрузки
- [ ] **Plugin system** — система плагинов для расширения

### 🟢 Низкий приоритет
- [ ] **Web UI v2** — расширенный веб-интерфейс
- [ ] **Mobile app v2** — расширенная функциональность
- [ ] **Documentation** — дополнительные руководства

---

## 📊 Статистика

| Метрика | Значение | Изменение |
|---------|----------|-----------|
| **Всего коммитов** | 506 | +1 (Auto-scaler) |
| **C/H файлов** | 428+ | +2 (auto-scaler.c/h) |
| **Workflow** | 8 | 0 |
| **Тестов** | 270 (250 C + 8 Python + 4 Dart) | +20 (Auto-scaler) |
| **Бенчмарков** | 9 | 0 |
| **Документов** | 36 | 0 |
| **Релизов** | 4 (v1.0.27, v1.0.28, v1.0.29, v1.0.30) | 0 |

---

## 📚 Документация

### Основная (36 файлов)
- README.md, PROJECT_INDEX.md, TESTING.md, BENCHMARKS.md
- CHANGELOG.md, SECURITY.md, CONTRIBUTING.md, DEPLOYMENT.md
- API_REFERENCE.md, ROADMAP.md, RELEASE_NOTES_v1.0.28.md, RELEASE_NOTES_v1.0.29.md, RELEASE_NOTES_v1.0.30.md
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
| CLI утилита | Q4 2026 | 🔴 Высокий | ✅ Выполнено |
| Alert manager | Q4 2026 | 🔴 Высокий | ✅ Выполнено |
| Health checks | Q4 2026 | 🔴 Высокий | ✅ Выполнено |
| Кластеризация | Q4 2026 | 🟡 Средний | ✅ Выполнено |
| Load balancing | Q4 2026 | 🟡 Средний | ✅ Выполнено |

---

## 📝 Правила

- **Качество важнее количества** — фокус на стабильность и тесты
- **Workflow:** dev → проверка (тесты, сборка) → main
- **Без документации без запроса** — только код и исправления
- **Синхронизация:** все изменения в dev, затем merge в main

---

*Последнее обновление: 29 марта 2026 г. — Auto-scaling ✅ (506 коммитов)*
