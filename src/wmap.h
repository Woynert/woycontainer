
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

typedef struct {
    int id;
} wmap__zid_t;
inline int         wmap__zid_get(wmap__zid_t id)           { return id.id -1; }
inline wmap__zid_t wmap__zid_make(int id)                  { return (wmap__zid_t) { id +1 }; }
inline bool        wmap__zid_valid(wmap__zid_t id)         { return id.id > 0; }

#endif

#pragma push_macro("ID")
#pragma push_macro("ID_valid")
#pragma push_macro("ID_get")
#pragma push_macro("ID_make")
#pragma push_macro("ID_equals")
#pragma push_macro("ID_INVALID")
#pragma push_macro("SWAP")
#undef ID
#undef ID_valid
#undef ID_get
#undef ID_make
#undef ID_equals
#undef ID_INVALID
#define ID           wmap__zid_t
#define ID_valid(id) wmap__zid_valid(id)
#define ID_get(id)   wmap__zid_get(id)
#define ID_make(id)  wmap__zid_make(id)
#define ID_equals(a, b) ((a).id == (b).id)
#define ID_INVALID ((ID){0})
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

#define ARRAY__TYPE WMAP__KEY
#define ARRAY__NAMESPACE WMAP__PRI(Arr_Key)
#include "array.h"
#define ARRAY__TYPE WMAP__TYPE
#define ARRAY__NAMESPACE WMAP__PRI(Arr_Value)
#include "array.h"
#define ARRAY__TYPE ID
#define ARRAY__NAMESPACE WMAP__PRI(Arr_Int)
#include "array.h"
#define DYNA__TYPE WMAP__TYPE
#define DYNA__NAMESPACE WMAP__PRI(Vec_Value)
#include "da.h"

#define pub WMAP__PFX
#define pri WMAP__PRI
#define KEY WMAP__KEY
#define TYPE WMAP__TYPE
#define WMap WMAP__NAMESPACE
#define WMAP__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define WMAP__DEFAULT_SIZE_EXP 8
#define WMAP__REHASH_FACTOR 0.9
#define WMAP__MAX_COUNT (INT_MAX -2)


typedef struct {
    pri(Arr_Key) keys;
    pri(Arr_Int) val_ids;
    pri(Arr_Int) next;
    pri(Arr_Int) prev;
    pri(Arr_Int) bucket_next;
    pri(Arr_Int) bucket_prev;
    pri(Arr_Int) bucket_tail;
    pri(Vec_Value) values;
    int capacity_exp;
    int bucket_count_exp;

    int pair_count;        // Amount of pairs in total.
    int collision_count;   // Amount of pairs in 'collision storage'.
    ID first_node;         // For orderly iteration.
    ID last_node;          // For reverse iteration.
    void *userdata;        // Used for user provided hash/comparison functions.
} WMap;

typedef struct {
    TYPE *value;
    KEY key;
    ID __id;
    ID __value_id;
} pub(It);



bool pri(default_equal)(KEY a, KEY b, void *data) { (void)data; return 0 == memcmp(&a, &b, sizeof(a)); }
uint64_t pri(default_hash)(KEY k, void *data) { (void)data; return wmap__hash((char*)&k, sizeof(k)); }
pub(It) pub(make_it)(const WMap *m) { return (pub(It)) { .__id = m->first_node, }; }
int pri(allocate_direct_storage)(WMap *m, int cap_exp);
int pri(grow_collision_storage)(WMap *m, int new_cap_exp);
void pri(clear)(WMap *m, int from, int to);

void pri(init)(WMap *m, WMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (WMap) { 0 };
    m->keys        = pri(Arr_Key_create_with_allocator)(allocator, allocator_userdata);
    m->next        = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->prev        = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->bucket_next = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->bucket_prev = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->val_ids     = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->bucket_tail = pri(Arr_Int_create_with_allocator)(allocator, allocator_userdata);
    m->values      = pri(Vec_Value_create_with_allocator)(allocator, allocator_userdata);
}

