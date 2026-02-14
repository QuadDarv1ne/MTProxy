# MTProxy

**MTProxy** — это высокопроизводительный прокси-сервер для протокола `MTProto`, используемого `Telegram` для обеспечения безопасной связи между клиентами и серверами `Telegram`

`MTProxy` позволяет обходить блокировки и фильтрацию, обеспечивая быстрый и надежный доступ к сервисам `Telegram`

## Особенности

- ⚡ Высокая производительность и низкая задержка
- 🔒 Полная совместимость с протоколом `MTProto`
- 🛡️ Защита от блокировок через случайное заполнение
- 👥 Поддержка нескольких секретных ключей
- 📊 Интегрированная система статистики
- 🔄 Автоматическое обновление конфигурации
- 🚀 Оптимизированная архитектура с модульной структурой
- 🔐 Расширенные функции безопасности
- ⚙️ Улучшенное управление соединениями и потоками

## Сборка

**Для сборки проекта установите необходимые зависимости:**

**В Debian/Ubuntu:**

```bash
apt update
apt install git curl build-essential libssl-dev zlib1g-dev cmake
```

**В CentOS/RHEL/Fedora:**

```bash
# Для CentOS/RHEL
yum install openssl-devel zlib-devel cmake
yum groupinstall "Development Tools"

# Для Fedora
dnf install openssl-devel zlib-devel make automake gcc gcc-c++ cmake
```

### Использование Make (оригинальный метод):

```bash
make && cd objs/bin
```

### Использование CMake (новый метод):

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
# Исполняемый файл будет создан в build/bin/
```

Для получения более подробной информации о сборке с использованием CMake см. файл [BUILD_CMAKE.md](BUILD_CMAKE.md).

**Клонируйте репозиторий:**

```bash
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
```

**Для сборки просто выполните `make`, бинарный файл будет находиться в `objs/bin/mtproto-proxy`:**

```bash
make && cd objs/bin
```

Если сборка завершилась неудачно, перед повторной сборкой следует выполнить `make clean`

## Быстрый старт

1. **Получите секретный ключ** для подключения к серверам Telegram:

```bash
curl -s https://core.telegram.org/getProxySecret -o proxy-secret
```

2. **Получите текущую конфигурацию** Telegram (рекомендуется обновлять ежедневно):

```bash
curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
```

3. **Создайте секретный ключ** для подключения клиентов к вашему прокси:

```bash
head -c 16 /dev/urandom | xxd -ps
```

4. **Запустите `mtproto-proxy`:**

```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S <ваш_секретный_ключ> --aes-pwd proxy-secret proxy-multi.conf -M 1
```

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
./mtproto-proxy -u nobody -p 8888 -H 443 -S <ваш_секретный_ключ> --aes-pwd proxy-secret proxy-multi.conf -M 1
```

### Параметры запуска

- `-u nobody` — имя пользователя, под которым будет работать процесс (рекомендуется использовать отдельного пользователя)
- `-p 8888` — локальный порт для получения статистики (доступен только через localhost)
- `-H 443` — порт, который будут использовать клиенты для подключения к прокси
- `-S <секретный_ключ>` — секретный ключ, созданный на шаге 3 (можно указать несколько: `-S ключ1 -S ключ2`)
- `--aes-pwd proxy-secret` — путь к файлу с секретом от Telegram
- `proxy-multi.conf` — путь к файлу конфигурации от Telegram
- `-M 1` — количество рабочих процессов (увеличьте при высокой нагрузке)

Также ознакомьтесь с другими параметрами, используя `mtproto-proxy --help`

### Получение статистики

Вы можете получить статистику работы прокси, выполнив:

```bash
wget localhost:8888/stats
```

> ⚠️ Доступ к статистике возможен только с локального хоста

### Регистрация прокси

1. Создайте ссылку для подключения к вашему прокси по схеме: 
   `tg://proxy?server=ВАШ_СЕРВЕР&port=ПОРТ&secret=ВАШ_СЕКРЕТНЫЙ_КЛЮЧ`
