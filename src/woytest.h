#ifndef WOYTEST_H
#define WOYTEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WOYTEST_RESET   "\033[0m"
#define WOYTEST_RED     "\033[1;31m"
#define WOYTEST_BOLD    "\033[1m"
#define WOYTEST_GREEN   "\033[1;32m"
#define WOYTEST_YELLOW  "\033[33m"
#define WOYTEST_GRAY    "\033[90m"


#define WOYTEST__PRINT_LINE_INFO()                                       \
    do {                                                                 \
        printf("%s:%d: In function ‘%s’", __FILE__, __LINE__, __func__); \
        printf("%s", WOYTEST_YELLOW);                                    \
        printf("%s", WOYTEST_GRAY);                                      \
        printf("%s\n", WOYTEST_RESET);                                   \
    } while (0)


#define WOYTEST_TEMPL(a, b, TYPE, COMP_FUNC, COMP_FUNC_NAME, PRINT_FUNC, COMP_STR)           \
    do {                                                                                     \
        TYPE copy_a = (a);                                                                   \
        TYPE copy_b = (b);                                                                   \
        bool success = COMP_FUNC(copy_a, copy_b);                                            \
        if (!success) {                                                                      \
            const char* left_str = #a;                                                       \
            const char* right_str = #b;                                                      \
            const char* func_str = COMP_FUNC_NAME;                                           \
            const char* comp_str = COMP_STR;                                                 \
            WOYTEST__PRINT_LINE_INFO();                                                      \
            /* We could just stringify the entire contents of inside of the macro instead. */\
            printf("Expression:\n        %s( %s, %s )\n", func_str, left_str, right_str);    \
            printf("Got values:\n        %s", WOYTEST_BOLD);                                 \
            PRINT_FUNC(copy_a);                                                              \
            printf("%s", WOYTEST_RESET);                                                     \
            printf(" %s ", comp_str);                                                        \
            printf("%s", WOYTEST_BOLD);                                                      \
            PRINT_FUNC(copy_b);                                                              \
            printf("%s", WOYTEST_RESET);                                                     \
            printf("\n");                                                                    \
            return false;                                                                    \
        }                                                                                    \
    } while(0)


