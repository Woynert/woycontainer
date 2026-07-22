/*
   String hash map.

   Notes:
   * Con: pop_at_preserve_order Is slow but we have no other option
     since we spit the keys and values so they cannot be unordered...
   * TODO: Maybe join key and value so we don't need this cost.
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


#define DYNA__TYPE int
#define DYNA__NAMESPACE STRMAP__PRI(key_da)
#include "da.h"


#define DYNA__TYPE STRMAP__TYPE
#define DYNA__NAMESPACE STRMAP__PRI(value_da)
#include "da.h"


typedef struct STRMAP__PRI(Bucket) {
    /* Values and keys are separated collections so that
       it's faster to iterate the keys individually. */
    STRMAP__PRI(key_da) keys;
    STRMAP__PRI(value_da) values;
} STRMAP__PRI(Bucket);


#define ARRAY__TYPE STRMAP__PRI(Bucket)
#include "array.h"


#define pub STRMAP__PFX
#define pri STRMAP__PRI
#define TYPE STRMAP__TYPE
#define Strmap STRMAP__NAMESPACE

#define STRMAP__ALLOC_PROTOTYPE(x) void* (x) (void* user_data, void* ptr, size_t size, int align)


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
        bucket->keys   = pri(key_da_create_with_allocator)(m->allocator, m->allocator_userdata);
        bucket->values = pri(value_da_create_with_allocator)(m->allocator, m->allocator_userdata);
    }
    return 0;
}


int pub(create_with_allocator)(Strmap *m, STRMAP__ALLOC_PROTOTYPE(*allocator), void *allocator_userdata) {
    *m = (Strmap) { 0 };

    int err = strpool_create_with_allocator(&m->strpool, allocator, allocator_userdata);
    if (err != 0) {
        return -1;
    }

    m->allocator = allocator;
    m->allocator_userdata = allocator_userdata;
    m->buckets = pri(Bucket_Array_create_with_allocator)(allocator, allocator_userdata);


    return pri(grow)(m, 8); // DEFAULT CAPACITY.
}


int pub(create)(Strmap *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}

void pub(free)(Strmap *m) {
    for (int i = 0; i < m->buckets.size; ++i) {
        pri(Bucket) *bucket = &m->buckets.items[i];
        pri(key_da_free)(&bucket->keys);
        pri(value_da_free)(&bucket->values);
    }
    pri(Bucket_Array_destroy)(&m->buckets);
    strpool_destroy(&m->strpool);
    *m = (Strmap) { 0 };
}

//#define FAST_MODULO (size_t)(hash & (uint64_t)(table->capacity - 1))

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
    return memcmp(str1.data, str1.data, (size_t)str1.size) == 0;
}

#endif // !STRMAP__UTILS



static inline pri(Bucket) *pri(hash_and_get_bucket)(Strmap *m, strmap__view key) {
    uint64_t hash = strmap__hash(key);
    int bucket_id = (int)(hash & (uint64_t)(m->buckets.size - 1));
    return &m->buckets.items[bucket_id];
}


void pri(rehash_if_needed)(Strmap *old_m) {
    float factor = (float)old_m->pair_count / (float)old_m->buckets.size;
    if (factor < 0.75) {
        return;
    }

    // Create new map.

    Strmap new_map;
    Strmap *new_m = &new_map;

    int err = pub(create_with_allocator)(new_m, old_m->allocator, old_m->allocator_userdata);
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
        for (int j = 0; j < bucket->keys.size; ++j) {

            // Insert every single pair.

            strmap__view key = strmap__strpool_view(strpool_get(&old_m->strpool, bucket->keys.items[i]));
            //uint64_t hash = strmap__hash(key);
            //int bucket_id = (int)(hash & (uint64_t)(new_m->buckets.size - 1));
            //pri(Bucket) *new_bucket = &new_m->buckets.items[bucket_id];
            pri(Bucket) *new_bucket = pri(hash_and_get_bucket)(new_m, key);

            err = pri(set_pair_with_final_key)(new_m, new_bucket, bucket->keys.items[i], bucket->values.items[i]);
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

    return;
}


/// @Returns error.
int pub(set_pair)(Strmap *m, strmap__view key, STRMAP__TYPE value) {
    //uint64_t hash = strmap__hash(key);
    //int bucket_id = (int)(hash & (uint64_t)(m->buckets.size - 1));
    //pri(Bucket) *bucket = &m->buckets.items[bucket_id];
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    // Entry exists already?

    for (int i = 0; i < bucket->keys.size; ++i) {
        int str_internal_storage_key = bucket->keys.items[i];
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            // Update existing entry.
            bucket->values.items[i] = value;
            return 0;
        }
    }

    // Register new string key.

    int strpool_key_id = strpool_append(&m->strpool, strmap__view_to_strpool_str(key));
    if (strpool_key_id < 0) {
        printfd("A");
        return -1;
    }

    // Set pair.

    int err = pri(set_pair_with_final_key)(m, bucket, strpool_key_id, value);
    if (err != 0) {
        strpool_remove(&m->strpool, strpool_key_id);
        printfd("B");
        return -1;
    }

    pri(rehash_if_needed)(m);

    return err;
}


static STRMAP__TYPE *pub(get)(Strmap *m, strmap__view key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    for (int i = 0; i < bucket->keys.size; ++i) {
        int str_internal_storage_key = bucket->keys.items[i];
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            return &bucket->values.items[i];
        }
    }
    return NULL;
}


/// @Returns Error.
static int pub(remove)(Strmap *m, strmap__view key) {
    pri(Bucket) *bucket = pri(hash_and_get_bucket)(m, key);

    for (int i = 0; i < bucket->keys.size; ++i) {
        int str_internal_storage_key = bucket->keys.items[i];
        strmap__view stored_key = strmap__strpool_view(strpool_get(&m->strpool, str_internal_storage_key));

        if (strmap__view_equals(stored_key, key))
        {
            // Found it.
            int err = strpool_remove(&m->strpool, str_internal_storage_key);
            err += pri(key_da_pop_at_preserve_order)(&bucket->keys, i, NULL);
            err += pri(value_da_pop_at_preserve_order)(&bucket->values, i, NULL);
            if (err != 0) {
                printfd("ERROR %d. Couldn't fully delete.", err);
            }
            return 0;
        }
    }
    return -1;
}


/// @Returns error.
static inline int pri(set_pair_with_final_key)(Strmap *m, pri(Bucket) *bucket, int strpool_key, STRMAP__TYPE value) {

    int da_key_id      = pri(key_da_append)(&bucket->keys, strpool_key);
    int da_value_id    = pri(value_da_append)(&bucket->values, value);
    if (da_key_id == -1
        || da_value_id == -1
    ) {
        if (da_key_id   >= 0) { pri(key_da_pop_at_preserve_order)(&bucket->keys, da_key_id, NULL); }
        if (da_value_id >= 0) { pri(value_da_pop_at_preserve_order)(&bucket->values, da_value_id, NULL); }
        return -1; // Abort.
    }

    ++m->pair_count;
    return 0;
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
