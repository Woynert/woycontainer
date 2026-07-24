#include "stdio.h"
#include "../portable_utils.h"
#include "../woytest.h"
#include "string.h"


#include "../arena.h"


typedef struct {
    char name[20];
} Fruit;


#define MAP__TYPE Fruit
#define MAP__NAMESPACE MapA
#include "npmap_str2.h"

#define STRMAP__TYPE Fruit
#define STRMAP__NAMESPACE MapB
#include "../strmap.h"


/* CONVENIENCE STRING FUNCTIONS. */

#define str strpool__str
#define PRIstr ".*s"
#define PRIstrarg(arg) ((arg).size),((arg).data)
#define STRVIEW_INVALID ((strpool__str){.data = NULL, .size = 0})
#define STRVIEW_INVALID2 ((strmap__view){.data = NULL, .size = 0})
#define cstr_SL(sl_arg) ((str){.data=(sl_arg), .size=sizeof(sl_arg)-1})
strpool__str cstr(const char* c_str) {
    return c_str ? (strpool__str) { .data = c_str, .size = (int)strlen(c_str) } : STRVIEW_INVALID;
}
bool str_equals(strpool__str str1, strpool__str str2) {
    if (str1.size != str2.size) { return false; }
    return !str1.size || !memcmp(str1.data, str2.data, (size_t)str1.size);
    // !str1.size is necessary see https://nullprogram.com/blog/2025/01/19/#strings
}

strmap__view cstr2(const char* c_str) {
    return c_str ? (strmap__view) { .data = c_str, .size = (int)strlen(c_str) } : STRVIEW_INVALID2;
}

/*strmap__view strpool__str_2_strmap__str(strpool__str s) { return (strmap__view) { .data = s.data, .size = s.size }; }*/

#define makefruit(name) ((Fruit) { name })

#define EXP 20


#define ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)

ALLOC_PROTOTYPE(*allocator) = NULL;
void *allocator_user_data = NULL;
static void* arena_allocator(void* ptr, size_t size, int align, void* user_data) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0
    if (size == 0) return NULL; // No freeing for arena.
    Arena *arena = (Arena*)user_data;
    void *result = arena_alloc(arena, sizeof(char), align, (i64)size);
    if (ptr != NULL) {
        memmove(result, ptr, size);
        // Reallocation must copy what we had. Don't use memcpy.
    }
    return result;
}

int map_create(void *m, strpool__str key, Fruit value);
int map_upsert(void *m, strpool__str key, Fruit value);
static Fruit *map_get(void *m, strmap__view key);
int map_remove(void *m, strpool__str key);



#define TEMPLATE(mymap, mycstr, map_upsert, map_get, map_remove, map_count, mystr_equals, COLOR)              \
    start = get_system_ms();                                                                                  \
    {                                                                                                         \
        for (int i = 0; i < (1 << EXP); ++i) {                                                                \
            char key_str[100] = { 0 };                                                                        \
            Fruit fruit = (Fruit) { 0 };                                                                      \
            sprintf(fruit.name, "fruit%d", i);                                                                \
            sprintf(key_str, "FRUIT%d", i);                                                                   \
            err = map_upsert(&mymap, mycstr(key_str), fruit);                                                 \
            ASSERT_INT(err, 0);                                                                               \
        }                                                                                                     \
    }                                                                                                         \
    printfd(COLOR"insertion: %f secs.", (double)(get_system_ms() - start)/(double)1000);                      \
                                                                                                              \
    ASSERT_INT(map_count, 1 << EXP);                                                                          \
                                                                                                              \
    start = get_system_ms();                                                                                  \
    {                                                                                                         \
        for (int i = 0; i < (1 << EXP); ++i) {                                                                \
            char key_str[100] = { 0 };                                                                        \
            Fruit fruit = (Fruit) { 0 };                                                                      \
            sprintf(fruit.name, "fruit%d", i);                                                                \
            sprintf(key_str, "FRUIT%d", i);                                                                   \
            Fruit *result = map_get(&mymap, mycstr(key_str));                                                 \
            ASSERT(result != NULL);                                                                           \
            ASSERT(mystr_equals(mycstr(fruit.name), mycstr(result->name)));                                   \
        }                                                                                                     \
    }                                                                                                         \
    printfd(COLOR"lookup: %f secs.", (double)(get_system_ms() - start)/(double)1000);                         \
                                                                                                              \
    start = get_system_ms();                                                                                  \
    {                                                                                                         \
        for (int i = 0; i < (1 << EXP); ++i) {                                                                \
            char key_str[100] = { 0 };                                                                        \
            Fruit fruit = (Fruit) { 0 };                                                                      \
            sprintf(fruit.name, "fruit%d", i);                                                                \
            sprintf(key_str, "FRUIT%d", i);                                                                   \
            err = map_remove(&mymap, mycstr(key_str));                                                        \
            ASSERT_INT(err, 0);                                                                               \
        }                                                                                                     \
    }                                                                                                         \
    printfd(COLOR"deletion: %f secs.", (double)(get_system_ms() - start)/(double)1000);                       \
                                                                                                              \
    ASSERT_INT(map_count, 0);                                                                                 \
                                                                                                              \
    start = get_system_ms();                                                                                  \
    {                                                                                                         \
        for (int i = 0; i < (1 << EXP); ++i) {                                                                \
            char key_str[100] = { 0 };                                                                        \
            Fruit fruit = (Fruit) { 0 };                                                                      \
            sprintf(fruit.name, "fruit%d", i);                                                                \
            sprintf(key_str, "FRUIT%d", i);                                                                   \
            err = map_upsert(&mymap, mycstr(key_str), fruit);                                                 \
            ASSERT_INT(err, 0);                                                                               \
        }                                                                                                     \
    }                                                                                                         \
    printfd(COLOR"re-insertion: %f secs.", (double)(get_system_ms() - start)/(double)1000);                   \