/// @Returns error.
int pub(create_with_allocator)(WMap *m, WMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    pri(init)(m, allocator, allocator_userdata);
    if (pri(allocate_direct_storage)(m, 1)) { return -1; }  // DEFAULT CAPACITY
    m->bucket_count_exp = m->capacity_exp;                  // DEFAULT BUCKETS
    return 0;
}

int pub(create)(WMap *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}

void pub(set_userdata)(WMap *m, void *data) { m->userdata = data; }

void pub(free)(WMap *m) {
    pri(Arr_Key_destroy)(&m->keys);
    pri(Arr_Int_destroy)(&m->next);
    pri(Arr_Int_destroy)(&m->prev);
    pri(Arr_Int_destroy)(&m->bucket_next);
    pri(Arr_Int_destroy)(&m->bucket_prev);
    pri(Arr_Int_destroy)(&m->val_ids);
    pri(Arr_Int_destroy)(&m->bucket_tail);
    pri(Vec_Value_free)(&m->values);
    *m = (WMap) { 0 };
}

void pri(clear)(WMap *m, int from, int to) {
    memset(m->keys.items    + from, 0, sizeof(m->keys.items[0]) * (size_t)(to - from));
    memset(m->next.items    + from, 0, sizeof(m->next.items[0]) * (size_t)(to - from));
    memset(m->prev.items    + from, 0, sizeof(m->prev.items[0]) * (size_t)(to - from));
    memset(m->val_ids.items + from, 0, sizeof(m->val_ids.items[0]) * (size_t)(to - from));
    memset(m->bucket_next.items  + from, 0, sizeof(m->bucket_next.items[0]) * (size_t)(to - from));
    memset(m->bucket_prev.items  + from, 0, sizeof(m->bucket_prev.items[0]) * (size_t)(to - from));
    // @Note: Bucket_limits aren't cleared here because they're
    //        can only be cleared once per map.
}

// @Note: Only call on newly created maps.
int pri(allocate_direct_storage)(WMap *m, int cap_exp) {
    int size = 1 << cap_exp;
    if (pri(Arr_Key_resize)(&m->keys, size))          { return -1; }
    if (pri(Arr_Int_resize)(&m->next, size))          { return -1; }
    if (pri(Arr_Int_resize)(&m->prev, size))          { return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_next, size))   { return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_prev, size))   { return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, size))       { return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, size))       { return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_tail, size)){ return -1; }
    pri(clear)(m, 0, size);
    memset(m->bucket_tail.items, 0, sizeof(m->bucket_tail.items[0]) * (size_t)(size));
    m->capacity_exp = cap_exp;
    return 0;
}

// @Note: The difference between this and 'allocate_direct_storage' is
//        this can be called more than once, since it doesn't touch 'bucket_limits'.
int pri(grow_collision_storage)(WMap *m, int new_cap_exp) {
    int size = 1 << m->capacity_exp;
    int new_size = 1 << new_cap_exp;
    if (pri(Arr_Key_resize)(&m->keys, new_size))       { return -1; }
    if (pri(Arr_Int_resize)(&m->next, new_size))       { return -1; }
    if (pri(Arr_Int_resize)(&m->prev, new_size))       { return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_next, new_size)){ return -1; }
    if (pri(Arr_Int_resize)(&m->bucket_prev, new_size)){ return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, new_size))    { return -1; }
    if (pri(Arr_Int_resize)(&m->val_ids, new_size))    { return -1; }
    pri(clear)(m, size, new_size);
    m->capacity_exp = new_cap_exp;
    return 0;
}


static inline int pri(hash_and_get_bucket)(WMap *m, KEY key) {
    uint64_t hash = WMAP__KEY_HASH(key, m->userdata);
    int bucket_id = (int)(hash & (uint64_t)((1 << m->bucket_count_exp) - 1));
    return bucket_id;
}

bool pub(it_next)(const WMap *m, pub(It) *it);

