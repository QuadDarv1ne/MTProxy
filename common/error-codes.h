/**
 * @file error-codes.h
 * @brief Централизованные коды ошибок для MTProxy
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#ifndef __ERROR_CODES_H__
#define __ERROR_CODES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Базовые коды ошибок
 * ============================================ */

/** Успешное выполнение */
#define MT_SUCCESS                  0

/** Общая ошибка */
#define MT_ERR_GENERAL              (-1)

/** Ошибка выделения памяти */
#define MT_ERR_NO_MEMORY            (-2)

/** Ошибка неверных аргументов */
#define MT_ERR_INVALID_ARGS         (-3)

/** Ошибка инициализации */
#define MT_ERR_INIT                 (-4)

/** Ошибка состояния (не тот порядок вызовов) */
#define MT_ERR_INVALID_STATE        (-5)

/** Ошибка переполнения */
#define MT_ERR_OVERFLOW             (-6)

/** Ошибка недопустимой операции */
#define MT_ERR_NOT_SUPPORTED        (-7)

/** Ошибка таймаута */
#define MT_ERR_TIMEOUT              (-8)

/* ============================================
 * Сетевые ошибки (100-199)
 * ============================================ */

/** Ошибка сокета */
#define MT_ERR_SOCKET               (-100)

/** Ошибка подключения */
#define MT_ERR_CONNECT              (-101)

/** Ошибка привязки */
#define MT_ERR_BIND                 (-102)

/** Ошибка прослушивания */
#define MT_ERR_LISTEN               (-103)

/** Ошибка принятия подключения */
#define MT_ERR_ACCEPT               (-104)

/** Ошибка отправки */
#define MT_ERR_SEND                 (-105)

/** Ошибка получения */
#define MT_ERR_RECV                 (-106)

/** Ошибка разрешения DNS */
#define MT_ERR_DNS                  (-107)

/** Ошибка SSL/TLS */
#define MT_ERR_SSL                  (-108)

/* ============================================
 * Ошибки фрагментации (200-299)
 * ============================================ */

/** Ошибка фрагментации */
#define MT_ERR_FRAGMENT             (-200)

/** Ошибка сборки фрагментов */
#define MT_ERR_ASSEMBLE             (-201)

/** Превышено количество фрагментов */
#define MT_ERR_TOO_MANY_FRAGMENTS   (-202)

/** Ошибка размера фрагмента */
#define MT_ERR_FRAGMENT_SIZE        (-203)

/* ============================================
 * Ошибки padding (300-399)
 * ============================================ */

/** Ошибка padding */
#define MT_ERR_PADDING              (-300)

/** Ошибка размера блока */
#define MT_ERR_BLOCK_SIZE           (-301)

/** Ошибка length prefix */
#define MT_ERR_LENGTH_PREFIX        (-302)

/* ============================================
 * Ошибки ML модулей (400-499)
 * ============================================ */

/** Ошибка ML модели */
#define MT_ERR_ML_MODEL             (-400)

/** Ошибка обучения ML */
#define MT_ERR_ML_TRAIN             (-401)

/** Ошибка предсказания ML */
#define MT_ERR_ML_PREDICT           (-402)

/** Недостаточно данных для ML */
#define MT_ERR_ML_INSUFFICIENT_DATA (-403)

/** Ошибка признаков ML */
#define MT_ERR_ML_FEATURES          (-404)

/* ============================================
 * Ошибки производительности (500-599)
 * ============================================ */

/** Ошибка мониторинга */
#define MT_ERR_PERF_MONITOR         (-500)

/** Ошибка сбора метрик */
#define MT_ERR_METRICS              (-501)

/** Превышен порог */
#define MT_ERR_THRESHOLD            (-502)

/* ============================================
 * Ошибки CLI (600-699)
 * ============================================ */

/** Ошибка CLI */
#define MT_ERR_CLI                  (-600)

/** Ошибка аутентификации CLI */
#define MT_ERR_CLI_AUTH             (-601)

/** Ошибка команды CLI */
#define MT_ERR_CLI_COMMAND          (-602)

/* ============================================
 * Макросы для обработки ошибок
 * ============================================ */

/** Проверка указателя на NULL */
#define MT_CHECK_PTR(ptr, err_code) \
    do { \
        if ((ptr) == NULL) { \
            return (err_code); \
        } \
    } while(0)

/** Проверка условия */
#define MT_CHECK_COND(cond, err_code) \
    do { \
        if (!(cond)) { \
            return (err_code); \
        } \
    } while(0)

/** Проверка результата функции */
#define MT_CHECK_RESULT(result) \
    do { \
        int __ret = (result); \
        if (__ret != MT_SUCCESS) { \
            return __ret; \
        } \
    } while(0)

/** Проверка выделения памяти */
#define MT_CHECK_ALLOC(ptr) MT_CHECK_PTR(ptr, MT_ERR_NO_MEMORY)

/* ============================================
 * Утилиты для работы с ошибками
 * ============================================ */

/**
 * @brief Получить строковое описание ошибки
 *
 * @param error_code Код ошибки
 * @return Строковое описание
 */
const char* mt_error_string(int error_code);

/**
 * @brief Получить категорию ошибки
 *
 * @param error_code Код ошибки
 * @return Категория ошибки (0=success, 1=general, 2=network, etc.)
 */
int mt_error_category(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_CODES_H__ */
