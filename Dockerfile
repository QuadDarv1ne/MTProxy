# MTProxy Docker Image
# Официальный образ для MTProxy
# Поддержка multi-arch: linux/amd64, linux/arm64, linux/arm/v7

ARG ALPINE_VERSION=3.19
ARG OPENSSL_VERSION=3

# =============================================================================
# Stage 1: Builder
# =============================================================================
FROM alpine:${ALPINE_VERSION} AS builder

ARG BUILD_TYPE=Release
ARG ENABLE_LTO=ON
ARG ENABLE_PGO=OFF

# Установка зависимостей для сборки
RUN apk add --no-cache \
    build-base \
    cmake \
    make \
    git \
    openssl-dev \
    zlib-dev \
    linux-headers \
    curl \
    binutils-gold

WORKDIR /build

# Копирование исходников
COPY . .

# Оптимизация сборки
RUN set -eux; \
    export CFLAGS="-O3 -march=native -mtune=native"; \
    export LDFLAGS="-Wl,--as-needed -Wl,--strip-all"; \
    cmake -B build \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DCMAKE_INSTALL_PREFIX=/install \
        -DENABLE_LTO=${ENABLE_LTO} \
        -DENABLE_PGO=${ENABLE_PGO} \
        -DBUILD_SHARED_LIB=ON \
        -DSTATIC_LINKING=OFF; \
    cmake --build build -j$(nproc); \
    cmake --install build --prefix /install; \
    # Strip бинарников для уменьшения размера \
    strip /install/bin/mtproto-proxy 2>/dev/null || true; \
    strip /install/bin/mtproxy-admin 2>/dev/null || true

# =============================================================================
# Stage 2: Runtime (Minimal)
# =============================================================================
FROM alpine:${ALPINE_VERSION} AS runtime

ARG OPENSSL_VERSION=3

# Установка runtime зависимостей
RUN apk add --no-cache \
    openssl-libs \
    zlib \
    ca-certificates \
    tzdata \
    && addgroup -g 1000 -S mtproxy \
    && adduser -u 1000 -S -G mtproxy -D mtproxy \
    && mkdir -p /var/lib/mtproxy /var/log/mtproxy /etc/mtproxy \
    && chown -R mtproxy:mtproxy /var/lib/mtproxy /var/log/mtproxy /etc/mtproxy

# Копирование бинарников из builder
COPY --from=builder /install/bin/mtproto-proxy /usr/local/bin/
COPY --from=builder /install/bin/mtproxy-admin /usr/local/bin/
COPY --from=builder /install/lib/*.so* /usr/local/lib/ 2>/dev/null || true

# Копирование конфигурации по умолчанию
COPY --from=builder /install/etc/mtproxy/* /etc/mtproxy/ 2>/dev/null || true

# Настройка LD_LIBRARY_PATH
ENV LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}

WORKDIR /var/lib/mtproxy

# Переключение на не-root пользователя
USER mtproxy

# Порты по умолчанию
EXPOSE 8080 443 3128 8888

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD wget -q --spider http://localhost:8888/stats || exit 1

# Метки для OCI образов
LABEL org.opencontainers.image.title="MTProxy" \
      org.opencontainers.image.description="High-performance MTProto proxy server" \
      org.opencontainers.image.vendor="MTProxy Team" \
      org.opencontainers.image.licenses="Apache-2.0"

# Запуск прокси
ENTRYPOINT ["mtproto-proxy"]
CMD ["--help"]

# =============================================================================
# Stage 3: Debug (опционально)
# =============================================================================
FROM alpine:${ALPINE_VERSION} AS debug

# Установка зависимостей для отладки
RUN apk add --no-cache \
    openssl-libs \
    zlib \
    ca-certificates \
    tzdata \
    strace \
    ltrace \
    gdb \
    valgrind \
    && addgroup -g 1000 -S mtproxy \
    && adduser -u 1000 -S -G mtproxy -D mtproxy

# Копирование бинарников из builder (с debug символами)
COPY --from=builder /install/bin/mtproto-proxy /usr/local/bin/
COPY --from=builder /install/bin/mtproxy-admin /usr/local/bin/
COPY --from=builder /install/lib/*.so* /usr/local/lib/ 2>/dev/null || true

WORKDIR /var/lib/mtproxy

USER mtproxy

EXPOSE 8080 443 3128 8888

ENTRYPOINT ["mtproto-proxy"]
CMD ["--help"]
