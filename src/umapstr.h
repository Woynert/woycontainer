/*
   Unordered hash map with strings as keys.
   */

#ifndef UMAPSTR__TYPE
#define UMAPSTR__TYPE double
#endif

#include "strpool.h"
#include "wstrview.h"
#include "portable_utils.h"

#ifndef UMAPSTR_H__UTILS
#define UMAPSTR_H__UTILS
uint64_t strumap__hash(strview_t view) {
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < view.size; i++) {
        h ^= view.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
    // https://nullprogram.com/blog/2025/01/19/
}

bool strumap__noop(ID a, ID b, void *data) {
    (void)a; (void)b; (void)data; wassert_msg(false, "Should never reach.");
}

uint64_t strumap__key_hash(ID key, void *data) {
    Strpool *strpool = (Strpool*)data;
    wassert(wstrview_is_valid(strpool_get(strpool, key)));
    return strumap__hash(strpool_get(strpool, key));
}
#endif

#define UMAPSTR__TOKCAT_(a, b) a ## b
#define UMAPSTR__TOKCAT(a, b) UMAPSTR__TOKCAT_(a, b)
#ifndef UMAPSTR__NAMESPACE
#define UMAPSTR__NAMESPACE UMAPSTR__TOKCAT(Strumap_, UMAPSTR__TYPE)
#endif
#define UMAPSTR__PFX(name) UMAPSTR__TOKCAT(UMAPSTR__TOKCAT(UMAPSTR__NAMESPACE, _), name)
#define UMAPSTR__PRI(name) UMAPSTR__TOKCAT(UMAPSTR__TOKCAT(UMAPSTR__NAMESPACE, __), name)

#define UMAP__TYPE UMAPSTR__TYPE
#define UMAP__KEY ID
#define UMAP__KEY_EQUAL strumap__noop
#define UMAP__KEY_HASH  strumap__key_hash
#define UMAP__NAMESPACE UMAPSTR__PRI(Umap)
#include "umap.h"

#define pub UMAPSTR__PFX
#define pri UMAPSTR__PRI
#define KEY UMAPSTR__KEY
#define TYPE UMAPSTR__TYPE
#define Strumap UMAPSTR__NAMESPACE
#define UMAPSTR__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)

typedef struct {
    Strpool strpool;
    pri(Umap) umap;
} Strumap;

typedef struct pub(It) {
    TYPE *value;
    strview_t key;
    int __bucket_id;
    int __pair_id;
} pub(It);


int            pub(create_with_allocator)(Strumap *m, UMAPSTR__ALLOC_PROTOTYPE(*allocator), void *user_data);
int            pub(create)    (Strumap *m);
void           pub(free)      (Strumap *m);
int            pub(upsert)    (Strumap *m, strview_t key_str, UMAPSTR__TYPE value);
int            pub(remove)    (Strumap *m, strview_t key_str);
int            pub(pair_count)(const Strumap *m) { return m->umap.pair_count; }
UMAPSTR__TYPE *pub(get)       (const Strumap *m, strview_t key_str);
bool           pub(it_next)   (const Strumap *m, pub(It) *it);
static inline  pri(Umap__Bucket) *pri(hash_and_get_bucket)(const Strumap *m, strview_t key);


int pub(create_with_allocator)(Strumap *m, UMAPSTR__ALLOC_PROTOTYPE(*allocator), void *user_data) {
    *m = (Strumap) { 0 };
    int err = strpool_create_with_allocator(&m->strpool, allocator, user_data);
    if (err) { return -1; }
    err = pri(Umap_create_with_allocator)(&m->umap, allocator, user_data);
    if (err) { strpool_destroy(&m->strpool); return -1; }
    m->umap.userdata = &m->strpool;
    return 0;
}

int pub(create)(Strumap *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}

void pub(free)(Strumap *m) {
    strpool_destroy(&m->strpool);
    pri(Umap_free)(&m->umap);
    *m = (Strumap) { 0 };
}


/// @Note. Cannot fail.
static inline pri(Umap__Bucket) *pri(hash_and_get_bucket)(const Strumap *m, strview_t key) {
    uint64_t hash = strumap__hash(key);
    int id = (int)(hash & (uint64_t)(m->umap.buckets.size - 1));
    return &m->umap.buckets.items[id];
}


int pub(upsert)(Strumap *m, strview_t key_str, UMAPSTR__TYPE value) {
    // Note: Rehash at beginning so we can calculate the bucket hash only once...
    //
    int err = pri(Umap__rehash_if_needed)(&m->umap);
    // Entry exists?
    pri(Umap__Bucket) *bucket = pri(hash_and_get_bucket)(m, key_str);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        if (wstrview_equals(saved_key, key_str)) {
            bucket->pairs.items[i].value = value; // Update.
            return 0;
        }
    }
    // Else insert.
    if (err) { printferr("Couldn't rehash."); }
    ID key = strpool_append(&m->strpool, key_str);
    if (!ID_valid(key)) { return -1; }
    err = pri(Umap__set_pair_with_final_key)(&m->umap, bucket, key, value);
    if (err) { strpool_remove(&m->strpool, key); }
    return err;
}


UMAPSTR__TYPE *pub(get)(const Strumap *m, strview_t key_str) {
    pri(Umap__Bucket) *bucket = pri(hash_and_get_bucket)(m, key_str);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        if (wstrview_equals(saved_key, key_str)) {
            return &bucket->pairs.items[i].value;
        }
    }
    return NULL;
}


/// @Returns Error.
int pub(remove)(Strumap *m, strview_t key_str) {
    pri(Umap__Bucket) *bucket = pri(hash_and_get_bucket)(m, key_str);
    for (int i = 0; i < bucket->pairs.size; ++i) {
        strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        if (wstrview_equals(saved_key, key_str)) {
            // Found it.
            int err = pri(Umap__Vec_Pair_remove_at)(&bucket->pairs, i);
            if (err != 0) { printfd("ERROR(%d) Vec_Pair. Couldn't delete.", err); }
            --m->umap.pair_count;
            return 0;
        }
    }
    return -1;
}

/// @Note. Modifying the map while iterating is UB.
bool pub(it_next)(const Strumap *m, pub(It) *it) {
    for (; it->__bucket_id < m->umap.buckets.size; ++it->__bucket_id, it->__pair_id = 0) {
        while (it->__pair_id < m->umap.buckets.items[it->__bucket_id].pairs.size) {
            ID key_internal = m->umap.buckets.items[it->__bucket_id].pairs.items[it->__pair_id].key;
            it->key = strpool_get(&m->strpool, key_internal);
            it->value = &m->umap.buckets.items[it->__bucket_id].pairs.items[it->__pair_id].value;
            ++it->__pair_id;
            return true;
        }
    }
    return false;
}

#undef UMAPSTR__TYPE
#undef UMAPSTR__TOKCAT_
#undef UMAPSTR__TOKCAT
#undef UMAPSTR__NAMESPACE
#undef UMAPSTR__PFX
#undef UMAPSTR__PRI
#undef pub
#undef pri
#undef KEY
#undef TYPE
#undef Strumap
#undef UMAPSTR__ALLOC_PROTOTYPE
