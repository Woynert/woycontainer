#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"
#include "arena.h"
#include "arenady.h"

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

#define WMAP__KEY int
#define WMAP__TYPE int
#define WMAP__NAMESPACE Map_Int
#define WMAP__KEY_CAN_DO_BINARY_COMPARISON_AND_HASH
#include "wmap.h"



/// START [NAIVE MAP]
typedef struct {
    int key;
    int value;
} NaivePair;
#define DYNA__TYPE NaivePair
#define DYNA__NAMESPACE Vec_NaivePair
#include "da.h"
typedef struct {
    Vec_NaivePair pairs;
} NaiveMap;
NaiveMap naive_map_create(void) { return (NaiveMap) {.pairs=Vec_NaivePair_create()}; }
void naive_map_free(NaiveMap *m) { Vec_NaivePair_free(&m->pairs); *m = (NaiveMap){0}; }
int *naive_map_get(NaiveMap *m, int key) {
    for (dyna_foreach_gnu(iter, m->pairs)) {
        if (iter.ref->key == key) { return &iter.ref->value; }
    }
    return NULL;
}
int naive_map_upsert(NaiveMap *m, int key, int item) {
    int *saved_item = naive_map_get(m, key);
    if (saved_item) { *saved_item = item; return 0; } // Update.
    Vec_NaivePair_append(&m->pairs, (NaivePair){.key=key, .value=item});
    return 0;
}
void naive_map_remove(NaiveMap *m, int key) {
    for (dyna_foreach_gnu(iter, m->pairs)) {
        if (iter.ref->key == key) { Vec_NaivePair_pop_at_preserve_order(&m->pairs, iter.index, NULL); }
    }
}
/// END [NAIVE MAP]
/// START [BOTH MAPS]
int both_maps_upsert(Map_Int *a, NaiveMap *b, int key, int item) {
    return Map_Int_upsert(a, key, item) + naive_map_upsert(b, key, item);
}
void both_maps_remove(Map_Int *a, NaiveMap *b, int key) {
    Map_Int_remove(a, key); naive_map_remove(b, key);
}
int both_maps_get(Map_Int *a, NaiveMap *b, int key, bool *out_equal) {
    int *result_a = Map_Int_get(a, key);
    int *result_b = naive_map_get(b, key);
    *out_equal = (*result_b == *result_a);
    return *result_a;
}
bool both_maps_compare_order_and_contents(Map_Int *a, NaiveMap *b) {
    if (Map_Int_pair_count(a) != b->pairs.size
        || Map_Int_pair_count(a) != a->values.count
    ) { printferr("Wrong pair count."); return false; }
    // Forward.
    {
        int k = 0;
        Map_Int_It it = Map_Int_make_it(a);
        while (Map_Int_it_next(a, &it)) {
            if (!int_in_range_inclusive(0, b->pairs.size-1, k)) { printferr("Worng pair count.."); return false; }
            if (it.key != b->pairs.items[k].key
                || *it.value != b->pairs.items[k].value)
            {
                printferr("Wrong key or value."); 
                return false;
            }
            ++k;
        }
        if (k != b->pairs.size) { printferr("Coudn't find all pairs"); return false; }
    }
    // Backwards.
    {
        int k = b->pairs.size-1;
        Map_Int_It it = Map_Int_make_it_end(a);
        while (Map_Int_it_prev(a, &it)) {
            if (!int_in_range_inclusive(0, b->pairs.size-1, k)) { printferr("Worng pair count.."); return false; }
            if (it.key != b->pairs.items[k].key
                || *it.value != b->pairs.items[k].value)
            {
                printferr("Wrong key or value."); 
                return false;
            }
            --k;
        }
        if (k != -1) { printferr("Coudn't find all pairs"); return false; }
    }
    return true;
}
/// END [BOTH MAPS]



typedef struct {
    View_Int ordered_keys;
    View_Vec_Int bucket_chains;
} IntegrityTest;

IntegrityTest get_integrity_snapshot(Map_Int *w, Arena *perm) {
    Map_Int__Table *m = &w->table;
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
            if (Map_Int__Table__slot_is_empty(m, node)) { continue; }
            while (ID_valid(node)) {
                Vec_Int_append(bucket_keys, m->keys.items[ID_get(node)]);
                node = m->bucket_next.items[ID_get(node)];
            }
        }
        integrity.bucket_chains = (View_Vec_Int) {bucket_chains.items, bucket_chains.size};
    }
    return integrity;
}

