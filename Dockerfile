# MTProxy Docker Image
# Официальный образ для MTProxy

FROM alpine:3.19 AS builder

# Установка зависимостей для сборки
RUN apk add --no-cache \
    build-base \
    cmake \
    make \
    git \
    openssl-dev \
    zlib-dev \
    linux-headers

WORKDIR /build

# Копирование исходников
COPY . .

# Сборка проекта
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc) && \
    cmake --install build --prefix /install

# Финальный образ
FROM alpine:3.19

# Установка runtime зависимостей
RUN apk add --no-cache \
    openssl-libs \
    zlib \
    && addgroup -g 1000 mtproxy \
    && adduser -u 1000 -G mtproxy -D mtproxy

# Копирование бинарников из builder
COPY --from=builder /install/bin/mtproto-proxy /usr/local/bin/
COPY --from=builder /install/bin/mtproxy-admin /usr/local/bin/

# Копирование конфигурации
COPY --from=builder /install/etc/mtproxy /etc/mtproxy

# Создание директорий для данных
RUN mkdir -p /var/lib/mtproxy /var/log/mtproxy && \
    chown -R mtproxy:mtproxy /var/lib/mtproxy /var/log/mtproxy

WORKDIR /var/lib/mtproxy

# Переключение на не-root пользователя
USER mtproxy

# Порты по умолчанию
EXPOSE 8080 443 3128

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD mtproto-proxy --health || exit 1

# Запуск прокси
ENTRYPOINT ["mtproto-proxy"]
CMD ["--help"]
