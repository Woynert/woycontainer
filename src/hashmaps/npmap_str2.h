/*
   npmap is for nullprogram.com inspired map.

   FEATURES:
   * Strings as keys.
   * Unlimited growth. ????? Show me then.
*/

#include <stdalign.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../strpool.h"
#include "src/portable_utils.h"


#ifndef MAP__TYPE
#define MAP__TYPE float
#endif
#define MAP__TOKCAT_(a, b) a ## b
#define MAP__TOKCAT(a, b) MAP__TOKCAT_(a, b)
#ifndef MAP__NAMESPACE
#define MAP__NAMESPACE MAP__TOKCAT(MAP__TYPE, _strmap)
#endif
#define pub(name) MAP__TOKCAT(MAP__TOKCAT(MAP__NAMESPACE, _), name)
#define pri(name) MAP__TOKCAT(MAP__TOKCAT(MAP__NAMESPACE, __), name)
#define Map MAP__NAMESPACE
#define MAP__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
#define MAP__DEFAULT_SIZE_EXP 8
#define MAP__REHASH_FACTOR 0.7


typedef struct pri(Node) {
    MAP__TYPE value;
    int key;           // Index into strpool.
} pri(Node);


typedef struct Map {
    strpool strpool;

    pri(Node) *items;
    int size_exp;
    int count;       // Number of valid items stored.

    struct {
        pri(Node) **hashmap;  // Hashmap is independent from our main container.
        pri(Node) gravestone;
    };

    MAP__ALLOC_PROTOTYPE(*allocator);
    void *allocator_userdata;
} Map;


/* A node is invalid when it points to the gravestone. */
static inline bool pri(node_is_gravestone)  (const Map *m, const pri(Node) *node) { return node == &m->gravestone; }
static inline bool pri(node_is_empty) (const Map *m, const pri(Node) *node) { return node == NULL || pri(node_is_gravestone(m, node)); }


static MAP__ALLOC_PROTOTYPE(pri(default_allocator));


uint64_t pri(hash_str64)(strview_t s) {
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < s.size; i++) {
        h ^= s.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}


/*
int32_t pri(ht_lookup)(uint64_t hash, int exp, int32_t idx) {
    uint32_t mask = ((uint32_t)1 << exp) - 1;
    uint32_t step = (uint32_t)(hash >> (64 - exp) | 1);
    return (int32_t)(((uint32_t)idx + step) & mask);
}
*/


pri(Node) **pri(lookup)(Map *m, strview_t key, int *out_idx, bool trying_to_insert) {
    pri(Node) **dest = NULL;
    uint64_t h = pri(hash_str64)(key);

    uint32_t mask = ((uint32_t)1 << m->size_exp) - 1;          // MSI
    uint32_t step = (uint32_t)(h >> (64 - m->size_exp) | 1);   // MSI

    for (int i = (int)h;;)
    {
       i = (int32_t)(((uint32_t)i + step) & mask);             // MSI

        pri(Node) *node = m->hashmap[i];

        if (node == NULL) {                                     // Found empty.
            if (m->count == ((1 << m->size_exp) -1)) {
                return NULL;
                /* Refusing to give away the last empty element. We
                   need at least one empty element to know when to stop
                   iterating. Also: We're out of memory. */
            }
            
            dest = dest ? dest : &m->hashmap[i];
            if (out_idx != NULL) { *out_idx = (int)(dest - m->hashmap); }
            return dest;
        }
        else if (pri(node_is_gravestone)(m, node)) {
            if (trying_to_insert) {
                dest = dest ? dest : &m->hashmap[i];
                /* Found gravestone. If no entry is found, it'll use the
                   first gravestone found. */
            }
            // When searching, skip over it.
        }
        else { // Compare keys.
            strview_t stored_key = strpool_get(&m->strpool, node->key);
            if (stored_key.data == NULL) { printfd("Error: Invalid key should never happen."); }
            if (wstrview_equals(key, stored_key)) { // Found it.
                if (out_idx != NULL) { *out_idx = i; }
                return &m->hashmap[i];
            }
        }
    }
}

int pri(grow)(Map *old_m, int new_size_exp);

/// @Returns error.
int pub(upsert)(Map *m, strview_t key, MAP__TYPE value) {
    float factor = (float)m->count / (float)(1 << m->size_exp);
    if (factor > MAP__REHASH_FACTOR) {
        pri(grow)(m, m->size_exp +1);  // Regrow & rehash if pushing the optimal factor.
    }

    pri(Node) *new_node = NULL;
    int new_node_idx = -1;

    pri(Node) **node_slot = pri(lookup)(m, key, &new_node_idx, true);

    if (node_slot == NULL || new_node_idx == -1) { return -1; }

    if (pri(node_is_empty)(m, *node_slot)) {
        // This key didn't exist before.

        int idx = strpool_append(&m->strpool, key);
        if (idx == -1) { return -1; }

        new_node = &m->items[new_node_idx];
        new_node->key = idx;

        *node_slot = new_node;
        ++m->count;
    } else {
        // Already exists.
        new_node = *node_slot;
    }

    new_node->value = value;
    return 0;
}

