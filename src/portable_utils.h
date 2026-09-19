#ifndef PORTABLE_UTILS_H
#define PORTABLE_UTILS_H

#include "stdio.h"   // printf
#include "stdbool.h" // bool, true
#include "stdlib.h"  // exit
#include "limits.h"
#include "math.h"
#include <stdint.h>
#include "assert.h"

typedef int64_t i64;

#ifdef WIN32
#include <windows.h>
#else
#include <time.h> // nanosleep
#endif
static void sleep_ms(int milliseconds){
    #ifdef WIN32
    Sleep(milliseconds);
    #else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
    #endif
}

#ifdef _WIN32
#include <windows.h>
#else // Linux, macOS, etc.
#include <sys/time.h>
#endif
static long get_system_ms(void) {
    #ifdef _WIN32
    return GetTickCount64();
    #else // Linux, macOS, etc.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    #endif
}

static long get_system_ns(void) {
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts); // Could fail but we still return 0.
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
static void sleep_ns(long long ns){
    struct timespec ts;
    ts.tv_sec = ns / 1000000000LL;
    ts.tv_nsec = (ns % 1000000000LL);
    nanosleep(&ts, NULL);
}
#define sec2ns(x) ((x) * 1000000000)
#define ms2ns(x) ((x) * 1000000)
#define ns2secf(x) ((x) / 1000000000.f)
#define ns2msf(x) ((x) / 1000000.f)
//#define ns2sec(x) ((x) / 1000000000.f)
//#define ns2ms(x) ((x) / 1000000.f)

static inline size_t size_t_max(size_t a, size_t b)                     { return a > b ? a : b; }
static inline size_t size_t_min(size_t a, size_t b)                     { return a < b ? a : b; }
static inline size_t size_t_clamp(size_t min, size_t max, size_t value) { return size_t_max(min, size_t_min(max, value)); }
static inline int int_max(int a, int b)                                 { return a > b ? a : b; }
static inline long long long_long_max(long long a, long long b)         { return a > b ? a : b; }
static inline int int_min(int a, int b)                                 { return a < b ? a : b; }
static inline int int_clamp(int min, int max, int value)                { return int_max(min, int_min(max, value)); }
static inline int int_sign(int x) { return (x > 0) - (x < 0); } // Returns -1 or 1.
static inline float float_sign(float x) { return (float)((x > 0.0) - (x < 0.0)); } // Returns -1 or 1.
static inline float float_clamp(float min, float max, float value)      { return fmaxf(min, fminf(max, value)); }
static inline bool int_in_range_inclusive(int min, int max, int value)  { return value >= min && value <= max; }

static int int_digit_places (int n) {
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1
        Add more ifs as needed and adjust this final return as well.
        https://stackoverflow.com/a/1068937
    */
    return 10;
}

// #define PYTHON_MODULO(n, M) ((((n) % (M)) + (M)) % (M))

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_GRE "\033[32m"
#define ANSI_YEL "\033[33m"
#define ANSI_BLU "\033[34m"
#define ANSI_MAG "\033[35m"
#define ANSI_CYA "\033[36m"

#ifdef DEBUG
    #define DEBUG_ASSERT wassert
#else
    #define DEBUG_ASSERT(...)
#endif

