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
- 🚀 Система адаптивной оптимизации производительности

### 🆕 Новые возможности (Март 2026)

#### Управление конфигурацией
- 📝 **Расширенная система конфигурации** с callback'ами и историей изменений
- 🔄 **Горячая перезагрузка** конфигурации без остановки сервиса
- 📦 **JSON экспорт/импорт** для удобного управления настройками
- ✅ **Валидация параметров** с поддержкой пользовательских правил
- 📊 **История изменений** до 1000 записей с временными метками
- 📦 **Batch режим** для атомарного применения множественных изменений

#### Кэширование и производительность
- 🚀 **Система кэширования** с 5 алгоритмами (LRU, LFU, FIFO, TTL, ARC)
- 🔀 **Partitioned кэш** для многопоточной работы
- 📈 **Мониторинг hit rate** и статистики кэширования
- 💾 **Персистентность** с сохранением на диск
- ⚡ **Атомарные операции** increment/decrement для счетчиков

#### Безопасность и ограничения
- 🛑 **Rate Limiting** для защиты от перегрузок
- 🔒 **5 алгоритмов** ограничения скорости (Token Bucket, Sliding Window, Fixed Window, Leaky Bucket, Adaptive)
- ⚪ **Whitelist/Blacklist** клиентов
- 🚨 **Circuit Breaker** для защиты от каскадных сбоев

#### Обработка ошибок
- 📋 **Централизованная система ошибок** с 12 категориями
- 🔄 **Стратегии восстановления**: retry, fallback, restart, shutdown
- 📊 **Мониторинг ошибок** со статистикой по уровням и категориям
- 🔍 **Correlation ID** для трассировки ошибок

#### FFI и Mobile
- 📱 **Shared library** для FFI интеграции
- 🎯 **Flutter/Dart mobile app** для кроссплатформенной разработки
- 🔌 **Публичный API** для внешней интеграции

## 📁 Структура проекта

```
MTProxy/
├── engine/              # Ядро движка
├── system/              # Системная оптимизация (82 файла)
│   ├── optimization/    # Оптимизация производительности
│   ├── monitoring/      # Мониторинг и метрики
│   ├── integration/     # Интеграция компонентов
│   ├── diagnostic/      # Диагностика
│   ├── debugging/       # Отладка
│   └── config/          # Конфигурация системы
├── security/            # Безопасность (6 модулей)
├── security_enhanced/   # Расширенная безопасность
├── net/                 # Сетевые модули (41 файл)
├── crypto/              # Криптография
├── mtproto/             # Протокол MTProto
├── common/              # Общие утилиты
├── perf_monitor/        # Мониторинг производительности
├── ml/                  # ML-компоненты
├── conn_pool/           # Пулы соединений
├── thread_system/       # Потоковая система
├── infrastructure/      # Инфраструктура
├── jobs/                # Система задач
├── vv/                  # VV-дерево
├── shadowsocks/         # Shadowsocks обфускация
├── mobile_app/          # Flutter/Dart приложение
├── admin/               # Admin CLI утилита
├── testing/             # Тесты
├── docs/                # Документация
├── scripts/             # Скрипты
└── build-scripts/       # Скрипты сборки
```

### 📊 Статистика проекта (Март 2026)

