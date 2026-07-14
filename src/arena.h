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
    char * beg;
    char * end;
} ArenaRoot;


typedef struct Arena {
    char *beg;
    char *end;
} Arena;


ArenaRoot ArenaRoot_create(i64 cap) {
    char *beg = (char *)malloc((size_t)cap);
    char *end = beg ? beg + cap : 0;
    return (ArenaRoot) { .beg = beg, .end = end };
}


void ArenaRoot_free(ArenaRoot *root) {
    if (root->beg == NULL) { return; }
    free(root->beg);
    memset(root, 0, sizeof(ArenaRoot)); // Invalidate it.
}


Arena ArenaRoot_get_arena(ArenaRoot root) {
    return (Arena) {.beg = root.beg, .end = root.end};
}


void *arena_alloc(Arena *a, i64 size, i64 align, i64 count)
{
    // Source https://nullprogram.com/blog/2023/09/27/
    // Question: Why the negative in the padding works?

    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    if (available <= 0 || count > available / size) {
        printf("OUT OF MEMORY.");
        //return NULL;
        wassert(false); // OOM.
    }
    void *p = a->beg + padding;
    a->beg += padding + count * size;
    return memset(p, 0, (size_t)(count * size));
}


#define arena_new_align(arena, T, align_T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(align_T), count)

#define arena_new(arena, T, count) \
    (T *)arena_alloc(arena, sizeof(T), _Alignof(T), count)


#endif // !ARENA_H
