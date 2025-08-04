#include "utils/memory.h"
#include "utils/error.h"
#include <stdint.h>
#include <pthread.h>

#ifndef BONGOCAT_DISABLE_MEMORY_STATISTICS
static memory_stats_t g_memory_stats = {0};
static pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef DEBUG
typedef struct allocation_record {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    struct allocation_record *next;
} allocation_record_t;

static allocation_record_t *allocations = NULL;
#endif

void* bongocat_malloc(size_t size) {
    if (size == 0) {
        BONGOCAT_LOG_WARNING("Attempted to allocate 0 bytes");
        return NULL;
    }
    
    void *ptr = malloc(size);
    if (!ptr) {
        BONGOCAT_LOG_ERROR("Failed to allocate %zu bytes", size);
        return NULL;
    }

#ifndef BONGOCAT_DISABLE_MEMORY_STATISTICS
    pthread_mutex_lock(&memory_mutex);
    g_memory_stats.total_allocated += size;
    g_memory_stats.current_allocated += size;
    if (g_memory_stats.current_allocated > g_memory_stats.peak_allocated) {
        g_memory_stats.peak_allocated = g_memory_stats.current_allocated;
    }
    g_memory_stats.allocation_count++;
    pthread_mutex_unlock(&memory_mutex);
#endif
    
    return ptr;
}

void* bongocat_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) {
        BONGOCAT_LOG_WARNING("Attempted to allocate 0 bytes");
        return NULL;
    }
    
    // Check for overflow
    if (count > SIZE_MAX / size) {
        BONGOCAT_LOG_ERROR("Integer overflow in calloc");
        return NULL;
    }
    
    void *ptr = calloc(count, size);
    if (!ptr) {
        BONGOCAT_LOG_ERROR("Failed to allocate %zu bytes", count * size);
        return NULL;
    }

#ifndef BONGOCAT_DISABLE_MEMORY_STATISTICS
    pthread_mutex_lock(&memory_mutex);
    size_t total_size = count * size;
    g_memory_stats.total_allocated += total_size;
    g_memory_stats.current_allocated += total_size;
    if (g_memory_stats.current_allocated > g_memory_stats.peak_allocated) {
        g_memory_stats.peak_allocated = g_memory_stats.current_allocated;
    }
    g_memory_stats.allocation_count++;
    pthread_mutex_unlock(&memory_mutex);
#endif
    
    return ptr;
}

void* bongocat_realloc(void *ptr, size_t size) {
    if (size == 0) {
        bongocat_free(ptr);
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        BONGOCAT_LOG_ERROR("Failed to reallocate to %zu bytes", size);
        return NULL;
    }
    
    // Note: We can't track size changes accurately without storing original sizes
    // This is a limitation of this simple tracking system
    
    return new_ptr;
}

void bongocat_free(void *ptr) {
    if (!ptr) return;
    
    free(ptr);

#ifndef BONGOCAT_DISABLE_MEMORY_STATISTICS
    pthread_mutex_lock(&memory_mutex);
    g_memory_stats.free_count++;
    pthread_mutex_unlock(&memory_mutex);
#endif
}

memory_pool_t* memory_pool_create(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) {
        BONGOCAT_LOG_ERROR("Invalid memory pool parameters");
        return NULL;
    }
    
    memory_pool_t *pool = bongocat_malloc(sizeof(memory_pool_t));
    if (!pool) return NULL;
    
    pool->data = bongocat_malloc(size);
    if (!pool->data) {
        bongocat_free(pool);
        return NULL;
    }
    
    pool->size = size;
    pool->used = 0;
    pool->alignment = alignment;
    
    return pool;
}

void* memory_pool_alloc(memory_pool_t *pool, size_t size) {
    if (!pool || size == 0) return NULL;
    
    // Align the size
    size_t aligned_size = (size + pool->alignment - 1) & ~(pool->alignment - 1);
    
    if (pool->used + aligned_size > pool->size) {
        BONGOCAT_LOG_ERROR("Memory pool exhausted");
        return NULL;
    }
    
    void *ptr = (char*)pool->data + pool->used;
    pool->used += aligned_size;
    
    return ptr;
}

void memory_pool_reset(memory_pool_t *pool) {
    if (pool) {
        pool->used = 0;
    }
}

void memory_pool_destroy(memory_pool_t *pool) {
    if (pool) {
        bongocat_free(pool->data);
        bongocat_free(pool);
    }
}

#ifndef BONGOCAT_DISABLE_MEMORY_STATISTICS
void memory_get_stats(memory_stats_t *stats) {
    if (!stats) return;
    
    pthread_mutex_lock(&memory_mutex);
    *stats = g_memory_stats;
    pthread_mutex_unlock(&memory_mutex);
}

void memory_print_stats(void) {
    memory_stats_t stats;
    memory_get_stats(&stats);
    
    bongocat_log_info("Memory Statistics:");
    bongocat_log_info("  Total allocated: %zu bytes (%.2f MB)", stats.total_allocated, stats.total_allocated / (1024.0 * 1024.0));
    bongocat_log_info("  Current allocated: %zu bytes (%.2f MB)", stats.current_allocated, stats.current_allocated / (1024.0 * 1024.0));
    bongocat_log_info("  Peak allocated: %zu bytes (%.2f MB)", stats.peak_allocated, stats.peak_allocated / (1024.0 * 1024.0));
    bongocat_log_info("  Allocations: %zu", stats.allocation_count);
    bongocat_log_info("  Frees: %zu", stats.free_count);
    bongocat_log_info("  Potential leaks: %zu", stats.allocation_count - stats.free_count);
}
#endif

#ifdef DEBUG
void* bongocat_malloc_debug(size_t size, const char *file, int line) {
    void *ptr = bongocat_malloc(size);
    if (!ptr) return NULL;
    
    allocation_record_t *record = malloc(sizeof(allocation_record_t));
    if (record) {
        record->ptr = ptr;
        record->size = size;
        record->file = file;
        record->line = line;
        record->next = allocations;
        allocations = record;
    }
    
    return ptr;
}

void bongocat_free_debug(void *ptr, const char *file, int line) {
    if (!ptr) return;
    
    allocation_record_t **current = &allocations;
    while (*current) {
        if ((*current)->ptr == ptr) {
            allocation_record_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            break;
        }
        current = &(*current)->next;
    }
    
    bongocat_free(ptr);
}

void memory_leak_check(void) {
    if (!allocations) {
        bongocat_log_info("No memory leaks detected");
        return;
    }
    
    BONGOCAT_LOG_ERROR("Memory leaks detected:");
    allocation_record_t *current = allocations;
    while (current) {
        BONGOCAT_LOG_ERROR("  %zu bytes at %s:%d", current->size, current->file, current->line);
        current = current->next;
    }
}
#endif