int pri(find)(WMap *m, ID start, KEY key, ID *out_prev_id, ID *out_id);
static inline int pri(set_new_pair)(WMap *m, ID node_prev, ID node_new, KEY key, ID value_id, ID bucket_id);

int pri(rehash_if_needed)(WMap *old_m) {
    const float factor = (float)old_m->pair_count / (float)(1 << old_m->bucket_count_exp);
    if (factor < WMAP__REHASH_FACTOR) { return 0; }
    printfd(ANSI_RED"REHASHING IS NEEDED!!");

    // Create new map.
    WMap __new_map;
    WMap *new_m = &__new_map;
    pri(init)(new_m, old_m->keys.allocator, old_m->keys.allocator_userdata);
    int err = pri(allocate_direct_storage)(new_m, old_m->bucket_count_exp +1);
    if (err) { pub(free)(new_m); return -1; }
    new_m->bucket_count_exp = new_m->capacity_exp;

    // Rehash keys.
    pub(It) it = pub(make_it)(old_m);
    while (pub(it_next)(old_m, &it)) {
        // @Note: Keep this procedure in sync with 'upsert' function.
        ID bucket_id = ID_make(pri(hash_and_get_bucket)(new_m, it.key));
        ID i_prev = { 0 };
        ID i = bucket_id;
        bool found = 0 == pri(find)(new_m, i, it.key, &i_prev, &i);
        if (found) {
            printferr("wtf: Key already exists. Should never happen");
            goto quit_abort;
        }
        if (ID_get(i) >= (1 << new_m->capacity_exp)) {
            printfd("growing");
            err = pri(grow_collision_storage)(new_m, new_m->capacity_exp + 1);
            if (err) { printferr("No memory?"); goto quit_abort; }
        }
        pri(set_new_pair)(new_m, i_prev, i, it.key, it.__value_id, bucket_id);
        if (ID_get(i) >= (1 << new_m->bucket_count_exp)) { ++new_m->collision_count; }
    }

    // Swap data and free.
    pri(Vec_Value) bk = new_m->values;
    new_m->values = old_m->values;
    old_m->values = bk;
    pub(free)(old_m);
    *old_m = *new_m;

    if ((0)) {
        quit_abort:
        pub(free)(new_m);
        return -1;
    }
    return 0;
}


/// @Param. i_prev. Can be invalid.
/// @Param. i_new. Must exist.
/// @Note. Can't fail.
static inline int pri(set_new_pair)(WMap *m, ID i_bucket_prev, ID i_new, KEY key, ID value_id, ID bucket_id) {
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
    // ↓↓ Setting limits for current bucket.
    m->bucket_tail.items[ID_get(bucket_id)] = i_new;
    //
    return 0;
}

inline bool pri(slot_is_empty(WMap *m, ID i)) { return !ID_valid(m->val_ids.items[ID_get(i)]); }

/// @Note. Returns possible id where you should insert it.
/// @Returns 0 if found. -1 if not.
int pri(find)(WMap *m, const ID start, KEY key, ID *out_prev_id, ID *out_id) {
    ID i_prev = ID_INVALID;
    ID i = start;

    while (ID_valid(i)) {
        if (pri(slot_is_empty)(m, i)) { break; }
        if (WMAP__KEY_EQUAL(m->keys.items[ID_get(i)], key, m->userdata)) {
            *out_id = i;
            *out_prev_id = i_prev;
            return 0;
        }
        i_prev = i;
        i = m->bucket_next.items[ID_get(i)];
    }

    if ((!ID_valid(i)) || (ID_valid(i) && !pri(slot_is_empty)(m, i))) {
        i = ID_make((1 << m->bucket_count_exp) + m->collision_count);
    }

    *out_id = i;
    *out_prev_id = i_prev;
    return -1;
}


