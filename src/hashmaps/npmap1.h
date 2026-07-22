/*
   npmap is for nullprogram.com inspired map.

   FEATURES:
   * Strings as keys.
   * Unlimited growth.
*/

#include <stdalign.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef MAP__TYPE
#define MAP__TYPE float
#endif

#define SIZE_EXP 2

typedef struct Node {
    int key;           // Index into strpool.
    MAP__TYPE value;
} Node;

typedef struct Map {

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

uint32_t hash(uint32_t a) {
    // https://gist.github.com/badboy/6267743
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

int32_t ht_lookup(uint64_t hash, int exp, int32_t idx) {
    uint32_t mask = ((uint32_t)1 << exp) - 1;
    uint32_t step = (uint32_t)(hash >> (64 - exp) | 1);
    return (int32_t)(((uint32_t)idx + step) & mask);
}

Node **lookup(Map *m, int key) {
    uint32_t h = hash((uint32_t)key);
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
            return &m->nodes[i];
        }
        else if (node != NULL && node->key == key) { // Found it.
            return &m->nodes[i];
        }
    }
}

/// @Returns error.
int upsert(Map *m, int key, MAP__TYPE value) {
    if (m->size >= ((1 << SIZE_EXP)-1)) { return -1; }

    Node *new_node = NULL;

    Node **node_slot = lookup(m, key);
    if (node_slot == NULL) { return -1; }

    if (*node_slot == NULL) { 
        // This key didn't exist before.
        new_node = &m->items[m->size];
        new_node->key = key;

        *node_slot = new_node;
        ++m->size;
    } else {
        // Already exists.
        new_node = *node_slot;
    }

    new_node->value = value;
    return 0;
}

/// @Returns pointer to stored value or NULL.
MAP__TYPE *get(Map *m, int key) {
    Node **node_slot = lookup(m, key);
    if (node_slot == NULL || *node_slot == NULL) { return NULL; }
    return &(*node_slot)->value;
}

/// @Returns error.
int map_remove(Map *m, int key) {
    Node **node_slot = lookup(m, key);
    if (node_slot == NULL || *node_slot == NULL) { return -1; }
    *node_slot = NULL;
    --m->size;
    return 0;
}


Map create(void) {
    Map m = { 0 };
    return m;
}
