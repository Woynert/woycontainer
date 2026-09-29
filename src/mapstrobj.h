#ifndef MapStrObj_H
#define MapStrObj_H

#include "wstrview.h"
#include "slotpool.h"

#define UMAPSTR__KEY ID
#define UMAPSTR__TYPE ID
#define UMAPSTR__NAMESPACE MapStrObj__MapStrID
#include "umapstr.h"

#define MapStrObj__ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)

typedef struct {
    MapStrObj__MapStrID keys;
    Slotpool values;
} MapStrObj;

void mapstrobj_create_with_allocator(MapStrObj *m, MapStrObj__ALLOC_PROTOTYPE(*allocator), void *alloc_userdata) {
    *m = (MapStrObj) { 0 };
    MapStrObj__MapStrID_create_with_allocator(&m->keys, allocator, alloc_userdata);
    slotpool_create_with_allocator(&m->values, allocator, alloc_userdata);
}

void mapstrobj_create(MapStrObj *m) { mapstrobj_create_with_allocator(m, NULL, NULL); }

void mapstrobj_free(MapStrObj *m) {
    MapStrObj__MapStrID_free(&m->keys);
    slotpool_destroy(&m->values);
    *m = (MapStrObj) { 0 };
}

/// @Returns error.
int mapstrobj_upsert(MapStrObj *m, const strview_t key, const void *data, const int size) {
    ID id = slotpool__append(&m->values, data, size, NULL);
    if (ID_invalid(id)) { return -1; }
    int err = MapStrObj__MapStrID_upsert(&m->keys, key, id);
    if (err) {
        slotpool_remove(&m->values, id);
        return -1;
    }
    return 0;
}

void *mapstrobj_get(MapStrObj *m, const strview_t key, const int expected_size) {
    ID *id = MapStrObj__MapStrID_get(&m->keys, key);
    if (!id) { return NULL; }
    void *data = slotpool__get(&m->values, *id, expected_size);
    return data;
}

void mapstrobj_remove(MapStrObj *m, const strview_t key) {
    ID *id = MapStrObj__MapStrID_get(&m->keys, key);
    if (!id) { return; }
    int err = slotpool_remove(&m->values, *id);
    if (err) { printferr("W: Couldn't remove from slotpool."); }
    err = MapStrObj__MapStrID_remove(&m->keys, key);
    if (err) { printferr("W: Couldn't remove from MapStrID."); }
    return;
}


static MapStrObj__ALLOC_PROTOTYPE(map_str_data__default_allocator) {
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


#endif
