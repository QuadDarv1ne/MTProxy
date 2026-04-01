/*
    MTProxy Auto-Scaler
    Система автоматического масштабирования
    
    Поддерживаемые функции:
    - Мониторинг нагрузки (CPU, Memory, Connections)
    - Автоматическое добавление/удаление узлов
    - Политики масштабирования
    - Предотвращение колебаний (hysteresis)
    - Интеграция с Cluster Manager и Load Balancer
    
    Примеры использования:
    auto_scaler_init();
    auto_scaler_set_policy(AUTO_SCALER_POLICY_AGGRESSIVE);
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CONNECTIONS, 8000, 2000);
    auto_scaler_set_limits(1, 10);  // min 1, max 10 узлов
    auto_scaler_start();
    auto_scaler_cleanup();
*/

#ifndef AUTO_SCALER_H
#define AUTO_SCALER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Auto-Scaler
#define AUTO_SCALER_VERSION "1.0.0"

// Максимальные размеры
#define AUTO_SCALER_MAX_METRICS 8
#define AUTO_SCALER_MAX_NODES 32
#define AUTO_SCALER_MAX_NAME_LEN 64

// Политики масштабирования
typedef enum {
    AUTO_SCALER_POLICY_CONSERVATIVE = 0,  // Медленное масштабирование
    AUTO_SCALER_POLICY_MODERATE = 1,      // Умеренное масштабирование
    AUTO_SCALER_POLICY_AGGRESSIVE = 2,    // Агрессивное масштабирование
    AUTO_SCALER_POLICY_CUSTOM = 3         // Пользовательская политика
} auto_scaler_policy_t;

// Типы метрик
typedef enum {
    AUTO_SCALER_METRIC_CPU = 0,
    AUTO_SCALER_METRIC_MEMORY = 1,
    AUTO_SCALER_METRIC_CONNECTIONS = 2,
    AUTO_SCALER_METRIC_REQUESTS_PER_SEC = 3,
    AUTO_SCALER_METRIC_LATENCY = 4,
    AUTO_SCALER_METRIC_CUSTOM = 5
} auto_scaler_metric_type_t;

// Направление масштабирования
typedef enum {
    AUTO_SCALER_SCALE_NONE = 0,
    AUTO_SCALER_SCALE_UP = 1,
    AUTO_SCALER_SCALE_DOWN = 2
} auto_scaler_scale_direction_t;

// Конфигурация метрики
typedef struct {
    auto_scaler_metric_type_t type;
    char name[64];
    double scale_up_threshold;    // Порог для увеличения
    double scale_down_threshold;  // Порог для уменьшения
    int cooldown_seconds;         // Задержка перед следующим масштабированием
    int weight;                   // Вес метрики (1-10)
    bool enabled;
} auto_scaler_metric_config_t;

// Статус метрики
typedef struct {
    auto_scaler_metric_type_t type;
    char name[64];
    double current_value;
    double avg_value;
    double max_value;
    double min_value;
    int64_t last_updated;
    bool triggered;
} auto_scaler_metric_status_t;

// Конфигурация Auto-Scaler
typedef struct {
    auto_scaler_policy_t policy;
    int min_nodes;
    int max_nodes;
    int current_nodes;
    int scale_up_cooldown_sec;
    int scale_down_cooldown_sec;
    int check_interval_sec;
    bool enabled;
    char cluster_name[64];
} auto_scaler_config_t;

// Статистика Auto-Scaler
typedef struct {
    int64_t start_time;
    int total_scale_up_events;
    int total_scale_down_events;
    int failed_scale_events;
    int64_t last_scale_time;
    auto_scaler_scale_direction_t last_scale_direction;
    double avg_cpu_usage;
    double avg_memory_usage;
    double avg_connections;
    int peak_nodes;
    int current_nodes;
} auto_scaler_stats_t;

// Callback функции
typedef int (*auto_scaler_scale_up_callback_t)(int current_nodes, int desired_nodes);
typedef int (*auto_scaler_scale_down_callback_t)(int current_nodes, int desired_nodes);
typedef double (*auto_scaler_metric_reader_t)(auto_scaler_metric_type_t type);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация Auto-Scaler
 * @param cluster_name Имя кластера
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_init(const char *cluster_name);

/**
 * Очистка Auto-Scaler
 */
void auto_scaler_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирован
 */
bool auto_scaler_is_initialized(void);

// ============================================================================
// Конфигурация
// ============================================================================

