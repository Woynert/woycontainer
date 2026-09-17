/*
    Usage:

    #define MAKEVIEW__TYPE <type>
    #define MAKEVIEW__NAMESPACE <type> (Optional)
    #include "make_view.h"

    Example:

    #define MAKEVIEW__TYPE Entity
    #include "make_view.h"
    ...
    const Entity_view view = { .items = entities, .size = entity_count };
*/

#ifndef MAKEVIEW__TYPE
#define MAKEVIEW__TYPE int
#endif

/* Token concatenation. */
#define MAKEVIEW__TOKCAT_(a, b) a ## b
#define MAKEVIEW__TOKCAT(a, b) MAKEVIEW__TOKCAT_(a, b)
#ifndef MAKEVIEW__NAMESPACE
#define MAKEVIEW__NAMESPACE MAKEVIEW__TOKCAT(View_, MAKEVIEW__TYPE)
#endif

typedef struct {
    MAKEVIEW__TYPE *items;
    int size;
} MAKEVIEW__NAMESPACE;

#undef MAKEVIEW__TYPE
#undef MAKEVIEW__TOKCAT
#undef MAKEVIEW__TOKCAT_
#undef MAKEVIEW__NAMESPACE

#ifndef MAKEVIEW__MACROS
#define MAKEVIEW__MACROS
#define view_foreach(type, iter, array) \
    struct { int index; const type *ref; } iter = { .index = 0, .ref = array.items }; iter.index < array.size; ++iter.index, ++iter.ref
#endif
