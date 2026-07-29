/*
    Type safe insert only list.

    * Pro: Arena friendly.
    * Con: Cannot remove items.
*/

#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef LIST__TYPE
#define LIST__TYPE float
#endif
#define LIST__TOKCAT_(a, b) a ## b
#define LIST__TOKCAT(a, b) LIST__TOKCAT_(a, b)
#ifndef LIST__NAMESPACE
#define LIST__NAMESPACE LIST__TOKCAT(List_, LIST__TYPE)
#endif
#define pub(name) LIST__TOKCAT(LIST__TOKCAT(LIST__NAMESPACE, _), name)
#define pri(name) LIST__TOKCAT(LIST__TOKCAT(LIST__NAMESPACE, __), name)
#define LIST LIST__NAMESPACE
#define TYPE LIST__TYPE
#define LIST__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
static LIST__ALLOC_PROTOTYPE(pri(default_allocator));

typedef struct pri(Node) pri(Node);
typedef struct pri(Node) {
    pri(Node) *next;
    TYPE item;
} pri(Node);

typedef struct LIST {
    pri(Node) *root;
    void *allocator_userdata;
    LIST__ALLOC_PROTOTYPE(*allocator);
} LIST;

typedef struct pub(It) {
    pri(Node) *__node;
    TYPE *item;
} pub(It);


static LIST pub(create) (void) {
    return (LIST) { 0 };
}

static LIST pub(create_with_allocator) (LIST__ALLOC_PROTOTYPE(*allocator), void *user_data) {
    return (LIST) { .allocator = allocator, .allocator_userdata = user_data };
}

static void pub(free) (LIST *l) {
    LIST__ALLOC_PROTOTYPE(*allocator) = l->allocator ? l->allocator : pri(default_allocator);
    pri(Node) *node = l->root;
    while (node != NULL) {
        pri(Node) *next = node->next;
        allocator(node, 0, 0, l->allocator_userdata); // Free.
        node = next;
    }
    *l = (LIST) {0};
}

static int pub(append) (LIST *l, TYPE item) {
    pri(Node) **node = &l->root;
    while (*node != NULL) {
        node = &(*node)->next;
    }
    LIST__ALLOC_PROTOTYPE(*allocator) = l->allocator ? l->allocator : pri(default_allocator);
    *node = (pri(Node)*)allocator(NULL, sizeof(pri(Node)), alignof(pri(Node)), l->allocator_userdata);
    if (*node == NULL) { return -1; }
    **node = (pri(Node)) { 0 };
    (*node)->item = item;
    return 0;
}

static bool pub(it_next) (LIST *l, pub(It) *it) {
    if (it->__node == NULL) { it->__node = l->root; }
    else { it->__node = it->__node->next; }
    if (it->__node != NULL) { it->item = &it->__node->item; }
    return it->__node != NULL;
}

static LIST__ALLOC_PROTOTYPE(pri(default_allocator)) {
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

#undef LIST__TOKCAT_
#undef LIST__TOKCAT
#undef pub
#undef pri
#undef TYPE
#undef LIST
#undef LIST__TYPE
#undef LIST__NAMESPACE
#undef LIST__ALLOC_PROTOTYPE
