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

#define SIZE_EXP 2

typedef struct Node {
    int key;           // Index into strpool.
    MAP__TYPE value;
} Node;

typedef struct Map {
    strpool strpool;

    Node items[1 << SIZE_EXP];
    int size; // Number of valid items stored.

    struct {
        // The hashmap is separate from our main collection.
        Node *nodes[1 << SIZE_EXP];

        // You could add other indexes here if you wanted.
        // Node *nodes_with_keys_starting_in_1[1 << SIZE_EXP];

        // Note: In this specific case since we're storing references to array
        // entries we can instead store integer indexes instead of pointers.
        // But then zero won't work as default empty value. (Because it's a
        // Valid index). So You would have to initialize all the hashmap to -1.
    };

} Map;

bool str_equals(strpool__str str1, strpool__str str2) {
    if (str1.size != str2.size) { return false; }
    return !str1.size || !memcmp(str1.data, str2.data, (size_t)str1.size);
    // str1.size seems superfluous but it's necessary.
    // See https://nullprogram.com/blog/2025/01/19/#strings
}


uint64_t hash_str64(strpool__str s)
{
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < s.size; i++) {
        h ^= s.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}

int32_t ht_lookup(uint64_t hash, int exp, int32_t idx) {
    uint32_t mask = ((uint32_t)1 << exp) - 1;
    uint32_t step = (uint32_t)(hash >> (64 - exp) | 1);
    return (int32_t)(((uint32_t)idx + step) & mask);
}

Node **lookup(Map *m, strpool__str key, int *out_idx) {
    uint64_t h = hash_str64(key);
    for (int i = (int)h;;)
    {
        i = ht_lookup(h, SIZE_EXP, i);
        Node *node = m->nodes[i];

        if (node == NULL) { // Found empty or available!
            if (m->size == ((1 << SIZE_EXP) -1)) {
                /* Refusing to give away the last empty element. We
                   need at least one empty element to know when to stop
                   iterating. Also: We're out of memory. */
                return NULL;
            }
            if (out_idx != NULL) { *out_idx = i; }
            return &m->nodes[i];
        }
        else if (node != NULL) {
            // Get string from strpol
            strpool__str stored_key = strpool_get(&m->strpool, node->key);
            if (stored_key.data == NULL) {
                printfd("error: Should never happen.");
            }
            if (str_equals(key, stored_key)) { // Found it.
                if (out_idx != NULL) { *out_idx = i; }
                return &m->nodes[i];
            }
        }
    }
}

/// @Returns error.
int upsert(Map *m, strpool__str key, MAP__TYPE value) {
    if (m->size >= ((1 << SIZE_EXP)-1)) { return -1; }

    Node *new_node = NULL;
    int new_node_idx = -1;

    Node **node_slot = lookup(m, key, &new_node_idx);

    if (node_slot == NULL) {
        // Out of memory.
        // TODO: Regrow, rehash and try again.
        return -1;
    }

    if (*node_slot == NULL) { 
        // This key didn't exist before.

        int idx = strpool_append(&m->strpool, key);
        if (idx == -1) { return -1; }

        new_node = &m->items[new_node_idx];
        new_node->key = idx;

        *node_slot = new_node;
        ++m->size;
    } else {
        // Already exists.
        new_node = *node_slot;
    }

    printfd("Set it here mate! SUCESSS item_id (%d)", (int)((ptrdiff_t)(new_node - m->items)));
    new_node->value = value;
    return 0;
}

/// @Returns pointer to stored value or NULL.
MAP__TYPE *get(Map *m, strpool__str key) {
    Node **node_slot = lookup(m, key, NULL);
    if (node_slot == NULL || *node_slot == NULL) { return NULL; }
    return &(*node_slot)->value;
}

/// @Returns error.
int map_remove(Map *m, strpool__str key) {
    //int node_id = -1;
    Node **node_slot = lookup(m, key, NULL);
    if (node_slot == NULL || *node_slot == NULL) { return -1; }
    strpool_remove(&m->strpool, (*node_slot)->key);
    *node_slot = NULL;
    --m->size;
    return 0;
}

Map create(void) {
    Map m = { 0 };
    strpool_create(&m.strpool);
    return m;
}

void map_free(Map *m) {
    strpool_destroy(&m->strpool);
}

/*
npmap0.h
npmap1.h
npmap1_test.c
npmap_str0.h
npmap_str1.h
npmap_str1_test.c
npmap_str0_test.c
npmap0_test.c


npmapA.h
npmapA_test.c
npmapB.h
npmapB_test.c
npmap_strA.h
npmap_strA_test.c
npmap_strB.h
npmap_strB_test.c
   */
