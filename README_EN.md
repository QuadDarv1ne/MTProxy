# MTProxy

**MTProxy** is a high-performance proxy server for the `MTProto` protocol used by `Telegram` to ensure secure communication between clients and Telegram servers.

`MTProxy` allows bypassing blocks and filtering, providing fast and reliable access to Telegram services.

## Features

- ⚡ High performance and low latency
- 🔒 Full compatibility with `MTProto` protocol
- 🛡️ Protection against blocking through random padding
- 👥 Support for multiple secret keys
- 📊 Integrated statistics system
- 🔄 Automatic configuration updates
- 🚀 Optimized architecture with modular structure
- 🔐 Advanced security features
- ⚙️ Enhanced connection and thread management
- 🚀 Adaptive performance optimization system

## Quick Start

### Prerequisites

**Debian/Ubuntu:**
```bash
apt update
apt install git curl build-essential libssl-dev zlib1g-dev cmake
```

**CentOS/RHEL/Fedora:**
```bash
# For CentOS/RHEL
yum install openssl-devel zlib-devel cmake
yum groupinstall "Development Tools"

# For Fedora
dnf install openssl-devel zlib-devel make automake gcc gcc-c++ cmake
```

### Building

**Clone the repository:**
```bash
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy
```

**Using Make (original method):**
```bash
make && cd objs/bin
```

**Using CMake (new method):**
```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
# Executable will be created in build/bin/
```

For more details on building with CMake, see [build-scripts/README.md](build-scripts/README.md).

## Running

1. **Get the secret** used to connect to Telegram servers:
```bash
curl -s https://core.telegram.org/getProxySecret -o proxy-secret
```

2. **Get current Telegram configuration** (recommended to update daily):
```bash
curl -s https://core.telegram.org/getProxyConfig -o proxy-multi.conf
```

3. **Generate a secret** for clients to connect to your proxy:
```bash
head -c 16 /dev/urandom | xxd -ps
```

4. **Run mtproto-proxy:**
```bash
./mtproto-proxy -u nobody -p 8888 -H 443 -S <your_secret> --aes-pwd proxy-secret proxy-multi.conf -M 1
```

### Launch Parameters

- `-u nobody` — username to run the process (recommended to use a separate user)
- `-p 8888` — local port for statistics (accessible only via localhost)
- `-H 443` — port that clients will use to connect to the proxy
- `-S <secret>` — secret key generated in step 3 (multiple keys supported: `-S key1 -S key2`)
- `--aes-pwd proxy-secret` — path to Telegram secret file
- `proxy-multi.conf` — path to Telegram configuration file
- `-M 1` — number of worker processes (increase for high load)

See other parameters using `mtproto-proxy --help`

### Getting Statistics

You can get proxy statistics by running:
```bash
wget localhost:8888/stats
```

> ⚠️ Statistics are only accessible from localhost

## Random Padding Mode

Some providers may block MTProxy by packet size. To bypass such blocks, enable random padding mode.

**How to enable:**
- Add prefix `dd` to your secret key
- Example: `cafebabe12345678` → `ddcafebabe12345678`
- Clients using such a key will receive additional random padding

## Installing as a Service (Systemd)

1. **Create systemd service file:**
```bash
sudo nano /etc/systemd/system/MTProxy.service
```

2. **Add configuration** (adjust paths and parameters for your system):
```ini
[Unit]
Description=MTProxy Service
After=network.target

[Service]
Type=simple
User=nobody
WorkingDirectory=/opt/MTProxy
ExecStart=/opt/MTProxy/mtproto-proxy -u nobody -p 8888 -H 443 -S <your_secret> --aes-pwd proxy-secret proxy-multi.conf -M 1
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

3. **Reload systemd configuration:**
```bash
sudo systemctl daemon-reload
```

4. **Start and check the service:**
```bash
sudo systemctl start MTProxy.service
sudo systemctl status MTProxy.service
```

5. **Enable autostart after reboot:**
```bash
sudo systemctl enable MTProxy.service
```

## Advanced Features

### 📊 Monitoring System
- Extended metrics and logging system
- Alerting by thresholds (CPU, memory, connections)
- Component statistics and response time
- Data export for external systems
- Logging levels (ERROR, WARNING, INFO, DEBUG)

### 🔐 Security Features
- Certificate pinning protection
- DDoS attack mitigation
- Rate limiting and filtering
- HSM integration support (planned)

### ⚡ Performance Optimization
- NUMA optimization
- io_uring for asynchronous I/O
- DPDK for high-performance networking
- Automatic tuning under current load
- 4 optimization levels from BASIC to MAXIMUM

### 🌐 Protocol Support
- MTProto v3 with Perfect Forward Secrecy
- WebSocket and WSS support
- Shadowsocks integration
- Pluggable transports

For detailed documentation, see the [docs](docs/) directory.

## Documentation

- [Advanced Logging (RU)](docs/ADVANCED_LOGGING_RU.md)
- [Crypto Optimizations (RU)](docs/CRYPTO_OPTIMIZATIONS_RU.md)
- [Memory Optimization (RU)](docs/MEMORY_OPTIMIZATION_RU.md)
- [Modular Architecture (RU)](docs/MODULAR_ARCHITECTURE_RU.md)
- [Performance Optimizations (RU)](docs/PERFORMANCE_OPTIMIZATIONS_RU.md)

## Security

- Use a separate user to run the proxy (not root)
- Restrict access to the statistics port (usually 8888) to localhost only
- Regularly update configuration from Telegram
- Store secret files in a secure location

## Troubleshooting

- If connection issues occur, check firewall settings
- Ensure used ports are open for external connections
- Check access rights to `proxy-secret` and `proxy-multi.conf` files
- For debugging, use system logs and statistics command output

## License

**The project is distributed under a dual license:**
- Main application: GNU General Public License version 2 (GPLv2)
- Library components: GNU Lesser General Public License version 2.1 (LGPLv2.1)

See [licenses/](licenses/) directory for details.

---

### 💼 Author

**Dupley Maxim Igorevich**

📲 **Telegram №1:** [@quadd4rv1n7](https://t.me/quadd4rv1n7)  
📲 **Telegram №2:** [@dupley_maxim_1999](https://t.me/dupley_maxim_1999)  
🌐 **Website:** [school-maestro7it.ru](https://school-maestro7it.ru/)

📅 **Date:** 12.02.2026
