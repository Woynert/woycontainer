/*
   String hash map.

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
#include "strpool.h"


#ifndef STRMAP__TYPE
#define STRMAP__TYPE float
#endif


/* Token concatenation. */
#define STRMAP__TOKCAT_(a, b) a ## b
#define STRMAP__TOKCAT(a, b) STRMAP__TOKCAT_(a, b)
#ifndef STRMAP__NAMESPACE
#define STRMAP__NAMESPACE STRMAP__TOKCAT(STRMAP__TYPE, _strmap)
#endif
#define STRMAP__PFX(name) STRMAP__TOKCAT(STRMAP__TOKCAT(STRMAP__NAMESPACE, _), name)
#define STRMAP__PRI(name) STRMAP__TOKCAT(STRMAP__TOKCAT(STRMAP__NAMESPACE, __), name)
#if (defined(pub) | defined(TYPE) | defined(Strmap))
#error "These macros should not be defined: pub, TYPE, Strmap"
#endif


typedef struct STRMAP__PRI(Pair) {
    STRMAP__TYPE value;
    int key;
} STRMAP__PRI(Pair);


#define DYNA__TYPE STRMAP__PRI(Pair)
#define DYNA__NAMESPACE STRMAP__PRI(Pair_da)
#include "da.h"


typedef struct STRMAP__PRI(Bucket) {
    STRMAP__PRI(Pair_da) pairs;
} STRMAP__PRI(Bucket);


#define ARRAY__TYPE STRMAP__PRI(Bucket)
#include "array.h"


#define pub STRMAP__PFX
#define pri STRMAP__PRI
#define TYPE STRMAP__TYPE
#define Strmap STRMAP__NAMESPACE
#define STRMAP__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define STRMAP__DEFAULT_SIZE_EXP 8
#define STRMAP__REHASH_FACTOR 0.7


typedef struct {
    pri(Bucket_Array) buckets;
    strpool strpool;
    int pair_count;

    STRMAP__ALLOC_PROTOTYPE(*allocator);
    void *allocator_userdata;
} Strmap;


typedef struct strmap__view {
    const char *data;
    int size;
} strmap__view;


int pub(set_pair)(Strmap *m, strmap__view key, STRMAP__TYPE value);


static inline int pri(set_pair_with_final_key)(Strmap *m, pri(Bucket) *bucket, int strpool_key, STRMAP__TYPE value);


int pri(grow)(Strmap *m, int new_size) {
    if (m->buckets.size != 0) { return -1; } // Can only grow newly created maps.
    int prev_size = m->buckets.size;
    int err = pri(Bucket_Array_resize)(&m->buckets, new_size);
    if (err == -1) { return -1; }

    for (int i = prev_size; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];

        // ↓↓↓ Can't fail.
        bucket->pairs = pri(Pair_da_create_with_allocator)(m->allocator, m->allocator_userdata);
    }
    return 0;
}


int pri(init)(Strmap *m, STRMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (Strmap) { 0 };
    int err = strpool_create_with_allocator(&m->strpool, allocator, allocator_userdata);
    if (err != 0) { return -1; }
    m->allocator = allocator;
    m->allocator_userdata = allocator_userdata;
    m->buckets = pri(Bucket_Array_create_with_allocator)(allocator, allocator_userdata);
    return 0;
}


int pub(create_with_allocator)(Strmap *m, STRMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (Strmap) { 0 };
    int err = pri(init)(m, allocator, allocator_userdata);
    if (err != 0) { return -1; }
    return pri(grow)(m, STRMAP__DEFAULT_SIZE_EXP); // DEFAULT CAPACITY.
}


int pub(create)(Strmap *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}


void pub(free)(Strmap *m) {
    for (int i = 0; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];
        pri(Pair_da_free)(&bucket->pairs);
    }
    pri(Bucket_Array_destroy)(&m->buckets);
    strpool_destroy(&m->strpool);
    *m = (Strmap) { 0 };
}


#ifndef STRMAP__UTILS
#define STRMAP__UTILS


uint64_t strmap__hash(strmap__view view) {
    // https://nullprogram.com/blog/2025/01/19/
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < view.size; i++) {
        h ^= view.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}

strmap__view strmap__strpool_view(strpool__str str) {
    return (strmap__view) { .data = str.data, .size = str.size };
}

strpool__str strmap__view_to_strpool_str(strmap__view str) {
    return (strpool__str) { .data = str.data, .size = str.size };
}

bool strmap__view_equals(strmap__view str1, strmap__view str2) {
    if (str1.size != str2.size) { return false; }
    return !str1.size || !memcmp(str1.data, str2.data, (size_t)str1.size);
    // !str1.size it's necessary see https://nullprogram.com/blog/2025/01/19/#strings
}

#endif // !STRMAP__UTILS



static inline pri(Bucket) *pri(hash_and_get_bucket)(Strmap *m, strmap__view key) {
    uint64_t hash = strmap__hash(key);
    int bucket_id = (int)(hash & (uint64_t)(m->buckets.size - 1));
    return &m->buckets.items[bucket_id];
}


