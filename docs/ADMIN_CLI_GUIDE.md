# Admin CLI User Guide

Руководство пользователя Admin CLI для MTProxy.

## Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Режимы работы](#режимы-работы)
3. [Команды](#команды)
4. [Примеры использования](#примеры-использования)
5. [Настройка](#настройка)
6. [Скрипты и автоматизация](#скрипты-и-автоматизация)

---

## Быстрый старт

### Запуск CLI

**Интерактивный режим:**
```bash
# Windows
mtproxy-admin.exe

# Linux/macOS
./mtproxy-admin
```

**Одиночная команда:**
```bash
mtproxy-admin.exe status
mtproxy-admin.exe "stats detail"
mtproxy-admin.exe "cache-stats"
```

**С подключением к удалённому серверу:**
```bash
mtproxy-admin.exe --host 192.168.1.100 --port 8888 --token mysecret
```

---

## Режимы работы

### Интерактивный режим

```bash
mtproxy-admin.exe
```

**Возможности:**
- Многострочный ввод команд
- История команд (сохраняется между сессиями)
- Автодополнение по Tab
- Цветной вывод

**Пример сессии:**
```
MTProxy Admin CLI
Type 'help' for available commands.

mtproxy> help
MTProxy Admin CLI - Available Commands:

  status          Show server status
  stats           Show server statistics
  reload          Reload configuration
  config          Show configuration
  cache-stats     Show cache statistics
  ...

mtproxy> status
Server Status: running
Uptime: 3600 seconds
Workers: 4
Connections: 150
...

mtproxy> exit
Exiting...
```

### Режим одной команды

```bash
mtproxy-admin.exe <command>
```

**Примеры:**
```bash
mtproxy-admin.exe status
mtproxy-admin.exe "cache-stats"
mtproxy-admin.exe "config show network"
```

### JSON режим

```bash
mtproxy-admin.exe --json stats
```

**Вывод:**
```json
{
  "server": {
    "status": "running",
    "uptime": 3600,
    "workers": 4,
    "connections": 150
  },
  "traffic": {
    "bytes_sent": 1048576,
    "bytes_received": 2097152
  }
}
```

---

## Команды

### Управление сервером

| Команда | Описание | Пример |
|---------|----------|--------|
| `status` | Статус сервера | `status` |
| `reload` | Перезагрузка конфигурации | `reload [config\|all]` |
| `restart` | Перезапуск сервера | `restart` |
| `stop` | Остановка сервера | `stop` |
| `health` | Проверка здоровья | `health` |

### Статистика и метрики

| Команда | Описание | Пример |
|---------|----------|--------|
| `stats` | Статистика сервера | `stats [detail]` |
| `metrics` | Prometheus метрики | `metrics [json\|text]` |
| `connections` | Список подключений | `connections [active\|all]` |

### Конфигурация

| Команда | Описание | Пример |
|---------|----------|--------|
| `config show` | Показать конфигурацию | `config show [section]` |
| `config-set` | Установить параметр | `config-set network.port 8889` |
| `config-save` | Сохранить конфигурацию | `config-save [filename]` |

### Кэш

| Команда | Описание | Пример |
|---------|----------|--------|
| `cache-stats` | Статистика кэша | `cache-stats` |
| `cache-get` | Получить значение | `cache-get mykey` |
| `cache-set` | Установить значение | `cache-set mykey myvalue 3600` |
| `cache-delete` | Удалить значение | `cache-delete mykey` |
| `cache-clear` | Очистить кэш | `cache-clear [section]` |

### Rate Limiting

| Команда | Описание | Пример |
|---------|----------|--------|
| `ratelimit` | Статус rate limits | `ratelimit [key]` |
| `ratelimit-add` | Добавить limit | `ratelimit-add api 100 60` |
| `ratelimit-remove` | Удалить limit | `ratelimit-remove api` |
| `whitelist` | Управление whitelist | `whitelist add 192.168.1.1` |
| `blacklist` | Управление blacklist | `blacklist add 10.0.0.1` |

### Логирование

| Команда | Описание | Пример |
|---------|----------|--------|
| `log-level` | Уровень логирования | `log-level [debug\|info\|warn\|error]` |
| `log-flush` | Сброс буферов | `log-flush` |

### Ошибки

| Команда | Описание | Пример |
|---------|----------|--------|
| `errors` | Показать ошибки | `errors [count]` |
| `errors-clear` | Очистить историю | `errors-clear` |

### Справка

| Команда | Описание | Пример |
|---------|----------|--------|
| `help` | Общая справка | `help` |
| `help <cmd>` | Справка по команде | `help cache-stats` |
| `exit` / `quit` | Выход | `exit` |

---

## Примеры использования

### Мониторинг сервера

```bash
# Статус сервера
mtproxy-admin.exe status

# Подробная статистика
mtproxy-admin.exe "stats detail"

# Метрики для Prometheus
mtproxy-admin.exe --json metrics
```

### Управление конфигурацией

```bash
# Показать текущую конфигурацию
mtproxy-admin.exe "config show"

# Показать секцию network
mtproxy-admin.exe "config show network"

# Изменить порт
mtproxy-admin.exe "config-set network.port 8889"

# Сохранить конфигурацию
mtproxy-admin.exe "config-save backup.conf"
```

### Управление кэшем

```bash
# Статистика кэша
mtproxy-admin.exe cache-stats

# Получить значение
mtproxy-admin.exe "cache-get session:12345"

# Установить значение с TTL 1 час
mtproxy-admin.exe "cache-set session:12345 userdata 3600"

# Очистить сессионный кэш
mtproxy-admin.exe "cache-clear sessions"
```

### Rate Limiting

```bash
# Показать все limits
mtproxy-admin.exe ratelimit

# Добавить limit: 100 запросов в минуту
mtproxy-admin.exe "ratelimit-add api 100 60"

# Добавить IP в whitelist
mtproxy-admin.exe "whitelist add 192.168.1.100"

# Добавить IP в blacklist
mtproxy-admin.exe "blacklist add 10.0.0.1"
```

### Подключения

```bash
# Показать активные подключения
mtproxy-admin.exe "connections active"

# Показать все подключения
mtproxy-admin.exe connections

# Убить подключение
mtproxy-admin.exe "kill conn_12345"
```

---

## Настройка

### Переменные окружения

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `MTPROXY_HOST` | Хост сервера | `localhost` |
| `MTPROXY_PORT` | Порт сервера | `8888` |
| `MTPROXY_TOKEN` | Токен аутентификации | - |
| `MTPROXY_JSON` | JSON вывод | `0` |
| `MTPROXY_COLOR` | Цветной вывод | `1` |
| `MTPROXY_HISTORY` | Файл истории | `~/.mtproxy_history` |

### Конфигурационный файл

**Расположение:** `~/.mtproxyrc` или `%USERPROFILE%\.mtproxyrc`

**Формат:**
```ini
[connection]
host = localhost
port = 8888
token = mysecret

[output]
format = text
color = true

[history]
file = ~/.mtproxy_history
max_size = 1000
```

---

## Скрипты и автоматизация

### Bash скрипт мониторинга

```bash
#!/bin/bash
# monitor.sh

HOST="${MTPROXY_HOST:-localhost}"
PORT="${MTPROXY_PORT:-8888}"

echo "=== MTProxy Monitor ==="
echo "Host: $HOST:$PORT"
echo "Time: $(date)"
echo ""

# Статус
echo "=== Status ==="
mtproxy-admin.exe --host $HOST --port $PORT status

# Статистика
echo ""
echo "=== Statistics ==="
mtproxy-admin.exe --host $HOST --port $PORT "stats detail"

# Подключения
echo ""
echo "=== Connections ==="
mtproxy-admin.exe --host $HOST --port $PORT "connections active" | head -20
```

### PowerShell скрипт

```powershell
# monitor.ps1

$host = $env:MTPROXY_HOST ?? "localhost"
$port = $env:MTPROXY_PORT ?? "8888"

Write-Host "=== MTProxy Monitor ==="
Write-Host "Host: $host`:$port"
Write-Host "Time: $(Get-Date)"
Write-Host ""

Write-Host "=== Status ==="
& mtproxy-admin.exe --host $host --port $port status

Write-Host ""
Write-Host "=== Statistics ==="
& mtproxy-admin.exe --host $host --port $port "stats detail"
```

### Python скрипт

```python
#!/usr/bin/env python3
# mtproxy_cli.py

import subprocess
import json
import sys

def run_command(cmd, host='localhost', port=8888, token=None):
    args = ['mtproxy-admin.exe']
    
    if host != 'localhost':
        args.extend(['--host', host])
    if port != 8888:
        args.extend(['--port', str(port)])
    if token:
        args.extend(['--token', token])
    
    args.extend(['--json', cmd])
    
    result = subprocess.run(args, capture_output=True, text=True)
    
    if result.returncode == 0:
        return json.loads(result.stdout)
    else:
        print(f"Error: {result.stderr}", file=sys.stderr)
        return None

# Пример использования
if __name__ == '__main__':
    status = run_command('status')
    if status:
        print(f"Server Status: {status['server']['status']}")
        print(f"Uptime: {status['server']['uptime']}s")
        print(f"Connections: {status['server']['connections']}")
```

### Cron задача (Linux)

```bash
# Мониторинг каждые 5 минут
*/5 * * * * /path/to/monitor.sh >> /var/log/mtproxy_monitor.log 2>&1

# Сбор метрик для Prometheus
*/1 * * * * mtproxy-admin.exe --json metrics > /var/lib/prometheus/mtproxy.metrics
```

### Task Scheduler (Windows)

```powershell
# Создание задачи
$action = New-ScheduledTaskAction -Execute "mtproxy-admin.exe" `
  -Argument "status"
$trigger = New-ScheduledTaskTrigger -Once -At (Get-Date) `
  -RepetitionInterval (New-TimeSpan -Minutes 5)
Register-ScheduledTask -TaskName "MTProxy Monitor" `
  -Action $action -Trigger $trigger
```

---

## Интеграция с системами мониторинга

### Grafana Dashboard

**Импорт метрик:**
```bash
# Экспорт метрик в Prometheus формат
mtproxy-admin.exe metrics > /var/lib/prometheus/mtproxy.metrics
```

**Пример дашборда:**
- Server Status (UP/DOWN)
- Active Connections
- Requests per Second
- Cache Hit Rate
- Error Rate

### Zabbix Integration

**Item:**
```
Type: External check
Key: system.run["mtproxy-admin.exe status"]
Type of information: Text
Update interval: 60s
```

**Trigger:**
```
Expression: {MTProxy:system.run["mtproxy-admin.exe status"].str("running")}=0
Severity: High
```

---

## Советы и рекомендации

### Производительность

1. **Используйте JSON для скриптов:**
   ```bash
   mtproxy-admin.exe --json stats | jq '.server.connections'
   ```

2. **Кэшируйте часто используемые команды:**
   ```bash
   # Alias в ~/.bashrc
   alias mtps='mtproxy-admin.exe status'
   alias mtpc='mtproxy-admin.exe "connections active"'
   ```

3. **Используйте detail режим только когда нужно:**
   ```bash
   # Быстро
   mtproxy-admin.exe stats
   
   # Подробно (медленнее)
   mtproxy-admin.exe "stats detail"
   ```

### Безопасность

1. **Не храните токены в скриптах:**
   ```bash
   # Плохо
   mtproxy-admin.exe --token mysecret status
   
   # Хорошо
   mtproxy-admin.exe --token $MTPROXY_TOKEN status
   ```

2. **Используйте .mtproxyrc с правильными правами:**
   ```bash
   chmod 600 ~/.mtproxyrc
   ```

3. **Ограничьте доступ к CLI:**
   ```bash
   # Только для администраторов
   chown root:admin mtproxy-admin.exe
   chmod 750 mtproxy-admin.exe
   ```

---

## Устранение проблем

### Ошибка: Connection refused

**Причина:** Сервер не запущен или неправильный порт

**Решение:**
```bash
# Проверка сервера
netstat -tlnp | grep mtproxy

# Проверка порта по умолчанию
mtproxy-admin.exe --port 8888 status
```

### Ошибка: Authentication failed

**Причина:** Неправильный токен

**Решение:**
```bash
# Проверка токена
mtproxy-admin.exe --token your_token status

# Проверка файла токенов
cat proxy-secret
```

### Ошибка: Command not found

**Причина:** Опечатка в команде

**Решение:**
```bash
# Показать все команды
mtproxy-admin.exe help

# Автодополнение (Tab)
mtproxy-admin.exe <Tab>
```

---

## Дополнительные ресурсы

- [API_REFERENCE.md](../API_REFERENCE.md) — REST API документация
- [TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md) — Диагностика проблем
- [PERFORMANCE_TUNING.md](../docs/PERFORMANCE_TUNING.md) — Оптимизация производительности
