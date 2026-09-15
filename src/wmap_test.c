
#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"

#include "arena.h"

#define DYNA__TYPE int
#define DYNA__NAMESPACE Vec_Int
#include "da.h"


#define WMAP__KEY int
#define WMAP__TYPE int
#define WMAP__NAMESPACE Map_Int
#define WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
#include "wmap.h"


#define ID           wmap__zid_t
#define ID_valid(id) wmap__zid_valid(id)
#define ID_get(id)   wmap__zid_get(id)
#define ID_make(id)  wmap__zid_make(id)
#define ID_equals(a, b) ((a).id == (b).id)
#define ID_INVALID ((ID){0})


typedef struct {
    const int *data;
    int size;
} intview_t;

#define intview_literal(...) (intview_t) {.data = (int[]){ __VA_ARGS__ }, .size=(int)(sizeof((int[]){ __VA_ARGS__ })/sizeof(int)) }


/// Inclusive
int rand_range(int from, int to) {
    /*return (to <= from) ? from : (rand() % (to - from +1)) + from;*/
    return (rand() % (to - from +1)) + from;
}

ID force_get_bucket_root(Map_Int *m, ID a) {
    ID found = a;
    ID prev = m->bucket_prev.items[ID_get(a)];
    while(ID_valid(prev)) {
        found = prev;
        prev = m->bucket_prev.items[ID_get(prev)];
    }
    return found;
}

void swap_nodes(Map_Int *m, ID a, ID b) {
    printfd("a is %d of root %d", ID_get(a), ID_get(force_get_bucket_root(m, a)));
    printfd("b is %d of root %d", ID_get(b), ID_get(force_get_bucket_root(m, b)));
    Map_Int__swap_nodes_diff_bucket(m, a, b, force_get_bucket_root(m, a), force_get_bucket_root(m, b));
}

/*
   Swaps all nodes of the same bucket and test whether they're still all present.
   */
bool shuffle_nodes_same_bucket(Map_Int *m, Arena arena) {
    intview_t ordered_keys;
    {
        Vec_Int order_keys = Vec_Int_create_with_allocator(arena_allocator, &arena);
        ID node = m->first_node;
        while(ID_valid(node)) {
            Vec_Int_append(&order_keys, m->keys.items[ID_get(node)]);
            node = m->next.items[ID_get(node)];
        }
        ordered_keys = (intview_t){.data = order_keys.items, .size = order_keys.size };
    }
    Map_Int_print_order(m, false);
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        Arena scratch = arena;
        Vec_Int bucket_nodes = Vec_Int_create_with_allocator(arena_allocator, &scratch);
        Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, &scratch);

        const ID bucket = ID_make(i);
        printfd(ANSI_BLU"bucket %d", ID_get(bucket));
        if (!ID_valid(bucket)) { continue; }
        ID node = bucket;
        while (ID_valid(node)) {
            Vec_Int_append(&bucket_nodes, ID_get(node));
            Vec_Int_append(&keys, m->keys.items[ID_get(node)]);
            node = m->bucket_next.items[ID_get(node)];
        }

        // Now shuffle them all like a maniac.
        printfd("Range [%d %d], valid? %d", 0, bucket_nodes.size-1, ID_valid(bucket));
        for (int k = 0; k < 50; ++k) {
            int a = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            int b = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            printfd("Shuffling %d and %d", a, b);
            Map_Int__swap_nodes_same_bucket(m, ID_make(a), ID_make(b), bucket);
        }

        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, false);

        // Check integrity:

        node = bucket;
        while (ID_valid(node)) {
            bool found = false;
            printfd("node %d", ID_get(node));
            for (int l = 0; l < keys.size; ++l) {
                printfd("(size %d) Checking %d and %d", keys.size, keys.items[l], m->keys.items[ID_get(node)]);
                if (keys.items[l] == m->keys.items[ID_get(node)]) {
                    found = true;
                    Vec_Int_remove_at(&keys, l);
                    break;
                }
            }
            if (!found) { return false; }
            node = m->bucket_next.items[ID_get(node)];
        }
        if (keys.size != 0) {
            printferr("Couldn't find all.");
            for (int l = 0; l < keys.size; ++l) {
                printferr("Missing %d", keys.items[i]);
            }
            return false;
        }
    }
    printf("Expected order forward:\n");
    for (int i = 0; i < ordered_keys.size; ++i) {
        printf("%d,", ordered_keys.data[i]);
    }
    printf("\nActual ");
    Map_Int_print_order(m, false);
    // Check order.
    {
        int i = 0;
        ID node = m->first_node;
        while(ID_valid(node)) {
            if (i >= ordered_keys.size) { return false; }
            printfd("Cheking order %d should be %d", ordered_keys.data[i], m->keys.items[ID_get(node)]);
            if (ordered_keys.data[i] != m->keys.items[ID_get(node)]) {
                return false;
            }
            node = m->next.items[ID_get(node)];
            ++i;
        }
    }
    return true;
}