// @Note: Keep this function in sync with 'rehash insert'.
int pub(upsert)(WMap *m, KEY key, TYPE item) {
    ID bucket_id = ID_make(pri(hash_and_get_bucket)(m, key));
    ID i_prev = { 0 };
    ID i = bucket_id;
    bool found = 0 == pri(find)(m, i, key, &i_prev, &i);
    if (found) {
        m->values.items[ID_get(m->val_ids.items[ID_get(i)])] = item; // Update.
        printfd("Updated");
        return 0;
    }
    if (ID_get(i) >= (1 << m->capacity_exp)) {
        printfd("growing");
        int err = pri(grow_collision_storage)(m, m->capacity_exp + 1);
        if (err) { return -1; }
    }
    ID value_id = ID_make(pri(Vec_Value_append)(&m->values, item));
    if (!ID_valid(value_id)) { return -1; }
    pri(set_new_pair)(m, i_prev, i, key, value_id, bucket_id);
    if (ID_get(i) >= (1 << m->bucket_count_exp)) { ++m->collision_count; }
    //if (pri(rehash_if_needed)(m)) { printferr("W: Rehashing failed."); }
    return 0;
}


void pri(swap_nodes)(WMap *m, ID a, ID b, ID a_bucket, ID b_bucket) {
    if (ID_equals(a, b)) { return; }
    // Note: Must never try to swap root nodes.
    //       aka. both nodes must have a bucket_previous.
    wassert(ID_valid(m->bucket_prev.items[ID_get(a)]));
    wassert(ID_valid(m->bucket_prev.items[ID_get(b)]));
    SWAP(m->keys.items[ID_get(a)], m->keys.items[ID_get(b)]);
    SWAP(m->val_ids.items[ID_get(a)], m->val_ids.items[ID_get(b)]);
    //SWAP(m->next.items[ID_get(a)], m->next.items[ID_get(b)]);
    //SWAP(m->prev.items[ID_get(a)], m->prev.items[ID_get(b)]);
    ID a_next = m->bucket_next.items[ID_get(a)];
    ID b_next = m->bucket_next.items[ID_get(b)];
    ID a_prev = m->bucket_prev.items[ID_get(a)];
    ID b_prev = m->bucket_prev.items[ID_get(b)];

    //SWAP(m->bucket_next.items[ID_get(m->bucket_prev.items[ID_get(a)])], m->bucket_next.items[ID_get(m->bucket_prev.items[ID_get(b)])]);
    //SWAP(m->bucket_prev.items[ID_get(m->bucket_prev.items[ID_get(a)])], m->bucket_prev.items[ID_get(m->bucket_prev.items[ID_get(b)])]);

    //if (ID_valid(m->bucket_next.items[ID_get(a)]) || ID_valid(m->bucket_next.items[ID_get(b)]))
    //{ SWAP(m->bucket_prev.items[ID_get(m->bucket_next.items[ID_get(a)])], m->bucket_prev.items[ID_get(m->bucket_next.items[ID_get(b)])]); }
    //if (ID_valid(a_next)) {
        //m->bucket_prev.items[ID_get(a_next)] = b;
    //} else { // This means this is the tail.
        //m->bucket_tail.items[ID_get(a_bucket)] = b;
    //}

    if (ID_valid(a_next)) { m->bucket_prev.items[ID_get(a_next)] = b; }
    else { m->bucket_tail.items[ID_get(a_bucket)] = b; }
    // ↑↑↑ Else this means this is the tail.

    if (ID_valid(b_next)) { m->bucket_prev.items[ID_get(b_next)] = a; }
    else { m->bucket_tail.items[ID_get(b_bucket)] = a; }
    // ↑↑↑ Else this means this is the tail.

    //if (ID_valid(b_next)) { m->bucket_prev.items[ID_get(b_next)] = a; }

    SWAP(m->bucket_next.items[ID_get(a_prev)], m->bucket_next.items[ID_get(b_prev)]);
    //SWAP(m->bucket_prev.items[ID_get(a_prev)], m->bucket_prev.items[ID_get(b_prev)]);

    //m->bucket_next.items[ID_get(a)] = b_next;
    //m->bucket_next.items[ID_get(b)] = a_next;
    //m->bucket_prev.items[ID_get(a)] = b_prev;
    //m->bucket_prev.items[ID_get(b)] = a_prev;
    SWAP(m->bucket_next.items[ID_get(a)], m->bucket_next.items[ID_get(b)]);
    SWAP(m->bucket_prev.items[ID_get(a)], m->bucket_prev.items[ID_get(b)]);
    {
        //if (ID_valid(i_bucket_prev)) {
            //ID next = m->bucket_next.items[ID_get(i)];
            //m->bucket_next.items[ID_get(i_bucket_prev)] = ID_valid(next) ? next : ID_INVALID;
        //}
    }
    //m->bucket_next.items[xxx]; // ???
    /*
    pri(Arr_Key) keys;
    pri(Arr_Int) val_ids;
    pri(Arr_Int) next;
    pri(Arr_Int) prev;
    pri(Arr_Int) bucket_next;
    pri(Arr_Pair) bucket_limits;
    pri(Vec_Value) values;
       */
}

