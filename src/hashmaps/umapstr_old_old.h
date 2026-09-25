#ifndef UMAPSTR_H
#define UMAPSTR_H

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

//bool strumap__noop(strpool__view a, strpool__view b, void *data) {
    //(void)a; (void)b; (void)data; wassert_msg(false, "Should never reach.");
//}

bool strumap__key_equal(strpool__view a, strpool__view b, void *data) {
    return wstrview_equals(strpool_get_from_view((Strpool*)data, a), strpool_get_from_view((Strpool*)data, a));
}

bool strumap__key_hash(strpool__view key, void *data) {
    return strumap__hash(strpool_get_from_view((Strpool*)data, key));
}
#endif

//#define UMAPSTR__TOKCAT_(a, b) a ## b
//#define UMAPSTR__TOKCAT(a, b) UMAPSTR__TOKCAT_(a, b)
//#ifndef UMAPSTR__NAMESPACE
//#define UMAPSTR__NAMESPACE UMAPSTR__TOKCAT(UMap_, UMAPSTR__TYPE)
//#endif
//#define UMAPSTR__PFX(name) UMAPSTR__TOKCAT(UMAPSTR__TOKCAT(UMAPSTR__NAMESPACE, _), name)
//#define UMAPSTR__PRI(name) UMAPSTR__TOKCAT(UMAPSTR__TOKCAT(UMAPSTR__NAMESPACE, __), name)

#define UMAP__TYPE UMAPSTR__TYPE
#define UMAP__KEY strpool__view
#define UMAP__KEY_EQUAL strumap__key_equal
#define UMAP__KEY_HASH  strumap__key_hash
#define UMAP__NAMESPACE Strumap__umap
#include "umap.h"

typedef struct {
    Strpool strpool;
    Strumap__umap umap;
} Strumap;

void Strumap_create(Strumap *m) {
    *m = (Strumap) { 0 };
    strpool_create(&m->strpool);
    Strumap__umap_create(&m->umap);
}

void Strumap_free(Strumap *m) {
    strpool_destroy(&m->strpool);
    Strumap__umap_free(&m->umap);
    *m = (Strumap) { 0 };
}


/// @Note. Cannot fail.
static inline Strumap__umap__Bucket *strumap__hash_and_get_bucket(Strumap *m, strview_t key) {
    uint64_t hash = strumap__hash(key);
    int id = (int)(hash & (uint64_t)(m->umap.buckets.size - 1));
    return &m->umap.buckets.items[id];
}

//int strumap_upsert(Strumap *m, strview_t key_str, UMAPSTR__TYPE value) {
    //// Entry exists?
    //Strumap__umap__Bucket *bucket = strumap__hash_and_get_bucket(m, key_str);
    //for (int i = 0; i < bucket->pairs.size; ++i) {
        //strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        //if (wstrview_equals(saved_key, key_str)) {
            //bucket->pairs.items[i].value = value; // Update.
            //return 0;
        //}
    //}
    //// Else insert.
    //Strumap__umap__rehash_if_needed(&m->umap);
    //int key = strpool_append(&m->strpool, key_str);
    //if (key < 0) { return -1; }
    //int err = Strumap__umap__set_pair_with_final_key(&m->umap, bucket, key, value);
    //if (err) { strpool_remove(&m->strpool, key); }
    //return err;
//}


//static UMAPSTR__TYPE *strumap_get(Strumap *m, strview_t key_str) {
    //Strumap__umap__Bucket *bucket = strumap__hash_and_get_bucket(m, key_str);
    //for (int i = 0; i < bucket->pairs.size; ++i) {
        //strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        //if (wstrview_equals(saved_key, key_str)) {
            //return &bucket->pairs.items[i].value;
        //}
    //}
    //return NULL;
//}


///// @Returns Error.
//static int strumap_remove(Strumap *m, strview_t key_str) {
    //Strumap__umap__Bucket *bucket = strumap__hash_and_get_bucket(m, key_str);
    //for (int i = 0; i < bucket->pairs.size; ++i) {
        //strview_t saved_key = strpool_get(&m->strpool, bucket->pairs.items[i].key);
        //if (wstrview_equals(saved_key, key_str)) {
            //// Found it.
            //int err = Strumap__umap__Vec_Pair_remove_at(&bucket->pairs, i);
            //if (err != 0) { printfd("ERROR(%d) Vec_Pair. Couldn't delete.", err); }
            //--m->umap.pair_count;
            //return 0;
        //}
    //}
    //return -1;
//}


#endif
