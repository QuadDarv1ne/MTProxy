/*
 * Plugin System API for MTProxy
 * Система плагинов для расширения функциональности MTProxy
 * 
 * Возможности:
 * - Динамическая загрузка плагинов (.so файлы)
 * - Система хуков для вмешательства в различные этапы обработки
 * - Контекст плагина с данными и состоянием
 * - Приоритеты выполнения хуков
 * - Безопасная выгрузка плагинов
 */

#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Версия API плагинов
 * ============================================================================ */

#define MTPLUGIN_API_VERSION_MAJOR 1
#define MTPLUGIN_API_VERSION_MINOR 0
#define MTPLUGIN_API_VERSION_PATCH 0
#define MTPLUGIN_API_VERSION ((MTPLUGIN_API_VERSION_MAJOR << 16) | \
                             (MTPLUGIN_API_VERSION_MINOR << 8) | \
                             MTPLUGIN_API_VERSION_PATCH)

#define MTPLUGIN_NAME_MAX_LEN 128
#define MTPLUGIN_DESC_MAX_LEN 512
#define MTPLUGIN_PATH_MAX_LEN 512
#define MTPLUGIN_MAX_HOOKS 64
#define MTPLUGIN_MAX_PLUGINS 32

/* ============================================================================
 * Типы хуков (точки расширения)
 * ============================================================================ */

typedef enum {
    HOOK_NONE = 0,
    
    // Инициализация/завершение
    HOOK_PLUGIN_INIT,           // Инициализация плагина
    HOOK_PLUGIN_SHUTDOWN,       // Завершение плагина
    
    // Сетевые хуки
    HOOK_CONNECTION_ACCEPT,     // Новое подключение
    HOOK_CONNECTION_CLOSE,      // Закрытие подключения
    HOOK_DATA_RECEIVED,         // Получение данных
    HOOK_DATA_SENT,             // Отправка данных
    
    // MTProto хуки
    HOOK_MTPROTO_HANDSHAKE,     // Handshake MTProto
    HOOK_MTPROTO_ENCRYPT,       // Шифрование
    HOOK_MTPROTO_DECRYPT,       // Расшифровка
    HOOK_MTPROTO_VALIDATE,      // Валидация пакета
    
    // Безопасность
    HOOK_SECURITY_CHECK,        // Проверка безопасности
    HOOK_RATE_LIMIT_CHECK,      // Rate limiting
    HOOK_AUTH_CHECK,            // Аутентификация
    
    // Конфигурация
    HOOK_CONFIG_LOAD,           // Загрузка конфигурации
    HOOK_CONFIG_RELOAD,         // Перезагрузка конфигурации
    
    // Статистика
    HOOK_STATS_COLLECT,         // Сбор статистики
    HOOK_STATS_REPORT,          // Отчет статистики
    
    // Логирование
    HOOK_LOG_MESSAGE,           // Логирование сообщения
    
    // Пользовательские хуки
    HOOK_CUSTOM_1,
    HOOK_CUSTOM_2,
    HOOK_CUSTOM_3,
    
    HOOK_COUNT
} plugin_hook_type_t;

/* ============================================================================
 * Коды возврата
 * ============================================================================ */

typedef enum {
    PLUGIN_OK = 0,              // Успех
    PLUGIN_ERROR = -1,          // Общая ошибка
    PLUGIN_SKIP = 1,            // Пропустить следующий плагин
    PLUGIN_STOP = 2,            // Остановить цепочку
    PLUGIN_REJECT = 3,          // Отклонить (для accept хуков)
} plugin_result_t;

/* ============================================================================
 * Структуры данных
 * ============================================================================ */

/* Контекст хука */
typedef struct {
    plugin_hook_type_t hook_type;   // Тип хука
    void *data;                     // Данные хука
    size_t data_size;               // Размер данных
    void *user_data;                // Пользовательские данные
    void *plugin_data;              // Данные плагина
    int connection_fd;              // FD подключения (если есть)
    char client_ip[64];             // IP клиента
    uint16_t client_port;           // Порт клиента
    uint64_t timestamp;             // Временная метка
    uint64_t sequence;              // Порядковый номер
    int priority;                   // Приоритет
    char result_data[1024];         // Результаты обработки
    int result_code;                // Код результата
} plugin_hook_context_t;

/* Информация о плагине */
typedef struct {
    uint32_t api_version;           // Версия API
    char name[MTPLUGIN_NAME_MAX_LEN];     // Имя плагина
    char description[MTPLUGIN_DESC_MAX_LEN]; // Описание
    char version[32];               // Версия плагина
    char author[128];               // Автор
    char license[64];               // Лицензия
    const char **dependencies;      // Зависимости
    int dependency_count;           // Количество зависимостей
    const char **supported_hooks;   // Поддерживаемые хуки
    int supported_hook_count;       // Количество хуков
} plugin_info_t;

