/*
    Ordered hash map.

    Features:

    * All operations are constant time O(1), for example: upsert, get, remove.
        * Degrades to linked list search on collisions.
    * Orderly forward and reverse iteration.
    * Good memory locality.
       * Uses struct of arrays (ECS like).
       * Operations only read what they need. (Most of the time).

    Example 1: Use binary comparison.

        #define WMAP__KEY  int
        #define WMAP__TYPE Car
        #define WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
        #include "wmap.h"

    Example 2: Use custom comparison and hash.

        #define WMAP__KEY  Human
        #define WMAP__TYPE Car
        #define WMAP__KEY_EQUAL Human_equals
        #define WMAP__KEY_HASH  Human_hash
        #include "wmap.h"

    Define WMAP__NAMESPACE to set custom struct prefix.
*/

#include <stddef.h>
#include <stdint.h>
#include "portable_utils.h"


#ifndef WMAP__UTILS
#define WMAP__UTILS

inline uint64_t wmap__hash(char *data, int size) {
    // https://nullprogram.com/blog/2025/01/19/
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < size; i++) {
        h ^= data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}


#endif

#undef SWAP
#define SWAP(a, b) do { typeof(a) bk = a; a = b; b = bk; } while(0)

#define WMAP__TOKCAT_(a, b) a ## b
#define WMAP__TOKCAT(a, b) WMAP__TOKCAT_(a, b)
#ifndef WMAP__NAMESPACE
#define WMAP__NAMESPACE WMAP__TOKCAT(UMap_, WMAP__TYPE)
#endif
#define WMAP__PFX(name) WMAP__TOKCAT(WMAP__TOKCAT(WMAP__NAMESPACE, _), name)
#define WMAP__PRI(name) WMAP__TOKCAT(WMAP__TOKCAT(WMAP__NAMESPACE, __), name)
#if (defined pub | defined pri | defined TYPE | defined WMap)
#error "These macros should not be defined: pub, pri, TYPE, WMap"
#endif

#if !defined WMAP__KEY || !defined WMAP__TYPE
    #define WMAP__KEY double
    #define WMAP__TYPE float
    #define WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
#endif

#ifdef WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
    #define WMAP__KEY_EQUAL WMAP__PRI(default_equal)
    #define WMAP__KEY_HASH  WMAP__PRI(default_hash)
#endif

#if !defined WMAP__KEY_EQUAL || !defined WMAP__KEY_HASH
    #error "WMAP__KEY_EQUAL or WMAP__KEY_HASH missing"
#endif


#define SLOT__TYPE WMAP__TYPE
#define SLOT__NAMESPACE WMAP__PRI(Slot_Value)
#include "slot.h"


#define WMAPINDEX__KEY  WMAP__KEY
#define WMAPINDEX__TYPE WMAP__TYPE
#define WMAPINDEX__NAMESPACE WMAP__PRI(Table)
#include "wmap_index.h"

#define pub WMAP__PFX
#define pri WMAP__PRI
#define KEY WMAP__KEY
#define TYPE WMAP__TYPE
#define WMap WMAP__NAMESPACE
#define WMAP__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define WMAP__DEFAULT_SIZE 8
#define WMAP__REHASH_FACTOR 0.9
#define WMAP__MAX_COUNT (INT_MAX -2)


typedef struct {
    pri(Table) table;
    pri(Slot_Value) values;
    void *userdata;        // Used for user provided hash/comparison functions.
} WMap;

typedef struct {
    TYPE *value;
    KEY key;
    ID __id;
    ID __value_id;
} pub(It);



