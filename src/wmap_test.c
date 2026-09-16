
#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"

#include "arena.h"

#define DYNA__TYPE int
#define DYNA__NAMESPACE Vec_Int
#include "da.h"

#define DYNA__TYPE Vec_Int
#define DYNA__NAMESPACE Vec_Vec_Int
#include "da.h"

#define MAKEVIEW__TYPE int
#define MAKEVIEW__NAMESPACE View_Int
#include "make_view.h"

#define MAKEVIEW__TYPE Vec_Int
#define MAKEVIEW__NAMESPACE View_Vec_Int
#include "make_view.h"

#define View_Int_literal(...) (View_Int) {.data = (int[]){ __VA_ARGS__ }, .size=(int)(sizeof((int[]){ __VA_ARGS__ })/sizeof(int)) }

View_Int mimos = { 0 };
View_Vec_Int globalmi = { 0 };

#define WMAP__KEY int
#define WMAP__TYPE int
#define WMAP__NAMESPACE Map_Int
#define WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
#include "wmap.h"


/*typedef struct {*/
    /*const int *data;*/
    /*int size;*/
/*} intview_t;*/

/*#define intview_literal(...) (intview_t) {.data = (int[]){ __VA_ARGS__ }, .size=(int)(sizeof((int[]){ __VA_ARGS__ })/sizeof(int)) }*/


// Useful ID macros.
#define ID           wmap__zid_t
#define ID_valid(id) wmap__zid_valid(id)
#define ID_get(id)   wmap__zid_get(id)
#define ID_make(id)  wmap__zid_make(id)
#define ID_equals(a, b) ((a).id == (b).id)
#define ID_INVALID ((ID){0})


/// Inclusive
int rand_range(int from, int to) { return (rand() % (to - from +1)) + from; }

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
    Map_Int__swap_nodes_diff_bucket(m, a, b);
}


bool mimosaisd(void) { return NULL; }
int jlasf (void) { return (int)mimosaisd(); }



typedef struct {
    View_Int ordered_keys;
    View_Vec_Int bucket_chains;
} IntegrityTest;

IntegrityTest get_integrity_snapshot(Map_Int *m, Arena *perm) {
    IntegrityTest integrity = { 0 };
    {
        Vec_Int order_keys = Vec_Int_create_with_allocator(arena_allocator, perm);
        ID node = m->first_node;
        while(ID_valid(node)) {
            Vec_Int_append(&order_keys, m->keys.items[ID_get(node)]);
            node = m->next.items[ID_get(node)];
        }
        integrity.ordered_keys = (View_Int){order_keys.items, order_keys.size};
    }
    {
        Vec_Vec_Int bucket_chains = Vec_Vec_Int_create_with_allocator(arena_allocator, perm);

        for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
            Vec_Int *bucket_keys;
            {
                Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, perm);
                int id = Vec_Vec_Int_append(&bucket_chains, keys);
                bucket_keys = Vec_Vec_Int_get_safe(&bucket_chains, id);
                wassert(bucket_keys);
            };
            ID node = ID_make(i);
            if (Map_Int__slot_is_empty(m, node)) { continue; }
            while (ID_valid(node)) {
                Vec_Int_append(bucket_keys, m->keys.items[ID_get(node)]);
                node = m->bucket_next.items[ID_get(node)];
            }
        }
        integrity.bucket_chains = (View_Vec_Int) {bucket_chains.items, bucket_chains.size};
    }
    return integrity;
}

