/*
 * memory-allocator.h - Memory allocator abstraction layer
 *
 * Provides unified interface for:
 * - Standard malloc/free (default)
 * - jemalloc (high performance, low fragmentation)
 * - tcmalloc (thread-caching malloc)
 *
 * Usage:
 *   #include "memory-allocator.h"
 *   void *ptr = mt_malloc(size);
 *   mt_free(ptr);
 */

#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// jemalloc support
#ifdef USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#define mt_malloc je_malloc
#define mt_calloc je_calloc
#define mt_realloc je_realloc
#define mt_free je_free
#define mt_mallocx je_mallocx
#define mt_dallocx je_dallocx
#define mt_malloc_usable_size je_malloc_usable_size

// tcmalloc support
#elif defined(USE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#define mt_malloc tc_malloc
#define mt_calloc tc_calloc
#define mt_realloc tc_realloc
#define mt_free tc_free
#define mt_mallocx(s, f) tc_malloc(s)
#define mt_dallocx(p, f) tc_free(p)
#define mt_malloc_usable_size tc_malloc_size

// Standard malloc (default)
#else
#include <stdlib.h>
#define mt_malloc malloc
#define mt_calloc calloc
#define mt_realloc realloc
#define mt_free free
#define mt_mallocx(s, f) malloc(s)
#define mt_dallocx(p, f) free(p)
#define mt_malloc_usable_size(ptr) 0
#endif

// Extended allocation flags
#define MT_MALLOC_ALIGN_16    0x00000001
#define MT_MALLOC_ALIGN_32    0x00000002
#define MT_MALLOC_ALIGN_64    0x00000004
#define MT_MALLOC_ZERO        0x00000008
#define MT_MALLOC_NO_RECLAIM  0x00000010

// Helper functions
static inline void* mt_malloc_aligned(size_t size, size_t alignment) {
#ifdef USE_JEMALLOC
    return je_aligned_alloc(alignment, size);
#elif defined(USE_TCMALLOC)
    // tcmalloc doesn't have aligned alloc, fallback to malloc
    void *ptr = tc_malloc(size);
    if (ptr && ((uintptr_t)ptr % alignment != 0)) {
        // Misaligned, allocate new and copy
        void *aligned = tc_malloc(size + alignment);
        if (aligned) {
            uintptr_t addr = (uintptr_t)aligned + alignment;
            addr &= ~(alignment - 1);
            void *aligned_ptr = (void*)addr;
            ((void**)aligned_ptr)[-1] = aligned;  // Store original pointer
            tc_free(ptr);
            return aligned_ptr;
        }
        tc_free(ptr);
        return NULL;
    }
    return ptr;
#else
    // Standard malloc - try to return aligned memory
    void *ptr = malloc(size + alignment);
    if (!ptr) return NULL;
    uintptr_t addr = (uintptr_t)ptr + alignment;
    addr &= ~(alignment - 1);
    ((void**)addr)[-1] = ptr;  // Store original pointer for free
    return (void*)addr;
#endif
}

static inline void mt_free_aligned(void *ptr) {
#ifdef USE_JEMALLOC
    je_free(ptr);
#elif defined(USE_TCMALLOC)
    // Check if we stored original pointer
    void *original = ((void**)ptr)[-1];
    if (original) {
        tc_free(original);
    } else {
        tc_free(ptr);
    }
#else
    // Retrieve original pointer
    void *original = ((void**)ptr)[-1];
    if (original) {
        free(original);
    } else {
        free(ptr);
    }
#endif
}

// Statistics and introspection
static inline size_t mt_get_allocated_size(void *ptr) {
    return mt_malloc_usable_size(ptr);
}

// jemalloc-specific: arena management
#ifdef USE_JEMALLOC
static inline unsigned mt_arena_create(void) {
    unsigned arena;
    size_t sz = sizeof(arena);
    if (je_mallctl("arenas.create", &arena, &sz, NULL, 0) == 0) {
        return arena;
    }
    return 0;
}

static inline void mt_arena_destroy(unsigned arena) {
    je_mallctl("arena.0.destroy", NULL, NULL, &arena, sizeof(arena));
}
#endif

// tcmalloc-specific: release memory to system
#ifdef USE_TCMALLOC
static inline void mt_release_free_memory(void) {
    tc_release_free_memory();
}
#endif

// Generic memory release (works for all allocators)
static inline void mt_compact_memory(void) {
#ifdef USE_TCMALLOC
    tc_release_free_memory();
#elif defined(USE_JEMALLOC)
    // Force garbage collection
    je_mallctl("arena.0.purge", NULL, NULL, NULL, 0);
#endif
    // Standard malloc: nothing to do
}

// Debug helpers
#ifdef DEBUG
#define MT_MALLOC_DEBUG(size) \
    do { \
        fprintf(stderr, "[MT_MALLOC] Allocating %zu bytes at %s:%d\n", \
                size, __FILE__, __LINE__); \
    } while(0)

#define MT_FREE_DEBUG(ptr) \
    do { \
        fprintf(stderr, "[MT_FREE] Freeing %p at %s:%d\n", \
                ptr, __FILE__, __LINE__); \
    } while(0)
#else
#define MT_MALLOC_DEBUG(size)
#define MT_FREE_DEBUG(ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif // MEMORY_ALLOCATOR_H