/// @Returns pointer to stored value or NULL.
MAP__TYPE *pub(get)(Map *m, strview_t key) {
    pri(Node) **node_slot = pri(lookup)(m, key, NULL, false);
    if (node_slot == NULL || *node_slot == NULL) { return NULL; }
    return &(*node_slot)->value;
}

/// @Returns error.
int pub(remove)(Map *m, strview_t key) {
    pri(Node) **node_slot = pri(lookup)(m, key, NULL, false);
    if (node_slot == NULL || *node_slot == NULL) { return -1; }
    strpool_remove(&m->strpool, (*node_slot)->key);
    *node_slot = &m->gravestone;
    --m->count;
    return 0;
}


void pri(rehash)(Map *old_m, Map *new_m);

int pri(grow)(Map *old_m, int new_size_exp) {

    if (new_size_exp < old_m->size_exp) { return -1; }

    Map new_map = { 0 };
    Map *new_m = &new_map;

    new_m->size_exp = new_size_exp;
    new_m->allocator = old_m->allocator;
    new_m->allocator_userdata = old_m->allocator_userdata;
    new_m->strpool = old_m->strpool;

    MAP__ALLOC_PROTOTYPE(*allocator) = new_m->allocator ? new_m->allocator : pri(default_allocator);

    new_m->items   = (pri(Node)*) allocator(NULL, (size_t)(1 << new_m->size_exp) * sizeof(pri(Node)) , alignof(pri(Node)), new_m->allocator_userdata);
    new_m->hashmap = (pri(Node)**)allocator(NULL, (size_t)(1 << new_m->size_exp) * sizeof(pri(Node)*), alignof(pri(Node)*), new_m->allocator_userdata);

    if (new_m->items == NULL || new_m->hashmap == NULL) {
        if (new_m->items   != NULL) { allocator(new_m->items  , 0, 0, old_m->allocator_userdata); }
        if (new_m->hashmap != NULL) { allocator(new_m->hashmap, 0, 0, old_m->allocator_userdata); }
        return -1;
    }

    memset(new_m->items  , 0, (1 << new_m->size_exp) * sizeof(*new_m->items));
    memset(new_m->hashmap, 0, (1 << new_m->size_exp) * sizeof(*new_m->hashmap));

    if (old_m->size_exp > 0) { pri(rehash)(old_m, new_m); }

    // Free old map.

    if (old_m->items != NULL) { allocator(old_m->items  , 0, 0, old_m->allocator_userdata); }
    if (old_m->items != NULL) { allocator(old_m->hashmap, 0, 0, old_m->allocator_userdata); }

    *old_m = *new_m;

    return 0;
}


// @Note. Should only be called from 'grow' function.
void pri(rehash)(Map *old_m, Map *new_m) {

    // Rehash every valid entry.

    for (int i = 0; i < (1 << old_m->size_exp); ++i) {
        pri(Node) *node = old_m->hashmap[i];
        if (pri(node_is_empty)(old_m, node)) { continue; }

        strview_t stored_key = strpool_get(&old_m->strpool, node->key);
        if (stored_key.data == NULL) { printfd("ERR: Couldn't find stored key."); continue; }

        // Insert.

        int item_id;
        pri(Node) **new_node = pri(lookup)(new_m, stored_key, &item_id, true);
        *new_node = &new_m->items[item_id];
        (*new_node)->key = node->key;
        (*new_node)->value = node->value;
        ++new_m->count;
    }

    printfd("DEBUG: Rehashed.");
}


/// @Returns error.
int pub(create_with_allocator)(Map *m, MAP__ALLOC_PROTOTYPE(*allocator), void *user_data) {
    *m = (Map) { 0 };
    m->allocator = allocator;
    m->allocator_userdata = user_data;
    strpool_create_with_allocator(&m->strpool, allocator, user_data);
    return pri(grow)(m, MAP__DEFAULT_SIZE_EXP);
}


/// @Returns error.
int pub(create)(Map *m) {
    return pub(create_with_allocator)(m, NULL, NULL);
}


void pub(free)(Map *m) {
    strpool_destroy(&m->strpool);
    MAP__ALLOC_PROTOTYPE(*allocator) = m->allocator ? m->allocator : pri(default_allocator);
    if (m->items != NULL) { allocator(m->items  , 0, 0, m->allocator_userdata); }
    if (m->items != NULL) { allocator(m->hashmap, 0, 0, m->allocator_userdata); }
    *m = (Map) { 0 };
}


size_t pub(report_memory)(Map *m) {
    size_t count = sizeof(Map);
    count += (1 << m->size_exp) * sizeof(*m->items);
    count += (1 << m->size_exp) * sizeof(*m->hashmap);
    count += strpool_report_memory(&m->strpool);
    return count;
}


static MAP__ALLOC_PROTOTYPE(pri(default_allocator)) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0
    (void)align; (void)user_data; // Unused: malloc guarantees alignment.
    void* result = NULL;
    if      (size == 0)   { free(ptr);                   }
    else if (ptr == NULL) { result = malloc(size);       }
    else                  { result = realloc(ptr, size); }
    return result;
}


#undef MAP__TYPE
#undef MAP__TOKCAT_
#undef MAP__TOKCAT
#undef MAP__NAMESPACE
#undef pub
#undef pri
#undef Map
#undef MAP__ALLOC_PROTOTYPE
#undef MAP__DEFAULT_SIZE_EXP