int     pub(create)(WMap *m);
int     pub(create_with_allocator)(WMap *m, WMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
void    pub(set_userdata)(WMap *m, void *data) { m->userdata = data; }
void    pub(free)(WMap *m);
int     pub(upsert)(WMap *m, KEY key, TYPE item);
void    pub(remove)(WMap *m, KEY key);
TYPE *  pub(get)(WMap *m, KEY key);
bool    pub(it_next)(const WMap *m, pub(It) *it);
bool    pub(it_prev)(const WMap *m, pub(It) *it);
pub(It) pub(make_it)(const WMap *m) { return (pub(It)) { .__id = m->table.first_node, }; }
pub(It) pub(make_it_end)(const WMap *m) { return (pub(It)) { .__id = m->table.last_node, }; }
int     pub(pair_count)(const WMap *m) { return m->table.pair_count; }

int  pri(rehash_if_needed)(WMap *old_m);
int  pri(find)(WMap *m, KEY key, ID *out_prev_id, ID *out_id);
bool pri(default_equal)(KEY a, KEY b, void *data) { (void)data; return 0 == memcmp(&a, &b, sizeof(a)); }
uint64_t pri(default_hash)(KEY k, void *data) { (void)data; return wmap__hash((char*)&k, sizeof(k)); }



/// @Returns error.
int pub(create_with_allocator)(WMap *m, WMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (WMap) { 0 };
    pri(Table_create_with_allocator)(&m->table, allocator, allocator_userdata);
    pri(Slot_Value_create_with_allocator)(&m->values, allocator, allocator_userdata);
    return 0;
}

int pub(create)(WMap *m) { return pub(create_with_allocator)(m, NULL, NULL); }

void pub(free)(WMap *m) {
    pri(Table_free)(&m->table);
    pri(Slot_Value_free)(&m->values);
    *m = (WMap) { 0 };
}


static inline ID pri(table_get_bucket)(pri(Table) *t, uint64_t hash) {
    return t->bucket_count == 0 ? ID_INVALID : ID_make((int)(hash & (uint64_t)(t->bucket_count - 1)));
    // bucket_count must be power of 2.
}


int pri(rehash_if_needed)(WMap *old_m) {
    pri(Table) *old_table = &old_m->table;
    int new_bucket_count = old_table->bucket_count * 2;
    if (new_bucket_count == 0) { new_bucket_count = WMAP__DEFAULT_SIZE; }
    else {
        const float factor = (float)old_table->pair_count / (float)old_table->bucket_count;
        if (factor < WMAP__REHASH_FACTOR) { return 0; }
    }

    // Create new map.
    WMap __new_map;
    WMap *new_m = &__new_map;
    pub(create_with_allocator)(new_m, old_table->keys.allocator, old_table->keys.allocator_userdata);
    int err = pri(Table_grow)(&new_m->table, new_bucket_count);
    if (err) { pub(free)(new_m); printferr("OOM?"); return -1; }
    new_m->table.bucket_count = new_m->table.capacity;
    new_m->userdata = old_m->userdata;

    // Rehash keys.
    pub(It) it = pub(make_it)(old_m);
    while (pub(it_next)(old_m, &it)) {
        // @Note: Keep this procedure in sync with 'upsert' function.
        ID i_prev = ID_INVALID, i = ID_INVALID;
        bool found = 0 == pri(find)(new_m, it.key, &i, &i_prev);
        if (found) { printferr("wtf: Key already exists."); goto quit_abort; }
        if (ID_get(i) >= new_m->table.capacity) {
            err = pri(Table_grow)(&new_m->table, new_m->table.capacity * 2);
            if (err) { printferr("OOM?"); goto quit_abort; }
        }
        pri(Table_set_new_pair)(&new_m->table, i_prev, i, it.key, it.__value_id);
    }

    // Swap data and free.
    SWAP(new_m->values, old_m->values);
    pub(free)(old_m);
    *old_m = *new_m;

    if ((0)) {
        quit_abort:
        pub(free)(new_m);
        return -1;
    }
    return 0;
}


/// @Param[out] out_id. If found, corresponding id, else id where you should insert it.
/// @Param[out] out_prev_id. Optional.
/// @Returns 0 if found. -1 if not.
int pri(find)(WMap *m, KEY key, ID *out_id, ID *out_prev_id) {
    ID prev_id = ID_INVALID;
    ID id = pri(table_get_bucket)(&m->table, WMAP__KEY_HASH(key, m->userdata));
    do {
        if (!ID_valid(id) || pri(Table_slot_is_empty)(&m->table, id)) { break; }
        do {
            if (WMAP__KEY_EQUAL(m->table.keys.items[ID_get(id)], key, m->userdata)) {
                if (out_prev_id) { *out_prev_id = prev_id; }
                *out_id = id;
                return 0;
            }
            prev_id = id;
            id = m->table.bucket_next.items[ID_get(id)];
        } while(ID_valid(id));
    } while(0);
    id = pri(Table_if_invalid_get_next_valid_id)(&m->table, id);
    if (out_prev_id) { *out_prev_id = prev_id; }
    *out_id = id;
    return -1;
}


/// @Return Error.
/// @Note: Keep this function in sync with 'rehash insert'.
int pub(upsert)(WMap *m, KEY key, TYPE item) {
    ID i_prev = ID_INVALID, i = ID_INVALID;
    bool found = 0 == pri(find)(m, key, &i, &i_prev);
    if (found) {
        // Update.
        pri(Slot_Value_update)(&m->values, ID_get(m->table.val_ids.items[ID_get(i)]), item);
        return 0;
    }
    if (ID_get(i) >= m->table.capacity) {
        int new_cap = m->table.capacity == 0 ? WMAP__DEFAULT_SIZE : m->table.capacity * 2;
        int err = pri(Table_grow)(&m->table, new_cap);
        if (err) { return -1; }
    }
    ID value_id = ID_make(pri(Slot_Value_append)(&m->values, item));
    if (!ID_valid(value_id)) { printferr("OOM?."); return -1; }
    pri(Table_set_new_pair)(&m->table, i_prev, i, key, value_id);
    if (pri(rehash_if_needed)(m)) { printfd("W: Rehashing failed."); }
    return 0;
}


void pub(remove)(WMap *m, KEY key) {
    ID i = ID_INVALID;
    bool found = 0 == pri(find)(m, key, &i, NULL);
    if (!found) { return; }
    int err = pri(Slot_Value_pop)(&m->values, ID_get(m->table.val_ids.items[ID_get(i)]));
    if (err) { printferr("WAR: Couldn't fully remove."); }
    pri(Table_remove)(&m->table, i);
}


TYPE * pub(get)(WMap *m, KEY key) {
    ID i = ID_INVALID;
    bool found = 0 == pri(find)(m, key, &i, NULL);
    if (!found) { return NULL; }
    return pri(Slot_Value_get)(&m->values, ID_get(m->table.val_ids.items[ID_get(i)]));
}


/// @Note. Modifying the map while iterating is UB.
bool pub(it_next)(const WMap *m, pub(It) *it) {
    const pri(Table) *t = &m->table;
    while (ID_valid(it->__id)) {
        it->key        = t->keys.items[ID_get(it->__id)];
        it->value      = pri(Slot_Value_get)(&m->values, ID_get(t->val_ids.items[ID_get(it->__id)]));
        it->__value_id = t->val_ids.items[ID_get(it->__id)];
        it->__id       = t->next.items[ID_get(it->__id)];
        return true;
    }
    return false;
}

/// @Note. Modifying the map while iterating is UB.
bool pub(it_prev)(const WMap *m, pub(It) *it) {
    const pri(Table) *t = &m->table;
    while (ID_valid(it->__id)) {
        it->key        = t->keys.items[ID_get(it->__id)];
        it->value      = pri(Slot_Value_get)(&m->values, ID_get(t->val_ids.items[ID_get(it->__id)]));
        it->__value_id = t->val_ids.items[ID_get(it->__id)];
        it->__id       = t->prev.items[ID_get(it->__id)];
        return true;
    }
    return false;
}



#undef pub
#undef pri
#undef KEY
#undef TYPE
#undef WMap
#undef WMAP__ALLOC_PROTOTYPE
#undef WMAP__DEFAULT_SIZE
#undef WMAP__REHASH_FACTOR
#undef WMAP__MAX_COUNT