bool check_order(Map_Int *m) {
  return false;
}


TEST test_general(void) {
    ArenaRoot arenaroot = ArenaRoot_create(1 << 20);
    Arena arena = ArenaRoot_get_arena(arenaroot);
    int err;
    Map_Int _map = { 0 };
    Map_Int *m = &_map;
    Map_Int_create(m);

    Map_Int_print(m);
    printfd("---");
    err = Map_Int_upsert(m, 888, 80);
    Map_Int_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 1);
    printfd("---");
    err = Map_Int_upsert(m, 1, 10);
    Map_Int_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 2);
    printfd("---");
    err = Map_Int_upsert(m, 2, 20);
    Map_Int_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 3);
    printfd("---");
    err = Map_Int_upsert(m, 3, 30);
    Map_Int_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 4);
    printfd("---");
    err = Map_Int_upsert(m, 4, 40);
    Map_Int_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 5);
    printfd("---");
    err = Map_Int_upsert(m, 5, 50);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, 6);

    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    /*{*/
        /*Map_Int_It it = Map_Int_make_it(m);*/
        /*while (Map_Int_it_next(m, &it)) {*/
            /*printfd("%d -> %d", it.key, *it.value);*/
        /*}*/
    /*}*/

    /*printfd("--- Swapping");*/
    /*[>Map_Int__swap_nodes(m, (wmap__zid_t){.id=3}, (wmap__zid_t){.id=4},<]*/
            /*[>(wmap__zid_t){.id=2}, (wmap__zid_t){.id=2});<]*/
    /*swap_nodes(m, ID_make(2), ID_make(3));*/
    /*Map_Int_print(m);*/
    /*Map_Int_print_bucket_chains(m, true);*/

    /*[>{<]*/
        /*[>Map_Int_It it = Map_Int_make_it(m);<]*/
        /*[>while (Map_Int_it_next(m, &it)) {<]*/
            /*[>printfd(ANSI_BLU"%d -> %d", it.key, *it.value);<]*/
        /*[>}<]*/
    /*[>}<]*/
    /*printfd("--- Swapping");*/
    /*swap_nodes(m, ID_make(2), ID_make(3));*/
    /*Map_Int_print(m);*/
    /*Map_Int_print_bucket_chains(m, true);*/
    /*printfd("--- Swapping");*/
    /*swap_nodes(m, ID_make(3), ID_make(4));*/
    /*Map_Int_print(m);*/
    /*Map_Int_print_bucket_chains(m, true);*/

    /*[>for (<]*/
    /*srand(0);*/
    if ((1)) {
        printfd("↓↓↓");
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, true);
        printfd("---");
        for (int i = 0; i < 100; ++i) {
            int a = rand_range(1 << m->bucket_count_exp, (1 << m->bucket_count_exp) + m->collision_count-1);
            int b = rand_range(1 << m->bucket_count_exp, (1 << m->bucket_count_exp) + m->collision_count-1);
            swap_nodes(m, ID_make(a), ID_make(b));
        }
        printfd("---");
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, true);
        printfd("↑↑↑");
    }

    if ((0)) {
        printfd("↓↓↓");
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, false);
        printfd("---");
        Map_Int__swap_nodes_same_bucket(m, ID_make(2), ID_make(3), ID_make(1));
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, false);
        printfd("↑↑↑");
        ASSERT(shuffle_nodes_same_bucket(m, arena));
    }

    /*{*/
        /*Map_Int_It it = Map_Int_make_it(m);*/
        /*while (Map_Int_it_next(m, &it)) {*/
            /*printfd(ANSI_BLU"%d -> %d", it.key, *it.value);*/
        /*}*/
    /*}*/

    /*ASSERT_INT(err, 0);*/

    intview_t nums2 = intview_literal(100);
    for (int i = 0; i < nums2.size; ++i) {
        printfd("nums %d", nums2.data[i]);
    }

    Map_Int_free(m);
    TEST_PASS;
}



int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


