/*
   Stringpool data structure.

   FEATURES:
   * Pro: Stable user facing ids.
   * Pro: Unlimited growth.
   * Pro: Fast insert.
   * Pro: Fast deletion.
   * Neutral: Finds space using a "free list". (Red-Black tree would be better).
   * Con: Previously deleted ids will be reutilized often.
 */

#ifndef STRPOOL_GENERAL
#define STRPOOL_GENERAL

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STRMAP__ALLOC_PROTOTYPE(x) void* (x) (void* user_data, void* ptr, ssize_t size, int align)
static STRMAP__ALLOC_PROTOTYPE(strmap__default_allocator);

typedef struct strpool__str {
    char *data;
    int size;
} strpool__str;

typedef struct strpool__view {
    int offset; /* Offset from buf start. Aka. index into pool->nodes[]. */
    int size;
} strpool__view;


#define SLOT__TYPE strpool__view
#include "slot.h"

typedef struct strpool__Node strpool__Node;
typedef struct strpool__Node {
    int free_chunks;
    int i_next_node; // Index to nodes.
} strpool__Node;

#define STRPOOL_CHUNK ((int)sizeof(strpool__Node))

typedef struct strpool{
    struct {
        int capacity; // Measured in chunks or nodes.
        int cursor;
        strpool__Node *nodes;
    };

    int i_first_free_node; // Index to nodes[].

    strpool__view_Slot views; // Maps user facing id to an internal strpool__view.

} strpool;


/// @Returns Error.
int strpool_create(strpool *p) {
    *p = (strpool) { 0 };
    p->capacity = 8; // DEFAULT CAPACITY.

    p->nodes = (strpool__Node *)malloc((size_t)p->capacity * STRPOOL_CHUNK);
    if (p->nodes == NULL) {
        return -1;
    }

    int err = strpool__view_Slot_create(&p->views);
    if (err != 0) {
        free(p->nodes);
        return -1;
    }

    // Write strpool__Node at beginning.

    p->i_first_free_node = 0;
    strpool__Node *node = &p->nodes[0];
    node->i_next_node = -1;
    node->free_chunks = p->capacity;

    return 0;
}

void strpool_destroy(strpool *p) {
    free(p->nodes);
    strpool__view_Slot_free(&p->views);
    *p = (strpool) { 0 };
}


strpool__Node *strpool__get_node(strpool *p, int i) {
    if (i < 0 || i >= p->capacity) { return NULL; }
    return &p->nodes[i];
}


/// @Returns id or -1 on error.
int strpool__find_space(strpool *p, int space, int *out_i_prev_node) {
    int i_prev_node = -1;
    int i_node = p->i_first_free_node; 
    strpool__Node *node = strpool__get_node(p, i_node);
    while (node != NULL) {
        if ((node->free_chunks * STRPOOL_CHUNK) >= space) {
            *out_i_prev_node = i_prev_node;
            return i_node;
        } else {
            i_prev_node = i_node;
            i_node = node->i_next_node;
            if (i_node < i_prev_node) {
                return -1;
            }
            node = strpool__get_node(p, i_node);
        }
    }
    return -1;
}

/// @Returns id.
/// @Retval -1 If not found (or root).
int strpool__find_node_just_before(strpool *p, int target_offset) {
    int i_prev_node = -1;
    int i_node = p->i_first_free_node;
    for (;;) {
        strpool__Node *node = strpool__get_node(p, i_node);
        if (node == NULL) { break; }
        if (i_node >= target_offset) { break; }
        i_prev_node = i_node;
        i_node = node->i_next_node;
        if (i_node < i_prev_node) { break; }
    }
    return i_prev_node;
}


static inline int strpool__int_max(int x, int y) { return x > y ? x : y; }

// https://stackoverflow.com/questions/2745074
// WARNING: Only positive integers!!!
static inline int strpool__div_ceil(int x, int y) {
    return (x % y) ? x / y + 1 : x / y;
}

// https://stackoverflow.com/a/365068
// Round up to next higher power of 2 (return x if it's already a power of 2).
static inline int strpool__pow2roundup (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}


void strpool__integrate_new_free_node(strpool *p, int i_curr, int curr_chunks) {
    int i_prev = strpool__find_node_just_before(p, i_curr);
    int i_next = -1;
    strpool__Node *node_prev = strpool__get_node(p, i_prev);
    strpool__Node *node_curr = strpool__get_node(p, i_curr);

    // Try join with prev.
    if (node_prev != NULL) {
        if (node_prev->i_next_node == i_curr) {
            // Should never happen. But still let's keep the check.
            node_prev->i_next_node = node_curr->i_next_node;
        }
        i_next = node_prev->i_next_node;
        if (i_prev + node_prev->free_chunks == i_curr) {
            node_prev->free_chunks += curr_chunks;
            i_curr = i_prev;
            node_curr = node_prev;
        }
        // Cannot join with previous, in that case, create it's own node.
        else {
            node_prev->i_next_node = i_curr;
            node_curr->free_chunks = curr_chunks;
            node_curr->i_next_node = i_next;
        }
    }
    // No previous node means it is root.
    else {
        i_next = p->i_first_free_node;
        node_curr->free_chunks = curr_chunks;
        node_curr->i_next_node = i_next;
        p->i_first_free_node = i_curr;
    }
    // Try join with next.
    strpool__Node *node_next = strpool__get_node(p, i_next);
    if ((node_next != NULL) && (i_curr + node_curr->free_chunks == i_next)) {
        node_curr->free_chunks += node_next->free_chunks;
        node_curr->i_next_node = node_next->i_next_node;
    }
}


