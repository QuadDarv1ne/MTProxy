# Использование MTProxy на Windows

## Быстрый старт

### 1. Получение секретного ключа

Сгенерируйте секретный ключ (32 байта в hex):

```powershell
# PowerShell
$bytes = New-Object byte[] 32
[Security.Cryptography.RNGCryptoServiceProvider]::Create().GetBytes($bytes)
$secret = [BitConverter]::ToString($bytes).Replace("-","").ToLower()
echo $secret
```

Или используйте онлайн генератор: https://www.random.org/bytes/

### 2. Загрузка конфигурации Telegram

```bash
# В MSYS2 UCRT64 терминале
curl -s https://core.telegram.org/getProxySecret -o proxy-secret
curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
```

### 3. Запуск прокси

```bash
# Базовый запуск (single-worker режим для Windows)
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <ваш_секрет> proxy-secret proxy-multi.conf

# С логированием
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <ваш_секрет> -l mtproxy.log proxy-secret proxy-multi.conf

# С verbose режимом
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <ваш_секрет> -v proxy-secret proxy-multi.conf
```

### 4. Получение ссылки для Telegram

После запуска прокси создайте ссылку:

```
tg://proxy?server=<ваш_IP>&port=8888&secret=<ваш_секрет>
```

Или в формате `t.me`:

```
https://t.me/proxy?server=<ваш_IP>&port=8888&secret=<ваш_секрет>
```

## Параметры запуска

| Параметр | Описание | Пример |
|----------|----------|--------|
| `-M 1` | Single-worker режим (обязательно для Windows) | `-M 1` |
| `-p <port>` | Порт для прокси | `-p 8888` |
| `-S <secret>` | Секретный ключ (hex) | `-S ee1234...` |
| `-l <file>` | Файл логов | `-l mtproxy.log` |
| `-v` | Verbose режим (можно повторять: -vv, -vvv) | `-v` |
| `-H <port>` | HTTP порт для статистики | `-H 8080` |
| `-c <num>` | Максимум соединений | `-c 10000` |
| `-b <num>` | Backlog для listen() | `-b 8192` |

## Настройка Telegram клиента

### Telegram Desktop

1. Откройте ссылку `tg://proxy?server=...` в браузере
2. Telegram Desktop автоматически предложит добавить прокси
3. Нажмите "Connect"

### Telegram Mobile (Android/iOS)

1. Откройте Settings → Data and Storage → Proxy Settings
2. Нажмите "Add Proxy"
3. Выберите "MTProto"
4. Введите:
   - Server: ваш IP
   - Port: 8888
   - Secret: ваш секретный ключ

Или просто откройте ссылку `https://t.me/proxy?...` на телефоне.

## Проверка работы

### Проверка порта

```bash
# В MSYS2
netstat -an | grep 8888

# В PowerShell
netstat -an | Select-String "8888"
```

### Проверка логов

```bash
tail -f mtproxy.log
```

### HTTP статистика

Если запустили с `-H 8080`:

```bash
curl http://localhost:8080/stats
```

## Примеры использования

### Базовый прокси для личного использования

```bash
./build/bin/mtproto-proxy.exe -M 1 -p 443 -S ee1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcd proxy-secret proxy-multi.conf
```

### Прокси с HTTP статистикой

```bash
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -H 8080 -S <secret> -l mtproxy.log proxy-secret proxy-multi.conf
```

Статистика доступна на: http://localhost:8080/stats

### Прокси с максимальной производительностью

```bash
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -c 50000 -b 16384 -S <secret> proxy-secret proxy-multi.conf
```

## Автозапуск на Windows

### Через Task Scheduler

1. Откройте Task Scheduler (taskschd.msc)
2. Create Basic Task
3. Trigger: At startup
4. Action: Start a program
5. Program: `C:\msys64\ucrt64\bin\bash.exe`
6. Arguments: `-c "cd /c/Users/.../MTProxy && ./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <secret> proxy-secret proxy-multi.conf"`

### Через NSSM (Non-Sucking Service Manager)

```powershell
# Скачать NSSM: https://nssm.cc/download
nssm install MTProxy "C:\msys64\ucrt64\bin\bash.exe"
nssm set MTProxy AppParameters "-c \"cd /c/Users/.../MTProxy && ./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <secret> proxy-secret proxy-multi.conf\""
nssm start MTProxy
```

## Безопасность

### Firewall правила

```powershell
# Разрешить входящие соединения на порт 8888
New-NetFirewallRule -DisplayName "MTProxy" -Direction Inbound -Protocol TCP -LocalPort 8888 -Action Allow
```

### Рекомендации

- Используйте случайный порт (не 443, 8888)
- Меняйте секретный ключ регулярно
- Не публикуйте ссылку на прокси публично
- Используйте для личного использования или небольшой группы

## Ограничения Windows версии

- ⚠️ **Single-worker режим** (`-M 1`) - обязательно
- ⚠️ **fork() не поддерживается** - нельзя использовать multi-process режим
- ⚠️ **Загрузка секретов требует доработки** - функция `aes_load_pwd_file()` использует Unix API
- ⚠️ **Рекомендуется WSL2 или Docker** для полной функциональности
- ✅ **Proxy компилируется и запускается** - базовая функциональность работает

## Troubleshooting

### Прокси не запускается

```bash
# Проверьте, что порт свободен
netstat -an | grep 8888

# Проверьте права на файлы
ls -la proxy-secret proxy-multi.conf

# Запустите с verbose режимом
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -S <secret> -vvv proxy-secret proxy-multi.conf
```

### Telegram не подключается

1. Проверьте firewall
2. Проверьте, что прокси запущен: `netstat -an | grep 8888`
3. Проверьте секретный ключ (должен быть 64 hex символа)
4. Попробуйте другой порт

### Низкая производительность

```bash
# Увеличьте лимиты соединений
./build/bin/mtproto-proxy.exe -M 1 -p 8888 -c 50000 -b 16384 -S <secret> proxy-secret proxy-multi.conf
```

## Дополнительные ресурсы

- [README.md](README.md) - Общая информация о проекте
- [USAGE_EXAMPLES.md](examples/USAGE_EXAMPLES.md) - Примеры использования API
- [BUILD_WINDOWS.md](BUILD_WINDOWS.md) - Инструкции по сборке
- [FIXES_SUMMARY.md](FIXES_SUMMARY.md) - Список исправлений

---

*Последнее обновление: 23 марта 2026*