| Метрика | Значение |
|---------|----------|
| **C-файлов** | 183+ |
| **Сетевых модулей** | 41 |
| **Файлов в system/** | 82 |
| **Модулей безопасности** | 6 + security_enhanced |
| **Документов** | 12+ |
| **Тестов** | 45 C + 4 Dart (100% пройдено) |
| **Сборка** | mtproto-proxy (536 KB), mtproxy-admin |
| **Shared library** | ✅ BUILD_SHARED_LIB |
| **Mobile app** | ✅ Flutter/Dart (40+ файлов) |

## 📋 Статус сборок

| Платформа | Статус | CI/CD | Скрипт |
|-----------|--------|-------|--------|
| **Linux** | ✅ Работает | ✅ GitHub Actions | `make`, `cmake` |
| **Windows** | ✅ Работает | ✅ GitHub Actions | `build-native-windows.ps1` |
| **macOS** | ✅ Работает | ✅ GitHub Actions | `build-native-libs.sh macos` |
| **Android** | ⚠️ Требуется NDK | ✅ GitHub Actions | `build-native-libs.sh android` |
| **iOS** | ⚠️ Только macOS | ✅ GitHub Actions | `build-native-libs.sh ios` |

### Требования для сборки

**Linux (Debian/Ubuntu):**
```bash
apt update
apt install git curl build-essential libssl-dev zlib1g-dev cmake
```

**Linux (CentOS/RHEL/Fedora):**
```bash
# Для CentOS/RHEL
yum install openssl-devel zlib-devel cmake
yum groupinstall "Development Tools"

# Для Fedora
dnf install openssl-devel zlib-devel make automake gcc gcc-c++ cmake
```

**Windows:**
- MSYS2 с UCRT64 или MinGW-w64
- CMake 3.20+
- OpenSSL, ZLIB

**macOS:**
- Xcode Command Line Tools
- Homebrew: `brew install cmake openssl`

**Android:**
- Android NDK (переменная окружения `ANDROID_NDK`)
- CMake 3.20+

**iOS:**
- macOS (сборка только на macOS)
- Xcode 14+
- CMake 3.20+

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

### 📚 Система примеров и документации

- Практические примеры для всех сценариев использования
- Интерактивные примеры с пошаговыми инструкциями
- Автоматическая генерация конфигурационных файлов
- Поддержка Docker и containerized deployments
- Система difficulty levels для разных уровней подготовки
- Экспорт примеров в различные форматы

**Пример использования:**
```c
// Инициализация системы примеров
mtproxy_examples_t *examples = examples_init(8, 32);

// Просмотр доступных коллекций
char collections_list[1024];
examples_list_collections(examples, collections_list, sizeof(collections_list));
printf("%s\n", collections_list);

// Запуск базовых примеров
examples_run_examples_by_type(examples, EXAMPLE_TYPE_BASIC);

// Запуск примеров безопасности
examples_run_examples_by_scenario(examples, SCENARIO_SECURITY_HARDENED);

// Генерация конфигурационного файла для примера
examples_generate_configuration_file(examples, example_id, "config.json");

// Генерация Docker Compose для деплоя
examples_generate_docker_compose(examples, example_id, "docker-compose.yml");

// Экспорт всех примеров
examples_export_all_examples(examples, "all-examples.zip");

// Очистка
examples_cleanup(examples);
```

- Комплексное тестирование всех компонентов
- Модульные, интеграционные и стресс-тесты
- Автоматическая генерация отчетов
- Тесты производительности и безопасности
- Система assertions и проверок
- Поддержка различных форматов отчетов

**Пример использования:**
```c
// Инициализация системы тестирования
automated_testing_t *testing = testing_init(8, 64);

testing_config_t test_config = {0};
test_config.enable_parallel_execution = 1;
test_config.max_parallel_tests = 4;
test_config.verbose_output = 1;
test_config.test_timeout_ms = 30000;

// Создание тестовых наборов
testing_create_suite(testing, "Security Tests", "Security module testing");
testing_create_suite(testing, "Performance Tests", "Performance benchmarking");
testing_create_suite(testing, "Integration Tests", "Component integration tests");

// Добавление тестов
testing_add_test(testing, 1, "Buffer Overflow Protection",
                "Test buffer overflow protection mechanisms",
                TEST_TYPE_SECURITY, TEST_CRITICALITY_CRITICAL,
                test_buffer_overflow_protection);

testing_add_test(testing, 1, "DDoS Protection",
                "Test DDoS attack mitigation",
                TEST_TYPE_SECURITY, TEST_CRITICALITY_HIGH,
                test_ddos_protection);

testing_add_test(testing, 2, "Crypto Performance",
                "Test cryptographic performance optimization",
                TEST_TYPE_PERFORMANCE, TEST_CRITICALITY_MEDIUM,
                test_crypto_performance);

// Запуск тестов
int failed_tests = testing_run_all_tests(testing);
printf("Failed tests: %d\n", failed_tests);

// Получение статистики
testing_stats_t stats;
testing_get_stats(testing, &stats);
printf("Pass rate: %.2f%%\n", stats.pass_rate_percentage);

// Генерация отчета
testing_generate_report(testing, "test-results.json");

// Очистка
testing_cleanup(testing);
```

- Единая точка управления всеми подсистемами
- Автоматическая координация компонентов
- Мониторинг здоровья системы
- Автоматическое восстановление после сбоев
- Управление зависимостями между компонентами
- Централизованная статистика и отчеты

**Пример использования:**
```c
// Инициализация системы интеграции
component_integration_t *integration = integration_init(16);

// Регистрация компонентов
integration_register_component(integration, COMPONENT_TYPE_SECURITY,
                              "security-module", "Security protection system",
                              PRIORITY_CRITICAL, security_module);

integration_register_component(integration, COMPONENT_TYPE_NETWORK,
                              "network-stack", "Advanced networking",
                              PRIORITY_HIGH, network_module);

integration_register_component(integration, COMPONENT_TYPE_MONITORING,
                              "monitoring-system", "Performance monitoring",
                              PRIORITY_MEDIUM, monitoring_module);

// Запуск всех компонентов
integration_start_all_components(integration);

// Проверка здоровья системы
int healthy_components = integration_perform_health_check(integration);
printf("Healthy components: %d\n", healthy_components);

// Мониторинг состояния
integration_stats_t stats;
integration_get_stats(integration, &stats);
printf("Active components: %lld\n", stats.active_components);

// Автоматическое восстановление
integration_perform_auto_recovery(integration);

// Получение отчетов
char report[1024];
integration_get_system_report(integration, report, sizeof(report));
printf("System report: %s\n", report);

// Остановка системы
integration_stop_all_components(integration);
integration_cleanup(integration);
```

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

## Система оптимизации производительности

MTProxy теперь включает в себя продвинутую систему оптимизации производительности, которая автоматически адаптирует параметры под текущую нагрузку:

- **Простая система оптимизации**: Интегрирована с основным кодом MTProxy, обеспечивает базовую оптимизацию
- **Продвинутая система оптимизации**: Включает алгоритмы оптимизации привязки CPU и управления пулом потоков
- **Адаптивная настройка**: Автоматически регулирует параметры под текущую нагрузку
- **Мониторинг производительности**: Сбор и анализ метрик для принятия решений об оптимизации

Система оптимизации работает автоматически и не требует дополнительной настройки.

### Продвинутая система оптимизации

MTProxy теперь включает в себя продвинутую систему оптимизации, интегрирующую современные технологии:

- **NUMA-оптимизация**: Интеллектуальное распределение памяти с учетом архитектуры NUMA
- **io_uring**: Асинхронный ввод-вывод для максимальной производительности
- **DPDK**: Высокопроизводительная работа с сетью через userspace networking
- **Автоматическая настройка**: Адаптивная оптимизация под текущую нагрузку

Система поддерживает 4 уровня оптимизации от BASIC до MAXIMUM и может автоматически адаптироваться под изменяющиеся условия.

Для получения более подробной информации об улучшениях см. файл [NOTES_RU.md](NOTES_RU.md). Документация по продвинутой системе оптимизации доступна в файле [system/ADVANCED_OPTIMIZATION_SYSTEM_RU.md](system/ADVANCED_OPTIMIZATION_SYSTEM_RU.md). Документация по расширенной системе обфускации доступна в файле [docs/OBFUSCATION_ENHANCEMENTS_RU.md](docs/OBFUSCATION_ENHANCEMENTS_RU.md). Документация по оптимизации производительности доступна в файле [docs/PERFORMANCE_OPTIMIZATIONS_RU.md](docs/PERFORMANCE_OPTIMIZATIONS_RU.md).

## 📚 Документация

### Основные руководства
- [CONFIGURATION_ENHANCEMENTS_RU.md](docs/CONFIGURATION_ENHANCEMENTS_RU.md) — Система управления конфигурацией
- [CACHE_SYSTEM_RU.md](docs/CACHE_SYSTEM_RU.md) — Система кэширования
- [IMPROVEMENTS_SUMMARY.md](docs/IMPROVEMENTS_SUMMARY.md) — Сводка всех улучшений
- [USAGE_EXAMPLES.md](docs/USAGE_EXAMPLES.md) — Примеры использования

### Безопасность
- [SECURITY_FEATURES.md](security/SECURITY_FEATURES.md) — Функции безопасности
- [SECURITY_NOTES_RU.md](security/SECURITY_NOTES_RU.md) — Заметки по безопасности
- [COMPREHENSIVE_SECURITY_RU.md](security/COMPREHENSIVE_SECURITY_RU.md) — Комплексная безопасность

### Производительность
- [ADVANCED_OPTIMIZATION_SYSTEM_RU.md](system/ADVANCED_OPTIMIZATION_SYSTEM_RU.md) — Продвинутая оптимизация
- [PERFORMANCE_OPTIMIZATIONS_RU.md](docs/PERFORMANCE_OPTIMIZATIONS_RU.md) — Оптимизация производительности
- [CRYPTO_OPTIMIZATIONS_RU.md](docs/CRYPTO_OPTIMIZATIONS_RU.md) — Криптографические оптимизации
- [MEMORY_OPTIMIZATION_RU.md](docs/MEMORY_OPTIMIZATION_RU.md) — Оптимизация памяти

### Архитектура
- [MODULAR_ARCHITECTURE_RU.md](docs/MODULAR_ARCHITECTURE_RU.md) — Модульная архитектура
- [ADVANCED_LOGGING_RU.md](docs/ADVANCED_LOGGING_RU.md) — Система логирования
- [GITIGNORE_DOCUMENTATION_RU.md](docs/GITIGNORE_DOCUMENTATION_RU.md) — Документация .gitignore

### Сборка и оптимизация
- [BUILD_CMAKE.md](BUILD_CMAKE.md) — Сборка с помощью CMake
- [CMAKE_OPTIMIZATIONS_RU.md](docs/CMAKE_OPTIMIZATIONS_RU.md) — Оптимизации CMake (PGO, кэширование)

### Интеграция
- [FFI_INTEGRATION.md](mobile_app/FFI_INTEGRATION.md) — FFI интеграция (Flutter/Dart)
- [MOBILE_APP_README.md](mobile_app/README.md) — Mobile приложение

## Безопасность

MTProxy теперь включает в себя расширенные функции безопасности:

- **Привязка сертификатов (Certificate Pinning)**: Защита от атак типа "человек посередине" путем проверки сертификатов серверов
- **Защита от DDoS-атак**: Механизмы ограничения скорости и фильтрации подозрительных подключений
- **Интеграция с HSM**: Поддержка аппаратных модулей безопасности для хранения ключей (планируется)

Более подробную информацию о функциях безопасности см. в файле [security/SECURITY_FEATURES.md](security/SECURITY_FEATURES.md). Также доступны русскоязычные заметки в файле [security/SECURITY_NOTES_RU.md](security/SECURITY_NOTES_RU.md) и комплексная документация по безопасности в файле [security/COMPREHENSIVE_SECURITY_RU.md](security/COMPREHENSIVE_SECURITY_RU.md).

## Troubleshooting

- Если возникают проблемы с подключением, проверьте настройки firewall
- Убедитесь, что используемые порты открыты для внешнего подключения
- Проверьте права доступа к файлам `proxy-secret` и `proxy-multi.conf`
- Для отладки используйте логи системы и вывод команды статистики

## Лицензия

**Проект распространяется под двойной лицензией:**

- Основное приложение: GNU General Public License версия 2 (GPLv2)
- Библиотечные компоненты: GNU Lesser General Public License версия 2.1 (LGPLv2.1)

См. директорию [licenses/](licenses/) для подробностей.

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
