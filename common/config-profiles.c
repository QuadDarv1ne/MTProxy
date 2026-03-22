/*
 * config-profiles.c - Профили конфигурации
 * Быстрое переключение между настройками, импорт/экспорт
 */

#include "config-profiles.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/* Внутренние функции */
static int find_profile(config_profiles_manager_t *mgr, const char *name);
static int save_profile_to_file(config_profiles_manager_t *mgr, config_profile_t *profile);
static int load_profile_from_file(config_profiles_manager_t *mgr, const char *filename);

/* Инициализация менеджера профилей */
int config_profiles_init(config_profiles_manager_t *mgr, const char *profiles_dir) {
    if (!mgr) {
        return -1;
    }
    
    memset(mgr, 0, sizeof(config_profiles_manager_t));
    
    if (pthread_mutex_init(&mgr->lock, NULL) != 0) {
        return -1;
    }
    
    if (profiles_dir) {
        strncpy(mgr->profiles_dir, profiles_dir, sizeof(mgr->profiles_dir) - 1);
    } else {
        strcpy(mgr->profiles_dir, "./profiles");
    }
    
    mgr->profile_count = 0;
    mgr->active_profile = -1;
    mgr->auto_save = 1;
    
    /* Создаём директорию */
    mkdir(mgr->profiles_dir, 0755);
    
    return 0;
}

/* Освобождение ресурсов */
void config_profiles_destroy(config_profiles_manager_t *mgr) {
    if (!mgr) {
        return;
    }
    
    pthread_mutex_destroy(&mgr->lock);
}

