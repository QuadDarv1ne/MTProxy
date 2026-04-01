#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>

/*
 * Пул памяти - это структура данных, которая предварительно выделяет большой блок памяти
 * и управляет распределением и освобождением меньших блоков из этого пула.
 * Это помогает уменьшить накладные расходы на выделение памяти и улучшить производительность.
 */

// Forward declaration for cross-platform mutex
#ifdef _WIN32
// On Windows, we'll define our mutex type as void*
typedef void* mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

typedef struct memory_block {
    struct memory_block *next;
    size_t size;
    char data[];
} memory_block_t;

typedef struct memory_pool {
    memory_block_t *free_list;
    char *buffer;
    size_t buffer_size;
    size_t allocated_size;
    mutex_t mutex;
} memory_pool_t;

/*
 * Создает новый пул памяти с указанным начальным размером
 * @param initial_size: начальный размер пула памяти в байтах
 * @return: указатель на новый пул памяти или NULL в случае ошибки
 */
memory_pool_t *create_memory_pool(size_t initial_size);

/*
 * Выделяет блок памяти из пула заданного размера
 * @param pool: указатель на пул памяти
 * @param size: размер выделяемого блока в байтах
 * @return: указатель на выделенную память или NULL в случае ошибки
 */
void *memory_pool_alloc(memory_pool_t *pool, size_t size);

/*
 * Освобождает ранее выделенный блок памяти обратно в пул
 * @param pool: указатель на пул памяти
 * @param ptr: указатель на блок памяти для освобождения
 */
void memory_pool_free(memory_pool_t *pool, void *ptr);

/*
 * Уничтожает пул памяти и освобождает всю связанную с ним память
 * @param pool: указатель на пул памяти для уничтожения
 */
void destroy_memory_pool(memory_pool_t *pool);

#endif