2. Зарегистрируйте ваш прокси с [@MTProxybot](https://t.me/MTProxybot) в Telegram
3. После регистрации получите тег и добавьте его при запуске: `-P <тег_прокси>`

## Режим случайного заполнения

Некоторые провайдеры могут блокировать MTProxy по размеру пакетов. Для обхода таких блокировок введите режим случайного заполнения.

**Как включить:**

- Добавьте префикс `dd` к вашему секретному ключу
- Пример: `cafebabe12345678` → `ddcafebabe12345678`
- Клиенты, использующие такой ключ, будут получать дополнительное случайное заполнение

## Установка как сервиса (Systemd)

1. **Создайте файл службы** systemd (стандартный путь для большинства дистрибутивов Linux):

```bash
sudo nano /etc/systemd/system/MTProxy.service
```

2. **Добавьте конфигурацию** (обязательно измените пути и параметры под вашу систему):

```ini
[Unit]
Description=MTProxy Service
After=network.target

[Service]
Type=simple
User=nobody
WorkingDirectory=/opt/MTProxy
ExecStart=/opt/MTProxy/mtproto-proxy -u nobody -p 8888 -H 443 -S <ваш_секретный_ключ> --aes-pwd proxy-secret proxy-multi.conf -M 1
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

3. **Перезагрузите конфигурацию** systemd:

```bash
sudo systemctl daemon-reload
```

4. **Запустите и проверьте** работу службы:

```bash
sudo systemctl start MTProxy.service
sudo systemctl status MTProxy.service
```

5. **Включите автозапуск** после перезагрузки:

```bash
sudo systemctl enable MTProxy.service
```

## Дополнительные параметры

Полный список параметров можно получить с помощью:

```bash
./mtproto-proxy --help
```

## Мониторинг и логирование

### 📊 Система мониторинга

- Расширенная система метрик и логирования
- Алертинг по порогам (CPU, память, подключения)
- Статистика по компонентам и времени отклика
- Экспорт данных для внешних систем
- Уровни логирования (ERROR, WARNING, INFO, DEBUG)

### 🖥️ Web-интерфейс администратора

- Современный REST API для управления MTProxy
- Веб-панель управления с аутентификацией
- Система пользователей и ролей доступа
- API ключи для программного доступа
- Мониторинг в реальном времени
- Логирование безопасности и аудит

**Пример использования:**
```c
// Инициализация веб-интерфейса
web_server_config_t web_config = {0};
web_config.enable_http = 1;
web_config.http_port = 8080;
web_config.enable_https = 1;
web_config.https_port = 8443;
web_config.enable_rate_limiting = 1;
web_config.requests_per_minute = 60;

admin_web_interface_t *admin_web = admin_web_init(&web_config);
admin_web_start_server(admin_web);

// Создание администратора
admin_web_create_user(admin_web, "admin", "secure_password123", 
                     "admin@example.com", ADMIN_ACCESS_ADMIN);

// Создание API ключа
c
har api_key[128];
admin_web_create_api_key(admin_web, user_id, "Monitoring service", 
                        API_KEY_TYPE_READ, 86400, api_key);

// Аутентификация через API
uint64_t user_id;
api_key_type_t key_type;
if (admin_web_validate_api_key(admin_web, api_key, &key_type, &user_id) == 0) {
    printf("API ключ валиден, тип: %d\n", key_type);
}

// Обработка API запросов
char response[1024];
int status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                         API_ENDPOINT_STATS, NULL, 
                                         api_key, response, sizeof(response));

// Мониторинг статистики
web_interface_stats_t stats;
admin_web_get_stats(admin_web, &stats);
printf("Активные соединения: %lld\n", stats.active_sessions);

// Закрытие веб-интерфейса
admin_web_stop_server(admin_web);
admin_web_cleanup(admin_web);
```

- Полная реализация WebSocket RFC 6455
- Поддержка WSS (WebSocket Secure) через TLS
- MTProto туннелирование через WebSocket
- Автоматический handshake и управление соединениями
- Поддержка текстовых и бинарных фреймов
- Интеграция с существующей системой MTProto

**Пример использования:**
```c
// Инициализация WebSocket поддержки
websocket_config_t ws_config = {0};
ws_config.enable_server = 1;
ws_config.enable_client = 1;
ws_config.max_connections = 16384;
ws_config.enable_mtproto_tunnel = 1;

websocket_support_t *websocket = websocket_init(&ws_config);

// Принятие WebSocket соединения
int client_fd = accept(server_socket, NULL, NULL);
websocket_accept_connection(websocket, client_fd);

// Выполнение WebSocket handshake
websocket_perform_server_handshake(websocket, connection_id);

// Включение MTProto туннелирования
unsigned char mtproto_key[32] = { /* ключ */ };
websocket_enable_mtproto_tunnel(websocket, connection_id, mtproto_key, session_id);

