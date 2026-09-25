/*
    Core low level indexing table to create hash maps. See wmap.h and wmapstr.h.

    Features:

    * All operations are constant time O(1), for example: upsert, get, remove.
        * Degrades to linked list search on collisions.
    * Good memory locality.
       * Uses struct of arrays (ECS like).
       * Operations only read what they need. (Most of the time).

    Define WMAPINDEX__NAMESPACE to set custom struct prefix.
*/

#include <stddef.h>
#include "portable_utils.h"


#undef SWAP
#define SWAP(a, b) do { typeof(a) bk = a; a = b; b = bk; } while(0)

#define WMAPINDEX__TOKCAT_(a, b) a ## b
#define WMAPINDEX__TOKCAT(a, b) WMAPINDEX__TOKCAT_(a, b)
#ifndef WMAPINDEX__NAMESPACE
#define WMAPINDEX__NAMESPACE UMapindex_
#endif
#define WMAPINDEX__PFX(name) WMAPINDEX__TOKCAT(WMAPINDEX__TOKCAT(WMAPINDEX__NAMESPACE, _), name)
#define WMAPINDEX__PRI(name) WMAPINDEX__TOKCAT(WMAPINDEX__TOKCAT(WMAPINDEX__NAMESPACE, __), name)
#if (defined pub | defined pri | defined WMapIndex)
#error "These macros should not be defined: pub, pri, WMapIndex"
#endif

#if !defined WMAPINDEX__KEY
    #define WMAPINDEX__KEY double
#endif


#define ARRAY__TYPE WMAPINDEX__KEY
#define ARRAY__NAMESPACE WMAPINDEX__PRI(Arr_Key)
#include "array.h"
#define ARRAY__TYPE ID
#define ARRAY__NAMESPACE WMAPINDEX__PRI(Arr_Int)
#include "array.h"

#define pub WMAPINDEX__PFX
#define pri WMAPINDEX__PRI
#define KEY WMAPINDEX__KEY
#define WMapIndex WMAPINDEX__NAMESPACE
#define WMAPINDEX__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define WMAPINDEX__DEFAULT_SIZE_EXP 8
#define WMAPINDEX__REHASH_FACTOR 0.9
#define WMAPINDEX__MAX_COUNT (INT_MAX -2)


typedef struct {
    pri(Arr_Key) keys;
    pri(Arr_Int) val_ids;
    pri(Arr_Int) next;
    pri(Arr_Int) prev;
    pri(Arr_Int) bucket_next;
    pri(Arr_Int) bucket_prev;

    int capacity;
    int bucket_count;

    int pair_count;        // Amount of pairs in total.
    int collision_count;   // Amount of pairs in 'collision storage'.
    ID first_node;         // For orderly iteration.
    ID last_node;          // For reverse iteration.
} WMapIndex;


