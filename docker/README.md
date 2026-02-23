# Docker Quick Start Guide

This guide will help you quickly deploy MTProxy using Docker.

## Prerequisites

- Docker installed ([Install Docker](https://docs.docker.com/get-docker/))
- Docker Compose installed ([Install Docker Compose](https://docs.docker.com/compose/install/))

## Quick Start (5 minutes)

### 1. Generate Secret Key

```bash
# On Linux/macOS
head -c 16 /dev/urandom | xxd -ps

# On Windows (PowerShell)
$bytes = New-Object byte[] 16; (New-Object Random).NextBytes($bytes); [BitConverter]::ToString($bytes).Replace("-","").ToLower()
```

Copy the generated secret key.

### 2. Create Environment File

```bash
# Copy example environment file
cp .env.example .env

# Edit .env and set your secret
nano .env  # or use any text editor
```

Set `MTPROXY_SECRET` to your generated secret key.

### 3. Build and Run

```bash
# Build the image
docker-compose build

# Start the service
docker-compose up -d

# Check logs
docker-compose logs -f
```

### 4. Verify It's Running

```bash
# Check container status
docker-compose ps

# Get statistics
curl http://localhost:8888/stats
```

### 5. Get Connection Link

Your MTProxy connection link:
```
tg://proxy?server=YOUR_SERVER_IP&port=443&secret=YOUR_SECRET
```

Replace:
- `YOUR_SERVER_IP` with your server's public IP
- `YOUR_SECRET` with your generated secret

## Configuration

### Environment Variables

Edit `.env` file:

```bash
# Required: Your secret key
MTPROXY_SECRET=your_generated_secret_here

# Optional: Proxy tag from @MTProxybot
MTPROXY_TAG=

# Optional: Number of workers (default: 1)
WORKERS=2
```

### Custom Ports

Edit `docker-compose.yml` to change ports:

```yaml
ports:
  - "8443:443"      # Change 8443 to your desired port
  - "127.0.0.1:8888:8888"
```

### Resource Limits

Adjust in `docker-compose.yml`:

```yaml
deploy:
  resources:
    limits:
      cpus: '4'      # Increase for better performance
      memory: 2G     # Increase for high load
```

## Management Commands

### Start/Stop

```bash
# Start
docker-compose up -d

# Stop
docker-compose down

# Restart
docker-compose restart
```

### View Logs

```bash
# Follow logs
docker-compose logs -f

# Last 100 lines
docker-compose logs --tail=100

# Specific service
docker-compose logs mtproxy
```

### Update Configuration

```bash
# Update Telegram secrets
docker-compose exec mtproxy curl -s https://core.telegram.org/getProxySecret -o /opt/mtproxy/config/proxy-secret
docker-compose exec mtproxy curl -s https://core.telegram.org/getProxyConfig -o /opt/mtproxy/config/proxy-multi.conf

# Restart to apply
docker-compose restart
```

### Statistics

```bash
# Get current statistics
curl http://localhost:8888/stats

# Pretty print with jq (if installed)
curl -s http://localhost:8888/stats | jq .
```

## Troubleshooting

### Container Won't Start

```bash
# Check logs
docker-compose logs

# Check if port is already in use
netstat -tulpn | grep 443

# Try different port in docker-compose.yml
```

### Can't Connect to Proxy

1. Check firewall:
   ```bash
   # Allow port 443
   sudo ufw allow 443/tcp
   ```

2. Verify container is running:
   ```bash
   docker-compose ps
   ```

3. Check logs for errors:
   ```bash
   docker-compose logs --tail=50
   ```

### High Memory Usage

Reduce workers in `.env`:
```bash
WORKERS=1
```

Or adjust memory limits in `docker-compose.yml`.

### Update Secrets

```bash
# Enter container
docker-compose exec mtproxy sh

# Download new secrets
curl -s https://core.telegram.org/getProxySecret -o /opt/mtproxy/config/proxy-secret
curl -s https://core.telegram.org/getProxyConfig -o /opt/mtproxy/config/proxy-multi.conf

# Exit and restart
exit
docker-compose restart
```

## Advanced Usage

### Multiple Secrets

Edit `docker-compose.yml` command section:

```yaml
command: >
  -u mtproxy
  -p 8888
  -H 443
  -S ${MTPROXY_SECRET}
  -S ${MTPROXY_SECRET_2}
  -S ${MTPROXY_SECRET_3}
  --aes-pwd /opt/mtproxy/config/proxy-secret
  /opt/mtproxy/config/proxy-multi.conf
  -M ${WORKERS:-1}
```

Add to `.env`:
```bash
MTPROXY_SECRET_2=second_secret_here
MTPROXY_SECRET_3=third_secret_here
```

### Custom Network

```yaml
networks:
  mtproxy_net:
    driver: bridge
    ipam:
      config:
        - subnet: 172.25.0.0/16
```

### Persistent Logs

Create logs directory:
```bash
mkdir -p logs
chmod 777 logs
```

Already configured in `docker-compose.yml`:
```yaml
volumes:
  - ./logs:/opt/mtproxy/logs
```

### Health Monitoring

Check health status:
```bash
docker inspect --format='{{.State.Health.Status}}' mtproxy
```

### Backup Configuration

```bash
# Backup secrets
docker-compose exec mtproxy cat /opt/mtproxy/config/proxy-secret > backup-proxy-secret
docker-compose exec mtproxy cat /opt/mtproxy/config/proxy-multi.conf > backup-proxy-multi.conf

# Backup .env
cp .env .env.backup
```

## Production Deployment

### Security Checklist

- [ ] Use strong, unique secret keys
- [ ] Restrict statistics port to localhost only
- [ ] Enable firewall rules
- [ ] Use non-root user (already configured)
- [ ] Enable resource limits
- [ ] Set up log rotation
- [ ] Regular secret updates (daily)
- [ ] Monitor container health

### Monitoring

Set up monitoring with Prometheus/Grafana:

```yaml
# Add to docker-compose.yml
  prometheus:
    image: prom/prometheus
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"
```

### Auto-restart

Already configured in `docker-compose.yml`:
```yaml
restart: unless-stopped
```

### Log Rotation

Configure in `docker-compose.yml`:
```yaml
logging:
  driver: "json-file"
  options:
    max-size: "10m"
    max-file: "3"
```

## Getting Help

- **Documentation:** See [README.md](README.md) and [README_EN.md](README_EN.md)
- **Issues:** [GitHub Issues](https://github.com/TelegramMessenger/MTProxy/issues)
- **Telegram:** [@quadd4rv1n7](https://t.me/quadd4rv1n7)

## Next Steps

1. Register your proxy with [@MTProxybot](https://t.me/MTProxybot)
2. Get a proxy tag and add it to `.env`
3. Set up monitoring and alerts
4. Configure automatic secret updates
5. Review security settings

---

Happy proxying! 🚀
