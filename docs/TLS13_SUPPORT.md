# TLS 1.3 Поддержка в MTProxy

## Обзор

MTProxy поддерживает TLS 1.3 для обеспечения максимальной безопасности соединений.

## Возможности

- ✅ Полная поддержка TLS 1.3 (RFC 8446)
- ✅ TLS 1.3 0-RTT (ранние данные)
- ✅ Perfect Forward Secrecy (PFS)
- ✅ Modern cipher suites
- ✅ Certificate pinning
- ✅ OCSP stapling
- ✅ Session tickets

## Требования

### OpenSSL 3.0+

TLS 1.3 требует OpenSSL 3.0 или новее:

```bash
# Проверка версии
openssl version

# Должно быть: OpenSSL 3.0.x или новее
```

### Сборка с TLS 1.3

```bash
# CMake
cmake -DENABLE_TLS13=ON ..
cmake --build . --parallel

# Make
make ENABLE_TLS13=1 -j4
```

## Конфигурация

### Базовая настройка TLS 1.3

```ini
# mtproxy.conf
[tls]
enabled = true
min_version = TLS1.3
max_version = TLS1.3

# Сертификаты
cert_file = /etc/mtproxy/server.crt
key_file = /etc/mtproxy/server.key
ca_file = /etc/mtproxy/ca.crt

# Cipher suites (TLS 1.3)
cipher_suites = TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256

# Session settings
session_timeout = 300
session_cache_size = 10000

# 0-RTT
early_data = true
max_early_data = 16384
```

### Расширенная настройка

```ini
[tls]
# Кривые для key exchange
curves = X25519:P-256:P-384

# Signature algorithms
signature_algorithms = ed25519:ecdsa_secp384r1_sha384:rsa_pss_rsae_sha384

# Certificate verification
verify_mode = peer
verify_depth = 3

# OCSP stapling
ocsp_stapling = true
ocsp_file = /etc/mtproxy/ocsp_response.der

# Certificate transparency
ct_validation = true

# Session tickets
session_tickets = true
ticket_lifetime = 86400

# Key logging (для отладки)
keylog_file = /var/log/mtproxy/tls_keys.log
```

## Генерация сертификатов

### Самоподписанный сертификат (тестирование)

```bash
# Генерация приватного ключа
openssl genpkey -algorithm Ed25519 -out server.key

# Генерация сертификата
openssl req -new -x509 -key server.key -out server.crt \
  -days 365 -subj "/CN=localhost" \
  -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

# Проверка
openssl x509 -in server.crt -text -noout
```

### Сертификат от Let's Encrypt

```bash
# Установка certbot
apt install certbot

# Получение сертификата
certbot certonly --standalone -d your-domain.com

# Сертификаты будут в:
# /etc/letsencrypt/live/your-domain.com/fullchain.pem
# /etc/letsencrypt/live/your-domain.com/privkey.pem
```

### Конвертация в PEM

```bash
# Конвертация из PKCS#12
openssl pkcs12 -in cert.pfx -out cert.pem -nodes

# Разделение ключа и сертификата
openssl pkcs12 -in cert.pfx -nocerts -out key.pem
openssl pkcs12 -in cert.pfx -clcerts -nokeys -out cert.pem
```

## Cipher Suites TLS 1.3

TLS 1.3 поддерживает только 5 cipher suites:

| Cipher Suite | Key Exchange | Authentication | Encryption | MAC |
|--------------|--------------|----------------|------------|-----|
| TLS_AES_256_GCM_SHA384 | ECDHE | RSA/ECDSA | AES-256-GCM | SHA384 |
| TLS_CHACHA20_POLY1305_SHA256 | ECDHE | RSA/ECDSA | ChaCha20-Poly1305 | SHA256 |
| TLS_AES_128_GCM_SHA256 | ECDHE | RSA/ECDSA | AES-128-GCM | SHA256 |
| TLS_AES_128_CCM_SHA256 | ECDHE | RSA/ECDSA | AES-128-CCM | SHA256 |
| TLS_AES_128_CCM_8_SHA256 | ECDHE | RSA/ECDSA | AES-128-CCM-8 | SHA256 |

### Рекомендуемая конфигурация

```ini
# Для максимальной безопасности
cipher_suites = TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256

# Для совместимости
cipher_suites = TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256
```

## 0-RTT (Early Data)

### Включение 0-RTT

```ini
[tls]
early_data = true
max_early_data = 16384  # 16 KB
```

### Использование 0-RTT

```c
#include "crypto/tls-enhanced.h"

tls_config_t config = {0};
config.early_data_enabled = 1;
config.max_early_data = 16384;

tls_context_t* ctx = tls_context_create(&config);

// Клиент может отправить 0-RTT данные
tls_connection_t* conn = tls_connect(ctx, "example.com", 443);

if (tls_connection_is_early_data(conn)) {
    printf("Connected with 0-RTT!\n");
}
```

### Безопасность 0-RTT

⚠️ **Важно:** 0-RTT данные уязвимы для replay атак.

**Рекомендации:**
- Используйте 0-RTT только для идемпотентных запросов (GET, HEAD)
- Не используйте для POST, PUT, DELETE
- Включите anti-replay защиту

```ini
[tls]
# Anti-replay защита
early_data_replay_protection = true
early_data_max_age = 600  # 10 минут
```

## Certificate Pinning

### Настройка pinning

