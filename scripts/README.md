# MTProxy Scripts

This directory contains utility scripts for managing MTProxy.

## Available Scripts

### update-secrets.sh / update-secrets.bat

Automatically updates Telegram secrets and configuration files.

**Features:**
- Downloads latest proxy-secret and proxy-multi.conf from Telegram
- Creates backups before updating
- Automatically restarts MTProxy service or Docker container
- Cleans up old backups (keeps last 7 days)
- Logs all operations

**Usage:**

Linux/macOS:
```bash
# Make executable
chmod +x scripts/update-secrets.sh

# Run manually
./scripts/update-secrets.sh

# Or with custom paths
CONFIG_DIR=/custom/path ./scripts/update-secrets.sh
```

Windows:
```cmd
scripts\update-secrets.bat
```

**Automated Updates (Linux/macOS):**

Add to crontab for daily updates:
```bash
# Edit crontab
crontab -e

# Add this line to run daily at 3 AM
0 3 * * * /path/to/MTProxy/scripts/update-secrets.sh
```

**Automated Updates (Windows):**

Use Task Scheduler:
1. Open Task Scheduler
2. Create Basic Task
3. Set trigger: Daily at 3:00 AM
4. Action: Start a program
5. Program: `C:\path\to\MTProxy\scripts\update-secrets.bat`

**Environment Variables:**

- `CONFIG_DIR` - Directory for config files (default: `./config`)
- `BACKUP_DIR` - Directory for backups (default: `./backups`)
- `LOG_FILE` - Log file path (default: `./logs/update-secrets.log`)

**Example with custom paths:**
```bash
CONFIG_DIR=/opt/mtproxy/config \
BACKUP_DIR=/opt/mtproxy/backups \
LOG_FILE=/var/log/mtproxy-update.log \
./scripts/update-secrets.sh
```

## Logs

All scripts log their operations to `logs/` directory:
- `update-secrets.log` - Secret update operations

View logs:
```bash
# Linux/macOS
tail -f logs/update-secrets.log

# Windows
type logs\update-secrets.log
```

## Backups

Backups are stored in `backups/` directory with timestamps:
- `proxy-secret.YYYYMMDD_HHMMSS`
- `proxy-multi.conf.YYYYMMDD_HHMMSS`

Old backups (>7 days) are automatically cleaned up.

## Troubleshooting

### Script fails to download secrets

**Check internet connection:**
```bash
curl -I https://core.telegram.org/getProxySecret
```

**Check firewall:**
```bash
# Allow HTTPS
sudo ufw allow 443/tcp
```

### Permission denied

**Linux/macOS:**
```bash
chmod +x scripts/update-secrets.sh
```

**Windows:**
Run Command Prompt as Administrator

### Service not restarting

**Check service status:**
```bash
# Systemd
systemctl status MTProxy.service

# Docker
docker-compose ps
```

**Manual restart:**
```bash
# Systemd
sudo systemctl restart MTProxy.service

# Docker
docker-compose restart
```

## Contributing

To add new scripts:
1. Create script in `scripts/` directory
2. Add documentation to this README
3. Make executable (Linux/macOS): `chmod +x script.sh`
4. Test thoroughly before committing

## Security Notes

- Scripts create backups before making changes
- All config files have restricted permissions (600)
- Logs may contain sensitive information - protect accordingly
- Review scripts before running with elevated privileges

---

For more information, see the main [README.md](../README.md)
