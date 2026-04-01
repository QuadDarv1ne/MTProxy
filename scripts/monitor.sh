#!/bin/bash
# MTProxy Monitoring Script
# Скрипт для мониторинга состояния MTProxy сервера

set -e

# Конфигурация
MTProxy_HOST="${MTProxy_HOST:-127.0.0.1}"
MTProxy_PORT="${MTProxy_PORT:-8888}"
MTProxy_ADMIN_PORT="${MTProxy_ADMIN_PORT:-8080}"
LOG_FILE="${LOG_FILE:-/var/log/mtproxy-monitor.log}"
ALERT_EMAIL="${ALERT_EMAIL:-}"
CHECK_INTERVAL="${CHECK_INTERVAL:-60}"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Логирование
log() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "$LOG_FILE"
}

log_info() {
    echo -e "${GREEN}$(date '+%Y-%m-%d %H:%M:%S') [INFO] $1${NC}"
}

log_warn() {
    echo -e "${YELLOW}$(date '+%Y-%m-%d %H:%M:%S') [WARN] $1${NC}"
}

log_error() {
    echo -e "${RED}$(date '+%Y-%m-%d %H:%M:%S') [ERROR] $1${NC}"
}

# Проверка доступности порта
check_port() {
    local host="$1"
    local port="$2"
    
    if command -v nc &> /dev/null; then
        nc -z "$host" "$port" &> /dev/null
        return $?
    elif command -v bash &> /dev/null; then
        (echo > /dev/tcp/"$host"/"$port") &> /dev/null
        return $?
    else
        # Попытка через curl
        curl -s --connect-timeout 2 "http://$host:$port/" &> /dev/null
        return $?
    fi
}