#define wassert(value) \
    do { \
        if ((!!(value)) != true) { \
            printf("\nFAILED ASSERT %s|%s:%d\n", \
                    __func__, __FILE__, __LINE__); \
            printf(ANSI_RED#value); \
            printf(ANSI_RESET"\n"); \
            __asm__("int3"); \
            exit(1); \
        } \
    } while (0)

#define wassert_msg(value, msg) \
    do { \
        if ((!!(value)) != true) { \
            printf("\nFAILED ASSERT %s|%s:%d\n", \
                    __func__, __FILE__, __LINE__); \
            printf(msg"\n"); \
            printf(ANSI_RED#value); \
            printf(ANSI_RESET"\n"); \
            __asm__("int3"); \
            exit(1); \
        } \
    } while (0)

#define static_assert _Static_assert

#define wstatic_assert(x) static_assert(x, #x)

#define printfd(fmt, ...) \
    do { \
        printf(ANSI_YEL fmt, ##__VA_ARGS__); \
        printf(ANSI_RESET" %s:%s:%d\n", __func__, __FILE__, __LINE__); \
    } while (0)

#define printferr(fmt, ...) printfd(ANSI_RED"Err: "fmt, ##__VA_ARGS__)


#define printval(fmt, ...) do { printf(#__VA_ARGS__" "fmt"\n", ##__VA_ARGS__); } while(0)
#define printvalnum(var) do { printf( #var " = %d\n", var); } while(0)

#define Bool_Fmt "%s"
#define Bool_Arg(x) ((x) ? "true" : "false")

#define countof(a)         (sizeof(a) / sizeof(*(a)))
#define countofi(a)  ((int)(sizeof(a) / sizeof(*(a))))
//#define sizeof(x)    (ptrdiff_t)sizeof(x)
//#define lengthof(s)  (countof(s) - 1)

#define ZERO(x) do { memset(&x, 0, sizeof(x)); } while(0)

#define foreachi(type_i, type_item, iter, array, size) \
    struct { type_i index; type_item *ref; } iter = { .index = 0, .ref = (array) }; iter.index < size; ++iter.index, ++iter.ref
#define foreach(type_item, iter, array, size) \
    foreachi(int, type_item, iter, array, size)
#define foreach_auto(type_item, iter, array) \
    foreachi(int, type_item, iter, array, countofi(array))


// https://gist.github.com/dgoguerra/7194777?permalink_comment_id=6272723#gistcomment-6272723
#define PRIbyte "%s"
#define PRIbytearg(x) byte_human_format((uint64_t)(x))
const char *byte_human_format(uint64_t b) {
    enum { BUF_EXP_SIZE = 4, BUF_SIZE = 32 };
    static int idx = 0;
    static char storage_buf[1 << BUF_EXP_SIZE][BUF_SIZE];
    char *buf = storage_buf[idx];
    idx = (idx + 1) & ((1 << BUF_EXP_SIZE) -1);
    snprintf(buf, BUF_SIZE, "~%.02lf(%s)",
    b >= (1ull << 40) ? (double)(b) / (double)(1ull << 40) :
    b >= (1ull << 30) ? (double)(b) / (double)(1ull << 30) :
    b >= (1ull << 20) ? (double)(b) / (double)(1ull << 20) :
    b >= (1ull << 10) ? (double)(b) / (double)(1ull << 10) : (double)(b),
    b >= (1ull << 40) ? "TiB" : b >= (1ull << 30) ? "GiB" :
    b >= (1ull << 20) ? "MiB" : b >= (1ull << 10) ? "KiB" : "B");
    return buf;
    // 28 bytes -> "9223372036854775807.00(TiB)\0"
}

#define malloc_new(T, count) \
    (T *)malloc(sizeof(T) * (size_t)(count))


// Finds a multiple of a number which makes it less or equal to a cap.
// @returns Number or 0 if not found.
int find_multiple_max_fit(int n, int cap) {
    if (n <= 0 || cap <= 0) { return 0; }
    return (int)floorf((float)cap / (float)n);
}


// START [RANDOM NUMBERS]
// Source https://stackoverflow.com/a/39714913
typedef uint32_t uint_type; // can be any unsigned type.
#define RAND_UINT_MAX ((uint_type) -1)
uint_type rand_uint(void) {
    // These are all constant and factor is likely a power of two.
    // therefore, the compiler has enough information to unroll
    // the loop and can use an immediate form shl in-place of mul.
    uint_type factor = (uint_type) RAND_MAX + 1;
    uint_type factor_to_k = 1;
    uint_type cutoff = factor ? RAND_UINT_MAX / factor : 0;
    uint_type result = 0;
    while ( 1 ) {
        result += (uint_type)rand() * factor_to_k;
        if (factor_to_k <= cutoff) { factor_to_k *= factor; }
        else { return result; }
    }
}
// Note(woynert): I have the feeling this function is slow because of the
//     cast to double I guess it would be faster to directly generate the
//     bytes for the int.
//     Because if we are already casting to double then why not just do
//     something like (double)rand()/RAND_MAX ?
// Uniform int distribution.
int rand_range(int min, int max) {
    // [0,1) -> [min,max]
    double canonical = rand_uint() / (RAND_UINT_MAX + 1.0);
    return (int)floor(canonical * (1.0 + max - min) + min);
}
// END [RANDOM NUMBERS]




// START [ID]
typedef struct {
    int id;
} zid_t;
inline int   zid_get(zid_t id)   { return id.id -1; }
inline zid_t zid_make(int id)    { return (zid_t) { id +1 }; }
inline bool  zid_valid(zid_t id) { return id.id > 0; }
#define ID           zid_t
#define ID_valid(id) zid_valid(id)
#define ID_get(id)   zid_get(id)
#define ID_make(id)  zid_make(id)
#define ID_equals(a, b) ((a).id == (b).id)
#define ID_INVALID ((ID){0})
// END [ID]


#endif
