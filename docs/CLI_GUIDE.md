# MTProxy CLI Guide

Руководство по использованию утилиты командной строки `mtproxy-cli`.

## 📋 Обзор

`mtproxy-cli` — это кроссплатформенная утилита для управления и мониторинга MTProxy сервера через командную строку.

**Возможности:**
- 📊 Мониторинг статуса и статистики сервера
- ⚙️ Управление конфигурацией
- 🔑 Управление секретами
- 📝 Просмотр логов
- 🚨 Проверка здоровья (health checks)
- 🔄 Перезагрузка и перезапуск сервера

## 🚀 Быстрый старт

### Установка

Утилита поставляется вместе с MTProxy и находится в директории `bin/`:

```bash
# Linux/macOS
./bin/mtproxy-cli --help

# Windows
bin\mtproxy-cli.exe --help
```

### Подключение

```bash
# Локальное подключение (по умолчанию)
mtproxy-cli status

# Удалённое подключение
mtproxy-cli --host 192.168.1.100 --port 8080 status

# С API ключом
mtproxy-cli --api-key your-api-key status
```

## 📖 Команды

### Статус сервера

```bash
# Базовый статус
mtproxy-cli status

# В формате JSON
mtproxy-cli status --json
```

**Пример вывода:**
```
Server Status: Running
Version: 1.0.29
Uptime: 2d 5h 30m 15s
Platform: Linux x86_64
```

### Статистика

```bash
# Основная статистика
mtproxy-cli stats

# Подробная статистика в JSON
mtproxy-cli stats --json --verbose
```

**Пример вывода:**
```
Active Connections: 1234
Total Connections: 56789
Bytes Sent: 1.50 GB
Bytes Received: 3.20 GB
Requests/sec: 450
CPU Usage: 25.5%
Memory Usage: 128 MB
Cache Hit Rate: 85.2%
```

### Конфигурация

```bash
# Получить конфигурацию
mtproxy-cli config get

# Получить конкретный параметр
mtproxy-cli config get server_port

# Установить параметр
mtproxy-cli config set max_connections 2000

# Сохранить конфигурацию
mtproxy-cli config save
```

### Секреты

```bash
# Список секретов
mtproxy-cli secrets list

# Добавить секрет
mtproxy-cli secrets add --secret "dd1234567890abcdef" --desc "My proxy"

# Удалить секрет
mtproxy-cli secrets remove <hash>
```

### Логи

```bash
# Последние логи
mtproxy-cli logs

# Логи с уровнем error
mtproxy-cli logs --level error

# Последние 100 записей
mtproxy-cli logs --tail 100

# Режим реального времени (follow)
mtproxy-cli logs --follow
```

**Уровни логирования:**
- `debug` — отладочные сообщения
- `info` — информационные
- `warn` — предупреждения
- `error` — ошибки

### Подключения

```bash
# Активные подключения
mtproxy-cli connections active

# Все подключения
mtproxy-cli connections all

# Завершить подключение
mtproxy-cli kill <connection_id>
```

### Rate Limiting

```bash
# Статус rate limiting
mtproxy-cli ratelimit status

# Добавить лимит
mtproxy-cli ratelimit add 192.168.1.100 100 60

# Удалить лимит
mtproxy-cli ratelimit remove 192.168.1.100
```

### Health Check

```bash
# Проверка здоровья
mtproxy-cli health
```

**Пример вывода:**
```
Health Status: OK
Server: Running
API: Responding
Latency: 5ms
```

### Метрики

```bash
# Метрики в JSON
mtproxy-cli metrics json

# Метрики в формате Prometheus
mtproxy-cli metrics prometheus
```

### Управление сервером

```bash
# Перезагрузить конфигурацию
mtproxy-cli reload

# Перезапустить сервер
mtproxy-cli restart

# Остановить сервер
mtproxy-cli stop
```

## 🔧 Опции

### Подключение

| Опция | Кратко | Описание | По умолчанию |
|-------|--------|----------|--------------|
| `--host` | | Хост сервера | localhost |
| `--port` | | Порт REST API | 8080 |
| `--grpc-port` | | Порт gRPC | 50051 |
| `--api-key` | | API ключ для аутентификации | - |
| `--mode` | | Режим: rest, grpc, direct | rest |

### Формат вывода

| Опция | Кратко | Описание |
|-------|--------|----------|
| `--json` | | Вывод в формате JSON |
| `--verbose` | `-v` | Подробный вывод |

### Прочие

| Опция | Кратко | Описание | По умолчанию |
|-------|--------|----------|--------------|
| `--timeout` | | Таймаут подключения (мс) | 5000 |
| `--insecure` | | Не проверять SSL | false |
| `--help` | `-h` | Показать справку | - |
| `--version` | | Показать версию | - |

## 📝 Примеры использования

### Мониторинг сервера

