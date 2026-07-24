/*
   DRESCRIPTION:
   Not a full fledged string library but just a common string definition to
   use in my own libraries.
*/

#ifndef WSTRVIEW_H
#define WSTRVIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define PRIwstr ".*s"
#define PRIwstrarg(arg) ((arg).size),((arg).data)
#define WSTRVIEW_INVALID ((wstrview_t){.data = NULL, .size = 0})

typedef struct wstrview_t {
    const char *data;
    int size;
} wstrview_t;

wstrview_t wcstr(const char* c_str) {
    return c_str ? (wstrview_t) { .data = c_str, .size = (int)strlen(c_str) } : WSTRVIEW_INVALID;
}

#define wcstr_SL(sl_arg) ((wstrview_t){.data=(sl_arg), .size=sizeof(sl_arg)-1})

bool wstrview_equals(wstrview_t str1, wstrview_t str2) {
    if (str1.size != str2.size) { return false; }
    return !str1.size || !memcmp(str1.data, str2.data, (size_t)str1.size);
    // !str1.size is necessary see https://nullprogram.com/blog/2025/01/19/#strings
}

#endif
