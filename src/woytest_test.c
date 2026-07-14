#include "arena.h"
#include "woytest.h"

typedef struct V2 { int x; int y; } V2;
#define V2_Fmt "v2(%d, %d)"
#define V2_Arg(v) (v).x, (v).y
void print_v2(V2 v) { printf(V2_Fmt, V2_Arg(v)); }
bool v2_gt(V2 a, V2 b) { return a.x > b.x; }
bool v2i_lt(V2 a, V2 b) { return a.x < b.x; }

#define ASSERT_V2_GT(a, b) WOYTEST_MAKE_ASSERT(a, b, V2, v2_gt, print_v2, ">")
#define ASSERT_V2_LT(a, b) WOYTEST_MAKE_ASSERT(a, b, V2, v2_gt, print_v2, "<")



void check_invariants(ArenaRoot root) {
    wassert(root.beg <= root.end);
}


TEST test_int_size_float(void) {
    int i = INT_MAX-1;
    ASSERT_INT_EQ(i, INT_MAX-1);
    ASSERT_INT_GT(i, -330);
    ASSERT_INT_GTE(i, 10);
    ASSERT_INT_LT(i, INT_MAX);
    ASSERT_INT_LTE(i, INT_MAX-1);

    size_t size = 300;
    ASSERT_SIZE_EQ(size, 300);
    ASSERT_SIZE_GT(size, 299);
    ASSERT_SIZE_GTE(size, 300);
    ASSERT_SIZE_LT(size, INT_MAX);
    ASSERT_SIZE_LTE(size, 300);

    const float PI = 3.1415f;
    float mimo = PI;
    ASSERT_FLOAT_EQ(mimo, PI);
    ASSERT_FLOAT_GT(mimo, -99999);
    ASSERT_FLOAT_GTE(mimo, PI);
    ASSERT_FLOAT_LT(mimo, 9457);
    ASSERT_FLOAT_LTE(mimo, 12);

    TEST_SKIP;
}

TEST test1(void) {
    ArenaRoot root = arena_create(100);

    V2 pos = { 1, 2 };
    arena_free(&root);
    ASSERT_V2_GT(pos, ((V2) { 0, 0 }));

    bool is_alive = true;
    ASSERT_TRUE(is_alive);

    int money = 100;
    ASSERT_INT_EQ(money, 100);
    ASSERT_INT_GT(money, 100);
    ASSERT_INT_EQ(money, INT_MAX);

    /*ASSERT_SHORT_EQ(money, 10000);*/

    ASSERT_TRUE(!!!is_alive);
    ASSERT_FALSE(true);

    TEST_SKIP;
}

TEST test2_arena_nocase(void) {
    ArenaRoot root = arena_create(100);
    int money = 100;
    /*A(assert_int_eq(money, 1000));*/
    /*assert_eq(money, 1000);*/

    V2 pos = { 1, 2 };
    arena_free(&root);
    ASSERT_V2_GT(pos, ((V2) { 100, 100 }));

    TEST_SKIP;
}


int main(void) {
    TESTS_INIT();

    RUN_TEST(test2_arena_nocase);
    RUN_TEST(test1);
    RUN_TEST(test_int_size_float);

    TESTS_SHOW_RESULTS();
}