/* Конфигурация плагина */
typedef struct {
    char name[MTPLUGIN_NAME_MAX_LEN];
    bool enabled;
    int priority;
    const char *config_path;
    void *config_data;
    size_t config_size;
} plugin_config_t;

/* Статистика плагина */
typedef struct {
    uint64_t load_count;            // Количество загрузок
    uint64_t unload_count;          // Количество выгрузок
    uint64_t hook_calls[HOOK_COUNT]; // Вызовы хуков
    uint64_t hook_errors[HOOK_COUNT]; // Ошибки хуков
    uint64_t total_execution_time_us; // Общее время выполнения (мкс)
    uint64_t avg_execution_time_us;   // Среднее время (мкс)
    uint64_t max_execution_time_us;   // Макс время (мкс)
    uint64_t memory_used;           // Используемая память
    int last_error_code;            // Последняя ошибка
    char last_error_msg[256];       // Сообщение об ошибке
} plugin_stats_t;

/* Дескриптор плагина */
typedef struct plugin_handle plugin_handle_t;

/* ============================================================================
 * Callback типы для хуков
 * ============================================================================ */

/**
 * @brief Callback функция хука
 * @param ctx Контекст хука
 * @param plugin_data Данные плагина
 * @return Результат выполнения
 */
typedef plugin_result_t (*plugin_hook_callback_t)(plugin_hook_context_t *ctx, 
                                                  void *plugin_data);

/* ============================================================================
 * Экспортируемые функции плагина
 * ============================================================================ */

/*
 * Каждый плагин должен экспортировать следующие функции:
 */

/**
 * @brief Получение информации о плагине
 * @return Указатель на структуру plugin_info_t
 * 
 * Экспортируемое имя: plugin_get_info
 * Сигнатура: const plugin_info_t* plugin_get_info(void);
 */
typedef const plugin_info_t* (*plugin_get_info_func)(void);

/**
 * @brief Инициализация плагина
 * @param config Конфигурация
 * @param plugin_data Указатель на данные плагина
 * @return PLUGIN_OK при успехе
 * 
 * Экспортируемое имя: plugin_init
 * Сигнатура: int plugin_init(const plugin_config_t* config, void** plugin_data);
 */
typedef int (*plugin_init_func)(const plugin_config_t *config, void **plugin_data);

/**
 * @brief Завершение плагина
 * @param plugin_data Данные плагина
 * 
 * Экспортируемое имя: plugin_shutdown
 * Сигнатура: void plugin_shutdown(void* plugin_data);
 */
typedef void (*plugin_shutdown_func)(void *plugin_data);

/**
 * @brief Регистрация хука
 * @param hook_type Тип хука
 * @param callback Callback функция
 * @param priority Приоритет (чем выше, тем раньше выполняется)
 * @param plugin_data Данные плагина
 * @return PLUGIN_OK при успехе
 * 
 * Экспортируемое имя: plugin_register_hook
 * Сигнатура: int plugin_register_hook(plugin_hook_type_t hook_type, 
 *                                     plugin_hook_callback_t callback,
 *                                     int priority, void* plugin_data);
 */
typedef int (*plugin_register_hook_func)(plugin_hook_type_t hook_type,
                                         plugin_hook_callback_t callback,
                                         int priority,
                                         void *plugin_data);

/**
 * @brief Удаление хука
 * @param hook_type Тип хука
 * @param callback Callback функция
 * @return PLUGIN_OK при успехе
 * 
 * Экспортируемое имя: plugin_unregister_hook
 * Сигнатура: int plugin_unregister_hook(plugin_hook_type_t hook_type,
 *                                        plugin_hook_callback_t callback);
 */
typedef int (*plugin_unregister_hook_func)(plugin_hook_type_t hook_type,
                                           plugin_hook_callback_t callback);

/**
 * @brief Получение статистики
 * @param stats Указатель на структуру статистики
 * 
 * Экспортируемое имя: plugin_get_stats
 * Сигнатура: void plugin_get_stats(plugin_stats_t* stats);
 */
typedef void (*plugin_get_stats_func)(plugin_stats_t *stats);

/* ============================================================================
 * Менеджер плагинов (хост)
 * ============================================================================ */

/* Конфигурация менеджера плагинов */
typedef struct {
    char plugin_dir[MTPLUGIN_PATH_MAX_LEN];  // Директория плагинов
    bool auto_load;                          // Автозагрузка
    bool hot_reload;                         // Горячая перезагрузка
    int max_plugins;                         // Макс количество плагинов
    int hook_timeout_ms;                     // Таймаут хука (мс)
    bool enable_stats;                       // Статистика
    bool enable_logging;                     // Логирование
    const char **allowed_plugins;            // Белый список
    int allowed_plugin_count;
    const char **blocked_plugins;            // Черный список
    int blocked_plugin_count;
} plugin_manager_config_t;

