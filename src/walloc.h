/*
   The idea is that this can behave like an arena except
   I can delete stuff.
   */

#ifndef WALLOC_H
#define WALLOC_H

#include "stddef.h"
#include "portable_utils.h"

typedef union {
    struct {
        int free_chunks; // Amount of free chunks next to it including itself.
        ID  next;        // Index to next free node. (Index in nodes[]).
        ID  prev;        // Index to prev free node. (Index in nodes[]).
    };
    max_align_t __max_align;
} Walloc__Node;

static_assert(sizeof(Walloc__Node) == sizeof(max_align_t), "Must be able to hold any type align.");

#define ARRAY__TYPE Walloc__Node
#define ARRAY__NAMESPACE Walloc__Vec_Node
#include "array.h"

typedef struct {
    Walloc__Vec_Node nodes;
    ID first_free_node;
} Walloc__Page;

typedef struct {
    Walloc__Page *page;
} Walloc__PageRef;

#define DYNA__TYPE Walloc__PageRef
#define DYNA__NAMESPACE Walloc__Vec_Page
#include "da.h"

typedef struct {
    Walloc__Vec_Page pages;
} Walloc;

#endif
