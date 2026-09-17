#ifndef ARENA_H
#define ARENA_H

#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "portable_utils.h"
#include <limits.h>



typedef struct ArenaRoot {
    char *buf;
    i64 cap;
} ArenaRoot;


typedef struct Arena {
    char *beg;
    char *end;
} Arena;


ArenaRoot ArenaRoot_create(i64 cap) {
    char *buf = (char *)malloc((size_t)cap);
    return (ArenaRoot) {
        .buf = buf,
        .cap = buf ? cap : 0,
    };
}


void ArenaRoot_free(ArenaRoot *r) {
    if (r->buf == NULL) { return; }
    free(r->buf);
    *r = (ArenaRoot) { 0 };
}


Arena ArenaRoot_get_arena(ArenaRoot r) {
    return (Arena) {.beg = r.buf, .end = r.buf ? r.buf + r.cap : NULL};
}


void *arena_alloc(Arena *a, i64 size, i64 align, i64 count)
{
    // Source https://nullprogram.com/blog/2023/09/27/
    // Question: Why the negative in the padding works?
    // TODO: Realloc flag to expand the last allocated size.

    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    if (available <= 0 || count > available / size) {
        printfd("OUT OF MEMORY.");
        //return NULL;
        wassert(false); // OOM.
    }
    void *p = a->beg + padding;
    a->beg += padding + count * size;
    return memset(p, 0, (size_t)(count * size));
}
#define arena_new_align(arena, T, align_T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(align_T), (count))
#define arena_new(arena, T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(T), (count))


/*
bool arena__can_fit(Arena *a, i64 size, i64 align, i64 count) {
    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    return !(available <= 0 || count > available / size);
}
#define arena_can_fit(arena, T, count)\
    arena__can_fit(arena, sizeof(T), _Alignof(T), count)
*/


static void* arena_allocator(void* ptr, size_t size, int align, void* user_data) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0

    if (size == 0) return NULL; // No freeing for arena.

    Arena *arena = (Arena*)user_data;
    void *result = arena_alloc(arena, sizeof(char), align, (i64)size);

    if (ptr != NULL) {
        // Reallocation must copy what we had.
        // Don't use memcpy.
        memmove(result, ptr, size);
    }

    return result;
}



#endif // !ARENA_H