/**
 * Установить политику масштабирования
 * @param policy Политика
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_set_policy(auto_scaler_policy_t policy);

/**
 * Установить лимиты количества узлов
 * @param min_nodes Минимальное количество
 * @param max_nodes Максимальное количество
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_set_limits(int min_nodes, int max_nodes);

/**
 * Установить интервал проверки
 * @param interval_sec Интервал в секундах
 */
void auto_scaler_set_check_interval(int interval_sec);

/**
 * Добавить метрику для мониторинга
 * @param type Тип метрики
 * @param scale_up_threshold Порог для увеличения
 * @param scale_down_threshold Порог для уменьшения
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_add_metric(auto_scaler_metric_type_t type, 
                            double scale_up_threshold, 
                            double scale_down_threshold);

/**
 * Настроить метрику
 * @param type Тип метрики
 * @param config Конфигурация
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_configure_metric(auto_scaler_metric_type_t type,
                                  const auto_scaler_metric_config_t *config);

// ============================================================================
// Запуск и остановка
// ============================================================================

/**
 * Запустить Auto-Scaler
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_start(void);

/**
 * Остановить Auto-Scaler
 */
void auto_scaler_stop(void);

/**
 * Проверить, запущен ли Auto-Scaler
 * @return true если запущен
 */
bool auto_scaler_is_running(void);

/**
 * Выполнить разовую проверку и масштабирование
 * @return Направление масштабирования
 */
auto_scaler_scale_direction_t auto_scaler_check_and_scale(void);

// ============================================================================
// Callback функции
// ============================================================================

/**
 * Установить callback для увеличения масштаба
 * @param callback Функция обратного вызова
 */
void auto_scaler_set_scale_up_callback(auto_scaler_scale_up_callback_t callback);

/**
 * Установить callback для уменьшения масштаба
 * @param callback Функция обратного вызова
 */
void auto_scaler_set_scale_down_callback(auto_scaler_scale_down_callback_t callback);

/**
 * Установить callback для чтения метрик
 * @param callback Функция чтения метрик
 */
void auto_scaler_set_metric_reader(auto_scaler_metric_reader_t callback);

// ============================================================================
// Мониторинг
// ============================================================================

/**
 * Получить текущее значение метрики
 * @param type Тип метрики
 * @return Значение метрики
 */
double auto_scaler_get_metric_value(auto_scaler_metric_type_t type);

/**
 * Получить статус метрики
 * @param type Тип метрики
 * @return Статус метрики
 */
const auto_scaler_metric_status_t* auto_scaler_get_metric_status(auto_scaler_metric_type_t type);

/**
 * Получить общую нагрузку (0-100)
 * @return Процент нагрузки
 */
double auto_scaler_get_total_load_percent(void);

/**
 * Проверить, требуется ли масштабирование
 * @return true если требуется масштабирование
 */
bool auto_scaler_needs_scaling(void);

// ============================================================================
// Статистика
// ============================================================================

/**
 * Получить статистику Auto-Scaler
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_get_stats(auto_scaler_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void auto_scaler_reset_stats(void);

/**
 * Получить количество событий масштабирования
 * @return Количество событий
 */
int auto_scaler_get_scale_events_count(void);

// ============================================================================
// Управление узлами
// ============================================================================

/**
 * Установить текущее количество узлов
 * @param nodes Количество узлов
 * @return 0 при успехе, -1 при ошибке
 */
int auto_scaler_set_current_nodes(int nodes);

/**
 * Получить текущее количество узлов
 * @return Количество узлов
 */
int auto_scaler_get_current_nodes(void);

/**
 * Получить рекомендуемое количество узлов
 * @return Рекомендуемое количество
 */
int auto_scaler_get_desired_nodes(void);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Конвертировать политику в строку
 * @param policy Политика
 * @return Строка политики
 */
const char* auto_scaler_policy_to_string(auto_scaler_policy_t policy);

/**
 * Конвертировать тип метрики в строку
 * @param type Тип метрики
 * @return Строка типа
 */
const char* auto_scaler_metric_type_to_string(auto_scaler_metric_type_t type);

/**
 * Конвертировать направление масштабирования в строку
 * @param direction Направление
 * @return Строка направления
 */
const char* auto_scaler_scale_direction_to_string(auto_scaler_scale_direction_t direction);

#ifdef __cplusplus
}
#endif

#endif // AUTO_SCALER_H
