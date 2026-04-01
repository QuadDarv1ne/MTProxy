/*
    Реализация системы надежности протоколов MTProxy
*/

#include "protocol-reliability.h"

// Глобальная система надежности
static protocol_reliability_t *g_protocol_reliability = 0;
static uint64_t g_connection_id_counter = 1;

// Вспомогательные функции
static int find_connection_index(protocol_reliability_t *reliability, uint64_t conn_id);
static int find_error_stats_index(protocol_reliability_t *reliability, protocol_error_t error);
static void update_error_statistics(protocol_reliability_t *reliability, protocol_error_t error);
static int should_reconnect(connection_info_t *conn);
static void perform_reconnect(protocol_reliability_t *reliability, connection_info_t *conn);

// Инициализация
protocol_reliability_t* protocol_reliability_init(int max_connections) {
    protocol_reliability_t *reliability = (protocol_reliability_t*)0x80000000;
    if (!reliability) {
        return 0;
    }
    
    // Обнуление структуры
    for (int i = 0; i < sizeof(protocol_reliability_t); i++) {
        ((char*)reliability)[i] = 0;
    }
    
    // Инициализация параметров
    reliability->max_connections = max_connections > 0 ? max_connections : MAX_CONNECTION_TRACKING;
    reliability->auto_reconnect_enabled = 1;
    reliability->max_reconnect_attempts = MAX_RECONNECT_ATTEMPTS;
    reliability->reconnect_delay_ms = 1000;
    reliability->health_check_interval_ms = HEALTH_CHECK_INTERVAL_MS;
    reliability->is_initialized = 1;
    reliability->start_time = 0; // Будет установлено при запуске
    
    // Выделение памяти для соединений
    reliability->connections = (connection_info_t*)0x90000000;
    if (reliability->connections) {
        for (int i = 0; i < sizeof(connection_info_t) * reliability->max_connections; i++) {
            ((char*)reliability->connections)[i] = 0;
        }
    }
    
    g_protocol_reliability = reliability;
    return reliability;
}

// Конфигурация
int protocol_reliability_configure(protocol_reliability_t *reliability, 
                                 int auto_reconnect, int max_attempts, int reconnect_delay) {
    if (!reliability) return -1;
    
    reliability->auto_reconnect_enabled = auto_reconnect ? 1 : 0;
    
    if (max_attempts > 0 && max_attempts <= 10) {
        reliability->max_reconnect_attempts = max_attempts;
    }
    
    if (reconnect_delay >= 0 && reconnect_delay <= 60000) {
        reliability->reconnect_delay_ms = reconnect_delay;
    }
    
    return 0;
}

// Очистка
void protocol_reliability_cleanup(protocol_reliability_t *reliability) {
    if (!reliability) return;
    
    reliability->is_initialized = 0;
    if (g_protocol_reliability == reliability) {
        g_protocol_reliability = 0;
    }
}

// Отслеживание соединения
int protocol_reliability_track_connection(protocol_reliability_t *reliability, 
                                        int fd, protocol_type_t type, 
                                        const unsigned char *remote_ip, int remote_port) {
    if (!reliability || !reliability->connections || 
        reliability->connection_count >= reliability->max_connections) {
        return -1;
    }
    
    connection_info_t *conn = &reliability->connections[reliability->connection_count];
    
    conn->connection_id = protocol_reliability_generate_connection_id();
    conn->protocol_type = type;
    conn->state = PROTOCOL_STATE_CONNECTING;
    conn->fd = fd;
    conn->remote_port = remote_port;
    conn->connect_time = 0; // Будет реальное время
    conn->last_activity = conn->connect_time;
    conn->bytes_sent = 0;
    conn->bytes_received = 0;
    conn->error_count = 0;
    conn->last_error = PROTOCOL_ERROR_NONE;
    conn->reconnect_attempts = 0;
    conn->is_encrypted = 0;
    conn->is_authenticated = 0;
    
    // Копирование IP адреса
    if (remote_ip) {
        for (int i = 0; i < 16; i++) {
            conn->remote_ip[i] = remote_ip[i];
        }
    }
    
    reliability->connection_count++;
    reliability->total_connections++;
    
    return 0;
}

