/*
   Stringpool features:
   * Unlimited growth.
   * Fast insert.
   * Fast deletion.
   * Virtual stable indexes.
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
//#define SLOT__NAMESPACE strpool__views_
#include "slot.h"

typedef struct FreeNode FreeNode;
typedef struct FreeNode {
    //int offset;
    int free_chunks;
    int i_next_node; // Index to nodes.
} FreeNode;

#define STRPOOL_CHUNK ((int)sizeof(FreeNode))

typedef struct strpool{
    struct {
        int capacity; // Measured in chunks.
        int cursor;
        union {
            char *buf;
            FreeNode *nodes;
        };
    };
    struct {
        int i_first_free_node; // Index to nodes.
        //FreeNode *first_free_node;
    };
    struct {
        //int id_counter;
        //strpool__view *views_da; // Maps user facing id to an internal strpool__view.
        strpool__view_Slot views;
    };
} strpool;


/*
FreeNode *strpool__get_node_from_relative_pointer(strpool *p, int pointer) {
    // 0 relative pointer is NULL pointer.
    if (pointer == 0) {
        return NULL;
    }
    // TODO: boundary check.
    FreeNode *node = (FreeNode *)(p->buf + p->offset_first_free_node);
    return node;
}
*/


/// @Returns Error.
int strpool_create(strpool *p) {
    *p = (strpool) { 0 };
    p->capacity = 8; // DEFAULT CAPACITY.

    p->nodes = (FreeNode *)malloc((size_t)p->capacity * STRPOOL_CHUNK);
    if (p->buf == NULL) {
        return -1;
    }

    int err = strpool__view_Slot_create(&p->views);
    if (err != 0) {
        free(p->buf);
        return -1;
    }

    // Write FreeNode at beginning.

    p->i_first_free_node = 0;
    FreeNode *node = &p->nodes[0];
    node->i_next_node = -1;
    node->free_chunks = p->capacity;

    return 0;
}

void strpool_destroy(strpool *p) {
    free(p->nodes);
    strpool__view_Slot_free(&p->views);
    *p = (strpool) { 0 };
}

/// @Returns Node or NULL on error.
/*
FreeNode *strpool__find_space(strpool *p, int space) {
    FreeNode *node = (FreeNode *)(p->buf + p->offset_first_free_node);
    while (node != NULL) {
        if (node->size >= space) {
            return node;
        } else {
            node = strpool__get_node_from_relative_pointer(p, node->next_node);
        }
    }
    return NULL;
}
*/

/*
/// @Returns Node or NULL on error.
FreeNode *strpool__find_space(strpool *p, int space, FreeNode **out_prev_node) {
    FreeNode *prev_node = NULL;
    FreeNode *node = &p->nodes[p->i_first_free_node]; // TODO: Wrap these accesses into a "safe" function.
    for (;;) {
        if (node->size >= space) {
            *out_prev_node = prev_node;
            return node;
        } else {
            if (node->i_next_node == -1) { return NULL; }
            prev_node = node;
            node = &p->nodes[node->i_next_node];
        }
    }
    return NULL;
}
*/


FreeNode *strpool__get_node(strpool *p, int i) {
    if (i < 0 || i >= p->capacity) { return NULL; }
    return &p->nodes[i];
}



