#include "memory-pool.h"
#include <stdlib.h>
#include <string.h>

/*
 * Базовая реализация пула памяти без блокировок
 * Потокобезопасность будет добавлена в будущих итерациях
 */

/*
 * Создает новый пул памяти с указанным начальным размером
 * @param initial_size: начальный размер пула памяти в байтах
 * @return: указатель на новый пул памяти или NULL в случае ошибки
 */
memory_pool_t *create_memory_pool(size_t initial_size) {
    memory_pool_t *pool = (memory_pool_t *)malloc(sizeof(memory_pool_t));
    if (!pool) return NULL;
    
    pool->buffer = (char *)malloc(initial_size);
    if (!pool->buffer) {
        free(pool);
        return NULL;
    }
    
    pool->buffer_size = initial_size;
    pool->allocated_size = 0;
    pool->free_list = NULL;
    
    return pool;
}

/*
 * Выделяет блок памяти из пула заданного размера
 * @param pool: указатель на пул памяти
 * @param size: размер выделяемого блока в байтах
 * @return: указатель на выделенную память или NULL в случае ошибки
 */
void *memory_pool_alloc(memory_pool_t *pool, size_t size) {
    if (!pool || !size) return NULL;
    
    // Добавляем выравнивание для указателей
    size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    
    // Сначала пробуем выделить из списка свободных блоков
    memory_block_t *block = pool->free_list;
    memory_block_t *prev = NULL;
    
    while (block) {
        if (block->size >= size) {
            // Нашли подходящий блок
            if (prev) {
                prev->next = block->next;
            } else {
                pool->free_list = block->next;
            }
            
            return block->data;
        }
        prev = block;
        block = block->next;
    }
    
    // Если подходящего блока нет в списке, выделяем из буфера
    if (pool->allocated_size + sizeof(memory_block_t) + size <= pool->buffer_size) {
        char *ptr = pool->buffer + pool->allocated_size;
        memory_block_t *new_block = (memory_block_t *)ptr;
        
        pool->allocated_size += sizeof(memory_block_t) + size;
        
        return new_block->data;
    }
    
    return NULL; // Недостаточно памяти
}

/*
 * Освобождает ранее выделенный блок памяти обратно в пул
 * @param pool: указатель на пул памяти
 * @param ptr: указатель на блок памяти для освобождения
 */
void memory_pool_free(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return;
    
    // Вычисляем указатель на блок из указателя на данные
    memory_block_t *block = (memory_block_t *)((char *)ptr - sizeof(memory_block_t));
    
    // Добавляем в список свободных
    block->next = pool->free_list;
    pool->free_list = block;
}

/*
 * Уничтожает пул памяти и освобождает всю связанную с ним память
 * @param pool: указатель на пул памяти для уничтожения
 */
void destroy_memory_pool(memory_pool_t *pool) {
    if (!pool) return;
    
    free(pool->buffer);
    free(pool);
}