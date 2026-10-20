// arena.h - v1.00 - public domain - Simple arena.
// Woynert 2026
//
// Canonical home for this library:
//     https://github.com/woynert/woycontainer
//
// Version history:
//     0.01: (2026-10-17) Initial release.
//

#ifndef ARENA_H
#define ARENA_H

#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#include "limits.h"

typedef int64_t i64;

typedef struct ArenaRoot {
    char *buf;
    i64 cap;
} ArenaRoot;


typedef struct Arena {
    char *beg;
    char *end;
} Arena;


static ArenaRoot ArenaRoot_create(i64 cap) {
    char *buf = (char *)malloc((size_t)cap);
    return (ArenaRoot) {
        .buf = buf,
        .cap = buf ? cap : 0,
    };
}


static void ArenaRoot_free(ArenaRoot *r) {
    if (r->buf == NULL) { return; }
    free(r->buf);
    *r = (ArenaRoot) { 0 };
}


static Arena ArenaRoot_get_arena(ArenaRoot r) {
    return (Arena) {.beg = r.buf, .end = r.buf ? r.buf + r.cap : NULL};
}


static void *arena_alloc(Arena *a, i64 size, i64 align, i64 count)
{
    // Source https://nullprogram.com/blog/2023/09/27/
    // Question: Why the negative in the padding works?
    // TODO: Realloc flag to expand the last allocated size.

    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    if (available <= 0 || count > available / size) {
        printf("OUT OF MEMORY.\n");
        abort();
    }
    void *p = a->beg + padding;
    a->beg += padding + count * size;
    return memset(p, 0, (size_t)(count * size));
}
#define arena_new_align(arena, T, align_T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(align_T), (count))
#define arena_new(arena, T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(T), (count))


static bool arena__can_fit(Arena *a, i64 size, i64 align, i64 count) {
    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    return !(available <= 0 || count > available / size);
}
#define arena_can_fit(arena, T, count)\
    arena__can_fit(arena, sizeof(T), _Alignof(T), count)


static void* arena_allocator(void* ptr, size_t size, int align, void* user_data) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0
    if (size == 0) return NULL; // No freeing for arena.
    Arena *arena = (Arena*)user_data;
    void *result = arena_alloc(arena, sizeof(char), align, (i64)size);
    if (ptr != NULL) {
        memmove(result, ptr, size);
        // Reallocation must copy what we had. Don't use memcpy.
    }
    return result;
}


#endif // !ARENA_H
