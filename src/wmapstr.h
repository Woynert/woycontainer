/*
   Ordered hash map with strings as keys.
   */

#ifndef WMAPSTR__TYPE
#define WMAPSTR__TYPE double
#endif

#include "strpool.h"
#include "wstrview.h"
#include "portable_utils.h"

#ifndef WMAPSTR_H__UTILS
#define WMAPSTR_H__UTILS
uint64_t wmapstr__hash(strview_t view) {
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < view.size; i++) {
        h ^= view.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
    // https://nullprogram.com/blog/2025/01/19/
}

bool wmapstr__noop(int a, int b, void *data) {
    (void)a; (void)b; (void)data; wassert_msg(false, "Should never reach.");
}

#endif


#define WMAPSTR__TOKCAT_(a, b) a ## b
#define WMAPSTR__TOKCAT(a, b) WMAPSTR__TOKCAT_(a, b)
#ifndef WMAPSTR__NAMESPACE
#define WMAPSTR__NAMESPACE WMAPSTR__TOKCAT(Strumap_, WMAPSTR__TYPE)
#endif
#define WMAPSTR__PFX(name) WMAPSTR__TOKCAT(WMAPSTR__TOKCAT(WMAPSTR__NAMESPACE, _), name)
#define WMAPSTR__PRI(name) WMAPSTR__TOKCAT(WMAPSTR__TOKCAT(WMAPSTR__NAMESPACE, __), name)

#define SLOT__TYPE WMAPSTR__TYPE
#define SLOT__NAMESPACE WMAPSTR__PRI(Slot_Value)
#include "slot.h"

#define WMAPINDEX__KEY  ID
#define WMAPINDEX__TYPE WMAPSTR__TYPE
#define WMAPINDEX__NAMESPACE WMAPSTR__PRI(Table)
#include "wmap_index.h"

#define pub WMAPSTR__PFX
#define pri WMAPSTR__PRI
#define KEY WMAPSTR__KEY
#define TYPE WMAPSTR__TYPE
#define Wmapstr WMAPSTR__NAMESPACE
#define WMAPSTR__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define WMAPSTR__DEFAULT_SIZE 8
#define WMAPSTR__REHASH_FACTOR 0.9


typedef struct {
    pri(Table) table;
    pri(Slot_Value) values;
    Strpool strpool;
} Wmapstr;

typedef struct pub(It) {
    TYPE *value;
    strview_t key;
    ID __key_id;
    ID __id;
    ID __value_id;
} pub(It);


