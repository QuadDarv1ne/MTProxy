#!/bin/bash

# MTProxy Secrets Update Script
# This script automatically updates Telegram secrets and configuration
# Run this script daily via cron for best results

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
CONFIG_DIR="${CONFIG_DIR:-$PROJECT_DIR/config}"
BACKUP_DIR="${BACKUP_DIR:-$PROJECT_DIR/backups}"
LOG_FILE="${LOG_FILE:-$PROJECT_DIR/logs/update-secrets.log}"

# URLs
SECRET_URL="https://core.telegram.org/getProxySecret"
CONFIG_URL="https://core.telegram.org/getProxyConfig"

# Files
SECRET_FILE="$CONFIG_DIR/proxy-secret"
CONFIG_FILE="$CONFIG_DIR/proxy-multi.conf"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging function
log() {
    local level=$1
    shift
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_FILE"
}

# Error handling
error_exit() {
    log "ERROR" "$1"
    exit 1
}

# Create necessary directories
mkdir -p "$CONFIG_DIR" "$BACKUP_DIR" "$(dirname "$LOG_FILE")"

log "INFO" "Starting secrets update process"

# Backup existing files
if [ -f "$SECRET_FILE" ]; then
    BACKUP_SECRET="$BACKUP_DIR/proxy-secret.$(date +%Y%m%d_%H%M%S)"
    cp "$SECRET_FILE" "$BACKUP_SECRET"
    log "INFO" "Backed up proxy-secret to $BACKUP_SECRET"
fi

if [ -f "$CONFIG_FILE" ]; then
    BACKUP_CONFIG="$BACKUP_DIR/proxy-multi.conf.$(date +%Y%m%d_%H%M%S)"
    cp "$CONFIG_FILE" "$BACKUP_CONFIG"
    log "INFO" "Backed up proxy-multi.conf to $BACKUP_CONFIG"
fi

# Download new secret
log "INFO" "Downloading new proxy-secret from $SECRET_URL"
if curl -f -s -S "$SECRET_URL" -o "$SECRET_FILE.tmp"; then
    # Verify file is not empty
    if [ -s "$SECRET_FILE.tmp" ]; then
        mv "$SECRET_FILE.tmp" "$SECRET_FILE"
        chmod 600 "$SECRET_FILE"
        log "INFO" "Successfully updated proxy-secret"
    else
        rm -f "$SECRET_FILE.tmp"
        error_exit "Downloaded proxy-secret is empty"
    fi
else
    rm -f "$SECRET_FILE.tmp"
    error_exit "Failed to download proxy-secret"
fi

# Download new config
log "INFO" "Downloading new proxy-multi.conf from $CONFIG_URL"
if curl -f -s -S "$CONFIG_URL" -o "$CONFIG_FILE.tmp"; then
    # Verify file is not empty
    if [ -s "$CONFIG_FILE.tmp" ]; then
        mv "$CONFIG_FILE.tmp" "$CONFIG_FILE"
        chmod 600 "$CONFIG_FILE"
        log "INFO" "Successfully updated proxy-multi.conf"
    else
        rm -f "$CONFIG_FILE.tmp"
        error_exit "Downloaded proxy-multi.conf is empty"
    fi
else
    rm -f "$CONFIG_FILE.tmp"
    error_exit "Failed to download proxy-multi.conf"
fi

# Cleanup old backups (keep last 7 days)
log "INFO" "Cleaning up old backups (keeping last 7 days)"
find "$BACKUP_DIR" -name "proxy-secret.*" -mtime +7 -delete 2>/dev/null || true
find "$BACKUP_DIR" -name "proxy-multi.conf.*" -mtime +7 -delete 2>/dev/null || true

# Restart service if systemd service exists
if systemctl is-active --quiet MTProxy.service 2>/dev/null; then
    log "INFO" "Restarting MTProxy service"
    if systemctl restart MTProxy.service; then
        log "INFO" "MTProxy service restarted successfully"
    else
        error_exit "Failed to restart MTProxy service"
    fi
elif command -v docker-compose &> /dev/null && [ -f "$PROJECT_DIR/docker-compose.yml" ]; then
    log "INFO" "Restarting Docker container"
    cd "$PROJECT_DIR"
    if docker-compose restart; then
        log "INFO" "Docker container restarted successfully"
    else
        error_exit "Failed to restart Docker container"
    fi
else
    log "WARNING" "No service manager found. Please restart MTProxy manually."
fi

log "INFO" "Secrets update completed successfully"

# Print summary
echo -e "${GREEN}✓ Secrets updated successfully${NC}"
echo -e "  Secret file: $SECRET_FILE"
echo -e "  Config file: $CONFIG_FILE"
echo -e "  Backups: $BACKUP_DIR"
echo -e "  Log file: $LOG_FILE"
