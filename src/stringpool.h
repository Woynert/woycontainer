/*
   Data structure properties:
   * Unlimited growth.
   * Can add new string buffers.
   * Cannot delete existing buffers.
   * However, can clear the whole thing.
   */

#ifndef STRINGPOOL
#define STRINGPOOL

#include "strbuf.h"
#include "strbuf_extra.h"

typedef struct Stringpool__Pair {
    int start;
    int size;
} Stringpool__Pair;

#define DYNA__TYPE Stringpool__Pair
#include "da.h"

typedef struct Stringpool {
    Stringpool__Pair_Dyna pairs; // Maps PAIR_ID to -> Pair.
    strbuf_t *buffer;

    // This is to make ids nonreusable. `id = curr_id_base + <array id>`.
    int id_base;
} Stringpool;

static int Stringpool_append(Stringpool *s, strview_t string);

static void Stringpool_create(Stringpool *s) {
    *s = (Stringpool) { 0 };
    s->pairs = Stringpool__Pair_Dyna_create();
    s->buffer = strbuf_create(0, NULL);
    s->id_base = 1;
}

static void Stringpool_destroy(Stringpool *s) {
    Stringpool__Pair_Dyna_free(&s->pairs);
    strbuf_destroy(&s->buffer);
    *s = (Stringpool) { 0 };
}

/// @returns ID of newly saved string.
/// @reval   -1 Error.
static int Stringpool_append(Stringpool *s, strview_t string) {
    Stringpool__Pair pair = {
        .start = s->buffer->size,
        .size = string.size
    };
    strbuf_append_strview(&s->buffer, string);
    int id = Stringpool__Pair_Dyna_append(&s->pairs, pair);
    if (id == -1) { return -1; }
    return id + s->id_base;
}

/// @returns View inside buffer.
/// @retval STRVIEW_INVALID on Error.
static strview_t Stringpool_get(Stringpool *s, int pair_id) {
    pair_id -= s->id_base;

    Stringpool__Pair *pair = Stringpool__Pair_Dyna_get_safe(&s->pairs, pair_id);
    if (pair == NULL) { return STRVIEW_INVALID; }
    return (strview_t) {
        .data = s->buffer->cstr + pair->start,
        .size = pair->size
    };
}

static void Stringpool_clear_preserving(Stringpool *s) {
    s->id_base += s->pairs.size;

    Stringpool__Pair_Dyna_clear_preserving(&s->pairs);
    strbuf_empty(&s->buffer);
}

static void Stringpool_clear_freeing(Stringpool *s) {
    s->id_base += s->pairs.size;

    Stringpool__Pair_Dyna_clear_freeing(&s->pairs);
    strbuf_empty(&s->buffer);
    strbuf_shrink(&s->buffer);
}



/* @note: Super slow, prefer Stringpool_clear.*/
/* @note: Won't fail if pair_id points to an empty pair even if that pair is
                                                        considered "deleted". */
/* @returns Error. */
static int Stringpool_remove(Stringpool *s, int pair_id) {
    pair_id -= s->id_base;

    Stringpool__Pair *pair = Stringpool__Pair_Dyna_get_safe(&s->pairs, pair_id);
    if (pair == NULL) { return -1; }
    if (pair->size <= 0) { return 0; }

    // Update all pairs.

    for (int i = pair_id+1; i < s->pairs.size; ++i) {
        Stringpool__Pair *pair_other = &s->pairs.items[i];
        pair_other->start -= pair->size;
    }

    // Slice off section from buffer.

    strview_t right;
    strview_t left;

    right = strbuf_view(&s->buffer);
    left = strview_split_index(&right, pair->start);
    strview_split_index(&right, pair->size);

    strbuf_cat(&s->buffer, left, right);

    // Invalidate pair.
    // (pairs are actually never deleted but accumulate over time).
    pair->size = 0;

    return 0;
}


#endif // !STRINGPOOL
