/*
    Unordered hash map.

    Example 1: Use binary comparison.

        #define UMAP__KEY  int
        #define UMAP__TYPE Car
        #define UMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
        #include "umap.h"

    Example 2: Use custom comparison and hash.

        #define UMAP__KEY  Human
        #define UMAP__TYPE Car
        #define UMAP__KEY_EQUAL Human_equals
        #define UMAP__KEY_HASH  Human_hash
        #include "umap.h"

    Define UMAP__NAMESPACE to set custom struct prefix.

    Notes:
    * Design: In the past this map had keys and values split in different
      collections, however now they are joined in a Pair struct. Because
      removing required preserving order to keep the ids synced. Note however
      that in theory that old version should be faster and removing could
      use syncronized "swap and pop". But I guess I just prefer the
      simplicity of this one.
*/

#include <stdint.h>
#include "portable_utils.h"

#if !defined UMAP__KEY || !defined UMAP__TYPE
    #define UMAP__KEY double
    #define UMAP__TYPE float
    #define UMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
#endif

#ifdef UMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
    #define UMAP__KEY_EQUAL umap__default_equal
    #define UMAP__KEY_HASH  umap__default_hash
#endif

#if !defined UMAP__KEY_EQUAL || !defined UMAP__KEY_HASH
    #error "UMAP__KEY_EQUAL or UMAP__KEY_HASH missing"
#endif


/* Token concatenation. */
#define UMAP__TOKCAT_(a, b) a ## b
#define UMAP__TOKCAT(a, b) UMAP__TOKCAT_(a, b)
#ifndef UMAP__NAMESPACE
#define UMAP__NAMESPACE UMAP__TOKCAT(UMap_, UMAP__TYPE)
#endif
#define UMAP__PFX(name) UMAP__TOKCAT(UMAP__TOKCAT(UMAP__NAMESPACE, _), name)
#define UMAP__PRI(name) UMAP__TOKCAT(UMAP__TOKCAT(UMAP__NAMESPACE, __), name)
#if (defined pub | defined pri | defined TYPE | defined UMap)
#error "These macros should not be defined: pub, pri, TYPE, UMap"
#endif


typedef struct UMAP__PRI(Pair) {
    UMAP__TYPE value;
    UMAP__KEY key;
} UMAP__PRI(Pair);


#define DYNA__TYPE UMAP__PRI(Pair)
#define DYNA__NAMESPACE UMAP__PRI(Vec_Pair)
#include "da.h"


typedef struct UMAP__PRI(Bucket) {
    UMAP__PRI(Vec_Pair) pairs;
} UMAP__PRI(Bucket);


#define ARRAY__TYPE UMAP__PRI(Bucket)
#define ARRAY__NAMESPACE UMAP__PRI(Arr_Bucket)
#include "array.h"


#define pub UMAP__PFX
#define pri UMAP__PRI
#define KEY UMAP__KEY
#define TYPE UMAP__TYPE
#define UMap UMAP__NAMESPACE
#define UMAP__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define UMAP__DEFAULT_SIZE_EXP 8
#define UMAP__REHASH_FACTOR 0.7


typedef struct UMap {
    pri(Arr_Bucket) buckets;
    int pair_count;

    UMAP__ALLOC_PROTOTYPE(*allocator);
    void *allocator_userdata;
    void *userdata;            // Used during custom hash/comparison.
} UMap;

typedef struct pub(It) {
    TYPE *value;
    KEY key;
    int __bucket_id;
    int __pair_id;
} pub(It);


