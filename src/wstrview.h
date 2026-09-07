/*
   DRESCRIPTION:
   Not a full fledged string library but just a common string definition to
   use in my own libraries.

   It's a subset of "strview.h" from mickjc750.
*/

#ifndef WSTRVIEW_H
#define WSTRVIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifndef PRIstr
#define PRIstr ".*s"
#endif
#define PRIstrw "%.*s"
#ifndef PRIstrarg
#define PRIstrarg(arg) ((arg).size),((arg).data)
#endif
#ifndef STRVIEW_INVALID
#define STRVIEW_INVALID ((strview_t){.data = NULL, .size = 0})
#endif
#ifndef cstr_SL
#define cstr_SL(sl_arg) ((strview_t){.data=(sl_arg), .size=sizeof(sl_arg)-1})
#endif
#define cstr_SL_const(sl_arg) {.data=(sl_arg), .size=sizeof(sl_arg)-1}
#define cstr_SLc(sl_arg) {.data=(sl_arg), .size=sizeof(sl_arg)-1}

#ifndef _STRVIEW_STRUCT_TYPE_
#define _STRVIEW_STRUCT_TYPE_
typedef struct strview_t {
    const char *data;
    int size;
} strview_t;
#endif

strview_t wcstr(const char* c_str) {
    return c_str ? (strview_t) { .data = c_str, .size = (int)strlen(c_str) } : STRVIEW_INVALID;
}

bool wstrview_equals(strview_t str1, strview_t str2) {
    if (str1.size != str2.size) { return false; }
    return !str1.size || !memcmp(str1.data, str2.data, (size_t)str1.size);
    // !str1.size is necessary see https://nullprogram.com/blog/2025/01/19/#strings
}

bool wstrview_is_valid(strview_t str) { return !!str.data && str.size >= 0; }

#endif