/* Поиск профиля по имени */
static int find_profile(config_profiles_manager_t *mgr, const char *name) {
    for (int i = 0; i < mgr->profile_count; i++) {
        if (strcmp(mgr->profiles[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Создание нового профиля */
int config_profiles_create(config_profiles_manager_t *mgr, 
                            const char *name,
                            const char *description,
                            int type) {
    if (!mgr || !name) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    /* Проверяем, нет ли уже такого */
    if (find_profile(mgr, name) >= 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    /* Проверяем место */
    if (mgr->profile_count >= MAX_PROFILES) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    /* Создаём профиль */
    config_profile_t *profile = &mgr->profiles[mgr->profile_count];
    memset(profile, 0, sizeof(config_profile_t));
    
    strncpy(profile->name, name, PROFILE_NAME_LEN - 1);
    strncpy(profile->description, description ? description : "", PROFILE_DESC_LEN - 1);
    profile->type = type;
    profile->created = time(NULL);
    profile->modified = profile->created;
    profile->enabled = 1;
    profile->is_default = (mgr->profile_count == 0);
    
    mgr->profile_count++;
    
    if (mgr->active_profile < 0) {
        mgr->active_profile = 0;
    }
    
    pthread_mutex_unlock(&mgr->lock);
    
    /* Сохраняем на диск */
    if (mgr->auto_save) {
        save_profile_to_file(mgr, profile);
    }
    
    return mgr->profile_count - 1;
}

/* Удаление профиля */
int config_profiles_delete(config_profiles_manager_t *mgr, const char *name) {
    if (!mgr || !name) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    int idx = find_profile(mgr, name);
    if (idx < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    /* Нельзя удалить последний профиль */
    if (mgr->profile_count <= 1) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    /* Сдвигаем массив */
    for (int i = idx; i < mgr->profile_count - 1; i++) {
        mgr->profiles[i] = mgr->profiles[i + 1];
    }
    mgr->profile_count--;
    
    /* Обновляем активный профиль */
    if (mgr->active_profile >= mgr->profile_count) {
        mgr->active_profile = mgr->profile_count - 1;
    }
    
    pthread_mutex_unlock(&mgr->lock);
    
    /* Удаляем файл */
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.profile", mgr->profiles_dir, name);
    remove(filepath);
    
    return 0;
}

/* Переключение на профиль */
int config_profiles_switch(config_profiles_manager_t *mgr, const char *name) {
    if (!mgr || !name) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    int idx = find_profile(mgr, name);
    if (idx < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    mgr->active_profile = idx;
    
    pthread_mutex_unlock(&mgr->lock);
    
    return 0;
}

/* Получение активного профиля */
config_profile_t* config_profiles_get_active(config_profiles_manager_t *mgr) {
    if (!mgr || mgr->active_profile < 0) {
        return NULL;
    }
    
    return &mgr->profiles[mgr->active_profile];
}

/* Получение профиля по имени */
config_profile_t* config_profiles_get_by_name(config_profiles_manager_t *mgr, const char *name) {
    if (!mgr || !name) {
        return NULL;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    int idx = find_profile(mgr, name);
    if (idx < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return NULL;
    }
    
    config_profile_t *profile = &mgr->profiles[idx];
    
    pthread_mutex_unlock(&mgr->lock);
    
    return profile;
}

/* Сохранение профиля на диск */
static int save_profile_to_file(config_profiles_manager_t *mgr, config_profile_t *profile) {
    if (!mgr || !profile) {
        return -1;
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.profile", mgr->profiles_dir, profile->name);
    
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return -1;
    }
    
    /* Записываем структуру */
    fwrite(profile, sizeof(config_profile_t), 1, f);
    fclose(f);
    
    return 0;
}

/* Сохранение профиля */
int config_profiles_save(config_profiles_manager_t *mgr, const char *name) {
    if (!mgr || !name) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    int idx = find_profile(mgr, name);
    if (idx < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    config_profile_t *profile = &mgr->profiles[idx];
    profile->modified = time(NULL);
    
    int ret = save_profile_to_file(mgr, profile);
    
    pthread_mutex_unlock(&mgr->lock);
    
    return ret;
}

/* Загрузка профиля из файла */
static int load_profile_from_file(config_profiles_manager_t *mgr, const char *filename) {
    if (!mgr || !filename) {
        return -1;
    }
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return -1;
    }
    
    if (mgr->profile_count >= MAX_PROFILES) {
        fclose(f);
        return -1;
    }
    
    config_profile_t *profile = &mgr->profiles[mgr->profile_count];
    
    if (fread(profile, sizeof(config_profile_t), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    mgr->profile_count++;
    
    return 0;
}

/* Загрузка профиля */
int config_profiles_load(config_profiles_manager_t *mgr, const char *name) {
    if (!mgr || !name) {
        return -1;
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.profile", mgr->profiles_dir, name);
    
    return load_profile_from_file(mgr, filepath);
}

/* Экспорт профиля в JSON */
int config_profiles_export_json(config_profiles_manager_t *mgr, 
                                 const char *name, 
                                 char *buffer, 
                                 size_t buffer_size) {
    if (!mgr || !name || !buffer || buffer_size < 512) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    int idx = find_profile(mgr, name);
    if (idx < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }
    
    config_profile_t *profile = &mgr->profiles[idx];
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"profile\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"name\": \"%s\",\n", profile->name);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"description\": \"%s\",\n", profile->description);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"type\": %d,\n", profile->type);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"created\": %llu,\n", 
                       (unsigned long long)profile->created);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"modified\": %llu,\n", 
                       (unsigned long long)profile->modified);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"enabled\": %s,\n", 
                       profile->enabled ? "true" : "false");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"is_default\": %s,\n", 
                       profile->is_default ? "true" : "false");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"data_size\": %d\n", profile->data_size);
    offset += snprintf(buffer + offset, buffer_size - offset, "  }\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    pthread_mutex_unlock(&mgr->lock);
    
    return offset;
}

/* Импорт профиля из JSON (упрощённо) */
int config_profiles_import_json(config_profiles_manager_t *mgr, const char *json) {
    if (!mgr || !json) {
        return -1;
    }
    
    /* Парсим JSON (упрощённо, ищем name) */
    const char *name_start = strstr(json, "\"name\":");
    if (!name_start) {
        return -1;
    }
    
    name_start += 7;
    while (*name_start == ' ' || *name_start == '"') name_start++;
    
    char name[PROFILE_NAME_LEN];
    int i = 0;
    while (*name_start && *name_start != '"' && i < PROFILE_NAME_LEN - 1) {
        name[i++] = *name_start++;
    }
    name[i] = '\0';
    
    /* Создаём профиль */
    int idx = config_profiles_create(mgr, name, "Imported profile", PROFILE_TYPE_CUSTOM);
    if (idx < 0) {
        return -1;
    }
    
    return 0;
}

/* Сохранение всех профилей */
int config_profiles_save_all(config_profiles_manager_t *mgr) {
    if (!mgr) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    for (int i = 0; i < mgr->profile_count; i++) {
        save_profile_to_file(mgr, &mgr->profiles[i]);
    }
    
    pthread_mutex_unlock(&mgr->lock);
    
    return 0;
}

/* Загрузка всех профилей */
int config_profiles_load_all(config_profiles_manager_t *mgr) {
    if (!mgr) {
        return -1;
    }
    
    DIR *dir = opendir(mgr->profiles_dir);
    if (!dir) {
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".profile")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", mgr->profiles_dir, entry->d_name);
            load_profile_from_file(mgr, filepath);
        }
    }
    
    closedir(dir);
    
    if (mgr->profile_count > 0 && mgr->active_profile < 0) {
        mgr->active_profile = 0;
    }
    
    return 0;
}

/* Создание профилей по умолчанию */
int config_profiles_create_defaults(config_profiles_manager_t *mgr) {
    if (!mgr) {
        return -1;
    }
    
    /* Performance профиль */
    config_profiles_create(mgr, "performance", 
                           "Высокая производительность, минимальная задержка",
                           PROFILE_TYPE_PERFORMANCE);
    
    /* Security профиль */
    config_profiles_create(mgr, "security", 
                           "Максимальная безопасность, строгая проверка",
                           PROFILE_TYPE_SECURITY);
    
    /* Compatibility профиль */
    config_profiles_create(mgr, "compatibility", 
                           "Максимальная совместимость, старые протоколы",
                           PROFILE_TYPE_COMPATIBILITY);
    
    return 0;
}

/* Получение статистики */
void config_profiles_get_stats(config_profiles_manager_t *mgr, config_profiles_stats_t *stats) {
    if (!mgr || !stats) {
        return;
    }
    
    pthread_mutex_lock(&mgr->lock);
    
    stats->total_profiles = mgr->profile_count;
    stats->active_profile = mgr->active_profile;
    stats->enabled_profiles = 0;
    stats->last_switch = 0;
    
    for (int i = 0; i < mgr->profile_count; i++) {
        if (mgr->profiles[i].enabled) {
            stats->enabled_profiles++;
        }
    }
    
    pthread_mutex_unlock(&mgr->lock);
}
