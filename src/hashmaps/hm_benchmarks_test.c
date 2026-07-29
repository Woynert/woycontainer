#include "stdio.h"
#include "string.h"
#include "../portable_utils.h"
#include "../woytest.h"
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


#define Str strview_t
#define makefruit(name) ((Fruit) { name })
#define EXP 19


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



#define TEST_TEMPLATE(mymap, map_upsert, map_get, map_remove, map_count, COLOR)             \
    start = get_system_ms();                                                                \
    {                                                                                       \
        for (int i = 0; i < (1 << EXP); ++i) {                                              \
            char key_str[100] = { 0 };                                                      \
            Fruit fruit = (Fruit) { 0 };                                                    \
            sprintf(fruit.name, "fruit%d", i);                                              \
            sprintf(key_str, "FRUIT%d", i);                                                 \
            err = map_upsert(&mymap, wcstr(key_str), fruit);                                \
            ASSERT_INT(err, 0);                                                             \
        }                                                                                   \
    }                                                                                       \
    printfd(COLOR"insertion: %f secs.", (double)(get_system_ms() - start)/(double)1000);    \
                                                                                            \
    ASSERT_INT(map_count, 1 << EXP);                                                        \
                                                                                            \
    start = get_system_ms();                                                                \
    {                                                                                       \
        for (int i = 0; i < (1 << EXP); ++i) {                                              \
            char key_str[100] = { 0 };                                                      \
            Fruit fruit = (Fruit) { 0 };                                                    \
            sprintf(fruit.name, "fruit%d", i);                                              \
            sprintf(key_str, "FRUIT%d", i);                                                 \
            Fruit *result = map_get(&mymap, wcstr(key_str));                                \
            ASSERT(result != NULL);                                                         \
            ASSERT(wstrview_equals(wcstr(fruit.name), wcstr(result->name)));                \
        }                                                                                   \
    }                                                                                       \
    printfd(COLOR"lookup: %f secs.", (double)(get_system_ms() - start)/(double)1000);       \
                                                                                            \
    start = get_system_ms();                                                                \
    {                                                                                       \
        for (int i = 0; i < (1 << EXP); ++i) {                                              \
            char key_str[100] = { 0 };                                                      \
            Fruit fruit = (Fruit) { 0 };                                                    \
            sprintf(fruit.name, "fruit%d", i);                                              \
            sprintf(key_str, "FRUIT%d", i);                                                 \
            err = map_remove(&mymap, wcstr(key_str));                                       \
            ASSERT_INT(err, 0);                                                             \
        }                                                                                   \
    }                                                                                       \
    printfd(COLOR"deletion: %f secs.", (double)(get_system_ms() - start)/(double)1000);     \
                                                                                            \
    ASSERT_INT(map_count, 0);                                                               \
                                                                                            \
    start = get_system_ms();                                                                \
    {                                                                                       \
        for (int i = 0; i < (1 << EXP); ++i) {                                              \
            char key_str[100] = { 0 };                                                      \
            Fruit fruit = (Fruit) { 0 };                                                    \
            sprintf(fruit.name, "fruit%d", i);                                              \
            sprintf(key_str, "FRUIT%d", i);                                                 \
            err = map_upsert(&mymap, wcstr(key_str), fruit);                                \
            ASSERT_INT(err, 0);                                                             \
        }                                                                                   \
    }                                                                                       \
    printfd(COLOR"re-insertion: %f secs.", (double)(get_system_ms() - start)/(double)1000); \



TEST mapA_bechmark(void) {
    printfd("MAP A BENCHMARK");
    int err;
    long start;
    MapA map = { 0 };

    err = MapA_create_with_allocator(&map, allocator, allocator_user_data);
    ASSERT_INT(err, 0);
    TEST_TEMPLATE(map, MapA_upsert, MapA_get, MapA_remove, map.count, ANSI_GRE);
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


TEST mapB_bechmark(void) {
    printfd("MAP B BENCHMARK");
    int err;
    long start;
    MapB map = { 0 };

    err = MapB_create_with_allocator(&map, allocator, allocator_user_data);
    ASSERT_INT(err, 0);
    TEST_TEMPLATE(map, MapB_upsert, MapB_get, MapB_remove, map.pair_count, ANSI_BLU);
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
        int consumed = (int)(arena.beg - root.buf);
        printfd("Arena remaining memory "PRIbyte" out of "PRIbyte" (%.1f %%).", PRIbytearg(consumed), PRIbytearg(root.cap), ((float)consumed/(float)root.cap)*100.0);
    }

    arena = ArenaRoot_get_arena(root);
    allocator = arena_allocator;
    allocator_user_data = (void*)&arena;
    RUN_TEST(mapB_bechmark); 
    {
        int consumed = (int)(arena.beg - root.buf);
        printfd("Arena remaining memory "PRIbyte" out of "PRIbyte" (%.1f %%).", PRIbytearg(consumed), PRIbytearg(root.cap), ((float)consumed/(float)root.cap)*100.0);
    }

    ArenaRoot_free(&root);

    {
        printfd("Ideal size for %zu elements is "PRIbyte, (size_t)1<<EXP, PRIbytearg((1<<EXP) * sizeof(Fruit)));
    }

    TESTS_SHOW_RESULTS();
}