```ini
[tls]
# Pinning по public key
pin_type = pubkey
pin_sha256 = base64+sha256+hash+here==

# Pinning по сертификату
pin_type = cert
pin_sha256 = base64+sha256+hash+here==

# Multiple pins (backup)
pin_backup_sha256 = backup+hash+here==
```

### Получение pin hash

```bash
# Public key pin
openssl x509 -in server.crt -pubkey -noout | \
  openssl pkey -pubin -outform der | \
  openssl dgst -sha256 -binary | \
  openssl enc -base64

# Certificate pin
openssl x509 -in server.crt -outform der | \
  openssl dgst -sha256 -binary | \
  openssl enc -base64
```

## OCSP Stapling

### Включение OCSP stapling

```ini
[tls]
ocsp_stapling = true
ocsp_file = /etc/mtproxy/ocsp_response.der
ocsp_update_interval = 3600  # 1 час
```

### Обновление OCSP ответа

```bash
#!/bin/bash
# update-ocsp.sh

CERT_FILE="/etc/mtproxy/server.crt"
ISSUER_FILE="/etc/mtproxy/ca.crt"
OCSP_FILE="/etc/mtproxy/ocsp_response.der"

openssl ocsp -issuer $ISSUER_FILE \
  -cert $CERT_FILE \
  -url http://ocsp.example.com \
  -respout $OCSP_FILE
```

## Session Management

### Session cache

```ini
[tls]
session_cache_size = 10000
session_timeout = 300  # 5 минут
```

### Session tickets

```ini
[tls]
session_tickets = true
ticket_lifetime = 86400  # 24 часа

# Ключ для шифрования tickets (32 байта)
ticket_key_file = /etc/mtproxy/ticket_key.bin
```

### Генерация ticket key

```bash
openssl rand -out /etc/mtproxy/ticket_key.bin 32
chmod 600 /etc/mtproxy/ticket_key.bin
```

## Мониторинг

### Prometheus метрики

```promql
# TLS handshake
mtproxy_tls_handshakes_total{version="TLS1.3"}

# Cipher suites
mtproxy_tls_cipher_suite{suite="TLS_AES_256_GCM_SHA384"}

# Session resumption
mtproxy_tls_session_resumptions_total{type="0rtt"}

# Certificate expiry
mtproxy_tls_cert_expiry_days

# OCSP status
mtproxy_tls_ocsp_stapling_status
```

### Логирование TLS событий

```ini
[logging]
tls_log_level = info
tls_log_file = /var/log/mtproxy/tls.log

# Логировать handshake ошибки
log_handshake_errors = true

# Логировать истечение сертификатов
log_cert_expiry = true
```

## Troubleshooting

### Ошибка: "no ciphers available"

**Решение:** Проверьте cipher suites:

```bash
openssl ciphers -v -s 'TLSv1.3'
```

### Ошибка: "certificate verify failed"

**Решение:**
1. Проверьте цепочку сертификатов
2. Убедитесь что CA сертификат правильный
3. Проверьте дату истечения

```bash
openssl verify -CAfile ca.crt server.crt
```

### 0-RTT не работает

**Решение:**
1. Проверьте что `early_data = true`
2. Сервер должен поддерживать 0-RTT
3. Session ticket должен быть валидным

### OCSP stapling не работает

**Решение:**
1. Проверьте что OCSP responder доступен
2. Обновите OCSP ответ
3. Проверьте цепочку сертификатов

```bash
openssl ocsp -issuer ca.crt -cert server.crt \
  -url http://ocsp.example.com -text
```

## Безопасность

### Рекомендации

1. **Используйте только TLS 1.3**
   ```ini
   min_version = TLS1.3
   max_version = TLS1.3
   ```

2. **Включите Perfect Forward Secrecy**
   - TLS 1.3 всегда использует PFS

3. **Используйте современные cipher suites**
   ```ini
   cipher_suites = TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256
   ```

4. **Включите OCSP stapling**
   ```ini
   ocsp_stapling = true
   ```

5. **Настройте certificate pinning**
   ```ini
   pin_type = pubkey
   pin_sha256 = your+hash+here==
   ```

6. **Мониторьте истечение сертификатов**
   ```bash
   # Проверка даты истечения
   openssl x509 -in server.crt -noout -dates
   ```

## Производительность

### Бенчмарки

| Метрика | TLS 1.2 | TLS 1.3 | Улучшение |
|---------|---------|---------|-----------|
| Handshake (full) | 80ms | 40ms | -50% |
| Handshake (resume) | 40ms | 0ms | -100% (0-RTT) |
| RTT для handshake | 2 | 1 | -50% |

### Оптимизация

```ini
# Включите session cache для быстрого возобновления
session_cache_size = 10000

# Включите session tickets
session_tickets = true

# Включите 0-RTT для ещё более быстрого подключения
early_data = true
```

## API Reference

### Функции

| Функция | Описание |
|---------|----------|
| `tls_context_create()` | Создание TLS контекста |
| `tls_context_destroy()` | Уничтожение контекста |
| `tls_connect()` | Подключение клиента |
| `tls_accept()` | Принятие подключения сервера |
| `tls_handshake()` | Выполнение handshake |
| `tls_read()` | Чтение данных |
| `tls_write()` | Запись данных |
| `tls_close()` | Закрытие соединения |
| `tls_get_peer_cert()` | Получение сертификата пира |
| `tls_verify_cert()` | Проверка сертификата |
| `tls_session_save()` | Сохранение сессии |
| `tls_session_load()` | Загрузка сессии |

---

*Версия: 1.0.0*  
*Последнее обновление: 20 марта 2026 г.*
