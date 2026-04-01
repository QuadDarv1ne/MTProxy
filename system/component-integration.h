/*
    Система интеграции и координации компонентов MTProxy
    Единая точка управления всеми подсистемами
*/

#ifndef COMPONENT_INTEGRATION_H
#define COMPONENT_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>

// Состояния компонентов
typedef enum {
    COMPONENT_STATE_UNKNOWN = 0,
    COMPONENT_STATE_INITIALIZING = 1,
    COMPONENT_STATE_READY = 2,
    COMPONENT_STATE_RUNNING = 3,
    COMPONENT_STATE_PAUSED = 4,
    COMPONENT_STATE_ERROR = 5,
    COMPONENT_STATE_SHUTDOWN = 6
} component_state_t;

// Типы компонентов
typedef enum {
    COMPONENT_TYPE_SECURITY = 0,
    COMPONENT_TYPE_PERFORMANCE = 1,
    COMPONENT_TYPE_NETWORK = 2,
    COMPONENT_TYPE_MONITORING = 3,
    COMPONENT_TYPE_CRYPTO = 4,
    COMPONENT_TYPE_ADMIN = 5,
    COMPONENT_TYPE_WEBSOCKET = 6,
    COMPONENT_TYPE_PROTOCOL = 7
} component_type_t;

// Приоритеты компонентов
typedef enum {
    PRIORITY_CRITICAL = 0,   // Критически важные (безопасность)
    PRIORITY_HIGH = 1,       // Высокий приоритет (сетевые операции)
    PRIORITY_MEDIUM = 2,     // Средний приоритет (мониторинг)
    PRIORITY_LOW = 3         // Низкий приоритет (административные функции)
} component_priority_t;

// Компонент системы
typedef struct system_component {
    uint64_t component_id;
    component_type_t type;
    component_state_t state;
    component_priority_t priority;
    char name[64];
    char description[128];
    void *instance;          // Указатель на экземпляр компонента
    long long init_time;
    long long last_update;
    int error_count;
    char last_error[256];
    int is_enabled;
    int auto_restart;
} system_component_t;

// Конфигурация интеграции
typedef struct integration_config {
    int enable_auto_coordination;
    int enable_health_checks;
    int health_check_interval_ms;
    int enable_auto_recovery;
    int max_restart_attempts;
    int component_timeout_ms;
    int enable_logging;
    char log_level[16];
} integration_config_t;

// Статистика интеграции
typedef struct integration_stats {
    long long total_components;
    long long active_components;
    long long failed_components;
    long long restart_count;
    long long health_checks;
    long long recovery_attempts;
    double avg_response_time_ms;
    long long total_errors;
} integration_stats_t;

// Система интеграции
typedef struct component_integration {
    // Конфигурация
    integration_config_t config;
    
    // Компоненты
    system_component_t *components;
    int component_count;
    int max_components;
    
    // Статистика
    integration_stats_t stats;
    
    // Состояние
    int is_initialized;
    int is_running;
    long long start_time;
    component_state_t overall_state;
    
    // Callback функции
    void (*on_component_state_change)(system_component_t *component, component_state_t old_state, component_state_t new_state);
    void (*on_system_error)(const char *component_name, const char *error_message);
    void (*on_health_check)(system_component_t *component, int is_healthy);
    void (*on_auto_recovery)(system_component_t *component, int recovery_result);
} component_integration_t;

// Инициализация
component_integration_t* integration_init(int max_components);
int integration_configure(component_integration_t *integration, const integration_config_t *config);
void integration_cleanup(component_integration_t *integration);

// Управление компонентами
int integration_register_component(component_integration_t *integration, 
                                 component_type_t type, const char *name,
                                 const char *description, component_priority_t priority,
                                 void *instance);
int integration_unregister_component(component_integration_t *integration, uint64_t component_id);
int integration_start_component(component_integration_t *integration, uint64_t component_id);
int integration_stop_component(component_integration_t *integration, uint64_t component_id);
int integration_restart_component(component_integration_t *integration, uint64_t component_id);
int integration_pause_component(component_integration_t *integration, uint64_t component_id);
int integration_resume_component(component_integration_t *integration, uint64_t component_id);

// Состояние компонентов
component_state_t integration_get_component_state(component_integration_t *integration, uint64_t component_id);
int integration_set_component_state(component_integration_t *integration, uint64_t component_id, 
                                  component_state_t state);
system_component_t* integration_get_component(component_integration_t *integration, uint64_t component_id);
system_component_t* integration_find_component(component_integration_t *integration, component_type_t type);

// Координация системы
int integration_start_all_components(component_integration_t *integration);
int integration_stop_all_components(component_integration_t *integration);
int integration_pause_all_components(component_integration_t *integration);
int integration_resume_all_components(component_integration_t *integration);
int integration_restart_all_components(component_integration_t *integration);

// Мониторинг и здоровье
int integration_perform_health_check(component_integration_t *integration);
int integration_check_component_health(component_integration_t *integration, uint64_t component_id);
void integration_update_component_health(component_integration_t *integration, uint64_t component_id, 
                                       int is_healthy);
int integration_perform_auto_recovery(component_integration_t *integration);
int integration_recover_component(component_integration_t *integration, uint64_t component_id);

// Статистика и отчеты
void integration_get_stats(component_integration_t *integration, integration_stats_t *stats);
void integration_get_component_report(component_integration_t *integration, uint64_t component_id,
                                    char *buffer, size_t buffer_size);
void integration_get_system_report(component_integration_t *integration, char *buffer, size_t buffer_size);
void integration_get_health_report(component_integration_t *integration, char *buffer, size_t buffer_size);
void integration_reset_stats(component_integration_t *integration);

// Управление зависимостями
int integration_add_dependency(component_integration_t *integration, uint64_t component_id, 
                              uint64_t dependency_id);
int integration_remove_dependency(component_integration_t *integration, uint64_t component_id, 
                                 uint64_t dependency_id);
int integration_check_dependencies(component_integration_t *integration, uint64_t component_id);
int integration_resolve_dependencies(component_integration_t *integration);

// События и уведомления
void integration_set_state_change_callback(component_integration_t *integration,
                                          void (*callback)(system_component_t*, component_state_t, component_state_t));
void integration_set_error_callback(component_integration_t *integration,
                                   void (*callback)(const char*, const char*));
void integration_set_health_callback(component_integration_t *integration,
                                    void (*callback)(system_component_t*, int));
void integration_set_recovery_callback(component_integration_t *integration,
                                      void (*callback)(system_component_t*, int));

// Утилиты
const char* integration_state_to_string(component_state_t state);
const char* integration_type_to_string(component_type_t type);
const char* integration_priority_to_string(component_priority_t priority);
uint64_t integration_generate_component_id(void);
int integration_is_component_healthy(component_integration_t *integration, uint64_t component_id);
component_state_t integration_calculate_overall_state(component_integration_t *integration);

#endif // COMPONENT_INTEGRATION_H