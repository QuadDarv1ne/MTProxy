#ifndef THREAD_SYSTEM_H
#define THREAD_SYSTEM_H

#include <stdint.h>
#include "../conn_pool/conn-pool.h"

/*
 * Тип задачи для выполнения в потоке
 */
typedef enum {
    TASK_CLIENT_REQUEST,    // Обработка клиентского запроса
    TASK_ENCRYPTION,        // Шифрование данных
    TASK_DECRYPTION,        // Расшифровка данных
    TASK_LOAD_BALANCE,      // Балансировка нагрузки
    TASK_SECURITY_CHECK,    // Проверка безопасности
    TASK_MONITORING         // Мониторинг производительности
} task_type_t;

/*
 * Структура задачи для выполнения
 */
typedef struct task {
    task_type_t type;              // Тип задачи
    void (*function)(void *);      // Функция для выполнения
    void *arg;                     // Аргументы для функции
    long priority;                 // Приоритет задачи
    struct task *next;            // Следующая задача в очереди
} task_t;

/*
 * Структура рабочего потока
 */
typedef struct worker_thread {
    int id;                        // ID потока
    int active;                    // Активен ли поток
    task_t *current_task;          // Текущая выполняемая задача
    long tasks_processed;          // Количество обработанных задач
    struct worker_thread *next;   // Следующий рабочий поток
} worker_thread_t;

/*
 * Структура пула потоков
 */
typedef struct thread_pool {
    worker_thread_t *workers;      // Массив рабочих потоков
    task_t *task_queue;            // Очередь задач
    int max_workers;               // Максимальное количество рабочих потоков
    int active_workers;            // Количество активных рабочих потоков
    int min_workers;               // Минимальное количество рабочих потоков
    int max_queue_size;            // Максимальный размер очереди задач
    int queue_size;                // Текущий размер очереди задач
    conn_pool_t *conn_pool;        // Пул соединений
    int shutdown;                  // Флаг остановки пула
} thread_pool_t;

/*
 * Инициализирует пул потоков
 * @param min_workers: минимальное количество рабочих потоков
 * @param max_workers: максимальное количество рабочих потоков
 * @param max_queue_size: максимальный размер очереди задач
 * @param conn_pool: пул соединений для использования
 * @return: указатель на пул потоков или NULL в случае ошибки
 */
thread_pool_t *init_thread_pool(int min_workers, int max_workers, 
                               int max_queue_size, conn_pool_t *conn_pool);

/*
 * Добавляет задачу в очередь пула потоков
 * @param pool: указатель на пул потоков
 * @param task_func: функция для выполнения
 * @param arg: аргументы для функции
 * @param priority: приоритет задачи
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int add_task_to_pool(thread_pool_t *pool, void (*task_func)(void *), 
                    void *arg, long priority);

/*
 * Запускает пул потоков
 * @param pool: указатель на пул потоков
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int start_thread_pool(thread_pool_t *pool);

/*
 * Останавливает пул потоков
 * @param pool: указатель на пул потоков
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int stop_thread_pool(thread_pool_t *pool);

/*
 * Освобождает ресурсы пула потоков
 * @param pool: указатель на пул потоков
 */
void destroy_thread_pool(thread_pool_t *pool);

/*
 * Получает статистику пула потоков
 * @param pool: указатель на пул потоков
 * @return: количество обработанных задач
 */
long get_thread_pool_stats(thread_pool_t *pool);

#endif