int     pub(create)(WMapIndex *m);
int     pub(create_with_allocator)(WMapIndex *m, WMAPINDEX__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
void    pub(free)(WMapIndex *m);
void    pub(remove)(WMapIndex *m, ID i);
int     pub(grow)(WMapIndex *m, int new_cap);
static inline int pub(set_new_pair)(WMapIndex *m, ID i_bucket_prev, ID i_new, KEY key, ID value_id);
static inline bool pub(slot_is_empty(const WMapIndex *m, ID i)) { return !ID_valid(m->val_ids.items[ID_get(i)]); }

void    pri(clear)(WMapIndex *m, int from, int to);
void    pri(swap_nodes_same_bucket)(WMapIndex *m, const ID a, const ID b);
void    pri(swap_nodes_diff_bucket)(WMapIndex *m, ID a, ID b);
static inline ID  pub(if_invalid_get_next_valid_id)(const WMapIndex *m, ID i);


/// @Note. Doesn't allocate on creation. Aka default capacity is 0.
/// @Returns error.
int pub(create_with_allocator)(WMapIndex *m, WMAPINDEX__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (WMapIndex) { 0 };
    m->keys        = pri(Arr_Key_create_with_allocator)(allocator, allocator_userdata);
    m->next        = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->prev        = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->bucket_next = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->bucket_prev = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->val_ids     = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    return 0;
}

int pub(create)(WMapIndex *m) { return pub(create_with_allocator)(m, NULL, NULL); }

void pub(free)(WMapIndex *m) {
    pri(Arr_Key_destroy)(&m->keys);
    pri(Arr_Int_destroy)(&m->next);
    pri(Arr_Int_destroy)(&m->prev);
    pri(Arr_Int_destroy)(&m->bucket_next);
    pri(Arr_Int_destroy)(&m->bucket_prev);
    pri(Arr_Int_destroy)(&m->val_ids);
    *m = (WMapIndex) { 0 };
}

void pri(clear)(WMapIndex *m, int from, int to) {
    memset(m->keys.items    + from, 0, sizeof(m->keys.items[0]) * (size_t)(to - from));
    memset(m->next.items    + from, 0, sizeof(m->next.items[0]) * (size_t)(to - from));
    memset(m->prev.items    + from, 0, sizeof(m->prev.items[0]) * (size_t)(to - from));
    memset(m->val_ids.items + from, 0, sizeof(m->val_ids.items[0]) * (size_t)(to - from));
    memset(m->bucket_next.items  + from, 0, sizeof(m->bucket_next.items[0]) * (size_t)(to - from));
    memset(m->bucket_prev.items  + from, 0, sizeof(m->bucket_prev.items[0]) * (size_t)(to - from));
}


int pub(grow)(WMapIndex *m, int new_cap) {
    if (pri(Arr_Key_resize)(&m->keys, new_cap))       { return -1; }
    if (pri(Arr_Int_resize)(&m->next, new_cap))       { return -1; }
    if (pri(Arr_Int_resize)(&m->prev, new_cap))       { return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_next, new_cap)){ return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_prev, new_cap)){ return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, new_cap))    { return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, new_cap))    { return -1; }
    pri(clear)(m, m->capacity, new_cap);
    m->capacity = new_cap;
    return 0;
}


static inline ID pub(if_invalid_get_next_valid_id)(const WMapIndex *m, ID i) {
    return ((!ID_valid(i)) || (ID_valid(i) && !pub(slot_is_empty)(m, i))) ?
        ID_make(m->bucket_count + m->collision_count) :
        i;
}


/// @Param. i_bucket_prev. Can be invalid.
/// @Param. i_new. Must exist.
/// @Note. Can't fail.
static inline int pub(set_new_pair)(WMapIndex *m, ID i_bucket_prev, ID i_new, KEY key, ID value_id) {
    if (m->pair_count == 0) { m->first_node = i_new; }
    if (ID_valid(i_bucket_prev)) {
        m->bucket_next.items[ID_get(i_bucket_prev)] = i_new;
        m->bucket_prev.items[ID_get(i_new)] = i_bucket_prev;
    }
    if (ID_valid(m->last_node)) {
        m->prev.items[ID_get(i_new)] = m->last_node;
        m->next.items[ID_get(m->last_node)] = i_new;
    }
    m->bucket_next.items[ID_get(i_new)] = ID_INVALID;
    m->keys.items[ID_get(i_new)] = key;
    m->val_ids.items[ID_get(i_new)] = value_id;
    m->last_node = i_new;
    ++m->pair_count;
    if (ID_get(i_new) >= m->bucket_count) { ++m->collision_count; }
    return 0;
}