# Проверка процесса MTProxy
check_process() {
    if pgrep -f "mtproto-proxy" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Получение статистики через API
get_stats() {
    local response
    response=$(curl -s --connect-timeout 5 "http://$MTProxy_HOST:$MTProxy_PORT/stats" 2>/dev/null)
    echo "$response"
}

# Проверка использования памяти
check_memory() {
    local pid=$(pgrep -f "mtproto-proxy" | head -1)
    if [ -n "$pid" ]; then
        local mem=$(ps -o rss= -p "$pid" 2>/dev/null | tr -d ' ')
        if [ -n "$mem" ]; then
            echo $((mem / 1024))  # MB
        else
            echo "N/A"
        fi
    else
        echo "N/A"
    fi
}

# Проверка использования CPU
check_cpu() {
    local pid=$(pgrep -f "mtproto-proxy" | head -1)
    if [ -n "$pid" ]; then
        local cpu=$(ps -o %cpu= -p "$pid" 2>/dev/null | tr -d ' ')
        if [ -n "$cpu" ]; then
            echo "$cpu"
        else
            echo "N/A"
        fi
    else
        echo "N/A"
    fi
}

# Проверка количества соединений
check_connections() {
    local pid=$(pgrep -f "mtproto-proxy" | head -1)
    if [ -n "$pid" ]; then
        local conns=$(ss -tnp 2>/dev/null | grep "mtproto-proxy" | wc -l)
        echo "$conns"
    else
        echo "0"
    fi
}

# Отправка алерта
send_alert() {
    local subject="$1"
    local message="$2"
    
    log_error "ALERT: $subject - $message"
    
    if [ -n "$ALERT_EMAIL" ] && command -v mail &> /dev/null; then
        echo "$message" | mail -s "[MTProxy Alert] $subject" "$ALERT_EMAIL"
    fi
    
    # Можно добавить другие способы оповещения (Telegram, Slack, etc.)
}

# Основная проверка
perform_check() {
    local status="OK"
    local issues=()
    
    # Проверка процесса
    if ! check_process; then
        status="CRITICAL"
        issues+=("Process not running")
    fi
    
    # Проверка порта статистики
    if ! check_port "$MTProxy_HOST" "$MTProxy_PORT"; then
        status="WARNING"
        issues+=("Stats port $MTProxy_PORT not accessible")
    fi
    
    # Проверка памяти
    local mem_usage=$(check_memory)
    if [ "$mem_usage" != "N/A" ] && [ "$mem_usage" -gt 1024 ]; then
        if [ "$status" == "OK" ]; then
            status="WARNING"
        fi
        issues+=("High memory usage: ${mem_usage}MB")
    fi
    
    # Проверка CPU
    local cpu_usage=$(check_cpu)
    if [ "$cpu_usage" != "N/A" ]; then
        local cpu_int=${cpu_usage%.*}
        if [ "$cpu_int" -gt 80 ]; then
            if [ "$status" == "OK" ]; then
                status="WARNING"
            fi
            issues+=("High CPU usage: ${cpu_usage}%")
        fi
    fi
    
    # Проверка соединений
    local conns=$(check_connections)
    if [ "$conns" -gt 5000 ]; then
        if [ "$status" == "OK" ]; then
            status="WARNING"
        fi
        issues+=("High connection count: $conns")
    fi
    
    # Вывод результата
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    if [ "$status" == "OK" ]; then
        log_info "Status: OK | Memory: ${mem_usage}MB | CPU: ${cpu_usage}% | Connections: $conns"
    elif [ "$status" == "WARNING" ]; then
        log_warn "Status: WARNING | Issues: ${issues[*]}"
        send_alert "Warning" "${issues[*]}"
    else
        log_error "Status: CRITICAL | Issues: ${issues[*]}"
        send_alert "Critical" "${issues[*]}"
    fi
    
    return 0
}

# Показ текущего статуса
show_status() {
    echo "=================================="
    echo "  MTProxy Server Status"
    echo "=================================="
    echo ""
    
    # Процесс
    if check_process; then
        echo -e "Process: ${GREEN}Running${NC}"
        local pid=$(pgrep -f "mtproto-proxy" | head -1)
        echo "  PID: $pid"
    else
        echo -e "Process: ${RED}Not Running${NC}"
    fi
    echo ""
    
    # Память
    local mem=$(check_memory)
    echo "Memory Usage: ${mem} MB"
    
    # CPU
    local cpu=$(check_cpu)
    echo "CPU Usage: ${cpu}%"
    
    # Соединения
    local conns=$(check_connections)
    echo "Active Connections: $conns"
    
    # Порты
    echo ""
    echo "Ports:"
    if check_port "$MTProxy_HOST" "$MTProxy_PORT"; then
        echo -e "  Stats Port ($MTProxy_PORT): ${GREEN}Open${NC}"
    else
        echo -e "  Stats Port ($MTProxy_PORT): ${RED}Closed${NC}"
    fi
    
    # Статистика из API
    echo ""
    echo "Statistics from API:"
    local stats=$(get_stats)
    if [ -n "$stats" ]; then
        echo "$stats" | head -20
    else
        echo "  (Could not retrieve stats)"
    fi
    
    echo ""
    echo "=================================="
}

# Непрерывный мониторинг
monitor_loop() {
    log_info "Starting continuous monitoring (interval: ${CHECK_INTERVAL}s)"
    
    while true; do
        perform_check
        sleep "$CHECK_INTERVAL"
    done
}

# Показ помощи
show_help() {
    echo "MTProxy Monitoring Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  status      Show current server status"
    echo "  check       Perform single health check"
    echo "  monitor     Start continuous monitoring"
    echo "  connections Show active connections"
    echo "  help        Show this help message"
    echo ""
    echo "Environment Variables:"
    echo "  MTProxy_HOST       Server host (default: 127.0.0.1)"
    echo "  MTProxy_PORT       Stats port (default: 8888)"
    echo "  CHECK_INTERVAL     Monitor interval in seconds (default: 60)"
    echo "  ALERT_EMAIL        Email for alerts (optional)"
    echo "  LOG_FILE           Log file path (default: /var/log/mtproxy-monitor.log)"
    echo ""
}

# Показ соединений
show_connections() {
    echo "Active MTProxy Connections:"
    echo ""
    
    if command -v ss &> /dev/null; then
        ss -tnp | grep "mtproto-proxy" | awk '{print $5, $6}'
    elif command -v netstat &> /dev/null; then
        netstat -tnp | grep "mtproto-proxy" | awk '{print $5, $6}'
    else
        echo "Neither ss nor netstat available"
    fi
}

# Главная
case "${1:-status}" in
    status)
        show_status
        ;;
    check)
        perform_check
        exit $?
        ;;
    monitor)
        monitor_loop
        ;;
    connections)
        show_connections
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
