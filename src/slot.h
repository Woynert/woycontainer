/*
    Data structure: Slot, or Stable Index Array (SIV), or Sparse Set?

    FEATURES:
    * Pro: Fast insert.
    * Pro: Fast delete.
    * Pro: Stable ids.
    * Pro: Growable.
    * Pro: Valid items are contiguous.
    * Con: Order not preserved.
*/

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SLOT__TYPE
#define SLOT__TYPE int
#endif

/* Token concatenation. */
#define SLOT__TOKCAT_(a, b) a ## b
#define SLOT__TOKCAT(a, b) SLOT__TOKCAT_(a, b)
#ifndef SLOT__NAMESPACE
#define SLOT__NAMESPACE SLOT__TOKCAT(SLOT__TYPE, _Slot)
#endif

#define pfx(name) SLOT__TOKCAT(SLOT__TOKCAT(SLOT__NAMESPACE, _), name)
#define TYPE SLOT__TYPE
#define Slot SLOT__NAMESPACE

#define SLOT__ALLOC_PROTOTYPE(x) void* (x) (void* user_data, void* ptr, ssize_t size, int align)
static SLOT__ALLOC_PROTOTYPE(pfx(_default_allocator));


typedef struct Slot {
    int capacity;
    int count;

    int *userid_to_itemid; // Maps a stable id to an internal id (which we are allowed to change).
    int *itemid_to_userid;
    TYPE *items;

    SLOT__ALLOC_PROTOTYPE(*allocator);
    void *allocator_user_data;
} Slot;


// @Returns error.
int pfx(_grow)(Slot *s, int new_capacity) {

    SLOT__ALLOC_PROTOTYPE(*allocator) = s->allocator != NULL ? s->allocator : pfx(_default_allocator);

    bool free_on_failure = s->items == NULL; // In case it's first allocation. (Malloc).

    int* new_userid_to_itemid = (int*) allocator(s->allocator_user_data, s->userid_to_itemid, (ssize_t)new_capacity * (ssize_t)sizeof(int), alignof(int));
    int* new_itemid_to_userid = (int*) allocator(s->allocator_user_data, s->itemid_to_userid, (ssize_t)new_capacity * (ssize_t)sizeof(int), alignof(int));
    TYPE* new_items           = (TYPE*)allocator(s->allocator_user_data, s->items           , (ssize_t)new_capacity * (ssize_t)sizeof(TYPE), alignof(TYPE));

    if (new_userid_to_itemid == NULL
        || new_itemid_to_userid == NULL
        || new_items == NULL
    ) {
        if (free_on_failure) {
            if (new_userid_to_itemid != NULL) { allocator(s->allocator_user_data, new_userid_to_itemid, 0, 0); }
            if (new_itemid_to_userid != NULL) { allocator(s->allocator_user_data, new_itemid_to_userid, 0, 0); }
            if (new_items            != NULL) { allocator(s->allocator_user_data, new_items           , 0, 0); }
        }
        return -1;
    }

    // Update available free slots.
    for (int i = s->capacity; i < new_capacity; ++i) {
        new_itemid_to_userid[i] = i;
        new_userid_to_itemid[i] = i;
    }

    s->capacity   = new_capacity;
    s->userid_to_itemid = new_userid_to_itemid;
    s->itemid_to_userid  = new_itemid_to_userid;
    s->items      = new_items;
    return 0;
}


// @Returns error.
int pfx(create)(Slot *s) {
    *s = (Slot) { 0 };
    return pfx(_grow)(s, 2); // DEFAULT CAPACITY.
}

void pfx(free)(Slot *s) {
    pfx(_grow)(s, 0);
    *s = (Slot) { 0 };
}

void pfx(print_debug)(const Slot *s) {
    printf("\ncap %d count %d", s->capacity, s->count);
    printf("\nuserid_to_itemid ");
    for (int i = 0; i < s->capacity; ++i) {
        printf("%04d ", s->userid_to_itemid[i]);
    }
    printf("\nitemid_to_userid ");
    for (int i = 0; i < s->capacity; ++i) {
        printf("%04d ", s->itemid_to_userid[i]);
    }
    printf("\n");
}


// @Returns item id, or -1 on error.
int pfx(append)(Slot *s, TYPE item) {
    // Space left?
    if (s->count >= s->capacity) {
        int err = pfx(_grow)(s, s->capacity * 2);
        if (err != 0) { return -1; }
    }

    int item_id = s->count;
    s->items[item_id] = item;
    ++s->count;
    return s->itemid_to_userid[item_id];
}


// @Returns TYPE or NULL if not found.
TYPE *pfx(get)(Slot *s, int user_id) {
    if (user_id < 0 || user_id > s->capacity) { return NULL; }
    int item_id = s->userid_to_itemid[user_id];
    if (item_id > s->count) { return NULL; }
    return &s->items[item_id];
}


/// @Returns error.
int pfx(pop)(Slot *s, int user_id) {
    if (user_id < 0 || user_id > s->capacity) { return -1; }
    if (s->count <= 0) { return -1; }
    // Swap items.
    int item_id = s->userid_to_itemid[user_id];
    s->items[item_id] = s->items[s->count -1];
    //printf("[ %d %d ]\n", item_id, s->count -1);
    // Swap itemid_to_userid.
    int bk = s->itemid_to_userid[item_id];
    s->itemid_to_userid[item_id] = s->itemid_to_userid[s->count -1];
    s->itemid_to_userid[s->count -1] = bk;
    // Update userid_to_itemid.
    s->userid_to_itemid[s->itemid_to_userid[item_id]] = item_id;
    s->userid_to_itemid[s->itemid_to_userid[s->count -1]] = s->count -1;
    // Pop.
    --s->count;
    return 0;
}


static SLOT__ALLOC_PROTOTYPE(pfx(_default_allocator)) {
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


#undef SLOT__TYPE
#undef SLOT__TOKCAT_
#undef SLOT__TOKCAT
#undef SLOT__NAMESPACE
#undef pfx
#undef TYPE
#undef Slot
#undef SLOT__ALLOC_PROTOTYPE
