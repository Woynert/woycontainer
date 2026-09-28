/*
    Generic allocator data structure.
    Allocates objects of any size and returns a stable id.

    FEATURES:
    * Pro: Stable user facing ids.
    * Pro: Unlimited growth.
    * Pro: O(n) Insert.
        * Finds space using a "free list". (Red-Black tree would be better (O(log) I think)).
    * Pro: O(1) Get.
    * Pro: O(1) Deletion.
    * Con: Previously deleted ids will be reutilized often. (Because of slot.h)

    NOTE:
    * I want to implement a general purpose allocator without fixed chunk size.
*/

#ifndef SLOTPOOL_GENERAL
#define SLOTPOOL_GENERAL


#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "portable_utils.h"


#define SLOTPOOL__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
static SLOTPOOL__ALLOC_PROTOTYPE(slotpool__default_allocator);


typedef struct slotpool__view {
    ID  offset;   // Offset from buf start. (Index into nodes[]).
    int bytes;    // Size.
} slotpool__view;


typedef union slotpool__Node {
    struct {
        int free_chunks;
        ID  next;        // Index to next free node. (Index in nodes[]).
        ID  prev;        // Index to prev free node. (Index in nodes[]).
    };
    max_align_t __max_align;
} slotpool__Node;
static_assert(sizeof(slotpool__Node) == sizeof(max_align_t), "Must be able to hold any type align.");


#define STRPOOL__CHUNK ((int)sizeof(slotpool__Node))


#define SLOT__TYPE slotpool__view
#include "slot.h"


typedef struct Slotpool {
    struct {
        int capacity;          // Measured in chunks or nodes.
        slotpool__Node *nodes;
    };
    ID i_first_free_node;      // Index to nodes[].
    slotpool__view_Slot views; // Maps user facing id to an internal slotpool__view.

    SLOTPOOL__ALLOC_PROTOTYPE(*allocator);
    void *alloc_userdata;
} Slotpool;



int          slotpool_create(Slotpool *p);
int          slotpool_create_with_allocator(Slotpool *p, SLOTPOOL__ALLOC_PROTOTYPE(*allocator), void *allocator_user_data);
void         slotpool_destroy(Slotpool *p);
ID           slotpool__append(Slotpool *p, const char *data, const int size, void** out_item);
void*        slotpool__get(const Slotpool *p, ID view_id, int expected_size);
void*        slotpool__get_from_view(const Slotpool *p, slotpool__view view);
int          slotpool_remove(Slotpool *p, ID view_id);
void         slotpool_clear(Slotpool *p);
ID           slotpool_get_next_id(Slotpool *p);
size_t       slotpool_report_memory(const Slotpool *p);

int            slotpool__grow(Slotpool *p, int min_size);
slotpool__Node*slotpool__get_node(const Slotpool *p, ID id);
ID             slotpool__find_node_just_before(const Slotpool *p, ID target_offset);
void           slotpool__integrate_new_free_node(Slotpool *p, ID i_curr, int curr_chunks);
ID             slotpool__find_space(const Slotpool *p, int space, ID *out_i_prev_node);
static int     slotpool__pow2roundup(int x);
static int     slotpool__div_ceil(int x, int y);
static int     slotpool__int_max(int x, int y) { return x > y ? x : y; }
static SLOTPOOL__ALLOC_PROTOTYPE(slotpool__default_allocator);



/// @Returns Error.
int slotpool_create_with_allocator(Slotpool *p, SLOTPOOL__ALLOC_PROTOTYPE(*allocator), void *alloc_userdata) {
    *p = (Slotpool) { 0 };
    p->allocator = allocator;
    p->alloc_userdata = alloc_userdata;
    slotpool__view_Slot_create_with_allocator(&p->views, allocator, alloc_userdata);
    return 0;
}


/// @Returns Error.
int slotpool_create(Slotpool *p) { return slotpool_create_with_allocator(p, NULL, NULL); }



// https://stackoverflow.com/a/2745763
// WARNING: Only positive integers!!!
static inline int slotpool__div_ceil(int x, int y) {
    return (x % y) ? x / y + 1 : x / y;
}


// https://stackoverflow.com/a/365068
// Round up to next higher power of 2 (return x if it's already a power of 2).
static inline int slotpool__pow2roundup (int x) {
    if (x < 0) { return 0; }
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}


slotpool__Node *slotpool__get_node(const Slotpool *p, ID id) {
    if (ID_invalid(id) || ID_get(id) >= p->capacity) { return NULL; }
    return &p->nodes[ID_get(id)];
}