void pri(swap_nodes_same_bucket)(WMapIndex *m, const ID a, const ID b) {
    if (ID_equals(a, b)) { return; }

    ID a_prev_init_ = m->prev.items[ID_get(a)];
    ID b_prev_init_ = m->prev.items[ID_get(b)];
    ID a_next_init_ = m->next.items[ID_get(a)];
    ID b_next_init_ = m->next.items[ID_get(b)];

    {
        SWAP(m->next.items[ID_get(a)], m->next.items[ID_get(b)]);
        ID a_prev = ID_equals(a_prev_init_, b) ? a : a_prev_init_;
        ID b_prev = ID_equals(b_prev_init_, a) ? b : b_prev_init_;
        if (ID_valid(a_prev)) { m->next.items[ID_get(a_prev)] = b; }
        else { m->first_node = b; } // Must be root.
        if (ID_valid(b_prev)) { m->next.items[ID_get(b_prev)] = a; }
        else { m->first_node = a; } // Must be root.
    }

    {
        SWAP(m->prev.items[ID_get(a)], m->prev.items[ID_get(b)]);
        ID a_next = ID_equals(a_next_init_, b) ? a : a_next_init_;
        ID b_next = ID_equals(b_next_init_, a) ? b : b_next_init_;
        if (ID_valid(a_next)) { m->prev.items[ID_get(a_next)] = b; }
        else { m->last_node = b; } // Must be tail.
        if (ID_valid(b_next)) { m->prev.items[ID_get(b_next)] = a; }
        else { m->last_node = a; } // Must be tail.
    }

    SWAP(m->keys.items[ID_get(a)], m->keys.items[ID_get(b)]);
    SWAP(m->val_ids.items[ID_get(a)], m->val_ids.items[ID_get(b)]);

    // Keeping this here since it explains the swap logic:
    //     SWAP(m->next.items[ID_get(a_init)], m->next.items[ID_get(b_init)]);
    //     ID a_final = b_init;
    //     ID b_final = a_init;
    //     ID a_prev = ID_equals(a_prev_init, b_init) ? b_final : a_prev_init;
    //     ID b_prev = ID_equals(b_prev_init, a_init) ? a_final : b_prev_init;
    //     if (ID_valid(a_prev)) { m->next.items[ID_get(a_prev)] = a_final; }
    //     else { m->first_node = a_final; } // Must be root.
    //     if (ID_valid(b_prev)) { m->next.items[ID_get(b_prev)] = b_final; }
    //     else { m->first_node = b_final; } // Must be root.
}

void pri(swap_nodes_diff_bucket)(WMapIndex *m, ID a, ID b) {
    if (ID_equals(a, b)) { return; }
    // Note: Must never try to swap root nodes.
    //       aka. both nodes must have a bucket_previous.
    wassert(ID_valid(m->bucket_prev.items[ID_get(a)]));
    wassert(ID_valid(m->bucket_prev.items[ID_get(b)]));
    SWAP(m->keys.items[ID_get(a)], m->keys.items[ID_get(b)]);
    SWAP(m->val_ids.items[ID_get(a)], m->val_ids.items[ID_get(b)]);

    const ID a_next_init = m->next.items[ID_get(a)];
    const ID b_next_init = m->next.items[ID_get(b)];
    const ID a_prev_init = m->prev.items[ID_get(a)];
    const ID b_prev_init = m->prev.items[ID_get(b)];
    const ID a_bu_next_init = m->bucket_next.items[ID_get(a)];
    const ID b_bu_next_init = m->bucket_next.items[ID_get(b)];
    const ID a_bu_prev_init = m->bucket_prev.items[ID_get(a)];
    const ID b_bu_prev_init = m->bucket_prev.items[ID_get(b)];

    {
        SWAP(m->prev.items[ID_get(a)], m->prev.items[ID_get(b)]);
        ID a_next = ID_equals(a_next_init, b) ? a : a_next_init;
        ID b_next = ID_equals(b_next_init, a) ? b : b_next_init;
        if (ID_valid(a_next)) { m->prev.items[ID_get(a_next)] = b; }
        else { m->last_node = b; } // Must be tail
        if (ID_valid(b_next)) { m->prev.items[ID_get(b_next)] = a; }
        else { m->last_node = a; } // Must be tail
    }
    {
        SWAP(m->next.items[ID_get(a)], m->next.items[ID_get(b)]);
        ID a_prev = ID_equals(a_prev_init, b) ? a : a_prev_init;
        ID b_prev = ID_equals(b_prev_init, a) ? b : b_prev_init;
        if (ID_valid(a_prev)) { m->next.items[ID_get(a_prev)] = b; }
        else { m->first_node = b; } // Must be root
        if (ID_valid(b_prev)) { m->next.items[ID_get(b_prev)] = a; }
        else { m->first_node = a; } // Must be root
    }

    {
        SWAP(m->bucket_prev.items[ID_get(a)], m->bucket_prev.items[ID_get(b)]);
        ID a_prev = ID_equals(a_bu_next_init, b) ? a : a_bu_next_init;
        ID b_prev = ID_equals(b_bu_next_init, a) ? b : b_bu_next_init;
        if (ID_valid(a_prev)) { m->bucket_prev.items[ID_get(a_prev)] = b; }
        if (ID_valid(b_prev)) { m->bucket_prev.items[ID_get(b_prev)] = a; }
        // It's okay to not have a bucket_next.
    }
    {
        SWAP(m->bucket_next.items[ID_get(a)], m->bucket_next.items[ID_get(b)]);
        ID a_prev = ID_equals(a_bu_prev_init, b) ? a : a_bu_prev_init;
        ID b_prev = ID_equals(b_bu_prev_init, a) ? b : b_bu_prev_init;
        if (ID_valid(a_prev)) { m->bucket_next.items[ID_get(a_prev)] = b; }
        else { printferr("wtf: Trying to swap root."); }
        if (ID_valid(b_prev)) { m->bucket_next.items[ID_get(b_prev)] = a; }
        else { printferr("wtf: Trying to swap root."); }
        // If invalid this node must be root.
        // Note: Can't touch the root, shouldn't be swapping in the first place.
    }
}