// Отправка данных через WebSocket
unsigned char data[1024] = { /* MTProto данные */ };
websocket_mtproto_send_data(websocket, connection_id, data, sizeof(data));

// Обработка входящих сообщений
websocket_handle_data(websocket, connection_id);

// Отправка ping/pong
websocket_send_ping(websocket, connection_id);

// Закрытие соединения
websocket_close_connection(websocket, connection_id, 1000, "Normal closure");

// Получение статистики
websocket_stats_t stats;
websocket_get_stats(websocket, &stats);
printf("Active connections: %lld\n", stats.active_connections);
```

- Поддержка ARM64 и x86_64 архитектур
- Автоматическое определение доступных расширений (AVX-512, AES-NI, NEON)
- Интеллектуальная балансировка нагрузки между ядрами
- Оптимизированное использование памяти и пулы
- Расширенное кэширование криптографических операций

**Пример использования:**
```c
// Инициализация расширенной оптимизации
advanced_crypto_opt_t *crypto_opt = crypto_opt_init(2048, 64*1024*1024); // 2048 контекстов, 64MB памяти
crypto_opt_configure(crypto_opt, CRYPTO_OPT_AUTO, BALANCE_ADAPTIVE);

// Определение архитектуры
architecture_info_t arch_info;
crypto_opt_detect_architecture(&arch_info);
printf("Архитектура: %s\n", crypto_opt_get_architecture_name(arch_info.supported_extensions));

// Предвыделение контекстов для часто используемых ключей
const unsigned char *keys[] = {key1, key2, key3};
crypto_opt_precompute_keys(crypto_opt, keys, 3, 32);

// Оптимизированное шифрование
unsigned char plaintext[1024];
unsigned char ciphertext[1024];
int result = crypto_opt_cache_encrypt(crypto_opt, session_key, plaintext, ciphertext, sizeof(plaintext));

// Балансировка нагрузки между ядрами CPU
int cpu_core = crypto_opt_balance_operation(crypto_opt, CRYPTO_OPERATION_ENCRYPT);

// Мониторинг производительности
double cache_hit_rate = crypto_opt_get_cache_hit_rate(crypto_opt);
int optimal_threads = crypto_opt_get_optimal_thread_count(crypto_opt);

// Получение статистики
char stats[256];
crypto_opt_get_stats(crypto_opt, stats, sizeof(stats));
printf("%s\n", stats);
```

- Полноценная реализация протокола версии 3.0
- Perfect Forward Secrecy (PFS) для усиленной безопасности
- Улучшенные методы аутентификации и шифрования
- Совместимость с существующими версиями
- Поддержка расширенных возможностей протокола

**Пример использования:**
```c
// Инициализация MTProto v3 соединения
mtproto_connection_info_t conn;
mtproto_init_connection(&conn, MTPROTO_VERSION_3_0);

// Handshake с клиентом
unsigned char client_handshake[64];
// ... получение данных от клиента ...
int result = mtproto_handshake_v3(&conn, client_handshake, sizeof(client_handshake));

if (result == 0) {
    // Успешное рукопожатие
    int auth_valid = mtproto_validate_auth_v3(&conn);
    if (auth_valid) {
        printf("MTProto v3 соединение установлено\n");
    }
}

// Шифрование данных
unsigned char plaintext[1024];
unsigned char ciphertext[1088]; // +16 байт для тега аутентификации
int encrypted_len = mtproto_encrypt_v3(&conn, plaintext, ciphertext, sizeof(plaintext));

// Получение информации о соединении
char info[256];
mtproto_get_connection_info_v3(&conn, info, sizeof(info));
printf("%s\n", info);
```

- Система отслеживания состояния соединений
- Автоматическое восстановление при ошибках
- Мониторинг качества и времени отклика
- Интеллектуальная диагностика проблем
- Поддержка множества протоколов (MTProto, Shadowsocks, HTTP, SOCKS5)

**Пример использования:**
```c
// Инициализация системы надежности
protocol_reliability_t *reliability = protocol_reliability_init(65536);
protocol_reliability_configure(reliability, 1, 5, 1000); // auto-reconnect, 5 attempts, 1s delay

// Отслеживание соединения
protocol_reliability_track_connection(reliability, fd, PROTOCOL_TYPE_MTProto, 
                                    remote_ip, remote_port);

// Обработка ошибок
protocol_reliability_handle_error(reliability, conn_id, PROTOCOL_ERROR_TIMEOUT);