#define WOYTEST_MAKE_ASSERT(a, b, TYPE, COMP_FUNC, PRINT_FUNC, COMP_STR) \
    WOYTEST_TEMPL(a, b, TYPE, COMP_FUNC, #COMP_FUNC, PRINT_FUNC, COMP_STR)


#define TEST bool

#define TEST_PASS do { return true; } while(0)

#define TESTS_INIT()         \
    int _woytest_count = 0;  \
    int _woytest_passed = 0; \
    printf("TESTS STARTING\n---\n");

#define RUN_TEST(FUNC)                                                          \
    do {                                                                        \
        ++_woytest_count;                                                       \
            printf("%s[ RUN      ]%s "#FUNC"\n", WOYTEST_GREEN, WOYTEST_RESET); \
        if ((FUNC)()) {                                                         \
            printf("%s[       OK ]%s "#FUNC"\n", WOYTEST_GREEN, WOYTEST_RESET); \
            ++_woytest_passed;                                                  \
        } else {                                                                \
            printf("%s[  FAILED  ]%s "#FUNC"\n", WOYTEST_RED, WOYTEST_RESET);   \
        }                                                                       \
    } while(0)




#define TESTS_SHOW_RESULTS()                      \
    do {                                          \
        printf("---\nPassed %d out of %d tests.", \
            _woytest_passed,                      \
            _woytest_count                        \
        );                                        \
        if (_woytest_passed < _woytest_count) {   \
            printf(" %sFailed %d%s.",             \
                WOYTEST_RED,                      \
                _woytest_count - _woytest_passed, \
                WOYTEST_RESET                     \
            );                                    \
        }                                         \
        printf("\n");                             \
    } while (0)


// Default asserts.

void woytest__bool_print(bool v) { printf("%s", v ? "true" : "false"); }
bool woytest__bool_eq(bool a, bool b) { return a == b; }

#define WOYTEST__MAKE_NUMBER_FUNCTIONS(TYPE, format)                       \
void woytest__##TYPE##_print(TYPE v)       { printf(format, v); }          \
bool woytest__##TYPE##_eq(TYPE a, TYPE b)  { return a == b;     }          \
bool woytest__##TYPE##_neq(TYPE a, TYPE b) { return a != b;     }          \
bool woytest__##TYPE##_gt(TYPE a, TYPE b)  { return a > b;      }          \
bool woytest__##TYPE##_gte(TYPE a, TYPE b) { return a >= b;     }          \
bool woytest__##TYPE##_lt(TYPE a, TYPE b)  { return a < b;      }          \
bool woytest__##TYPE##_lte(TYPE a, TYPE b) { return a <= b;     }
//bool woytest__##TYPE##_in_range(TYPE start, TYPE end, TYPE v) { return start < v && v < end; }

// Vim tip: Use replace on visual mode for editing: :'<,'>s/\<define\>/define/gI

WOYTEST__MAKE_NUMBER_FUNCTIONS(int, "%d")
#define ASSERT_INT(a, b)  WOYTEST_TEMPL(a, b, int, woytest__int_eq , "ASSERT_INT_EQ" , woytest__int_print, "==")
#define ASSERT_INT_NEQ(a, b) WOYTEST_TEMPL(a, b, int, woytest__int_neq ,"ASSERT_INT_NEQ" , woytest__int_print, "!=")
#define ASSERT_INT_GT(a, b)  WOYTEST_TEMPL(a, b, int, woytest__int_gt , "ASSERT_INT_GT" , woytest__int_print, ">" )
#define ASSERT_INT_GTE(a, b) WOYTEST_TEMPL(a, b, int, woytest__int_gte, "ASSERT_INT_GTE", woytest__int_print, ">=")
#define ASSERT_INT_LT(a, b)  WOYTEST_TEMPL(a, b, int, woytest__int_lt , "ASSERT_INT_LT" , woytest__int_print, "<" )
#define ASSERT_INT_LTE(a, b) WOYTEST_TEMPL(a, b, int, woytest__int_lte, "ASSERT_INT_LTE", woytest__int_print, "<=")

WOYTEST__MAKE_NUMBER_FUNCTIONS(size_t, "%zu")
#define ASSERT_SIZE(a, b)  WOYTEST_TEMPL(a, b, size_t, woytest__size_t_eq , "ASSERT_SIZE_EQ" , woytest__size_t_print, "==")
#define ASSERT_SIZE_NEQ(a, b) WOYTEST_TEMPL(a, b, size_t, woytest__size_t_neq, "ASSERT_SIZE_NEQ", woytest__size_t_print, "!=")
#define ASSERT_SIZE_GT(a, b)  WOYTEST_TEMPL(a, b, size_t, woytest__size_t_gt , "ASSERT_SIZE_GT" , woytest__size_t_print, ">" )
#define ASSERT_SIZE_GTE(a, b) WOYTEST_TEMPL(a, b, size_t, woytest__size_t_gte, "ASSERT_SIZE_GTE", woytest__size_t_print, ">=")
#define ASSERT_SIZE_LT(a, b)  WOYTEST_TEMPL(a, b, size_t, woytest__size_t_lt , "ASSERT_SIZE_LT" , woytest__size_t_print, "<" )
#define ASSERT_SIZE_LTE(a, b) WOYTEST_TEMPL(a, b, size_t, woytest__size_t_lte, "ASSERT_SIZE_LTE", woytest__size_t_print, "<=")

WOYTEST__MAKE_NUMBER_FUNCTIONS(float, "%f")
#define ASSERT_FLOAT(a, b)  WOYTEST_TEMPL(a, b, float, woytest__float_eq , "ASSERT_FLOAT_EQ" , woytest__float_print, "==")
#define ASSERT_FLOAT_NEQ(a, b) WOYTEST_TEMPL(a, b, float, woytest__float_neq, "ASSERT_FLOAT_NEQ", woytest__float_print, "!=")
#define ASSERT_FLOAT_GT(a, b)  WOYTEST_TEMPL(a, b, float, woytest__float_gt , "ASSERT_FLOAT_GT" , woytest__float_print, ">" )
#define ASSERT_FLOAT_GTE(a, b) WOYTEST_TEMPL(a, b, float, woytest__float_gte, "ASSERT_FLOAT_GTE", woytest__float_print, ">=")
#define ASSERT_FLOAT_LT(a, b)  WOYTEST_TEMPL(a, b, float, woytest__float_lt , "ASSERT_FLOAT_LT" , woytest__float_print, "<" )
#define ASSERT_FLOAT_LTE(a, b) WOYTEST_TEMPL(a, b, float, woytest__float_lte, "ASSERT_FLOAT_LTE", woytest__float_print, "<=")

/*
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define ASSERT_EQ(a, b) _Generic((a), \
    int   : ASSERT_INT_EQ,     \
    size_t: ASSERT_SIZE_EQ,    \
    float : ASSERT_FLOAT_EQ    \
)(a, b)
#endif
*/

#define ASSERT_TRUE(a)  WOYTEST_TEMPL(a, true, bool, woytest__bool_eq, "ASSERT_TRUE", woytest__bool_print, "==")
#define ASSERT_FALSE(a) WOYTEST_TEMPL(a, false, bool, woytest__bool_eq, "ASSERT_FALSE", woytest__bool_print, "==")
#define ASSERT(a) ASSERT_TRUE(a)

// TODO: Equivalent to CHECK_CALL from greatest.


#endif // !WOYTEST_H