int          pub(create_with_allocator)(UMap *m, UMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
int          pub(create)               (UMap *m);
void         pub(free)                 (UMap *m);
void         pub(clear)                (UMap *m);
int          pub(upsert)               (UMap *m, KEY key, TYPE value);
static TYPE *pub(get)                  (UMap *m, KEY key);
static int   pub(remove)               (UMap *m, KEY key);
bool         pub(it_next)              (const UMap *m, pub(It) *it);
size_t       pub(report_memory)        (UMap *m);

int                        pri(grow)                   (UMap *m, int new_size);
int                        pri(init)                   (UMap *m, UMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata);
static inline int          pri(hash_and_get_bucket_id) (UMap *m, KEY key);
static inline pri(Bucket)* pri(hash_and_get_bucket)    (UMap *m, KEY key);
int                        pri(rehash_if_needed)       (UMap *old_m);
static inline int          pri(set_pair_with_final_key)(UMap *m, pri(Bucket) *bucket, KEY key, TYPE value);


int pri(grow)(UMap *m, int new_size) {
    if (m->buckets.size != 0) { return -1; } // Can only grow newly created maps.
    int err = pri(Arr_Bucket_resize)(&m->buckets, new_size);
    if (err == -1) { return -1; }

    for (int i = 0; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];

        // ↓↓↓ Can't fail.
        bucket->pairs = pri(Vec_Pair_create_with_allocator)(m->allocator, m->allocator_userdata);
    }
    return 0;
}


int pri(init)(UMap *m, UMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (UMap) { 0 };
    m->allocator = allocator;
    m->allocator_userdata = allocator_userdata;
    m->buckets = pri(Arr_Bucket_create_with_allocator)(allocator, allocator_userdata);
    return 0;
}


int pub(create_with_allocator)(UMap *m, UMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (UMap) { 0 };
    int err = pri(init)(m, allocator, allocator_userdata);
    if (err != 0) { return -1; }
    return pri(grow)(m, UMAP__DEFAULT_SIZE_EXP); // DEFAULT CAPACITY.
}


int pub(create)(UMap *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}


void pub(free)(UMap *m) {
    for (int i = 0; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];
        pri(Vec_Pair_free)(&bucket->pairs);
    }
    pri(Arr_Bucket_destroy)(&m->buckets);
    *m = (UMap) { 0 };
}


void pub(clear)(UMap *m) {
    for (int i = 0; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];
        pri(Vec_Pair_clear_preserving)(&bucket->pairs);
    }
    m->pair_count = 0;
}


#ifndef UMAP__UTILS
#define UMAP__UTILS
inline uint64_t umap__hash(char *data, int size) {
    // https://nullprogram.com/blog/2025/01/19/
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < size; i++) {
        h ^= data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}
bool umap__default_equal(KEY a, KEY b, void *data) { (void)data; return memcmp(&a, &b, sizeof(a)); }
uint64_t umap__default_hash(KEY k, void *data) { (void)data; return umap__hash((char*)&k, sizeof(k)); }
#endif


//static inline int pri(hash_and_get_bucket_id)(UMap *m, KEY key) {
    //uint64_t hash = UMAP__KEY_HASH(key, m->userdata);
    //return (int)(hash & (uint64_t)(m->buckets.size - 1));
//}


/// @Note. Cannot fail.
static inline pri(Bucket) *pri(hash_and_get_bucket)(UMap *m, KEY key) {
    uint64_t hash = UMAP__KEY_HASH(key, m->userdata);
    int bucket_id = (int)(hash & (uint64_t)(m->buckets.size - 1));
    return &m->buckets.items[bucket_id];
}