TEST mapA_bechmark(void) {
    printfd("MAP A BENCHMARK");
    int err;
    long start;
    MapA map = { 0 };

    err = MapA_create_with_allocator(&map, allocator, allocator_user_data);
    ASSERT_INT(err, 0);
    TEMPLATE(map, cstr, MapA_upsert, MapA_get, MapA_remove, map.count, str_equals, ANSI_GRE);
    {
        size_t total_memory = MapA_report_memory(&map);
        printfd("REPORTED TOTAL MEMORY "PRIbyte, PRIbytearg(total_memory));
        size_t strpool_memory = strpool_report_memory(&map.strpool);
        printfd("STRPOOL MEMORY "PRIbyte, PRIbytearg(strpool_memory));
        printfd("MAP MEMORY "PRIbyte, PRIbytearg(total_memory - strpool_memory));
    }
    MapA_free(&map);

    TEST_PASS;
}

int get_the_size_once(void) {
    static int i = 0;
    ++i;
    if (i > 1) { exit(-1); }
    return 1024;
}


TEST mapB_bechmark(void) {
    printfd("MAP B BENCHMARK");
    int err;
    long start;
    MapB map = { 0 };

    err = MapB_create_with_allocator(&map, allocator, allocator_user_data);
    ASSERT_INT(err, 0);
    TEMPLATE(map, cstr2, MapB_set_pair, MapB_get, MapB_remove, map.pair_count, strmap__view_equals, ANSI_BLU);
    {
        size_t total_memory = MapB_report_memory(&map);
        printfd("REPORTED TOTAL MEMORY "PRIbyte, PRIbytearg(total_memory));
        size_t strpool_memory = strpool_report_memory(&map.strpool);
        printfd("STRPOOL MEMORY "PRIbyte, PRIbytearg(strpool_memory));
        printfd("MAP MEMORY "PRIbyte, PRIbytearg(total_memory - strpool_memory));
    }
    MapB_free(&map);

    TEST_PASS;
}


int main(void) {

    TESTS_INIT();

    // Run with no allocator.
    RUN_TEST(mapA_bechmark);
    RUN_TEST(mapB_bechmark);

    // Run with arena allocator.
    ArenaRoot root = ArenaRoot_create(1024 * 1024 * 1024); // 1 GiB.
    Arena arena;

    arena = ArenaRoot_get_arena(root);
    allocator = arena_allocator;
    allocator_user_data = (void*)&arena;
    RUN_TEST(mapA_bechmark); 
    {
        int consumed = (int)(arena.beg - root.beg);
        int total = (int)(root.end - root.beg);
        printfd("Arena remaining memory "PRIbyte" out of "PRIbyte" (%.1f %%).", PRIbytearg(consumed), PRIbytearg(total), ((float)consumed/(float)total)*100.0);
    }

    arena = ArenaRoot_get_arena(root);
    allocator = arena_allocator;
    allocator_user_data = (void*)&arena;
    RUN_TEST(mapB_bechmark); 
    {
        int consumed = (int)(arena.beg - root.beg);
        int total = (int)(root.end - root.beg);
        printfd("Arena remaining memory "PRIbyte" out of "PRIbyte" (%.1f %%).", PRIbytearg(consumed), PRIbytearg(total), ((float)consumed/(float)total)*100.0);
    }

    ArenaRoot_free(&root);

    /*const int amount = 1 << EXP;*/
    const size_t amount_final = (1 << EXP) * sizeof(Fruit);
    printfd("Ideal size "PRIbyte, PRIbytearg(amount_final));

    TESTS_SHOW_RESULTS();
}