int pub(remove)(WMap *m, KEY key) {
    ID bucket_id = ID_make(pri(hash_and_get_bucket)(m, key));
    ID i_bucket_prev = { 0 };
    ID i = bucket_id;
    bool found = 0 == pri(find)(m, i, key, &i_bucket_prev, &i);
    if (!found) { return 0; }

    {
        // Order:
        // From: Prev -> Me -> Next.
        // To:   Prev -------> Next.
        ID prev = m->prev.items[ID_get(i)];
        ID next = m->next.items[ID_get(i)];
        if (ID_valid(prev)) {
            m->next.items[ID_get(prev)] = ID_valid(next) ? next : ID_INVALID;
        }
        // Order:
        // From: [Root] -> Me -> Next.
        // To:   [Root] -------> Next.
        if (ID_equals(m->first_node, i)) {
            m->first_node = ID_valid(next) ? next : ID_INVALID;
        }
        // Order:
        // From: Prev -> Me -> [End].
        // To:   Prev -------> [End].
        if (ID_equals(m->last_node, i)) {
            m->last_node = ID_valid(prev) ? prev : ID_INVALID;
        }
    }
    {
        // Bucket:
        // Limits
        // Note: The root of a bucket is always equal to it's ID...
        // bucket_beg == bucket_id <-- ALWAYS TRUE
        ID bucket_beg = bucket_id;
        ID bucket_end = m->bucket_tail.items[ID_get(bucket_id)];
        if (ID_equals(bucket_beg, i)) {
            // Here is when we need to do some smarter swapping.
            ID bucket_next = m->bucket_next.items[ID_get(bucket_id)];
            pri(swap_nodes)(m, bucket_beg, ID_valid(bucket_next) ? bucket_next : ID_INVALID, bucket_id, bucket_id);
        }
        if (ID_equals(bucket_end, i)) {
            ID bucket_next = m->bucket_next.items[ID_get(bucket_id)];
        }
    }
    {
        // Buckets:
        // From: Prev -> Me -> Next.
        // To:   Prev -------> Next.
        if (ID_valid(i_bucket_prev)) {
            ID next = m->bucket_next.items[ID_get(i)];
            m->bucket_next.items[ID_get(i_bucket_prev)] = ID_valid(next) ? next : ID_INVALID;
        }
    }


    // Order:
    // Delete 'Me'
    //m->keys.items[ID_get(i)] = 0; No need to clear the key.
    m->val_ids.items[ID_get(i)] = ID_INVALID;
    m->next.items[ID_get(i)] = ID_INVALID;
    m->prev.items[ID_get(i)] = ID_INVALID;
    m->bucket_next.items[ID_get(i)] = ID_INVALID;
    /*
    pri(Arr_Key) keys;
    pri(Arr_Int) val_ids;
    pri(Arr_Int) next;
    pri(Arr_Int) prev;
    pri(Arr_Int) bucket_next;
    pri(Arr_Pair) bucket_limits;
    pri(Vec_Value) values;
       */





    return 0;
}