int pri(rehash_if_needed)(UMap *old_m) {
    float factor = (float)old_m->pair_count / (float)old_m->buckets.size;
    if (factor < UMAP__REHASH_FACTOR) {
        return 0;
    }

    // Create new map.

    UMap new_map;
    UMap *new_m = &new_map;

    int err = pri(init)(new_m, old_m->allocator, old_m->allocator_userdata);
    if (err == -1) {
        printfd("ERR: Couldn't rehash, no memory?");
        return -1;
    }
    new_m->userdata = old_m->userdata;
    //void *mimio = malloc(sizeof(int) * 16);

    err = pri(grow)(new_m, old_m->buckets.size * 2);
    if (err) {
        goto exit_abort;
    }

    // Populate map.

    for (int i = 0; i < old_m->buckets.size; ++i) {
        pri(Bucket) *bucket = &old_m->buckets.items[i];
        for (int k = 0; k < bucket->pairs.size; ++k) {

            // Insert every single pair.

            pri(Pair) *pair = &bucket->pairs.items[k];
            pri(Bucket) *new_bucket = pri(hash_and_get_bucket)(new_m, pair->key);

            err = pri(set_pair_with_final_key)(new_m, new_bucket, pair->key, pair->value);
            if (err < 0) {
                goto exit_abort;
            }

        }
    }

    // Replace.
    pub(free)(old_m);
    *old_m = *new_m;

    if ((0)) {
        exit_abort:
        printfd("ERR: Couldn't rehash, no memory?");
        pub(free)(new_m);
        return -1;
    }

    printfd("DEBUG: Rehashed.");
    return 0;
}


/// @Returns error.
int pub(upsert)(UMap *m, KEY key, TYPE value) {
    // Entry exists?
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        if (UMAP__KEY_EQUAL(bucket->pairs.items[i].key, key, m->userdata)) {
            bucket->pairs.items[i].value = value; // Update.
            return 0;
        }
    }
    // Else insert.
    pri(rehash_if_needed)(m);
    return pri(set_pair_with_final_key)(m, bucket, key, value);
}


static TYPE *pub(get)(UMap *m, KEY key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        if (UMAP__KEY_EQUAL(bucket->pairs.items[i].key, key, m->userdata)) {
            return &bucket->pairs.items[i].value;
        }
    }
    return NULL;
}


/// @Returns Error.
static int pub(remove)(UMap *m, KEY key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        if (UMAP__KEY_EQUAL(bucket->pairs.items[i].key, key, m->userdata)) {
            // Found it.
            int err = pri(Vec_Pair_remove_at)(&bucket->pairs, i);
            if (err != 0) { printfd("ERROR(%d) Vec_Pair. Couldn't delete.", err); }
            --m->pair_count;
            return 0;
        }
    }
    return -1;
}


/// @Returns error.
static inline int pri(set_pair_with_final_key)(UMap *m, pri(Bucket) *bucket, KEY key, TYPE value) {
    int da_pair_id = pri(Vec_Pair_append)(&bucket->pairs, (pri(Pair)) { .key = key, .value = value, });
    if (da_pair_id < 0) {
        return -1;
    }
    ++m->pair_count;
    return 0;
}


/// @Note. Modifying the map while iterating is UB.
bool pub(it_next)(const UMap *m, pub(It) *it) {
    for (; it->__bucket_id < m->buckets.size; ++it->__bucket_id, it->__pair_id = 0) {
        while (it->__pair_id < m->buckets.items[it->__bucket_id].pairs.size) {
            it->key = m->buckets.items[it->__bucket_id].pairs.items[it->__pair_id].key;
            it->value = &m->buckets.items[it->__bucket_id].pairs.items[it->__pair_id].value;
            ++it->__pair_id;
            return true;
        }
    }
    return false;
}


size_t pub(report_memory)(UMap *m) {
    size_t count = sizeof(UMap);
    count += (size_t)m->buckets.size * sizeof(pri(Bucket));
    for (int i = 0; i < m->buckets.size; ++i) {
        count += (size_t)m->buckets.items[i].pairs.size * sizeof(pri(Pair));
    }
    return count;
}


#undef UMAP__TYPE
#undef UMAP__TOKCAT_
#undef UMAP__TOKCAT
#undef UMAP__NAMESPACE
#undef UMAP__PFX
#undef UMAP__PRI
#undef pub
#undef pri
#undef KEY
#undef TYPE
#undef UMap
#undef UMAP__ALLOC_PROTOTYPE
#undef UMAP__DEFAULT_SIZE_EXP