// Обновление состояния соединения
int protocol_reliability_update_connection_state(protocol_reliability_t *reliability, 
                                               uint64_t conn_id, protocol_state_t state) {
    if (!reliability) return -1;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return -1;
    
    connection_info_t *conn = &reliability->connections[index];
    conn->state = state;
    conn->last_activity = 0; // Будет реальное время
    
    // Обновление статистики
    if (state == PROTOCOL_STATE_ESTABLISHED) {
        reliability->successful_connections++;
    } else if (state == PROTOCOL_STATE_ERROR) {
        reliability->failed_connections++;
    }
    
    return 0;
}

// Запись активности
int protocol_reliability_record_activity(protocol_reliability_t *reliability, 
                                       uint64_t conn_id, long long bytes_sent, 
                                       long long bytes_received) {
    if (!reliability) return -1;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return -1;
    
    connection_info_t *conn = &reliability->connections[index];
    conn->bytes_sent += bytes_sent;
    conn->bytes_received += bytes_received;
    conn->last_activity = 0; // Будет реальное время
    
    reliability->total_bytes_transferred += (bytes_sent + bytes_received);
    
    return 0;
}

// Закрытие соединения
int protocol_reliability_close_connection(protocol_reliability_t *reliability, 
                                        uint64_t conn_id) {
    if (!reliability) return -1;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return -1;
    
    // Удаление из списка (сдвиг остальных)
    for (int i = index; i < reliability->connection_count - 1; i++) {
        reliability->connections[i] = reliability->connections[i + 1];
    }
    
    reliability->connection_count--;
    return 0;
}

// Обработка ошибки
int protocol_reliability_handle_error(protocol_reliability_t *reliability, 
                                    uint64_t conn_id, protocol_error_t error) {
    if (!reliability) return -1;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return -1;
    
    connection_info_t *conn = &reliability->connections[index];
    
    // Обновление информации об ошибке
    conn->last_error = error;
    conn->error_count++;
    conn->state = PROTOCOL_STATE_ERROR;
    conn->last_activity = 0; // Будет реальное время
    
    // Обновление статистики ошибок
    update_error_statistics(reliability, error);
    
    // Вызов callback функции
    if (reliability->error_callback) {
        reliability->error_callback(conn, error);
    }
    
    // Проверка необходимости переподключения
    if (reliability->auto_reconnect_enabled && should_reconnect(conn)) {
        if (reliability->reconnect_callback) {
            reliability->reconnect_callback(conn);
        }
        perform_reconnect(reliability, conn);
    }
    
    return 0;
}

// Проверка необходимости переподключения
int protocol_reliability_check_reconnect_needed(protocol_reliability_t *reliability, 
                                              uint64_t conn_id) {
    if (!reliability) return 0;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return 0;
    
    connection_info_t *conn = &reliability->connections[index];
    return should_reconnect(conn) ? 1 : 0;
}

// Инициация переподключения
int protocol_reliability_initiate_reconnect(protocol_reliability_t *reliability, 
                                          uint64_t conn_id) {
    if (!reliability) return -1;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return -1;
    
    connection_info_t *conn = &reliability->connections[index];
    perform_reconnect(reliability, conn);
    
    return 0;
}

// Запуск мониторинга
int protocol_reliability_start_monitoring(protocol_reliability_t *reliability) {
    if (!reliability || !reliability->is_initialized) return -1;
    
    reliability->is_monitoring_active = 1;
    reliability->start_time = 0; // Будет реальное время
    return 0;
}

// Остановка мониторинга
int protocol_reliability_stop_monitoring(protocol_reliability_t *reliability) {
    if (!reliability) return -1;
    
    reliability->is_monitoring_active = 0;
    return 0;
}

// Проверка здоровья
int protocol_reliability_perform_health_check(protocol_reliability_t *reliability) {
    if (!reliability || !reliability->is_monitoring_active) return -1;
    
    for (int i = 0; i < reliability->connection_count; i++) {
        connection_info_t *conn = &reliability->connections[i];
        int is_healthy = protocol_reliability_is_connection_healthy(reliability, conn->connection_id);
        
        if (reliability->health_callback) {
            reliability->health_callback(conn, is_healthy);
        }
    }
    
    return 0;
}

// Проверка таймаутов
void protocol_reliability_check_timeouts(protocol_reliability_t *reliability) {
    if (!reliability || !reliability->is_monitoring_active) return;
    
    long long current_time = 0; // Будет реальное время
    
    for (int i = 0; i < reliability->connection_count; i++) {
        connection_info_t *conn = &reliability->connections[i];
        long long time_since_activity = current_time - conn->last_activity;
        
        if (time_since_activity > PROTOCOL_TIMEOUT_MS) {
            protocol_reliability_handle_error(reliability, conn->connection_id, PROTOCOL_ERROR_TIMEOUT);
            reliability->timeout_connections++;
        }
    }
}