/// @Returns id or -1 on error.
int strpool__find_space(strpool *p, int space, int *out_i_prev_node) {
    int i_prev_node = -1;
    int i_node = p->i_first_free_node; 
    FreeNode *node = strpool__get_node(p, i_node);
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
    FreeNode *node = strpool__get_node(p, i_node);
    while (node != NULL) {
        if (i_node > target_offset) {
            return i_prev_node;
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

/// @parma[out] out_prev_i Returns id or -1 If not found (or root).
/// @parma[out] out_next_i Returns id or -1 If not found (or root).
void strpool__find_nodes_around(strpool *p, int target_i, int *out_prev_i, int *out_next_i) {
    int i_prev = -1;
    int i_curr = p->i_first_free_node;
    int i_next = -1;
    for (;;) {
        FreeNode *node = strpool__get_node(p, i_curr);
        if (node == NULL) {
            break;
        }
        i_next = node->i_next_node;
        if (i_next > target_i) {
            *out_prev_i = i_prev;
            *out_next_i = i_next;
            return;
        }
        i_prev = i_curr;
        i_curr = i_next;
        if (i_curr <= i_prev) {
            return;
        }
    }
    return;
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


/// @Returns error.
int strpool__grow(strpool *p, int min_size) {
    // I need: A power of two which is just bigger or equal that min_size.
    int new_capacity = strpool__pow2roundup(min_size);
    if (new_capacity < p->capacity) {
        return -1;
    }
    FreeNode *new_nodes = realloc(p->nodes, (size_t)new_capacity * STRPOOL_CHUNK);
    if (new_nodes == NULL) {
        return -1;
    }
    p->nodes = new_nodes;
    // Add newly allocated space as new_node.
    FreeNode *node = &p->nodes[p->capacity];
    node->i_next_node = -1;
    node->free_chunks = new_capacity - p->capacity;
    // Attach to node list.
    {
        int i_node = p->i_first_free_node;
        FreeNode *curr_node = strpool__get_node(p, i_node);
        while (curr_node != NULL) {
            if (curr_node->i_next_node == -1) {
                curr_node->i_next_node = p->capacity;
                break;
            }
            if (curr_node->i_next_node < i_node) { 
                // The list can only go forward.
                break;
            }
            curr_node = strpool__get_node(p, curr_node->i_next_node);
        }
    }
    // Update.
    p->capacity = new_capacity;
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


    FreeNode *node = &p->nodes[i_node];
    char *writing_area = (char *)node;

    int consumed_chunks = strpool__div_ceil(view.size, STRPOOL_CHUNK);
    int remaining_chunks = node->free_chunks - consumed_chunks;


    if (remaining_chunks <= 0) {
        // No remaining chunks, point prev_node to the next_node.
        i_node_next = node->i_next_node;
    } else {
        // If there are chunks left create a new node.
        i_node_next = i_node + consumed_chunks;
        FreeNode *new_node = &p->nodes[i_node_next];
        new_node->free_chunks = remaining_chunks;
        new_node->i_next_node = node->i_next_node;
        node = new_node;
    }

    // If no prev_node then use root.
    if (i_node_prev >= 0) {
        FreeNode *prev_node = &p->nodes[i_node_prev];
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
    // TODO: REMOVE.
    // TODO: MAKE SEPARATE FUNCTION TO SCAN NODES AND UNIFY ADJACENT EMPTY SPACES.
    // TODO: ALSO CALL IT AFTER GROW.

    /* Okay let's write a plan.
       1. First, get the view from the view_slots.
       2. Then, get the "offset" and save it.
       3. Then, iterate all free nodes list and find one which is just before this one.
       4. If couldn't find any then assume this one is root.
       5. If previous one ends just when this one starts then join then.
       6. Else create new node in place and link _prev_ to _this_ and _this_ to _next_ (if any).

       Cases:
       1. Can join prev + this.
       2. Can join        this + next.
       2. Can join prev + this + next.
    */

    int i_curr;
    int view_chunks;
    {
        strpool__view *view = strpool__view_Slot_get(&p->views, view_id);
        if (view == NULL) {
            return -1;
        }
        i_curr = view->offset;
        view_chunks = strpool__div_ceil(view->size, STRPOOL_CHUNK);
        strpool__view_Slot_pop(&p->views, view_id);
        if (view_chunks == 0) { return 0; } // Empty string.
    }
    int i_prev = strpool__find_node_just_before(p, i_curr);
    int i_next = -1;
    FreeNode *node_prev = strpool__get_node(p, i_prev);
    FreeNode *node_curr = strpool__get_node(p, i_curr);

    // Try join with prev.
    if (node_prev != NULL) {
        i_next = node_prev->i_next_node;
        if (i_prev + node_prev->free_chunks == i_curr) {
            node_prev->free_chunks += view_chunks;
            i_curr = i_prev;
            node_curr = node_prev;
        }
        // Cannot join with previous, in that case, create it's own node.
        else {
            node_prev->i_next_node = i_curr;
            node_curr->free_chunks = view_chunks;
            node_curr->i_next_node = i_next;
        }
    }
    // No previous node means it is root.
    else {
        i_next = p->i_first_free_node;
        node_curr->free_chunks = view_chunks;
        node_curr->i_next_node = i_next;
        p->i_first_free_node = i_curr;
    }
    // Try join with next.
    FreeNode *node_next = strpool__get_node(p, i_next);
    if ((node_next != NULL) && (i_curr + node_curr->free_chunks == i_next)) {
        node_curr->free_chunks += node_next->free_chunks;
        node_curr->i_next_node = node_next->i_next_node;
    }
    return 0;
}


/// @Returns view or INVALID_VIEW If not found. An invalid view is .data == NULL.
strpool__str strpool_get(strpool *p, int view_id) {
    strpool__view *view = strpool__view_Slot_get(&p->views, view_id);
    if (view == NULL) {
        return (strpool__str) { .data = NULL, .size = 0, };
    }
    return (strpool__str) { .data = (char *)((FreeNode *)p->nodes + view->offset), .size = view->size, };
}


/*
// @Returns id. -1 if failed.
int strpool_append(strpool *p, strpool__view str) {
    if (str.data == NULL) { return -1; }
    if (p->buf == NULL) {
        p->cursor = 0;
        p->capacity = STRPOOL_CHUNK * 8;
        p->buf = malloc((size_t)p->capacity);
        if (p->buf == NULL) { return -1; }
    }
    int required_space = int_max(str.size, STRPOOL_CHUNK);
    if ((p->capacity - p->cursor) < required_space) {
        int new_capacity = int_max(p->capacity * 2, p->capacity + required_space);
        char *new_buf = realloc(p->buf, (size_t)new_capacity);
        if (new_buf == NULL) { return -1; }
        p->capacity = new_capacity;
        p->buf = new_buf;
    }
    memcpy(p->buf + p->cursor, str.data, (size_t)str.size);
    p->cursor += str.size;
    // TODO: Save cursor and size and return id to that strpool__view.
    return 0;
}
*/



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