// Мониторинг
protocol_reliability_start_monitoring(reliability);
protocol_reliability_perform_health_check(reliability);

// Получение статистики
char stats[256];
double success_rate = protocol_reliability_get_success_rate(reliability);
protocol_reliability_get_stats(reliability, stats, sizeof(stats));

// Callback функции
protocol_reliability_set_error_callback(reliability, error_handler);
protocol_reliability_set_reconnect_callback(reliability, reconnect_handler);
```

- Автоматическая оптимизация под текущую нагрузку
- Самообучение и адаптация к изменяющимся условиям
- Прогнозирование и предотвращение проблем
- Стратегии настройки: консервативная, агрессивная, сбалансированная

**Пример использования:**
```c
// Инициализация
adaptive_tuning_t *tuning = adaptive_tuning_init(STRATEGY_BALANCED);
adaptive_tuning_configure(tuning, 0.9, 1.0); // target 90%, aggressiveness 1.0

// Определение параметров для настройки
adaptive_tuning_add_parameter(tuning, "thread_pool_size", "Размер пула потоков", 
                             PARAM_TYPE_INTEGER, 16, 32);
adaptive_tuning_add_parameter(tuning, "buffer_size", "Размер буферов",
                             PARAM_TYPE_INTEGER, 8192, 16384);

// Настройка целевых метрик
adaptive_tuning_add_metric(tuning, "throughput", 100.0, 200.0, 1.0);
adaptive_tuning_add_metric(tuning, "response_time", 20.0, 10.0, 1.0);

// Запуск адаптивной настройки
adaptive_tuning_start(tuning);

// Регулярная адаптация (в отдельном потоке)
while (running) {
    adaptive_tuning_collect_state(tuning);
    adaptive_tuning_analyze_performance(tuning);
    adaptive_tuning_make_adjustments(tuning);
    adaptive_tuning_apply_optimizations(tuning);
    
    char recommendations[256];
    adaptive_tuning_get_recommendations(tuning, recommendations, sizeof(recommendations));
    
    sleep_ms(5000);
}

adaptive_tuning_stop(tuning);
adaptive_tuning_cleanup(tuning);
```

## Безопасность

- Используйте отдельного пользователя для запуска прокси (не root)
- Ограничьте доступ к порту статистики (обычно 8888) только с локального хоста
- Регулярно обновляйте конфигурацию от Telegram
- Храните секретные файлы в защищенном месте

## Troubleshooting

- Если возникают проблемы с подключением, проверьте настройки firewall
- Убедитесь, что используемые порты открыты для внешнего подключения
- Проверьте права доступа к файлам `proxy-secret` и `proxy-multi.conf`
- Для отладки используйте логи системы и вывод команды статистики

## Лицензия

**Проект распространяется под двойной лицензией:**

- Основное приложение: GNU General Public License версия 2 (GPLv2)
- Библиотечные компоненты: GNU Lesser General Public License версия 2.1 (LGPLv2.1)

См. файлы `LICENSE`, `GPLv2` и `LGPLv2` для подробностей.

---

### 💼 Профиль на Profi.ru

[![Profi.ru Profile](https://img.shields.io/badge/Profi.ru-Дуплей%20М.И.-FF6B35?style=for-the-badge)](https://profi.ru/profile/DupleyMI)

> Консультации и услуги программирования на платформе Profi.ru

---

### 📚 Услуги обучения

[![Обучение технологиям и языкам программирования на Kwork](https://img.shields.io/badge/Kwork-Обучение%20Программированию-blue?style=for-the-badge&logo=kwork)](https://kwork.ru/usability-testing/42465951/obuchenie-tekhnologiyam-i-yazykam-programmirovaniya)

> Профессиональное обучение технологиям и языкам программирования. Персональные консультации и курсы от опытного преподавателя.

---

### 🏫 О школе

[![Website](https://img.shields.io/badge/Maestro7IT-school--maestro7it.ru-darkgreen?style=for-the-badge)](https://school-maestro7it.ru/)

> Инновационная школа программирования, специализирующаяся на подготовке специалистов в области современных технологий и языков программирования.

---

💼 **Автор:** Дуплей Максим Игоревич

📲 **Telegram №1:** [@quadd4rv1n7](https://t.me/quadd4rv1n7)

📲 **Telegram №2:** [@dupley_maxim_1999](https://t.me/dupley_maxim_1999)

📅 **Дата:** 12.02.2026