/// @Param target_offset. Is not bytes, it's chunks.
/// @Returns id.
/// @Retval -1 If not found (or root).
ID slotpool__find_node_just_before(const Slotpool *p, ID target_offset) {
    ID i_prev_node = ID_INVALID;
    ID i_node = p->i_first_free_node;
    for (;;) {
        slotpool__Node *node = slotpool__get_node(p, i_node);
        if (!node) { break; }
        if (ID_get(i_node) >= ID_get(target_offset)) { break; }
        i_prev_node = i_node;
        i_node = node->next;
        if (ID_get(i_node) < ID_get(i_prev_node)) { break; }
    }
    return i_prev_node;
}


void slotpool__integrate_new_free_node(Slotpool *p, ID i_curr, int curr_chunks) {
    ID i_prev = slotpool__find_node_just_before(p, i_curr);
    ID i_next = ID_INVALID;
    slotpool__Node *node_prev = slotpool__get_node(p, i_prev);
    slotpool__Node *node_curr = slotpool__get_node(p, i_curr);

    // Try join with prev.
    if (node_prev) {
        if (ID_equals(node_prev->next, i_curr)) {
            // Should never happen. But still let's keep the check.
            node_prev->next = node_curr->next;
        }
        i_next = node_prev->next;
        if (ID_get(i_prev) + node_prev->free_chunks == ID_get(i_curr)) {
            node_prev->free_chunks += curr_chunks;
            i_curr = i_prev;
            node_curr = node_prev;
        }
        // Cannot join with previous, in that case, create it's own node.
        else {
            node_prev->next = i_curr;
            node_curr->free_chunks = curr_chunks;
            node_curr->next = i_next;
        }
    }
    // No previous node means it is root.
    else {
        i_next = p->i_first_free_node;
        node_curr->free_chunks = curr_chunks;
        node_curr->next = i_next;
        p->i_first_free_node = i_curr;
    }
    // Try join with next.
    slotpool__Node *node_next = slotpool__get_node(p, i_next);
    if (node_next && (ID_get(i_curr) + node_curr->free_chunks == ID_get(i_next))) {
        node_curr->free_chunks += node_next->free_chunks;
        node_curr->next = node_next->next;
    }
}


/// @Returns error.
int slotpool__grow(Slotpool *p, int min_size) {
    // I need: A power of two which is just bigger or equal that min_size.
    int new_capacity = slotpool__pow2roundup(min_size);
    if (new_capacity == p->capacity) { return 0; }
    if (new_capacity < p->capacity) { return -1; }

    SLOTPOOL__ALLOC_PROTOTYPE(*allocator) = p->allocator ? p->allocator : slotpool__default_allocator;
    slotpool__Node *new_nodes = (slotpool__Node *)allocator(p->nodes, (size_t)new_capacity * STRPOOL__CHUNK, STRPOOL__CHUNK, p->alloc_userdata);
    if (!new_nodes) { return -1; }
    p->nodes = new_nodes;

    // Add newly allocated space as new_node.
    ID new_node_i = ID_make(p->capacity);
    int new_node_free_chunks = new_capacity - p->capacity;
    // Update.
    p->capacity = new_capacity;
    // Attach to node list.
    slotpool__integrate_new_free_node(p, new_node_i, new_node_free_chunks);
    return 0;
}


void slotpool_destroy(Slotpool *p) {
    SLOTPOOL__ALLOC_PROTOTYPE(*allocator) = p->allocator ? p->allocator : slotpool__default_allocator;
    allocator(p->nodes, 0, 0, p->alloc_userdata);
    slotpool__view_Slot_free(&p->views);
    *p = (Slotpool) { 0 };
}


/// @param[out] out_i_prev_node. Index for the node previous to the one with space.
/// @Returns id or -1 on error.
ID slotpool__find_space(const Slotpool *p, int space, ID *out_i_prev_node) {
    ID i_prev_node = ID_INVALID;
    ID i_node = p->i_first_free_node;
    slotpool__Node *node = slotpool__get_node(p, i_node);
    while (node) {
        if ((node->free_chunks * STRPOOL__CHUNK) >= space) {
            *out_i_prev_node = i_prev_node;
            return i_node;
        } else {
            i_prev_node = i_node;
            i_node = node->next;
            if (ID_get(i_node) < ID_get(i_prev_node)) { return ID_INVALID; }
            node = slotpool__get_node(p, i_node);
        }
    }
    return ID_INVALID;
}


