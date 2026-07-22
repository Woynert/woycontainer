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

typedef struct ht {
    char *ht[1 << SIZE_EXP];
} ht;

typedef struct Node {
    bool used;
    int key;           // Index into strpool.
    MAP__TYPE value;
} Node;

typedef struct Map {
    Node items[1 << SIZE_EXP];
    int pair_count; // Number of valid items stored.
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

Node *lookup(Map *m, int key) {
    uint32_t h = hash((uint32_t)key);
    for (int i = (int)h;;)
    {
        i = ht_lookup(h, SIZE_EXP, i);
        Node *node = &m->items[i];

        if (!node->used) { // Found empty or available!

            if (m->pair_count == ((1 << SIZE_EXP) -1)) {
                /* Refusing to give away the last empty element. We
                   need at least one empty element to know when to stop
                   iterating. Also: We're out of memory. */
                return NULL;
            }
            return node;
        }
        else if (node->key == key) { // Found it.
            return node;
        }
    }
}

/// @Returns error.
int upsert(Map *m, int key, MAP__TYPE value) {
    Node *node = lookup(m, key);
    if (node == NULL) { return -1; }
    if (!node->used) { 
        // This key didn't exist before.
        ++m->pair_count;
    }
    node->used = true;
    node->key = key;
    node->value = value;
    return 0;
}

/// @Returns pointer to stored value or NULL.
MAP__TYPE *get(Map *m, int key) {
    Node *node = lookup(m, key);
    if (node == NULL || !node->used) { return NULL; }
    return &node->value;
}

/// @Returns error.
int map_remove(Map *m, int key) {
    Node *node = lookup(m, key);
    if (node == NULL || !node->used) { return -1; }
    node->used = false;
    --m->pair_count;
    return 0;
}


Map create(void) {
    Map m = { 0 };
    return m;
}
