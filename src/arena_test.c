#include "arena.h"
#include "woytest.h"
#include <stdalign.h>
#include <stddef.h>

int imin(int a, int b) { return a < b ? a : b; }
size_t size_min(size_t a, size_t b) { return a < b ? a : b; }

bool check_invariants_root(ArenaRoot root) {
    return (root.beg <= root.end);
}

bool check_invariants_arena(Arena root) {
    return (root.beg <= root.end);
}

bool arena_is_full(Arena a) { return a.beg == a.end; }


TEST test_general(void) {
    ArenaRoot root = ArenaRoot_create(100);
    ASSERT(check_invariants_root(root));

    for (int i = 0; i < 10; ++i) {
        Arena arena = ArenaRoot_get_arena(root);

        size_t text_size = 50;
        #define STR "Hello there!"

        char *text1 = arena_new(&arena, char, (i64)text_size);
        strncpy(text1, STR, size_min(text_size, strlen(STR)));
        ASSERT_SIZE(strnlen(text1, 50), strlen(STR));

        char *text2 = arena_new(&arena, char, 50);
        strncpy(text2, STR, size_min(text_size, strlen(STR)));
        ASSERT_SIZE(strnlen(text2, 50), strlen(STR));

        ASSERT(arena_is_full(arena));
    }

    ArenaRoot_free(&root);
    ASSERT(root.beg == NULL);
    TEST_PASS;
}


TEST test_alignment(void) {
    ArenaRoot root = ArenaRoot_create(1024);
    Arena arena = ArenaRoot_get_arena(root);

    struct a8   { alignas(8)   char data[8];   };
    struct a16  { alignas(16)  char data[16];  };
    struct a32  { alignas(32)  char data[32];  };
    struct a64  { alignas(64)  char data[64];  };
    struct a128 { alignas(128) char data[128]; };
    struct a256 { alignas(256) char data[256]; };

    struct a8 *my8 = arena_new(&arena, struct a8, 1);
    ASSERT((uintptr_t)my8 % 8 == 0);
    struct a16 *my16 = arena_new(&arena, struct a16, 1);
    ASSERT((uintptr_t)my16 % 16 == 0);
    struct a32 *my32 = arena_new(&arena, struct a32, 1);
    ASSERT((uintptr_t)my32 % 32 == 0);
    struct a64 *my64 = arena_new(&arena, struct a64, 1);
    ASSERT((uintptr_t)my64 % 64 == 0);
    struct a128 *my128 = arena_new(&arena, struct a128, 1);
    ASSERT((uintptr_t)my128 % 128 == 0);
    struct a256 *my256 = arena_new(&arena, struct a256, 1);
    ASSERT((uintptr_t)my256 % 256 == 0);

    ArenaRoot_free(&root);
    TEST_PASS;
}


int main(void) {
    TESTS_INIT();

    RUN_TEST(test_general);
    RUN_TEST(test_alignment);

    TESTS_SHOW_RESULTS();
}