/// @Param      data.     Optional.
/// @Param[out] out_item. Optional.
/// @Returns id; On error returns INVALID ID. (Check with ID_valid).
ID slotpool__append(Slotpool *p, const char *data, const int size, void** out_item) {
    if (size <= 0) { return ID_INVALID; }
    ID i_node_prev = ID_INVALID;
    ID i_node = ID_INVALID;

    // Find space.

    i_node = slotpool__find_space(p, size, &i_node_prev);
    if (ID_invalid(i_node)) {
        enum { DEFAULT_CAP = 8, };
        int new_cap = p->capacity == 0 ? DEFAULT_CAP : p->capacity * 2;
        int err = slotpool__grow(p, slotpool__int_max(new_cap, size));
        if (err) { return ID_INVALID; }
        i_node = slotpool__find_space(p, size, &i_node_prev);
        if (ID_invalid(i_node)) { return ID_INVALID; }
    }

    // Check early for view space (for easy bail out in case of OOM).

    int view_id = slotpool__view_Slot_append(&p->views, (slotpool__view) { 0 });
    if (view_id == -1) { return ID_INVALID; }
    slotpool__view *new_view = slotpool__view_Slot_get(&p->views, view_id);
    if (!new_view) { return ID_INVALID; }

    // Calculate chunks.

    slotpool__Node *node = &p->nodes[ID_get(i_node)];
    char *writing_area = (char *)node;

    int consumed_chunks = slotpool__div_ceil(size, STRPOOL__CHUNK);
    int remaining_chunks = node->free_chunks - consumed_chunks;

    // Unlink or remove used node.
    {
        slotpool__Node *prev_node = slotpool__get_node(p, i_node_prev);
        if (prev_node) { prev_node->next = node->next; }
        else {             p->i_first_free_node = node->next; }
    }

    // Link or create new node if there was any space left.
    if (remaining_chunks > 0) {
        slotpool__integrate_new_free_node(p, ID_make(ID_get(i_node) + consumed_chunks), remaining_chunks);
    }

    // Write view.
    if (data) { memcpy(writing_area, data, (size_t)size); }
    new_view->offset = i_node;
    new_view->bytes = size;
    if (out_item) { *out_item = (void*)node; }
    return ID_make(view_id);
}


/// @Returns would-be next id if a new String where to be inserted.
ID slotpool_get_next_id(Slotpool *p) { return ID_make(slotpool__view_Slot_get_next_id(&p->views)); }


int slotpool_remove(Slotpool *p, ID view_id) {
    ID i_curr;
    int view_chunks;
    {
        slotpool__view *view = slotpool__view_Slot_get(&p->views, ID_get(view_id));
        if (!view) { return -1; }
        i_curr = view->offset;
        view_chunks = slotpool__div_ceil(view->bytes, STRPOOL__CHUNK);
    }
    int err = slotpool__view_Slot_pop(&p->views, ID_get(view_id));
    (void)err; // @Note. Probably want to print a warning here.
    if (view_chunks == 0) { return 0; } // Empty string.
    slotpool__integrate_new_free_node(p, i_curr, view_chunks);
    return 0;
}


inline void* slotpool__get_from_view(const Slotpool *p, slotpool__view view) {
    return (char *)((slotpool__Node *)p->nodes + ID_get(view.offset));
}


/// @Returns pointer or NULL If not found.
void* slotpool__get(const Slotpool *p, ID view_id, int expected_size) {
    slotpool__view *view = slotpool__view_Slot_get(&p->views, ID_get(view_id));
    if (!view || view->bytes != expected_size) { return NULL; }
    return slotpool__get_from_view(p, *view);
}

#define slotpool_get(pool, TYPE, id) ((TYPE*)slotpool__get((pool), (id), (int)sizeof(TYPE)))
#define slotpool_append(pool, TYPE, item) (slotpool__append((pool), (char*)(&(item)), (int)sizeof(TYPE), NULL))
#define slotpool_append_get(pool, TYPE, item, out) (slotpool__append((pool), (char*)(&(item)), (int)sizeof(TYPE), (void**)(&out)))


size_t slotpool_report_memory(const Slotpool *p) {
    size_t count = sizeof(Slotpool);
    count += (size_t)p->capacity * sizeof(slotpool__Node);
    count += slotpool__view_Slot_report_memory(&p->views);
    return count;
}


void slotpool_clear(Slotpool *p) {
    if (p->capacity <= 0) { return; }
    p->i_first_free_node = ID_INVALID;
    slotpool__view_Slot_clear(&p->views);
    memset(p->nodes, 0, sizeof(slotpool__Node) * (size_t)p->capacity);
    slotpool__integrate_new_free_node(p, ID_make(0), p->capacity);
}


static SLOTPOOL__ALLOC_PROTOTYPE(slotpool__default_allocator) {
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


#undef STRPOOL_STR
#undef SLOTPOOL__ALLOC_PROTOTYPE
#undef STRPOOL__CHUNK
#endif // !SLOTPOOL_GENERAL
