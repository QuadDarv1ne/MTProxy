# MTProxy
Простой MT-Proto прокси

## Сборка
Установите зависимости, вам понадобится стандартный набор инструментов для сборки из исходного кода и пакеты разработки для `openssl` и `zlib`.

В Debian/Ubuntu:
```bash
apt install git curl build-essential libssl-dev zlib1g-dev
```
В CentOS/RHEL:
```bash
yum install openssl-devel zlib-devel
yum groupinstall "Development Tools"
```

Клонируйте репозиторий:
```bash
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
```

Для сборки просто выполните `make`, бинарный файл будет находиться в `objs/bin/mtproto-proxy`:

```bash
make && cd objs/bin
```

Если сборка завершилась неудачно, перед повторной сборкой следует выполнить `make clean`.

## Запуск
1. Получите секретный ключ, используемый для подключения к серверам Telegram.
```bash
curl -s https://core.telegram.org/getProxySecret -o proxy-secret
```
2. Получите текущую конфигурацию Telegram. Она может меняться (иногда), поэтому мы рекомендуем обновлять её один раз в день.
```bash
curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
```
3. Создайте секретный ключ, который будут использовать пользователи для подключения к вашему прокси.
```bash
head -c 16 /dev/urandom | xxd -ps
```
4. Запустите `mtproto-proxy`:
```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S <секретный_ключ> --aes-pwd proxy-secret proxy-multi.conf -M 1
```
... где:
- `nobody` - имя пользователя. `mtproto-proxy` вызывает `setuid()` для понижения привилегий.
- `443` - порт, который используют клиенты для подключения к прокси.
- `8888` - локальный порт. Вы можете использовать его для получения статистики от `mtproto-proxy`. Например `wget localhost:8888/stats`. Эту статистику можно получить только через loopback.
- `<секретный_ключ>` - секретный ключ, созданный на шаге 3. Также вы можете задать несколько секретных ключей: `-S <секретный_ключ1> -S <секретный_ключ2>`.
- `proxy-secret` и `proxy-multi.conf` получаются на шагах 1 и 2.
- `1` - количество рабочих процессов. Вы можете увеличить количество рабочих процессов, если у вас мощный сервер.

Также ознакомьтесь с другими параметрами, используя `mtproto-proxy --help`.

5. Создайте ссылку по следующей схеме: `tg://proxy?server=ИМЯ_СЕРВЕРА&port=ПОРТ&secret=СЕКРЕТНЫЙ_КЛЮЧ` (или предоставьте официальному боту возможность сгенерировать её за вас).
6. Зарегистрируйте ваш прокси с [@MTProxybot](https://t.me/MTProxybot) в Telegram.
7. Установите полученный тег с аргументами: `-P <тег_прокси>`
8. Наслаждайтесь.

## Случайное заполнение
Из-за того, что некоторые провайдеры обнаруживают MTProxy по размеру пакетов, случайное заполнение добавляется к пакетам, если такой режим включен.

Он включается только для клиентов, которые запрашивают это.

Добавьте префикс `dd` к секретному ключу (`cafe...babe` => `ddcafe...babe`), чтобы включить этот режим на стороне клиента.

## Пример конфигурации Systemd
1. Создайте файл службы systemd (это стандартный путь для большинства дистрибутивов Linux, но проверьте его перед использованием):
```bash
nano /etc/systemd/system/MTProxy.service
```
2. Отредактируйте эту базовую службу (особенно пути и параметры):
```bash
[Unit]
Description=MTProxy
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/MTProxy
ExecStart=/opt/MTProxy/mtproto-proxy -u nobody -p 8888 -H 443 -S <секретный_ключ> -P <тег_прокси> <другие_параметры>
Restart=on-failure

[Install]
WantedBy=multi-user.target
```
3. Перезагрузите демонов:
```bash
systemctl daemon-reload
```
4. Протестируйте новую службу MTProxy:
```bash
systemctl restart MTProxy.service
# Проверьте статус, он должен быть активным
systemctl status MTProxy.service
```
5. Включите автозапуск службы после перезагрузки:
```bash
systemctl enable MTProxy.service
```

## Docker образ
Telegram также предоставляет [официальный Docker образ](https://hub.docker.com/r/telegrammessenger/proxy/).
Примечание: образ устарел.