void pub(remove)(WMapIndex *m, ID i) {

    // 1. If 'in direct' and has bucket_next -> Swap node to next in it's bucket.
    if (ID_get(i) < m->bucket_count && ID_valid(m->bucket_next.items[ID_get(i)])) {
        pri(swap_nodes_same_bucket)(m, i, m->bucket_next.items[ID_get(i)]);
        i = m->bucket_next.items[ID_get(i)];
    }
    // 2. If not 'in direct' -> Swap node to vector end.
    if (ID_get(i) >= m->bucket_count) {
        ID end = ID_make(m->bucket_count + m->collision_count-1);
        pri(swap_nodes_diff_bucket)(m, i, end);
        i = end;
        --m->collision_count;
    }
    // 3. Pop.
    {
        // Update indices for Order:
        ID prev = m->prev.items[ID_get(i)];
        ID next = m->next.items[ID_get(i)];
        if (ID_valid(prev)) { m->next.items[ID_get(prev)] = ID_valid(next) ? next : ID_INVALID; }
        if (ID_valid(next)) { m->prev.items[ID_get(next)] = ID_valid(prev) ? prev : ID_INVALID; }
        if (ID_equals(i, m->first_node)) { m->first_node = ID_valid(next) ? next : ID_INVALID; }
        if (ID_equals(i, m->last_node)) { m->last_node = ID_valid(prev) ? prev : ID_INVALID; }
        // Update indices for Bucket:
        prev = m->bucket_prev.items[ID_get(i)];
        next = m->bucket_next.items[ID_get(i)];
        if (ID_valid(prev)) { m->bucket_next.items[ID_get(prev)] = ID_valid(next) ? next : ID_INVALID; }
        if (ID_valid(next)) { m->bucket_prev.items[ID_get(next)] = ID_valid(prev) ? prev : ID_INVALID; }
        // Cleanup.
        m->keys.items[ID_get(i)] = 0; // No need to clear the key... But it feels right :(
        m->val_ids.items[ID_get(i)] = ID_INVALID;
        m->next.items[ID_get(i)] = ID_INVALID;
        m->prev.items[ID_get(i)] = ID_INVALID;
        m->bucket_next.items[ID_get(i)] = ID_INVALID;
        m->bucket_prev.items[ID_get(i)] = ID_INVALID;
        --m->pair_count;
    }
}



#undef pub
#undef pri
#undef KEY
#undef WMapIndex
#undef WMAPINDEX__ALLOC_PROTOTYPE
#undef WMAPINDEX__DEFAULT_SIZE_EXP
#undef WMAPINDEX__REHASH_FACTOR
#undef WMAPINDEX__MAX_COUNT
#undef WMAPINDEX__KEY
#undef WMAPINDEX__NAMESPACE