```bash
# Быстрая проверка статуса
mtproxy-cli status

# Подробная статистика
mtproxy-cli stats --json | jq .

# Проверка здоровья
mtproxy-cli health
```

### Управление конфигурацией

```bash
# Просмотр текущей конфигурации
mtproxy-cli config get

# Изменение порта
mtproxy-cli config set server_port 9999

# Включение IPv6
mtproxy-cli config set ipv6_enabled true

# Сохранение изменений
mtproxy-cli config save
```

### Управление секретами

```bash
# Создание нового секрета
SECRET=$(head -c 16 /dev/urandom | xxd -ps)
mtproxy-cli secrets add --secret "dd${SECRET}" --desc "New proxy key"

# Просмотр всех секретов
mtproxy-cli secrets list --json | jq .

# Удаление старого секрета
mtproxy-cli secrets remove <old-secret-hash>
```

### Диагностика проблем

```bash
# Проверка логов на ошибки
mtproxy-cli logs --level error --tail 100

# Проверка активных подключений
mtproxy-cli connections active

# Проверка rate limiting
mtproxy-cli ratelimit status
```

### Автоматизация (скрипты)

**Bash скрипт мониторинга:**
```bash
#!/bin/bash

STATUS=$(mtproxy-cli status --json | jq -r '.state')

if [ "$STATUS" != "running" ]; then
    echo "Server is not running! Restarting..."
    mtproxy-cli restart
    echo "Alert sent to admin@example.com"
fi
```

**Python скрипт:**
```python
import subprocess
import json

# Получение статистики
result = subprocess.run(
    ['mtproxy-cli', 'stats', '--json'],
    capture_output=True, text=True
)
stats = json.loads(result.stdout)

print(f"Active connections: {stats['active_connections']}")
print(f"Cache hit rate: {stats['cache_hit_rate']:.2f}%")
```

## 🔒 Безопасность

### API ключи

Для доступа к API необходим ключ аутентификации:

```bash
# Генерация API ключа
openssl rand -hex 32

# Использование ключа
mtproxy-cli --api-key <your-key> status

# Сохранение ключа в переменной окружения
export MTPROXY_API_KEY=<your-key>
mtproxy-cli status  # Ключ берётся из окружения
```

### HTTPS

Для production рекомендуется использовать HTTPS:

```bash
# С самоподписанным сертификатом (тесты)
mtproxy-cli --insecure --host example.com status

# С валидным SSL
mtproxy-cli --host example.com --port 8443 status
```

## 🐛 Troubleshooting

### Ошибка подключения

**Проблема:**
```
Error: Failed to connect to localhost:8080
```

**Решение:**
1. Проверьте, запущен ли MTProxy с REST API
2. Убедитесь, что порт открыт
3. Проверьте firewall

```bash
# Проверка REST API
curl http://localhost:8080/api/v1/server/status
```

### Ошибка аутентификации

**Проблема:**
```
Error: Unauthorized - Invalid API key
```

**Решение:**
1. Проверьте API ключ
2. Убедитесь, что ключ передан в заголовке

```bash
mtproxy-cli --api-key <correct-key> status
```

### Таймаут

**Проблема:**
```
Error: Request timeout
```

**Решение:**
1. Увеличьте таймаут
2. Проверьте сеть

```bash
mtproxy-cli --timeout 10000 status
```

## 📊 Интеграция

### Prometheus

```bash
# Экспорт метрик
mtproxy-cli metrics prometheus

# В Prometheus конфигурации
scrape_configs:
  - job_name: 'mtproxy'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/api/v1/metrics?format=prometheus'
```

### Grafana

Используйте REST API для получения данных:

```json
{
  "datasource": {
    "type": "prometheus",
    "url": "http://localhost:9090"
  },
  "queries": [
    {
      "expr": "mtproxy_connections_active"
    }
  ]
}
```

### Telegram Bot

```bash
# Скрипт отправки уведомлений
#!/bin/bash
STATUS=$(mtproxy-cli status --json | jq -r '.state')
if [ "$STATUS" != "running" ]; then
    curl -X POST "https://api.telegram.org/bot<TOKEN>/sendMessage" \
        -d "chat_id=<CHAT_ID>&text=MTProxy server is down!"
fi
```

## 📚 Дополнительные ресурсы

- [REST API Documentation](API_REFERENCE.md)
- [gRPC API Documentation](docs/GRPC_API.md)
- [Deployment Guide](DEPLOYMENT.md)
- [Troubleshooting Guide](docs/TROUBLESHOOTING.md)

## 🙀 Поддержка

При возникновении проблем:
1. Проверьте логи: `mtproxy-cli logs --level error`
2. Проверьте статус: `mtproxy-cli health`
3. Откройте issue на GitHub

---

*Версия CLI: 1.0.0*
*Последнее обновление: 29 марта 2026 г.*