bool check_integrity(Map_Int *m, IntegrityTest integrity, Arena scratch) {
    // Check buckets.
    {

        if (integrity.bucket_chains.size != (1 << m->bucket_count_exp)) {
            printferr("Wrong amount of buckets."); return false;
        }

        for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {

            Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, &scratch);

            {
                // Make copy.
                Vec_Int *keys_og = &integrity.bucket_chains.items[i];
                for (dyna_foreach_gnu(iter, *keys_og)) { Vec_Int_append(&keys, *iter.ref); }
            }

            ID node = ID_make(i);
            ID last_valid_node = node;
            if (Map_Int__slot_is_empty(m, node)) {
                if (keys.size == 0) { continue; }
                else {
                    printferr("Coudln't find any, expected %d (bucket %d).", keys.size, i);
                    return false;
                }
            }

            // Checks bucket integrity FORWARDS.
            {
                while (ID_valid(node)) {
                    bool found = false;
                    for (int l = 0; l < keys.size; ++l) {
                        if (keys.items[l] == m->keys.items[ID_get(node)]) {
                            found = true; Vec_Int_remove_at(&keys, l); break;
                        }
                    }
                    if (!found) { printferr("Failed to find key"); return false; }
                    node = m->bucket_next.items[ID_get(node)];
                    if (ID_valid(node)) { last_valid_node = node; }
                }
                if (keys.size != 0) {
                    printferr("Couldn't find all.");
                    for (int l = 0; l < keys.size; ++l) { printferr("Missing %d", keys.items[i]); }
                    return false;
                }
            }


            {
                // Make copy.
                Vec_Int_clear_preserving(&keys);
                Vec_Int *keys_og = &integrity.bucket_chains.items[i];
                for (dyna_foreach_gnu(iter, *keys_og)) { Vec_Int_append(&keys, *iter.ref); }
            }

            // Checks bucket integrity BACKWARDS.
            {
                node = last_valid_node;
                while (ID_valid(node)) {
                    bool found = false;
                    for (int l = 0; l < keys.size; ++l) {
                        if (keys.items[l] == m->keys.items[ID_get(node)]) {
                            found = true; Vec_Int_remove_at(&keys, l); break;
                        }
                    }
                    if (!found) { printferr("Failed to find key"); return false; }
                    node = m->bucket_prev.items[ID_get(node)];
                }
                if (keys.size != 0) {
                    printferr("Couldn't find all.");
                    for (int l = 0; l < keys.size; ++l) { printferr("Missing %d", keys.items[i]); }
                    return false;
                }
            }
        }
    }
    // Check order.
    {
        View_Int ordered_keys = integrity.ordered_keys;
        for (int i = 0; i < ordered_keys.size; ++i) {
            printfd("(size %d) Expected order %d", ordered_keys.size, ordered_keys.items[i]);
        }
        int i = 0;
        ID node = m->first_node;
        bool failed = false;
        while(ID_valid(node)) {
            if (i >= ordered_keys.size) { printferr("Wrong amount of keys."); return false; }
            printfd("%d !=? %d", ordered_keys.items[i], m->keys.items[ID_get(node)]);
            /*if (ordered_keys.items[i] != m->keys.items[ID_get(node)]) { printferr("Wrong order."); return false; }*/
            if (ordered_keys.items[i] != m->keys.items[ID_get(node)]) { failed = true; }
            node = m->next.items[ID_get(node)];
            ++i;
        }
        if (failed) { printferr("Wrong order."); return false; }
    }
    return true;
}

/*
   Swaps all nodes of the same bucket and test whether they're still all present.
   */
void shuffle_nodes_same_bucket(Map_Int *m, const int iterations, Arena arena) {
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        Arena scratch = arena;
        Vec_Int bucket_nodes = Vec_Int_create_with_allocator(arena_allocator, &scratch);

        const ID bucket = ID_make(i);
        if (Map_Int__slot_is_empty(m, bucket)) { continue; }
        ID node = bucket;
        while (ID_valid(node)) {
            Vec_Int_append(&bucket_nodes, ID_get(node));
            node = m->bucket_next.items[ID_get(node)];
        }

        // Now shuffle them all like a maniac.
        for (int k = 0; k < iterations; ++k) {
            int a = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            int b = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            Map_Int__swap_nodes_same_bucket(m, ID_make(a), ID_make(b));
        }
    }
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

    if ((1)) {
        IntegrityTest integrity = get_integrity_snapshot(m, &arena);
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, false);
        shuffle_nodes_same_bucket(m, 50, arena);
        Map_Int_print(m);
        Map_Int_print_bucket_chains(m, true);
        Map_Int_print_order(m, false);
        ASSERT(check_integrity(m, integrity, arena));
    }

    // Test remove.
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);

    int key;
    key = 888;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);
    key = 3;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);
    key = 4;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);
    key = 5;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);
    key = 1;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    Map_Int_print(m);
    Map_Int_print_bucket_chains(m, true);
    Map_Int_print_order(m, false);


    /*{*/
        /*Map_Int_It it = Map_Int_make_it(m);*/
        /*while (Map_Int_it_next(m, &it)) {*/
            /*printfd(ANSI_BLU"%d -> %d", it.key, *it.value);*/
        /*}*/
    /*}*/

    /*ASSERT_INT(err, 0);*/


    Map_Int_free(m);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}



int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