bool check_integrity(Map_Int *w, IntegrityTest integrity, Arena scratch) {
    Map_Int__Table *m = &w->table;
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
            if (Map_Int__Table__slot_is_empty(m, node)) {
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
    if (integrity.ordered_keys.size != m->pair_count) { printferr("Wrong pair count."); return false; }
    // Check order FORWARDS.
    {
        View_Int ordered_keys = integrity.ordered_keys;
        int i = 0;
        ID node = m->first_node;
        while(ID_valid(node)) {
            if (i >= ordered_keys.size) { printferr("Wrong amount of keys."); return false; }
            printfd("%d !=? %d", ordered_keys.items[i], m->keys.items[ID_get(node)]);
            if (ordered_keys.items[i] != m->keys.items[ID_get(node)]) { printferr("Wrong order."); return false; }
            node = m->next.items[ID_get(node)];
            ++i;
        }
        if (i != ordered_keys.size) { printferr("Couldn't find all pairs."); return false; }
    }
    // Check order BACKWARDS.
    {
        View_Int ordered_keys = integrity.ordered_keys;
        int i = ordered_keys.size-1;
        ID node = m->last_node;
        while(ID_valid(node)) {
            if (i >= ordered_keys.size) { printferr("Wrong amount of keys."); return false; }
            printfd("%d !=? %d", ordered_keys.items[i], m->keys.items[ID_get(node)]);
            if (ordered_keys.items[i] != m->keys.items[ID_get(node)]) { printferr("Wrong order."); return false; }
            node = m->prev.items[ID_get(node)];
            --i;
        }
        if (i != -1) { printferr("Couldn't find all pairs."); return false; }
    }
    return true;
}

/*
   Swaps all nodes of the same bucket and test whether they're still all present.
   */
void shuffle_collision_nodes_same_bucket(Map_Int *w, const int iterations, Arena arena) {
    Map_Int__Table *m = &w->table;
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        Arena scratch = arena;
        Vec_Int bucket_nodes = Vec_Int_create_with_allocator(arena_allocator, &scratch);

        const ID bucket = ID_make(i);
        if (Map_Int__Table__slot_is_empty(m, bucket)) { continue; }
        ID node = bucket;
        while (ID_valid(node)) {
            Vec_Int_append(&bucket_nodes, ID_get(node));
            node = m->bucket_next.items[ID_get(node)];
        }

        // Now shuffle them all like a maniac.
        for (int k = 0; k < iterations; ++k) {
            int a = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            int b = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            Map_Int__Table__swap_nodes_same_bucket(m, ID_make(a), ID_make(b));
        }
    }
}


// START [PRINTING]
void map_print(Map_Int *w) {
    Map_Int__Table *m = &w->table;
    printf("Printing map, size %d, buckets %d, pairs %d, cols %d, first %d, last %d",
            1 << m->capacity_exp, 1 << m->bucket_count_exp,
            m->pair_count, m->collision_count, ID_get(m->first_node), ID_get(m->last_node));
    printf("\nkeys:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", m->keys.items[i]);
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nvalueid: ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->val_ids.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nbuck_nxt:");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->bucket_next.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nbuck_prv:");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->bucket_prev.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nvalues:  ");
    for (int i = 0; i < w->values.count; ++i) {
        printf("[%d]=%d,", w->values.itemid_to_userid[i], w->values._items[i]);
    }
    printf("\nnext:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->next.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    printf("\nprev:    ");
    for (int i = 0; i < 1 << m->capacity_exp; ++i) {
        printf("%5d|", ID_get(m->prev.items[i]));
        if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    }
    //printf("\nbuk_limt:");
    //for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        //printf("%2d,%2d|", i, ID_get(m->bucket_tail.items[i]));
        //if (i+1 == 1 << m->bucket_count_exp) { printf("|"); }
    //}
    printf("\n");
}
void map_print_order(Map_Int *w, bool backwards) {
    Map_Int__Table *m = &w->table;
    const int MAX_CYCLES = 100;
    int cycles = 0;
    ID node = m->first_node;
    printf("Order Forward:\n");
    while(ID_valid(node)) {
        printf("%d,", m->keys.items[ID_get(node)]);
        node = m->next.items[ID_get(node)];
        if (++cycles > MAX_CYCLES) { printf("\n"); return; }
    }
    printf("\n");
    if (!backwards) { return; }
    cycles = 0;
    printf("Order Backwards:\n");
    node = m->last_node;
    while(ID_valid(node)) {
        printf("%d,", m->keys.items[ID_get(node)]);
        node = m->prev.items[ID_get(node)];
        if (++cycles > MAX_CYCLES) { printf("\n"); return; }
    }
    printf("\n");
}
void map_print_bucket_chains(Map_Int *w) {
    Map_Int__Table *m = &w->table;
    const int max_cycles = 100;
    int cycles = 0;
    printf("Forward:\n");
    for (int i = 0; i < 1 << m->bucket_count_exp; ++i) {
        ID id = ID_make(i);
        printf("(bucket %d): ", i);
        if (!ID_valid(m->val_ids.items[ID_get(id)])) { printf("\n"); continue; }
        while (ID_valid(id)) {
            //printf("%d"ANSI_GRE"(%d)"ANSI_RESET",", ID_get(id), m->keys.items[ID_get(id)]);
            printf("%d,", m->keys.items[ID_get(id)]);
            id = m->bucket_next.items[ID_get(id)];

            if (++cycles > max_cycles) { return; }
        }
        printf("\n");
    }
}
// END [PRINTING]