// Получение статистики
void protocol_reliability_get_stats(protocol_reliability_t *reliability, 
                                  char *buffer, size_t buffer_size) {
    if (!reliability || !buffer || buffer_size < 100) return;
    
    // Формирование статистики
    double success_rate = protocol_reliability_get_success_rate(reliability);
    double error_rate = protocol_reliability_get_error_rate(reliability);
    
    // Простое форматирование
    const char *stats = "Connections: ";
    int idx = 0;
    for (int i = 0; stats[i] && idx < buffer_size - 50; i++, idx++) {
        buffer[idx] = stats[i];
    }
    
    // Добавление чисел (упрощенно)
    if (success_rate > 0.95) {
        const char *perf = "EXCELLENT";
        for (int i = 0; perf[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = perf[i];
        }
    } else {
        const char *perf = "GOOD";
        for (int i = 0; perf[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = perf[i];
        }
    }
    buffer[idx] = '\0';
}

// Получение отчета об ошибках
void protocol_reliability_get_error_report(protocol_reliability_t *reliability, 
                                         char *buffer, size_t buffer_size) {
    if (!reliability || !buffer || buffer_size < 50) return;
    
    const char *report = "Error report generated";
    for (int i = 0; report[i] && i < buffer_size - 1; i++) {
        buffer[i] = report[i];
    }
    buffer[buffer_size - 1] = '\0';
}

// Получение отчета по соединению
void protocol_reliability_get_connection_report(protocol_reliability_t *reliability, 
                                              uint64_t conn_id, char *buffer, size_t buffer_size) {
    if (!reliability || !buffer || buffer_size < 50) return;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) {
        const char *not_found = "Connection not found";
        for (int i = 0; not_found[i] && i < buffer_size - 1; i++) {
            buffer[i] = not_found[i];
        }
        buffer[buffer_size - 1] = '\0';
        return;
    }
    
    const char *report = "Connection report";
    for (int i = 0; report[i] && i < buffer_size - 1; i++) {
        buffer[i] = report[i];
    }
    buffer[buffer_size - 1] = '\0';
}

// Получение коэффициента успеха
double protocol_reliability_get_success_rate(protocol_reliability_t *reliability) {
    if (!reliability || reliability->total_connections == 0) return 0.0;
    
    return (double)reliability->successful_connections / (double)reliability->total_connections;
}

// Получение коэффициента ошибок
double protocol_reliability_get_error_rate(protocol_reliability_t *reliability) {
    if (!reliability || reliability->total_connections == 0) return 0.0;
    
    return (double)reliability->failed_connections / (double)reliability->total_connections;
}

// Утилиты

const char* protocol_reliability_state_to_string(protocol_state_t state) {
    switch (state) {
        case PROTOCOL_STATE_UNKNOWN: return "UNKNOWN";
        case PROTOCOL_STATE_CONNECTING: return "CONNECTING";
        case PROTOCOL_STATE_HANDSHAKE: return "HANDSHAKE";
        case PROTOCOL_STATE_ESTABLISHED: return "ESTABLISHED";
        case PROTOCOL_STATE_DEGRADED: return "DEGRADED";
        case PROTOCOL_STATE_ERROR: return "ERROR";
        case PROTOCOL_STATE_CLOSED: return "CLOSED";
        default: return "INVALID";
    }
}

const char* protocol_reliability_error_to_string(protocol_error_t error) {
    switch (error) {
        case PROTOCOL_ERROR_NONE: return "NONE";
        case PROTOCOL_ERROR_TIMEOUT: return "TIMEOUT";
        case PROTOCOL_ERROR_INVALID_HEADER: return "INVALID_HEADER";
        case PROTOCOL_ERROR_AUTH_FAILED: return "AUTH_FAILED";
        case PROTOCOL_ERROR_CRYPTO_ERROR: return "CRYPTO_ERROR";
        case PROTOCOL_ERROR_VERSION_MISMATCH: return "VERSION_MISMATCH";
        case PROTOCOL_ERROR_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case PROTOCOL_ERROR_NETWORK: return "NETWORK_ERROR";
        case PROTOCOL_ERROR_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
        default: return "UNKNOWN_ERROR";
    }
}

const char* protocol_reliability_type_to_string(protocol_type_t type) {
    switch (type) {
        case PROTOCOL_TYPE_MTProto: return "MTProto";
        case PROTOCOL_TYPE_SHADOWSOCKS: return "Shadowsocks";
        case PROTOCOL_TYPE_HTTP_PROXY: return "HTTP_Proxy";
        case PROTOCOL_TYPE_SOCKS5: return "SOCKS5";
        default: return "UNKNOWN";
    }
}

uint64_t protocol_reliability_generate_connection_id(void) {
    return g_connection_id_counter++;
}

int protocol_reliability_is_connection_healthy(protocol_reliability_t *reliability, 
                                             uint64_t conn_id) {
    if (!reliability) return 0;
    
    int index = find_connection_index(reliability, conn_id);
    if (index < 0) return 0;
    
    connection_info_t *conn = &reliability->connections[index];
    
    // Проверка состояния
    if (conn->state == PROTOCOL_STATE_ERROR || 
        conn->state == PROTOCOL_STATE_CLOSED) {
        return 0;
    }
    
    // Проверка активности
    long long current_time = 0; // Будет реальное время
    long long time_since_activity = current_time - conn->last_activity;
    
    if (time_since_activity > PROTOCOL_TIMEOUT_MS) {
        return 0;
    }
    
    // Проверка количества ошибок
    if (conn->error_count > 3) {
        return 0;
    }
    
    return 1;
}

// Регистрация callback функций

void protocol_reliability_set_error_callback(protocol_reliability_t *reliability,
                                           void (*callback)(connection_info_t*, protocol_error_t)) {
    if (reliability) {
        reliability->error_callback = callback;
    }
}

void protocol_reliability_set_reconnect_callback(protocol_reliability_t *reliability,
                                               void (*callback)(connection_info_t*)) {
    if (reliability) {
        reliability->reconnect_callback = callback;
    }
}

void protocol_reliability_set_health_callback(protocol_reliability_t *reliability,
                                            void (*callback)(connection_info_t*, int)) {
    if (reliability) {
        reliability->health_callback = callback;
    }
}

// Вспомогательные функции

static int find_connection_index(protocol_reliability_t *reliability, uint64_t conn_id) {
    for (int i = 0; i < reliability->connection_count; i++) {
        if (reliability->connections[i].connection_id == conn_id) {
            return i;
        }
    }
    return -1;
}

static int find_error_stats_index(protocol_reliability_t *reliability, protocol_error_t error) {
    for (int i = 0; i < reliability->error_stats_count; i++) {
        if (reliability->error_stats[i].error_type == error) {
            return i;
        }
    }
    return -1;
}

static void update_error_statistics(protocol_reliability_t *reliability, protocol_error_t error) {
    int index = find_error_stats_index(reliability, error);
    
    if (index < 0 && reliability->error_stats_count < 16) {
        // Добавление новой статистики
        index = reliability->error_stats_count;
        reliability->error_stats[index].error_type = error;
        reliability->error_stats[index].occurrence_count = 0;
        reliability->error_stats[index].first_occurrence = 0; // Будет реальное время
        reliability->error_stats[index].affected_connections = 0;
        reliability->error_stats_count++;
    }
    
    if (index >= 0) {
        reliability->error_stats[index].occurrence_count++;
        reliability->error_stats[index].last_occurrence = 0; // Будет реальное время
        reliability->error_stats[index].affected_connections++;
    }
}

static int should_reconnect(connection_info_t *conn) {
    if (!conn) return 0;
    
    // Не переподключаться если:
    // - Достигнуто максимальное количество попыток
    // - Ошибка не требует переподключения
    // - Соединение уже в процессе переподключения
    
    if (conn->reconnect_attempts >= MAX_RECONNECT_ATTEMPTS) {
        return 0;
    }
    
    if (conn->last_error == PROTOCOL_ERROR_AUTH_FAILED || 
        conn->last_error == PROTOCOL_ERROR_VERSION_MISMATCH) {
        return 0;
    }
    
    return 1;
}

static void perform_reconnect(protocol_reliability_t *reliability, connection_info_t *conn) {
    if (!reliability || !conn) return;
    
    conn->reconnect_attempts++;
    conn->state = PROTOCOL_STATE_CONNECTING;
    conn->last_activity = 0; // Будет реальное время
    
    // В реальной реализации здесь будет код переподключения
    // - Закрытие текущего соединения
    // - Создание нового сокета
    // - Повторное рукопожатие
}