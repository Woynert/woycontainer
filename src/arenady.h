/*
   Dynamic arena that grows automatically.

   WARNING: Do not save pointers. They'll be invalidated on growth.
   */
#ifndef ARENADY_H
#define ARENADY_H
#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "portable_utils.h"
#include <limits.h>
#define ARENADY_DEFAULT_CAPACITY 16


typedef struct ArenaDy {
    char *root; // Root.
    char *beg;
    char *end;
} ArenaDy;


static inline i64 arenady__pow2roundup (i64 x) {
    if (x < 0) { return 0; }
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32; // <- Only line needed for 64 integers.
    return x+1;
    // https://stackoverflow.com/a/365068
    // Round up to next higher power of 2 (return x if it's already a power of 2).
}


/// @Retuns Error.
int arenady__grow(ArenaDy *a, i64 minimum) {
    i64 new_cap = arenady__pow2roundup((a->end - a->root) + minimum);
    char *newbuf = (char *)realloc(a->root, (size_t)new_cap);
    if (newbuf == NULL) { printfd("OUT OF MEMORY."); return -1; }
    a->beg = newbuf + (a->beg - a->root);
    a->end = newbuf + new_cap;
    a->root = newbuf;
    return 0;
}

ArenaDy arenady_create(void) {
    ArenaDy a = { 0 };
    arenady__grow(&a, ARENADY_DEFAULT_CAPACITY);
    return a;
}

void arenady_free(ArenaDy *a) {
    if (a->root != NULL) { free(a->root); }
    *a = (ArenaDy) { 0 };
}

void *arenady_alloc(ArenaDy *a, i64 size, i64 align, i64 count)
{
    // Source https://nullprogram.com/blog/2023/09/27/
    // Question: Why the negative in the padding works?

    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    if (available <= 0 || count > available / size) {
        int err = arenady__grow(a, size * count + padding);
        if (err != 0) {
            printfd("OUT OF MEMORY.");
            wassert(false); // OOM.
        }
    }

    // @Note: Cleaning the padding will make binary comparisons possible.
    if (padding > 0) { memset(a->beg, 0, (size_t)padding); }
    void *p = a->beg + padding;
    a->beg += padding + count * size;
    //return memset(p, 0, (size_t)(count * size)); // <--
    return p;
}
#define arenady_new(arena, T, count) \
    (T *)arenady_alloc(arena, sizeof(T), _Alignof(T), count)


void arenady_reset_beginning(ArenaDy *a) { a->beg = a->root; }


bool arenady__can_fit(ArenaDy *a, i64 size, i64 align, i64 count) {
    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    return !(available <= 0 || count > available / size);
}
#define arenady_can_fit(arena, T, count)\
    arenady__can_fit(arena, sizeof(T), _Alignof(T), count)


/// Note: Read only, won't clean padding nor touch any byte.
void *arenady_try_get(ArenaDy *a, i64 size, i64 align, i64 count)
{
    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a->beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a->end - a->beg - padding;
    if (available <= 0 || count > available / size) { return NULL; }
    void *p = a->beg + padding;
    a->beg += padding + count * size;
    return p;
}
#define arenady_try_get_one(arena, T) \
    (T *)arenady_try_get(arena, sizeof(T), _Alignof(T), 1)

#endif // !ARENADY_H