TEST test_manual(void) {
    ArenaRoot arenaroot = ArenaRoot_create(1 << 20);
    Arena arena = ArenaRoot_get_arena(arenaroot);
    int err;
    Map_Int _map = { 0 };
    Map_Int *m = &_map;
    Map_Int_create(m);

    map_print(m);
    printfd("---");
    err = Map_Int_upsert(m, 888, 80);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 1);
    printfd("---");
    err = Map_Int_upsert(m, 1, 10);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 2);
    printfd("---");
    err = Map_Int_upsert(m, 2, 20);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 3);
    printfd("---");
    err = Map_Int_upsert(m, 3, 30);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 4);
    printfd("---");
    err = Map_Int_upsert(m, 4, 40);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 5);
    printfd("---");
    err = Map_Int_upsert(m, 5, 50);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Int_pair_count(m), 6);

    map_print(m);
    map_print_bucket_chains(m);

    srand(0);
    if ((1)) {
        printfd("↓↓↓");
        map_print(m);
        map_print_bucket_chains(m);
        map_print_order(m, true);
        printfd("---");
        for (int i = 0; i < 100; ++i) {
            /*int a = rand_range(1 << m->bucket_count_exp, (1 << m->bucket_count_exp) + m->collision_count-1);*/
            /*int b = rand_range(1 << m->bucket_count_exp, (1 << m->bucket_count_exp) + m->collision_count-1);*/
            /*Map_Int__swap_nodes_diff_bucket(m, ID_make(a), ID_make(b));*/
        }
        printfd("---");
        map_print(m);
        map_print_bucket_chains(m);
        map_print_order(m, true);
        printfd("↑↑↑");
    }

    if ((1)) {
        IntegrityTest integrity = get_integrity_snapshot(m, &arena);
        map_print(m);
        map_print_bucket_chains(m);
        map_print_order(m, false);
        shuffle_collision_nodes_same_bucket(m, 50, arena);
        map_print(m);
        map_print_bucket_chains(m);
        map_print_order(m, false);
        ASSERT(check_integrity(m, integrity, arena));
    }

    // Test remove.
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);

    int key;
    key = 888;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = 3;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = 4;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = 5;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = 1;
    printfd("Removing %d", key);
    Map_Int_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);

    Map_Int_free(m);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}


