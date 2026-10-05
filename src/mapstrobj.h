/*
   Maps String to Anytypes with stable addresses.

   Allocates and frees on it's own.
   */

#ifndef MapStrObj_H
#define MapStrObj_H

#include "arena.h"
#include "wstrview.h"
#include "anytype.h"

typedef struct {
    Anytype any;
    int frame;
} MapStrObj__Anytype;

#define UMAPSTR__TYPE MapStrObj__Anytype
#define UMAPSTR__NAMESPACE MapStrObj__MapStrAnytype
#include "umapstr.h"

//#define MapStrObj__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)
//static MapStrObj__ALLOC_PROTOTYPE(mapstrobj__default_allocator);

typedef struct {
    MapStrObj__MapStrAnytype keys;
    int curr_frame;
} MapStrObj;


void mapstrobj_create(MapStrObj *m) {
    *m = (MapStrObj) { 0 };
    MapStrObj__MapStrAnytype_create(&m->keys);
}

void mapstrobj_free(MapStrObj *m) {
    MapStrObj__MapStrAnytype_It it = { 0 };
    while (MapStrObj__MapStrAnytype_it_next(&m->keys, &it)) {
        Anytype any = it.value->any;
        if (any.data) { free(any.data); }
    }
    MapStrObj__MapStrAnytype_free(&m->keys);
    *m = (MapStrObj) { 0 };
}


/// @Note. If value didn't exist then set, else just returns existing.
/// @Returns ANYTYPE_NULL on error.
Anytype mapstrobj_insert_or_get(MapStrObj *m, const strview_t key, const Anytype obj) {
    if (obj.size <= 0) { printferr("Size <= 0"); return ANYTYPE_NULL; }
    MapStrObj__Anytype *item = MapStrObj__MapStrAnytype_get(&m->keys, key);
    if (item) {
        item->frame = m->curr_frame;
        return item->any;
    }
    int err = MapStrObj__MapStrAnytype_upsert(&m->keys, key, (MapStrObj__Anytype){0});
    item = MapStrObj__MapStrAnytype_get(&m->keys, key);
    if (err || !item) { printferr("OOM?"); return ANYTYPE_NULL; }
    item->any = (Anytype) {
        .data = malloc((size_t)obj.size),
        .size = obj.size,
        .user_type = obj.user_type,
    };
    item->frame = m->curr_frame;
    if (!item->any.data) { return ANYTYPE_NULL; }
    memcpy(item->any.data, obj.data, (size_t)obj.size);
    return item->any;
}


void mapstrobj_remove(MapStrObj *m, const strview_t key) {
    MapStrObj__Anytype *item = MapStrObj__MapStrAnytype_get(&m->keys, key);
    if (!item) { return; }
    if (item->any.data) { free(item->any.data); }
    int err = MapStrObj__MapStrAnytype_remove(&m->keys, key);
    if (err) { printferr("W: Couldn't remove from MapStrID."); }
}


void mapstrobj_set_frame(MapStrObj *m, int frame) { m->curr_frame = frame; }

void mapstrobj_free_older_than_frame(MapStrObj *m, Arena scratch) {
    // @Note: Probably want a more arena friendly string array.
    Strpool keys_to_delete;
    strpool_create_with_allocator(&keys_to_delete, arena_allocator, &scratch);

    MapStrObj__MapStrAnytype_It it = { 0 };
    while (MapStrObj__MapStrAnytype_it_next(&m->keys, &it)) {
        if (it.value->frame < m->curr_frame) {
            strpool_append(&keys_to_delete, it.key);
        }
    }

    for (int i = 0; i < keys_to_delete.views.count; ++i) {
        strview_t view = strpool_get_from_view(&keys_to_delete, keys_to_delete.views._items[i]);
        mapstrobj_remove(m, view);
        printfd("D: Removing state for id ["PRIstrw"]", PRIstrarg(view));
    }
}

#define mapstrobj_get_default(map, TYPE, default, ID)                                                     \
    ({                                                                                                    \
        Anytype __any = mapstrobj_insert_or_get((map), (ID), Anytype_make((default), TYPE));              \
        typeof(default) *__instance = Anytype_read_as(__any, TYPE);                                       \
        if (!__instance) { printferr("WAR: Couldn't get from framearena");  }                             \
        __instance;                                                                                       \
    })

#define mapstrobj_get(map, TYPE, ID)                                                                      \
    ({                                                                                                    \
        Anytype __any = mapstrobj_insert_or_get((map), (ID), Anytype_make((TYPE){0}, TYPE));              \
        TYPE *__instance = Anytype_read_as(__any, TYPE);                                                  \
        if (!__instance) { printferr("WAR: Couldn't get from framearena");  }                             \
        __instance;                                                                                       \
    })

// Intentionally keep this in outer scope.
#define mapstrobj_if_null_use_literal(ref) ref = (ref ? ref : &((typeof(*ref)){0}))



//static MapStrObj__ALLOC_PROTOTYPE(mapstrobj__default_allocator) {
    //// New allocation: ptr == NULL && size > 0
    //// Reallocation:   ptr != NULL && size > 0
    //// Free:           ptr != NULL && size == 0
    //(void)align; (void)user_data; // Unused: malloc guarantees alignment.
    //void* result = NULL;
    //if      (size == 0)   { free(ptr);                   }
    //else if (ptr == NULL) { result = malloc(size);       }
    //else                  { result = realloc(ptr, size); }
    //return result;
//}


#endif