int     pub(create)(Wmapstr *m);
int     pub(create_with_allocator)(Wmapstr *m, WMAPSTR__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
void    pub(free)(Wmapstr *m);
void    pub(clear)(Wmapstr *m);
int     pub(upsert)(Wmapstr *m, strview_t key, TYPE item);
void    pub(remove)(Wmapstr *m, strview_t key);
TYPE *  pub(get)(const Wmapstr *m, strview_t key);
bool    pub(it_next)(const Wmapstr *m, pub(It) *it);
bool    pub(it_prev)(const Wmapstr *m, pub(It) *it);
pub(It) pub(make_it)(const Wmapstr *m) { return (pub(It)) { .__id = m->table.first_node, }; }
pub(It) pub(make_it_end)(const Wmapstr *m) { return (pub(It)) { .__id = m->table.last_node, }; }
int     pub(pair_count)(const Wmapstr *m) { return m->table.pair_count; }

int  pri(rehash_if_needed)(Wmapstr *old_m);
int  pri(find)(const Wmapstr *m, strview_t key, ID *out_id, ID *out_prev_id);



int pub(create_with_allocator)(Wmapstr *m, WMAPSTR__ALLOC_PROTOTYPE(*allocator), void *alloc_userdata) {
    *m = (Wmapstr) { 0 };
    pri(Table_create_with_allocator)(&m->table, allocator, alloc_userdata);
    pri(Slot_Value_create_with_allocator)(&m->values, allocator, alloc_userdata);
    strpool_create_with_allocator(&m->strpool, allocator, alloc_userdata);
    return 0;
}

int pub(create)(Wmapstr *m) { return pub(create_with_allocator)(m, NULL, NULL); }

void pub(free)(Wmapstr *m) {
    pri(Table_free)(&m->table);
    pri(Slot_Value_free)(&m->values);
    strpool_destroy(&m->strpool);
    *m = (Wmapstr) { 0 };
}

void pub(clear)(Wmapstr *m) {
    pri(Table_clear)(&m->table);
    pri(Slot_Value_clear)(&m->values);
    strpool_clear(&m->strpool);
}


int pri(rehash_if_needed)(Wmapstr *old_m) {
    pri(Table) *old_table = &old_m->table;
    int new_bucket_count = old_table->bucket_count * 2;
    if (new_bucket_count == 0) { new_bucket_count = WMAPSTR__DEFAULT_SIZE; }
    else {
        const float factor = (float)old_table->pair_count / (float)old_table->bucket_count;
        if (factor < WMAPSTR__REHASH_FACTOR) { return 0; }
    }

    // Create new map.
    Wmapstr __new_map;
    Wmapstr *new_m = &__new_map;
    pub(create_with_allocator)(new_m, old_table->keys.allocator, old_table->keys.allocator_userdata);
    int err = pri(Table_grow)(&new_m->table, new_bucket_count);
    if (err) { pub(free)(new_m); printferr("OOM?"); return -1; }
    new_m->table.bucket_count = new_m->table.capacity;

    // Rehash keys.
    pub(It) it = pub(make_it)(old_m);
    while (pub(it_next)(old_m, &it)) {
        //printfd("shall not pass: key ["PRIstrw"]", PRIstrarg(it.key));
        // @Note: Keep this procedure in sync with 'upsert' function.
        ID i_prev = ID_INVALID, i = ID_INVALID;
        bool found = 0 == pri(find)(new_m, it.key, &i, &i_prev);
        if (found) { printferr("wtf: Key already exists."); goto quit_abort; }
        if (ID_get(i) >= new_m->table.capacity) {
            err = pri(Table_grow)(&new_m->table, new_m->table.capacity * 2);
            if (err) { printferr("OOM?"); goto quit_abort; }
        }
        pri(Table_set_new_pair)(&new_m->table, i_prev, i, it.__key_id, it.__value_id);
    }

    // Swap data and free.
    SWAP(new_m->values, old_m->values);
    SWAP(new_m->strpool, old_m->strpool);
    pub(free)(old_m);
    *old_m = *new_m;

    if ((0)) {
        quit_abort:
        pub(free)(new_m);
        return -1;
    }
    return 0;
}


static inline ID pri(table_get_bucket)(const pri(Table) *t, uint64_t hash) {
    return t->bucket_count == 0 ? ID_INVALID : ID_make((int)(hash & (uint64_t)(t->bucket_count - 1)));
    // bucket_count must be power of 2.
}


/// @Note. Returns possible id where you should insert it.
/// @Returns 0 if found. -1 if not.
int pri(find)(const Wmapstr *m, strview_t key, ID *out_id, ID *out_prev_id) {
    ID prev_id = ID_INVALID;
    ID id = pri(table_get_bucket)(&m->table, wmapstr__hash(key));
    do {
        if (!ID_valid(id) || pri(Table_slot_is_empty)(&m->table, id)) { break; }
        do {
            strview_t saved_key = strpool_get(&m->strpool, m->table.keys.items[ID_get(id)]);
            if (wstrview_equals(saved_key, key)) {
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
int pub(upsert)(Wmapstr *m, strview_t key, TYPE item) {
    ID i_prev = ID_INVALID, i = ID_INVALID;
    bool found = 0 == pri(find)(m, key, &i, &i_prev);
    if (found) {
        // Update.
        pri(Slot_Value_update)(&m->values, ID_get(m->table.val_ids.items[ID_get(i)]), item);
        return 0;
    }
    if (ID_get(i) >= m->table.capacity) {
        int new_cap = m->table.capacity == 0 ? WMAPSTR__DEFAULT_SIZE : m->table.capacity * 2;
        int err = pri(Table_grow)(&m->table, new_cap);
        if (err) { return -1; }
    }
    ID value_id = ID_make(pri(Slot_Value_append)(&m->values, item));
    if (!ID_valid(value_id)) { printferr("OOM?."); return -1; }
    ID key_id = strpool_append(&m->strpool, key);
    if (!ID_valid(key_id)) {
        pri(Slot_Value_pop)(&m->values, ID_get(value_id));
        printferr("OOM?");
        return -1;
    }
    pri(Table_set_new_pair)(&m->table, i_prev, i, key_id, value_id);
    if (pri(rehash_if_needed)(m)) { printfd("W: Rehashing failed."); }
    return 0;
}


WMAPSTR__TYPE *pub(get)(const Wmapstr *m, strview_t key_str) {
    ID node; bool found = 0 == pri(find)(m, key_str, &node, NULL);
    return (!found) ? NULL : pri(Slot_Value_get)(&m->values, ID_get(m->table.val_ids.items[ID_get(node)]));
}


void pub(remove)(Wmapstr *m, strview_t key_str) {
    ID node;
    bool found = 0 == pri(find)(m, key_str, &node, NULL);
    if (!found) { return; }
    int err = pri(Slot_Value_pop)(&m->values, ID_get(m->table.val_ids.items[ID_get(node)]));
    if (err) { printferr("Couldn't remove value."); }
    err = strpool_remove(&m->strpool, m->table.keys.items[ID_get(node)]);
    if (err) { printferr("Couldn't remove key from strpool."); }
    pri(Table_remove)(&m->table, node);
}


/// @Note. Modifying the map while iterating is UB.
bool pub(it_next)(const Wmapstr *m, pub(It) *it) {
    const pri(Table) *t = &m->table;
    while (ID_valid(it->__id)) {
        it->__key_id   = t->keys.items[ID_get(it->__id)];
        it->__value_id = t->val_ids.items[ID_get(it->__id)];
        it->key        = strpool_get(&m->strpool, it->__key_id);
        it->value      = pri(Slot_Value_get)(&m->values, ID_get(it->__value_id));
        it->__id       = t->next.items[ID_get(it->__id)]; // <-- At last, advance id.
        return true;
    }
    return false;
}

/// @Note. Modifying the map while iterating is UB.
bool pub(it_prev)(const Wmapstr *m, pub(It) *it) {
    const pri(Table) *t = &m->table;
    while (ID_valid(it->__id)) {
        it->__key_id   = t->keys.items[ID_get(it->__id)];
        it->__value_id = t->val_ids.items[ID_get(it->__id)];
        it->key        = strpool_get(&m->strpool, it->__key_id);
        it->value      = pri(Slot_Value_get)(&m->values, ID_get(it->__value_id));
        it->__id       = t->prev.items[ID_get(it->__id)]; // <-- At last, advance id.
        return true;
    }
    return false;
}


/// @Note. If found, the iterator will be alredy filled.
///        Unlike 'make_it' and 'make_it_end' which provide an empty iterator.
/// @Returns error.
int pub(get_it_for)(const Wmapstr *m, strview_t key_str, pub(It) *out_it, bool next_or_prev) {
    ID node;
    bool found = 0 == pri(find)(m, key_str, &node, NULL);
    if (!found) { return -1; }
    const pri(Table) *t = &m->table;

    out_it->__key_id   = t->keys.items[ID_get(node)];
    out_it->__value_id = t->val_ids.items[ID_get(node)];
    out_it->key        = strpool_get(&m->strpool, out_it->__key_id);
    out_it->value      = pri(Slot_Value_get)(&m->values, ID_get(out_it->__value_id));

    // ↓-- At last, advance id.
    out_it->__id = next_or_prev ? t->next.items[ID_get(node)] : t->prev.items[ID_get(node)];

    return 0;
}


#undef WMAPSTR__TYPE
#undef WMAPSTR__TOKCAT_
#undef WMAPSTR__TOKCAT
#undef WMAPSTR__NAMESPACE
#undef WMAPSTR__PFX
#undef WMAPSTR__PRI
#undef pub
#undef pri
#undef KEY
#undef TYPE
#undef Wmapstr
#undef WMAPSTR__ALLOC_PROTOTYPE
