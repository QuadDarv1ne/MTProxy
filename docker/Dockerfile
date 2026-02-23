# Multi-stage build for MTProxy
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libssl-dev \
    zlib1g-dev \
    cmake \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build using CMake
RUN mkdir -p build && cd build && \
    cmake .. && \
    cmake --build . --parallel $(nproc) && \
    strip bin/mtproto-proxy

# Runtime stage
FROM debian:bookworm-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -r -s /bin/false mtproxy

# Set working directory
WORKDIR /opt/mtproxy

# Copy binary from builder
COPY --from=builder /build/build/bin/mtproto-proxy /opt/mtproxy/

# Create directories for secrets and configs
RUN mkdir -p /opt/mtproxy/config && \
    chown -R mtproxy:mtproxy /opt/mtproxy

# Download Telegram secrets and config
RUN curl -s https://core.telegram.org/getProxySecret -o /opt/mtproxy/config/proxy-secret && \
    curl -s https://core.telegram.org/getProxyConfig -o /opt/mtproxy/config/proxy-multi.conf && \
    chown mtproxy:mtproxy /opt/mtproxy/config/*

# Expose ports
# 443 - MTProxy port
# 8888 - Statistics port (should be restricted in production)
EXPOSE 443 8888

# Switch to non-root user
USER mtproxy

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8888/stats || exit 1

# Set entrypoint
ENTRYPOINT ["/opt/mtproxy/mtproto-proxy"]

# Default command (override with your own secret)
CMD ["-u", "mtproxy", "-p", "8888", "-H", "443", "-S", "CHANGE_ME_SECRET", "--aes-pwd", "/opt/mtproxy/config/proxy-secret", "/opt/mtproxy/config/proxy-multi.conf", "-M", "1"]