/// @Returns error.
int strpool__grow(strpool *p, int min_size) {
    // I need: A power of two which is just bigger or equal that min_size.
    int new_capacity = strpool__pow2roundup(min_size);
    if (new_capacity < p->capacity) {
        return -1;
    }
    strpool__Node *new_nodes = realloc(p->nodes, (size_t)new_capacity * STRPOOL_CHUNK);
    if (new_nodes == NULL) {
        return -1;
    }
    p->nodes = new_nodes;
    // Add newly allocated space as new_node.
    int new_node_i = p->capacity;
    strpool__Node *node = &p->nodes[new_node_i];
    node->i_next_node = -1;
    node->free_chunks = new_capacity - p->capacity;
    // Update.
    p->capacity = new_capacity;
    // Attach to node list.
    {
        strpool__integrate_new_free_node(p, new_node_i, node->free_chunks);
    }
    return 0;
}


/// @Returns index or Negative Number on error.
int strpool_append(strpool *p, strpool__str view) {
    int i_node_prev = -1;
    int i_node = -1;
    int i_node_next = -1;

    i_node = strpool__find_space(p, view.size, &i_node_prev);

    if (i_node == -1) {
        // No space, let's grow.
        int err = strpool__grow(p, strpool__int_max(p->capacity * 2, view.size));
        if (err != 0) { return -1; }
        i_node = strpool__find_space(p, view.size, &i_node_prev);
        if (i_node == -1) { return -2; }
    }

    // Ensure we can append early for easy bail out in case of OOM.
    int view_id = strpool__view_Slot_append(&p->views, (strpool__view) { 0 });
    if (view_id == -1) { return -3; }
    strpool__view *new_view = strpool__view_Slot_get(&p->views, view_id);
    if (new_view == NULL) { return -4; }


    strpool__Node *node = &p->nodes[i_node];
    char *writing_area = (char *)node;

    int consumed_chunks = strpool__div_ceil(view.size, STRPOOL_CHUNK);
    int remaining_chunks = node->free_chunks - consumed_chunks;


    if (remaining_chunks <= 0) {
        // No remaining chunks, point prev_node to the next_node.
        i_node_next = node->i_next_node;
    } else {
        // If there are chunks left create a new node.
        i_node_next = i_node + consumed_chunks;
        strpool__Node *new_node = &p->nodes[i_node_next];
        new_node->free_chunks = remaining_chunks;
        new_node->i_next_node = node->i_next_node;
        node = new_node;
    }

    // If no prev_node then use root.
    if (i_node_prev >= 0) {
        strpool__Node *prev_node = &p->nodes[i_node_prev];
        prev_node->i_next_node = i_node_next;
    } else {
        // No previous root this must mean it's the first.
        p->i_first_free_node = i_node_next;
    }

    // Write view.
    memcpy(writing_area, view.data, (size_t)view.size);
    new_view->offset = i_node;
    new_view->size = view.size;
    return view_id;
}


int strpool_remove(strpool *p, int view_id) {
    int i_curr;
    int view_chunks;
    {
        strpool__view *view = strpool__view_Slot_get(&p->views, view_id);
        if (view == NULL) {
            return -1;
        }
        i_curr = view->offset;
        view_chunks = strpool__div_ceil(view->size, STRPOOL_CHUNK);
    }
    strpool__view_Slot_pop(&p->views, view_id);
    if (view_chunks == 0) { return 0; } // Empty string.
    strpool__integrate_new_free_node(p, i_curr, view_chunks);
    return 0;
}


/// @Returns view or INVALID_VIEW If not found. An invalid view is .data == NULL.
strpool__str strpool_get(strpool *p, int view_id) {
    strpool__view *view = strpool__view_Slot_get(&p->views, view_id);
    if (view == NULL) {
        return (strpool__str) { .data = NULL, .size = 0, };
    }
    return (strpool__str) { .data = (char *)((strpool__Node *)p->nodes + view->offset), .size = view->size, };
}


static STRMAP__ALLOC_PROTOTYPE(strmap__default_allocator) {
    (void)align; (void)user_data; // Unused: malloc guarantees alignment.

    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0

    void* result = NULL;
    if (size == 0) {
        free(ptr);
    } else {
        if (ptr == NULL) {
            result = malloc((size_t)size);
        } else {
            result = realloc(ptr, (size_t)size);
        }
    }
    return result;
}


#endif // !STRPOOL_GENERAL
