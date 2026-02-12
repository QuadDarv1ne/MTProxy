# Подробное руководство по запуску MTProxy

## Содержание

1. [Требования к системе](#требования-к-системе)
2. [Установка зависимостей](#установка-зависимостей)
3. [Сборка проекта](#сборка-проекта)
4. [Получение необходимых файлов](#получение-необходимых-файлов)
5. [Генерация секретного ключа](#генерация-секретного-ключа)
6. [Запуск MTProxy](#запуск-mtproxy)
7. [Настройка как сервиса](#настройка-как-сервиса)
8. [Регистрация прокси в Telegram](#регистрация-прокси-в-telegram)
9. [Мониторинг и статистика](#мониторинг-и-статистика)
10. [Устранение неполадок](#устранение-неполадок)

## Требования к системе

- Операционная система: Linux (предпочтительно Ubuntu 18.04+, CentOS 7+, Debian 9+) или FreeBSD
- Архитектура: x86_64 (рекомендуется)
- Минимум 1 ГБ оперативной памяти
- Свободный порт для подключения клиентов (по умолчанию 443)
- Свободный порт для статистики (по умолчанию 8888)
- Доступ к интернету для получения конфигурации Telegram

## Установка зависимостей

### Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y git curl build-essential libssl-dev zlib1g-dev
```

### CentOS/RHEL/Fedora:

```bash
# Для CentOS/RHEL
sudo yum install -y git curl build-essential openssl-devel zlib-devel
sudo yum groupinstall -y "Development Tools"

# Для Fedora
sudo dnf install -y git curl make automake gcc gcc-c++ openssl-devel zlib-devel
```

## Сборка проекта

1. Клонируйте репозиторий:

```bash
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
```

2. Соберите проект:

```bash
make && cd objs/bin
```

Если сборка завершилась неудачно, очистите и попробуйте снова:

```bash
make clean && make
```

После успешной сборки вы получите исполняемый файл `mtproto-proxy` в директории `objs/bin`.

## Получение необходимых файлов

Для работы MTProxy требуются два файла от Telegram:

1. **Секретный ключ сервера**:

```bash
curl -s https://core.telegram.org/getProxySecret -o proxy-secret
```

2. **Конфигурация сервера**:

```bash
curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
```

> **Важно**: Эти файлы нужно обновлять регулярно (рекомендуется раз в день), так как конфигурация серверов Telegram может изменяться.

## Генерация секретного ключа

Создайте уникальный секретный ключ, который будут использовать ваши клиенты для подключения:

```bash
head -c 16 /dev/urandom | xxd -ps
```

Сохраните полученный 32-символьный HEX-ключ, он понадобится для запуска прокси.

## Запуск MTProxy

### Базовый запуск:

```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S YOUR_SECRET_KEY --aes-pwd proxy-secret proxy-multi.conf -M 1
```

### Объяснение параметров:

| Параметр | Описание |
|----------|----------|
| `-u nobody` | Имя пользователя, от которого запускать процесс (рекомендуется использовать отдельного пользователя) |
| `-p 8888` | Локальный порт для получения статистики (доступен только с localhost) |
| `-H 443` | Порт, который будут использовать клиенты для подключения к прокси |
| `-S YOUR_SECRET_KEY` | Секретный ключ, созданный ранее (можно указать несколько: `-S ключ1 -S ключ2`) |
| `--aes-pwd proxy-secret` | Путь к файлу с секретом сервера Telegram |
| `proxy-multi.conf` | Путь к файлу конфигурации сервера |
| `-M 1` | Количество рабочих процессов (увеличьте при высокой нагрузке) |

### Запуск с несколькими секретными ключами:

```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S SECRET1 -S SECRET2 --aes-pwd proxy-secret proxy-multi.conf -M 2
```

### Запуск с поддержкой случайного заполнения:

Для включения режима случайного заполнения (помогает обходить блокировки), добавьте префикс `dd` к секретному ключу:
```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S ddYOUR_SECRET_KEY --aes-pwd proxy-secret proxy-multi.conf -M 1
```

## Настройка как сервиса

### Настройка для systemd (Linux):

1. Создайте файл службы:

```bash
sudo nano /etc/systemd/system/MTProxy.service
```

2. Добавьте конфигурацию:

```ini
[Unit]
Description=MTProxy Service
After=network.target
Wants=network.target

[Service]
Type=simple
User=nobody
Group=nogroup
WorkingDirectory=/opt/MTProxy
ExecStart=/opt/MTProxy/mtproto-proxy -u nobody -p 8888 -H 443 -S YOUR_SECRET_KEY --aes-pwd proxy-secret proxy-multi.conf -M 1
Restart=always
RestartSec=5
StandardOutput=syslog
StandardError=syslog
SyslogIdentifier=MTProxy

[Install]
WantedBy=multi-user.target
```

3. Загрузите новую конфигурацию:

```bash
sudo systemctl daemon-reload
```

4. Включите автозапуск:

```bash
sudo systemctl enable MTProxy.service
```

5. Запустите сервис:

```bash
sudo systemctl start MTProxy.service
```

6. Проверьте статус:

```bash
sudo systemctl status MTProxy.service
```

## Регистрация прокси в Telegram

1. Установите MTProxy на вашем сервере и запустите его
2. Создайте ссылку для подключения к вашему прокси:
   ```
   tg://proxy?server=ВАШ_IP_АДРЕС&port=ПОРТ&secret=ВАШ_СЕКРЕТНЫЙ_КЛЮЧ
   ```
   
   Пример:
   ```
   tg://proxy?server=1.2.3.4&port=443&secret=cafebabe1234567890abcdef12345678
   ```

3. Откройте [@MTProxybot](https://t.me/MTProxybot) в Telegram
4. Следуйте инструкциям бота для регистрации вашего прокси
5. После регистрации вы получите тег, который можно использовать для статистики:
```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S YOUR_SECRET_KEY -P YOUR_TAG --aes-pwd proxy-secret proxy-multi.conf -M 1
```

## Мониторинг и статистика

### Получение статистики:

```bash
wget -qO- http://localhost:8888/stats
```

> **Важно**: Доступ к статистике возможен только с локального хоста (localhost)

### Основные метрики:

- `config_loaded_at` - Время последней загрузки конфигурации
- `queries_get` - Количество полученных запросов
- `workers` - Количество рабочих процессов
- `total_connections` - Общее количество соединений
- `http_connections` - Количество HTTP-соединений
- `window_clamp` - Настройка ограничения окна TCP
- `version` - Версия MTProxy

## Устранение неполадок

### Распространенные проблемы:

#### 1. Порт уже используется

**Ошибка**: `cannot open http/tcp server socket at port X: Address already in use`
**Решение**: Проверьте, занят ли порт другим процессом:
```bash
sudo netstat -tlnp | grep :PORT_NUMBER
```
Используйте другой порт или остановите конфликтующий процесс.

#### 2. Нет прав на выполнение

**Ошибка**: `Permission denied`
**Решение**: Проверьте права на исполняемый файл:
```bash
chmod +x mtproto-proxy
```

#### 3. Не удается получить конфигурацию

**Ошибка**: `cannot re-read config file`
**Решение**: Проверьте подключение к интернету и правильность расположения файлов `proxy-secret` и `proxy-multi.conf`.

#### 4. Клиенты не могут подключиться

**Проверьте**:

- Открыт ли порт в firewall:
```bash
# Для iptables
sudo iptables -L
# Для ufw
sudo ufw status
```

- Правильно ли указан IP-адрес в ссылке для подключения
- Правильность секретного ключа

#### 5. Высокое потребление ресурсов

- Уменьшите количество рабочих процессов (`-M`)
- Проверьте нагрузку на сервер
- Установите ограничения на количество соединений

### Команды диагностики:

```bash
# Проверка использования портов
sudo ss -tulnp | grep -E "(443|8888)"

# Мониторинг процесса
top -p $(pgrep -f mtproto-proxy)

# Проверка логов (если настроены)
journalctl -u MTProxy.service -f
```

## Безопасность

### Рекомендации:

- Используйте отдельного пользователя для запуска MTProxy (не root)
- Ограничьте доступ к порту статистики (обычно 8888) только с локального хоста
- Регулярно обновляйте конфигурацию от Telegram
- Храните секретные файлы в защищенном месте с ограниченным доступом
- Настройте firewall для ограничения доступа к вашему прокси
- Используйте SSL/TLS терминаторы при необходимости

### Права доступа к файлам:

```bash
# Установка прав на исполняемый файл
chmod 755 mtproto-proxy

# Установка прав на конфигурационные файлы
chmod 600 proxy-secret
chmod 644 proxy-multi.conf
```

## Дополнительные параметры

**Полный список параметров можно получить с помощью:**

```bash
./mtproto-proxy --help
```

### Популярные дополнительные параметры:

- `-v` - Уровень подробности вывода (например, `-v 2` для более подробного вывода)
- `-b` - Размер backlog для слушающих сокетов
- `-c` - Максимальное количество соединений
- `-W` - Ограничение размера окна TCP
- `-T` - Интервал пинга для локальных TCP-соединений

## Запуск на Windows

MTProxy в основном разработан для Linux, но его можно запустить на Windows несколькими способами:

### Вариант №1: Использование Windows Subsystem for Linux (WSL)

Это рекомендуемый подход:

1. Установите WSL2 из Microsoft Store или с помощью PowerShell:

```powershell
wsl --install
```

2. Установите дистрибутив Linux (рекомендуется Ubuntu)

3. Откройте WSL и следуйте стандартному процессу установки:

```bash
# Обновление пакетов
sudo apt update && sudo apt upgrade

# Установка зависимостей
sudo apt install git curl build-essential libssl-dev zlib1g-dev

# Клонирование и сборка MTProxy
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
make && cd objs/bin
```

4. Запустите MTProxy обычным способом:

```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S YOUR_SECRET_KEY --aes-pwd proxy-secret proxy-multi.conf -M 1
```

### Вариант №2: Использование Docker Desktop

1. Установите Docker Desktop для Windows
2. Запустите контейнер Linux:
```bash
docker run -it --name mtproxy -p 443:443 -p 8888:8888 ubuntu:latest
```

3. Внутри контейнера установите зависимости и соберите MTProxy:
```bash
apt update && apt install -y git curl build-essential libssl-dev zlib1g-dev
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
make && cd objs/bin
```

### Вариант №3: Кросс-компиляция (продвинутый уровень)

В теории, можно выполнить кросс-компиляцию для Windows, но это требует значительных изменений в кодовой базе, поскольку MTProxy использует системные вызовы Unix, недоступные в Windows.

### Важные замечания:

- MTProxy специально разработан для сред Linux/Unix
- Нативная поддержка Windows официально не предоставляется
- WSL2 обеспечивает наиболее бесшовный опыт запуска MTProxy на Windows
- Обязательно настройте брандмауэр Windows для разрешения трафика на используемых портах

Рекомендуется использовать WSL2, так как он предоставляет полную среду Linux, которую ожидает MTProxy, позволяя запускать его на вашем компьютере с Windows.