void pri(rehash_if_needed)(Strmap *old_m) {
    float factor = (float)old_m->pair_count / (float)old_m->buckets.size;
    if (factor < STRMAP__REHASH_FACTOR) {
        return;
    }

    // Create new map.

    Strmap new_map;
    Strmap *new_m = &new_map;

    int err = pri(init)(new_m, old_m->allocator, old_m->allocator_userdata);
    if (err == -1) {
        printfd("ERR: Couldn't rehash, no memory?");
        return;
    }

    err = pri(grow)(new_m, old_m->buckets.size * 2);
    if (err < 0) {
        goto exit_abort;
    }

    // Populate map.

    for (int i = 0; i < old_m->buckets.size; ++i) {
        pri(Bucket) *bucket = &old_m->buckets.items[i];
        for (int k = 0; k < bucket->pairs.size; ++k) {

            // Insert every single pair.

            pri(Pair) *pair = &bucket->pairs.items[k];
            strmap__view key = strmap__strpool_view(strpool_get(&old_m->strpool, pair->key));
            pri(Bucket) *new_bucket = pri(hash_and_get_bucket)(new_m, key);

            err = pri(set_pair_with_final_key)(new_m, new_bucket, pair->key, pair->value);
            if (err < 0) {
                goto exit_abort;
            }

        }
    }

    // Swap stringpools.
    {
        strpool bk = new_m->strpool;
        new_m->strpool = old_m->strpool;
        old_m->strpool = bk;
    }

    // Free old map.
    pub(free)(old_m);

    // Replace.
    *old_m = *new_m;

    if ((0)) {
        exit_abort:
        printfd("ERR: Couldn't rehash, no memory?");
        pub(free)(new_m);
    }

    printfd("DEBUG: Rehashed.");
    return;
}


/// @Returns error.
int pub(set_pair)(Strmap *m, strmap__view key, STRMAP__TYPE value) {

    pri(rehash_if_needed)(m);

    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    // Entry exists already?

    for (int i = 0; i < bucket->pairs.size; ++i) {
        int str_internal_storage_key = bucket->pairs.items[i].key;
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            // Update existing entry.
            bucket->pairs.items[i].value = value;
            return 0;
        }
    }

    // Register new string key.

    int strpool_key_id = strpool_append(&m->strpool, strmap__view_to_strpool_str(key));
    if (strpool_key_id < 0) {
        return -1;
    }

    // Set pair.

    int err = pri(set_pair_with_final_key)(m, bucket, strpool_key_id, value);
    if (err != 0) {
        strpool_remove(&m->strpool, strpool_key_id);
        return -1;
    }

    return err;
}


static STRMAP__TYPE *pub(get)(Strmap *m, strmap__view key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    for (int i = 0; i < bucket->pairs.size; ++i) {
        int str_internal_storage_key = bucket->pairs.items[i].key;
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            return &bucket->pairs.items[i].value;
        }
    }
    return NULL;
}


/// @Returns Error.
static int pub(remove)(Strmap *m, strmap__view key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    for (int i = 0; i < bucket->pairs.size; ++i) {
        int str_internal_storage_key = bucket->pairs.items[i].key;
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            // Found it.
            int err1 = strpool_remove(&m->strpool, str_internal_storage_key);
            int err2 = pri(Pair_da_remove_at)(&bucket->pairs, i);

            if (err1 != 0) { printfd("ERROR(%d) strpool. Couldn't delete.", err1); }
            if (err2 != 0) { printfd("ERROR(%d) pair_da. Couldn't delete.", err2); }

            --m->pair_count;
            return 0;
        }
    }
    return -1;
}


/// @Returns error.
static inline int pri(set_pair_with_final_key)(Strmap *m, pri(Bucket) *bucket, int strpool_key, STRMAP__TYPE value) {
    int da_pair_id = pri(Pair_da_append)(&bucket->pairs, (pri(Pair)) { .key = strpool_key, .value = value, });
    if (da_pair_id < 0) {
        return -1;
    }
    ++m->pair_count;
    return 0;
}


size_t pub(report_memory)(Strmap *m) {
    size_t count = sizeof(Strmap);
    count += (size_t)m->buckets.size * sizeof(pri(Bucket));
    for (int i = 0; i < m->buckets.size; ++i) {
        count += (size_t)m->buckets.items[i].pairs.size * sizeof(pri(Pair));
    }
    count += strpool_report_memory(&m->strpool);
    return count;
}


#undef STRMAP__TYPE
#undef STRMAP__TOKCAT_
#undef STRMAP__TOKCAT
#undef STRMAP__NAMESPACE
#undef STRMAP__PFX
#undef STRMAP__PRI
#undef pub
#undef pri
#undef TYPE
#undef Strmap
#undef STRMAP__ALLOC_PROTOTYPE
#undef STRMAP__DEFAULT_SIZE_EXP
