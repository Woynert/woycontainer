/*
   Fixed size array container. Can be resized to any desired size.
  
   Usage:
  
   #define ARRAY__TYPE <type>
   #define ARRAY__NAMESPACE <custom name> (optional)
   #define ARRAY__ENABLE_COMPARISONS      (optional)
   #define ARRAY__IS_BUFFER               (optional)
   #include "sizedbuffer.h"
*/

#include <limits.h>
#include <stdalign.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* User didn't specify type, using default. */
#ifndef ARRAY__TYPE
#define ARRAY__TYPE uint
#define ARRAY__ENABLE_COMPARISONS
#endif

/* Token concatenation. */
#define ARRAY__TOKCAT_(a, b) a ## b
#define ARRAY__TOKCAT(a, b) ARRAY__TOKCAT_(a, b)
#ifndef ARRAY__NAMESPACE
#define ARRAY__NAMESPACE ARRAY__TOKCAT(ARRAY__TYPE, _Array)
#endif

#if (defined(pfx) | defined(TYPE) | defined(Array))
#error "These macros should not be defined: pfx, TYPE, Array"
#endif
#define pfx(name) ARRAY__TOKCAT(ARRAY__TOKCAT(ARRAY__NAMESPACE, _), name)
#define TYPE ARRAY__TYPE
#define Array ARRAY__NAMESPACE

#define ARRAY__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)

typedef struct {
    int size;
    TYPE *items;

    ARRAY__ALLOC_PROTOTYPE(*allocator);
    void *allocator_userdata;
} Array;


static Array    pfx(create)                (void);
static Array    pfx(create_with_allocator) (ARRAY__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
static void     pfx(destroy)               (Array *a);
static int      pfx(resize)                (Array *a, int new_capacity);
static int      pfx(grow_zero)             (Array *a, int min_capacity);
static int      pfx(grow_trash)            (Array *a, int min_capacity);
static void     pfx(fill_zero)             (Array *a);
static void     pfx(fill_with_value)       (Array *a, TYPE item);
static int      pfx(set_safe)              (Array *a, int index, TYPE item);
static TYPE *   pfx(get_safe)              (const Array *a, int index);
static          ARRAY__ALLOC_PROTOTYPE(pfx(_default_allocator));


static inline Array pfx(create) (void) {
    return pfx(create_with_allocator) (NULL, NULL);
}


static Array pfx(create_with_allocator) (ARRAY__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    return (Array) {
        .allocator = allocator,
        .allocator_userdata = allocator_userdata,
    };
}


static inline void pfx(destroy) (Array *a) {
    if (a->items != NULL) {
        ARRAY__ALLOC_PROTOTYPE(*allocator) = a->allocator != NULL ? a->allocator : pfx(_default_allocator);
        // Free.
        allocator(a->items, 0, 0, a->allocator_userdata);
    }
    *a = (Array) { 0 };
}


/// @Note. Can't resize to 0. For that use destroy().
/// @returns error
static int pfx(resize) (Array *a, int new_size) {
    if (new_size <= 0) { return -1; }
    if (a->size == new_size) { return 0; }

    ARRAY__ALLOC_PROTOTYPE(*allocator) = a->allocator != NULL ? a->allocator : pfx(_default_allocator);

    TYPE *new_ptr = NULL;
    new_ptr = allocator(a->items, sizeof(TYPE) * (size_t)new_size, alignof(TYPE), a->allocator_userdata);
    if (new_ptr == NULL) { return -1; }

    a->size = new_size;
    a->items = new_ptr;
    return 0;
}


/* @returns Error */
static inline int pfx(grow_zero) (Array *a, int minimum_size) {
    if (a->size >= minimum_size) { return 0; }
    
    int old_size = a->size;

    int err = pfx(resize)(a, minimum_size);
    if (err != 0) { return -1; }

    memset(a->items + old_size, 0, sizeof(TYPE) * (size_t)(a->size - old_size));

    return err;
}


/* @returns Error */
static inline int pfx(grow_trash) (Array *a, int minimum_size) {
    if (a->size >= minimum_size) { return 0; }
    return pfx(resize)(a, minimum_size);
}


static void pfx(fill_zero) (Array *a) {
    memset(a->items, 0, sizeof(TYPE) * (size_t)a->size);
}


static void pfx(fill_with_value) (Array *a, TYPE item) {
    for (int i = 0; i < a->size; ++i) {
        a->items[i] = item;
    }
}


/* @note You can directly access items instead too. */
/* @returns Error. */
static int pfx(set_safe) (Array *a, int index, TYPE item) {
    if (index < 0 || index >= a->size) { return -1; }
    a->items[index] = item;
    return 0;
}


/* @note Safe get which checks boundares */
/* @note You can directly access items instead too. */
/* @returns Pointer or NULL. */
static TYPE *pfx(get_safe) (const Array *a, int index) {
    if (index < 0 || index >= a->size) { return NULL; }
    return &a->items[index];
}


static ARRAY__ALLOC_PROTOTYPE(pfx(_default_allocator)) {
    (void)align; (void)user_data; // Unused: malloc guarantees alignment.

    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0

    void* result = NULL;
    if (size == 0) {
        free(ptr);
    } else {
        if (ptr == NULL) {
            result = malloc((size_t)size);
        } else {
            result = realloc(ptr, (size_t)size);
        }
    }
    return result;
}


#undef pfx
#undef TYPE
#undef Array
#undef ARRAY__TYPE
#undef ARRAY__NAMESPACE
#undef ARRAY__ENABLE_COMPARISONS
#undef ARRAY__TOKCAT_
#undef ARRAY__TOKCAT