/// @Note. Modifying the map while iterating is UB.
bool pub(it_next)(const WMap *m, pub(It) *it) {
    while (ID_valid(it->__id)) {
        it->key = m->keys.items[ID_get(it->__id)];
        it->value = &m->values.items[ID_get(m->val_ids.items[ID_get(it->__id)])];
        it->__value_id = m->val_ids.items[ID_get(it->__id)];
        it->__id = m->next.items[ID_get(it->__id)];
        return true;
    }
    return false;
}


void pub(print)(const WMap *m) {
    printf("Printing map, size %d, buckets %d, pairs %d, cols %d, first %d, last %d",
            1 << m->capacity_exp, 1 << m->bucket_count_exp,
            m->pair_count, m->collision_count, ID_get(m->first_node), ID_get(m->last_node));
    printf("\nkeys:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", m->keys.items[i]);
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nvalueid: ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->val_ids.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nbuck_nxt:");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->bucket_next.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nbuck_prv:");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->bucket_prev.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nvalues:  ");
    for (int i = 0; i < m->values.size; ++i) {
        printf("[%d]=%d,", i, m->values.items[i]);
    }
    printf("\nnext:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->next.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nprev:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->prev.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nbuk_limt:");
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        printf("%2d,%2d|", i, ID_get(m->bucket_tail.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\n");
}

void pub(print_bucket_chains)(const WMap *m, bool backwards) {
    const int max_cycles = 100;
    int cycles = 0;
    printf("Forward:\n");
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        ID id = ID_make(i);
        while (ID_valid(id)) {
            //printf("%d"ANSI_GRE"(%d)"ANSI_RESET",", ID_get(id), m->keys.items[ID_get(id)]);
            printf("%d,", m->keys.items[ID_get(id)]);
            id = m->bucket_next.items[ID_get(id)];

            if (++cycles > max_cycles) { return; }
        }
        printf("\n");
    }
    if (!backwards) { return; }
    cycles = 0;
    printf("Backwards:\n");
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        ID id = m->bucket_tail.items[i];
        while (ID_valid(id)) {
            printf("%d,", m->keys.items[ID_get(id)]);
            id = m->bucket_prev.items[ID_get(id)];

            if (++cycles > max_cycles) { return; }
        }
        printf("\n");
    }
}

/*
    int i = pri(hash_and_get_bucket)(m, key);
    int node_last = i;
    while (i > 0) {
    for (int i = pri(hash_and_get_bucket)(m, key), node_last = i; i > 0; 
        if (WMAP__KEY_EQUAL(m->keys[i-1], key, m->userdata)) {
            bucket->pairs.items[i-1].value = value; // Update.
            return 0;
        }
        node_last = i;
        i = m->next[i-1];
    }
    */

/*
    int i = pri(hash_and_get_bucket)(m, key), i_prev = i;
    for (; i > 0 && m->keys.items[i-1].value_id != 0; i_prev = i, i = m->next.items[i-1]) {
        if (WMAP__KEY_EQUAL(m->keys.items[i-1].key, key, m->userdata)) {
            m->values_old.items[m->keys.items[i-1].value_id-1] = item; // Update.
            return 0;
        }
    }
   */


#undef pub
#undef pri
#undef KEY
#undef TYPE
#undef WMap
#undef WMAP__ALLOC_PROTOTYPE
#undef WMAP__DEFAULT_SIZE_EXP
#undef WMAP__REHASH_FACTOR
#undef WMAP__MAX_COUNT

#pragma pop_macro("ID")
#pragma pop_macro("ID_valid")
#pragma pop_macro("ID_get")
#pragma pop_macro("ID_make")
#pragma pop_macro("ID_equals")
#pragma pop_macro("ID_INVALID")
#pragma pop_macro("SWAP")