TEST test_auto(void) {
    ArenaRoot arenaroot = ArenaRoot_create(1 << 20);
    Arena arena = ArenaRoot_get_arena(arenaroot);
    int err;

    Map_Int _map = { 0 };
    Map_Int *m = &_map;
    Map_Int_create(m);
    NaiveMap _naive = naive_map_create();
    NaiveMap *nai = &_naive;

    srand(777);

    // Insert NEW pairs.
    {
        for (int i = 0; i < 100; ++i) {
            int key = rand_range(INT_MIN, INT_MAX);
            int value = key / 10;
            err = both_maps_upsert(m, nai, key, value);
            ASSERT(!err);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
    }

    // Update pairs (half total).
    {
        Arena scratch = arena;
        IntegrityTest integrity = get_integrity_snapshot(m, &scratch);
        for (int i = 0; i < nai->pairs.size/2; ++i) {
            int id = rand_range(0, nai->pairs.size-1);
            int key = nai->pairs.items[id].key;
            int value = key / 100;
            both_maps_upsert(m, nai, key, value);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
        ASSERT(check_integrity(m, integrity, scratch));
    }

    // Remove half pairs.
    {
        for (int i = 0; i < nai->pairs.size/2; ++i) {
            int id = rand_range(0, nai->pairs.size-1);
            int key = nai->pairs.items[id].key;
            both_maps_remove(m, nai, key);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
    }

    // Insert pairs again.
    {
        for (int i = 0; i < 200; ++i) {
            int key = rand_range(INT_MIN, INT_MAX);
            int value = key / 10;
            err = both_maps_upsert(m, nai, key, value);
            ASSERT(!err);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
    }

    map_print(m); map_print_bucket_chains(m); map_print_order(m, false);

    // Shuffle.
    {
        Arena scratch = arena;
        IntegrityTest integrity = get_integrity_snapshot(m, &scratch);
        shuffle_collision_nodes_same_bucket(m, 100, arena);
        ASSERT(check_integrity(m, integrity, scratch));
    }

    // DELME
    {
        int i = 0; Map_Int_It it = Map_Int_make_it(m);
        while(Map_Int_it_next(m, &it)) {
            int key = nai->pairs.items[i].key;
            printfd("%d. [%d]=>%d", i, key, nai->pairs.items[i].value);
            printfd("%d. [%d]=>%d", i, key, *Map_Int_get(m, key));
            ASSERT_INT(nai->pairs.items[i].value, *Map_Int_get(m, key));
            ++i;
        }
    }

    // Remove all pairs.
    {
        Arena scratch = arena;
        Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, &scratch);
        Map_Int_It it = Map_Int_make_it(m);
        while(Map_Int_it_next(m, &it)) {
            Vec_Int_append(&keys, it.key);
        }
        for (dyna_foreach_gnu(iter, keys)) {
            both_maps_remove(m, nai, *iter.ref);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
    }

    map_print(m); map_print_bucket_chains(m); map_print_order(m, false);

    Map_Int_free(m);
    naive_map_free(nai);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}


static Map_Int map_for_fuzzer;
void LLVMFuzzerCleanup(void) { Map_Int_free(&map_for_fuzzer); }
int LLVMFuzzerInitialize(int *argc, char ***argv) { (void)argc;(void)argv;atexit(LLVMFuzzerCleanup); return 0; }
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    static bool setup = false;
    static Map_Int *m = &map_for_fuzzer;
    if (!setup) {
        setup = true;
        Map_Int_create(m);
    }

    ArenaDy arena = { .root = (char*)data, .beg = (char*)data, .end = (char*)data + size };
    const int *action = arenady_try_get_one(&arena, int);
    if (!action) { return 0; }

    switch (*action) {
        case 0:
        {
            const int *key = arenady_try_get_one(&arena, int);
            const int *value = arenady_try_get_one(&arena, int);
            if (!key || !value) { break; }
            Map_Int_upsert(m, *key, *value);
            break;
        }
        case 1:
        {
            const int *key = arenady_try_get_one(&arena, int);
            if (!key) { break; }
            Map_Int_remove(m, *key);
            break;
        }
        case 2:
        {
            const int *key = arenady_try_get_one(&arena, int);
            if (!key) { break; }
            Map_Int_get(m, *key);
            break;
        }
        case 3:
        {
            Map_Int_It it = Map_Int_make_it(m);
            while(Map_Int_it_next(m, &it)) { (void)0; }
            break;
        }
        case 4:
        {
            Map_Int_It it = Map_Int_make_it_end(m);
            while(Map_Int_it_prev(m, &it)) { (void)0; }
            break;
        }
        default: break;
    }
    return 0;
}

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
int main(void) {
    TESTS_INIT();
    RUN_TEST(test_manual);
    RUN_TEST(test_auto);
    TESTS_SHOW_RESULTS();
}
#endif


