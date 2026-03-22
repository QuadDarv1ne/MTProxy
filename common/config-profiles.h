/*
 * config-profiles.h - Профили конфигурации
 * Быстрое переключение между настройками, импорт/экспорт
 */

#ifndef __CONFIG_PROFILES_H__
#define __CONFIG_PROFILES_H__

#include <stdint.h>
#include <pthread.h>

#define MAX_PROFILES 16
#define PROFILE_NAME_LEN 64
#define PROFILE_DESC_LEN 256
#define PROFILE_DATA_SIZE 4096

/* Типы профилей */
#define PROFILE_TYPE_CUSTOM     0
#define PROFILE_TYPE_PERFORMANCE 1
#define PROFILE_TYPE_SECURITY   2
#define PROFILE_TYPE_COMPATIBILITY 3

/* Профиль конфигурации */
typedef struct {
    char name[PROFILE_NAME_LEN];
    char description[PROFILE_DESC_LEN];
    int type;
    char data[PROFILE_DATA_SIZE];
    int data_size;
    time_t created;
    time_t modified;
    int enabled;
    int is_default;
} config_profile_t;

/* Менеджер профилей */
typedef struct {
    config_profile_t profiles[MAX_PROFILES];
    int profile_count;
    int active_profile;
    pthread_mutex_t lock;
    char profiles_dir[256];
    int auto_save;
} config_profiles_manager_t;

/* Инициализация менеджера профилей */
int config_profiles_init(config_profiles_manager_t *mgr, const char *profiles_dir);

/* Освобождение ресурсов */
void config_profiles_destroy(config_profiles_manager_t *mgr);

/* Создание нового профиля */
int config_profiles_create(config_profiles_manager_t *mgr, 
                            const char *name,
                            const char *description,
                            int type);

/* Удаление профиля */
int config_profiles_delete(config_profiles_manager_t *mgr, const char *name);

/* Переключение на профиль */
int config_profiles_switch(config_profiles_manager_t *mgr, const char *name);

/* Получение активного профиля */
config_profile_t* config_profiles_get_active(config_profiles_manager_t *mgr);

/* Получение профиля по имени */
config_profile_t* config_profiles_get_by_name(config_profiles_manager_t *mgr, const char *name);

/* Сохранение профиля на диск */
int config_profiles_save(config_profiles_manager_t *mgr, const char *name);

/* Загрузка профиля с диска */
int config_profiles_load(config_profiles_manager_t *mgr, const char *name);

/* Экспорт профиля в JSON */
int config_profiles_export_json(config_profiles_manager_t *mgr, 
                                 const char *name, 
                                 char *buffer, 
                                 size_t buffer_size);

/* Импорт профиля из JSON */
int config_profiles_import_json(config_profiles_manager_t *mgr, const char *json);

/* Сохранение всех профилей */
int config_profiles_save_all(config_profiles_manager_t *mgr);

/* Загрузка всех профилей */
int config_profiles_load_all(config_profiles_manager_t *mgr);

/* Создание профилей по умолчанию */
int config_profiles_create_defaults(config_profiles_manager_t *mgr);

/* Получение статистики */
typedef struct {
    int total_profiles;
    int active_profile;
    int enabled_profiles;
    time_t last_switch;
} config_profiles_stats_t;

void config_profiles_get_stats(config_profiles_manager_t *mgr, config_profiles_stats_t *stats);

#endif /* __CONFIG_PROFILES_H__ */
