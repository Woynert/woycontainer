#ifndef ANYTYPE_H
#define ANYTYPE_H


#define ANYTYPE__TOKCAT_(a, b) a ## b
#define ANYTYPE__TOKCAT(a, b) ANYTYPE__TOKCAT_(a, b)
#define ANYTYPE_GET_ENUM(type) ANYTYPE__TOKCAT(ANYTYPE_, type)

#include "portable_utils.h"

typedef struct {
    int user_type; // aka enum ANYTYPE.
    int size;
    void *data;
} Anytype;

#define ANYTYPE_NULL ((Anytype){0})

#define Anytype_make(item, TYPE) (Anytype) {                                         \
    .user_type = ANYTYPE_GET_ENUM(TYPE),                                             \
    .data=&(item),                                                                   \
    .size=sizeofi(item)                                                              \
    + STATIC_ASSERT_EXPR(!is_pointer(item), "Sure you wanna pass a literal pointer?")\
    + STATIC_ASSERT_EXPR(is_same_type(TYPE, typeof(item)), "Not same types")         \
}

#define Anytype_read_as(any, TYPE) \
    (((any).user_type == ANYTYPE_GET_ENUM(TYPE) && (any).size == sizeofi(TYPE)) ? (TYPE*)(any).data : NULL)


#endif