/* Статистика менеджера */
typedef struct {
    int loaded_plugins;
    int failed_plugins;
    int total_hooks;
    uint64_t total_hook_calls;
    uint64_t total_hook_errors;
    uint64_t total_execution_time_us;
} plugin_manager_stats_t;

/* Менеджер плагинов */
struct plugin_handle {
    char name[MTPLUGIN_NAME_MAX_LEN];
    char path[MTPLUGIN_PATH_MAX_LEN];
    void *dl_handle;
    plugin_info_t info;
    plugin_config_t config;
    plugin_stats_t stats;
    void *plugin_data;
    bool loaded;
    bool enabled;
    
    // Экспортируемые функции
    plugin_get_info_func get_info;
    plugin_init_func init;
    plugin_shutdown_func shutdown;
    plugin_register_hook_func register_hook;
    plugin_unregister_hook_func unregister_hook;
    plugin_get_stats_func get_stats;
};

typedef struct {
    plugin_manager_config_t config;
    plugin_manager_stats_t stats;
    
    plugin_handle_t *plugins[MTPLUGIN_MAX_PLUGINS];
    int plugin_count;
    
    // Хуки (список callback'ов для каждого типа)
    struct {
        plugin_hook_callback_t callback;
        void *plugin_data;
        int priority;
        plugin_handle_t *plugin;
    } hooks[HOOK_COUNT][MTPLUGIN_MAX_HOOKS];
    
    int hook_count[HOOK_COUNT];
    
    bool initialized;
    bool running;
} plugin_manager_t;

/* ============================================================================
 * API менеджера плагинов
 * ============================================================================ */

/**
 * @brief Инициализация менеджера плагинов
 */
int plugin_manager_init(plugin_manager_t *manager, 
                       const plugin_manager_config_t *config);

/**
 * @brief Запуск менеджера плагинов
 */
int plugin_manager_start(plugin_manager_t *manager);

/**
 * @brief Остановка менеджера плагинов
 */
void plugin_manager_stop(plugin_manager_t *manager);

/**
 * @brief Очистка менеджера плагинов
 */
void plugin_manager_cleanup(plugin_manager_t *manager);

/**
 * @brief Загрузка плагина из файла
 */
int plugin_manager_load(plugin_manager_t *manager, const char *path);

/**
 * @brief Выгрузка плагина по имени
 */
int plugin_manager_unload(plugin_manager_t *manager, const char *name);

/**
 * @brief Загрузка всех плагинов из директории
 */
int plugin_manager_load_all(plugin_manager_t *manager);

/**
 * @brief Выгрузка всех плагинов
 */
void plugin_manager_unload_all(plugin_manager_t *manager);

/**
 * @brief Получение плагина по имени
 */
plugin_handle_t *plugin_manager_get(plugin_manager_t *manager, 
                                   const char *name);

/**
 * @brief Включение/выключение плагина
 */
int plugin_manager_enable(plugin_manager_t *manager, const char *name);
int plugin_manager_disable(plugin_manager_t *manager, const char *name);

/**
 * @brief Выполнение хуков
 */
int plugin_manager_execute_hook(plugin_manager_t *manager,
                               plugin_hook_type_t hook_type,
                               plugin_hook_context_t *ctx);

/**
 * @brief Получение статистики
 */
void plugin_manager_get_stats(plugin_manager_t *manager,
                             plugin_manager_stats_t *stats);

/**
 * @brief Список загруженных плагинов
 */
int plugin_manager_list_plugins(plugin_manager_t *manager,
                               char names[][MTPLUGIN_NAME_MAX_LEN],
                               int max_count);

/* ============================================================================
 * Утилиты для плагинов
 * ============================================================================ */

/**
 * @brief Логирование из плагина
 */
void plugin_log(plugin_handle_t *plugin, int level, const char *format, ...);

/**
 * @brief Выделение памяти для плагина
 */
void *plugin_alloc(size_t size);

/**
 * @brief Освобождение памяти плагина
 */
void plugin_free(void *ptr);

/**
 * @brief Проверка версии API
 */
bool plugin_check_api_version(uint32_t version);

/**
 * @brief Получение версии API в строке
 */
const char *plugin_get_api_version_string(void);

/* ============================================================================
 * Макросы для объявления плагина
 * ============================================================================ */

#define PLUGIN_DECLARE_INFO(name, desc, version, author, license) \
    static plugin_info_t g_plugin_info = { \
        .api_version = MTPLUGIN_API_VERSION, \
        .name = name, \
        .description = desc, \
        .version = version, \
        .author = author, \
        .license = license, \
        .dependencies = NULL, \
        .dependency_count = 0, \
        .supported_hooks = NULL, \
        .supported_hook_count = 0 \
    }; \
    \
    const plugin_info_t* plugin_get_info(void) { \
        return &g_plugin_info; \
    }

#define PLUGIN_REGISTER_HOOK(manager, hook_type, callback, priority, data) \
    plugin_manager_execute_hook(manager, hook_type, callback, priority, data)

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_SYSTEM_H */
