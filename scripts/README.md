# MTProxy Scripts

## 📚 Обзор

Эта директория содержит утилиты и скрипты для администрирования и мониторинга MTProxy.

## 🔧 Скрипты

### 1. monitor.sh - Скрипт мониторинга

Bash-скрипт для мониторинга состояния MTProxy сервера.

**Использование:**

```bash
# Показать статус
./monitor.sh status

# Выполнить проверку
./monitor.sh check

# Непрерывный мониторинг
./monitor.sh monitor

# Показать соединения
./monitor.sh connections

# Помощь
./monitor.sh help
```

**Переменные окружения:**

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `MTProxy_HOST` | Хост сервера | 127.0.0.1 |
| `MTProxy_PORT` | Порт статистики | 8888 |
| `CHECK_INTERVAL` | Интервал проверки (сек) | 60 |
| `ALERT_EMAIL` | Email для алертов | - |
| `LOG_FILE` | Путь к лог-файлу | /var/log/mtproxy-monitor.log |

**Пример с переменными:**

```bash
MTProxy_HOST=192.168.1.100 \
MTProxy_PORT=9999 \
CHECK_INTERVAL=30 \
./monitor.sh monitor
```

**Оповещения:**

Скрипт поддерживает отправку алертов через:
- Email (требуется `mail` команда)
- Telegram (требуется доработка)
- Slack (требуется доработка)

---

### 2. metrics_collector.py - Сборщик метрик

Python-скрипт для сбора и экспорта метрик MTProxy.

**Требования:**
- Python 3.6+
- Стандартная библиотека (нет внешних зависимостей)

**Использование:**

```bash
# Показать статус
python3 metrics_collector.py status

# Показать метрики (текст)
python3 metrics_collector.py metrics

# Показать метрики (JSON)
python3 metrics_collector.py metrics --format json

# Показать метрики (Prometheus)
python3 metrics_collector.py metrics --format prometheus

# Проверка здоровья
python3 metrics_collector.py health

# Непрерывное наблюдение
python3 metrics_collector.py watch --interval 5

# Экспорт метрик
python3 metrics_collector.py export --format json --output metrics.json
```

**Параметры:**

| Параметр | Описание | По умолчанию |
|----------|----------|--------------|
| `--host` | Хост сервера | 127.0.0.1 |
| `--port` | Порт статистики | 8888 |
| `--timeout` | Таймаут запроса (сек) | 5 |

**Примеры:**

```bash
# Подключение к удаленному серверу
python3 metrics_collector.py --host 192.168.1.100 --port 9999 status

# Экспорт в Prometheus формат для Grafana
python3 metrics_collector.py export --format prometheus > /var/lib/prometheus/mtproxy.metrics

# Мониторинг с интервалом 10 секунд
python3 metrics_collector.py watch --interval 10 --format json
```

---

### 3. update-secrets.sh - Обновление секретов

Скрипт для обновления конфигурации и секретов от Telegram.

**Использование:**

```bash
# Обновить секреты
./update-secrets.sh

# Обновить только конфигурацию
./update-secrets.sh --config-only

# Обновить только секреты
./update-secrets.sh --secrets-only
```

**Автоматизация (cron):**

```bash
# Обновлять каждый день в 3:00
0 3 * * * /path/to/MTProxy/scripts/update-secrets.sh
```

---

### 4. update-secrets.bat - Обновление секретов (Windows)

Аналог update-secrets.sh для Windows.

**Использование:**

```batch
update-secrets.bat
```

---

### 5. build_windows.bat - Сборка на Windows

Скрипт для сборки MTProxy на Windows.

**Требования:**
- MinGW или Visual Studio
- OpenSSL для Windows
- zlib для Windows

**Использование:**

```batch
build_windows.bat
```

---

## 🔗 Интеграция

### Systemd сервис для мониторинга

Создайте файл `/etc/systemd/system/mtproxy-monitor.service`:

```ini
[Unit]
Description=MTProxy Monitor
After=network.target mtproxy.service

[Service]
Type=simple
ExecStart=/path/to/MTProxy/scripts/monitor.sh monitor
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Активация:

```bash
sudo systemctl daemon-reload
sudo systemctl enable mtproxy-monitor
sudo systemctl start mtproxy-monitor
```

---

### Grafana Dashboard

Импортируйте дашборд из `scripts/grafana-dashboard.json` (если доступен).

**Настройка:**

1. Добавьте Prometheus data source
2. Импортируйте дашборд
3. Настройте алерты

---

### Prometheus Configuration

Добавьте в `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:8888']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

---

## 📊 Метрики

Скрипт `metrics_collector.py` собирает следующие метрики:

### Общие метрики
- `mtproxy_up` - Статус сервера (1=OK, 0=DOWN)
- `mtproxy_uptime_seconds` - Время работы сервера
- `mtproxy_connections_active` - Активные соединения
- `mtproxy_connections_total` - Всего соединений

### Метрики производительности
- `mtproxy_cpu_usage_percent` - Использование CPU
- `mtproxy_memory_usage_bytes` - Использование памяти
- `mtproxy_requests_per_second` - Запросов в секунду

### Метрики кэша
- `mtproxy_cache_entries` - Количество записей в кэше
- `mtproxy_cache_hits_total` - Всего попаданий в кэш
- `mtproxy_cache_misses_total` - Всего промахов кэша
- `mtproxy_cache_hit_rate` - Процент попаданий

### Метрики rate limiting
- `mtproxy_ratelimit_requests_total` - Всего запросов
- `mtproxy_ratelimit_rejections_total` - Отклонено по лимиту
- `mtproxy_ratelimit_active_limits` - Активные лимиты

---

## 🛠️ Troubleshooting

### monitor.sh не работает

1. Проверьте права на выполнение:
   ```bash
   chmod +x monitor.sh
   ```

2. Проверьте доступность команд:
   ```bash
   which nc ss curl
   ```

3. Проверьте логи:
   ```bash
   tail -f /var/log/mtproxy-monitor.log
   ```

### metrics_collector.py выдает ошибку

1. Проверьте версию Python:
   ```bash
   python3 --version
   ```

2. Проверьте доступность порта:
   ```bash
   curl http://127.0.0.1:8888/stats
   ```

3. Запустите с отладкой:
   ```bash
   python3 -v metrics_collector.py status
   ```

---

## 📝 Лицензия

Скрипты распространяются под той же лицензией, что и MTProxy.

---

## 🤝 Contributing

Вносите изменения через pull requests в ветку `dev`.

---

*Последнее обновление: Март 2026*
