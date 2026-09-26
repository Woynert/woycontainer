#include "strpool.h"
#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"
#include "arena.h"
#include "arenady.h"
#include "wstrview.h"

#define DYNA__TYPE ID
#define DYNA__NAMESPACE Vec_Int
#include "da.h"

#define DYNA__TYPE Vec_Int
#define DYNA__NAMESPACE Vec_Vec_Int
#include "da.h"

#define DYNA__TYPE strview_t
#define DYNA__NAMESPACE Vec_Strview
#include "da.h"

#define MAKEVIEW__TYPE ID
#define MAKEVIEW__NAMESPACE View_Int
#include "make_view.h"

#define MAKEVIEW__TYPE Vec_Int
#define MAKEVIEW__NAMESPACE View_Vec_Int
#include "make_view.h"

#define View_Int_literal(...) (View_Int) {.data = (int[]){ __VA_ARGS__ }, .size=(int)(sizeof((int[]){ __VA_ARGS__ })/sizeof(int)) }

/*#define WMAP__KEY int*/
#define WMAPSTR__TYPE int
#define WMAPSTR__NAMESPACE MapStrInt
#include "wmapstr.h"



/// START [NAIVE MAP]
typedef struct {
    ID key_id;
    int value;
} NaivePair;
#define DYNA__TYPE NaivePair
#define DYNA__NAMESPACE Vec_NaivePair
#include "da.h"
typedef struct {
    Strpool strpool;
    Vec_NaivePair pairs;
} NaiveMap;
NaiveMap naive_map_create(void) {
    NaiveMap m = { 0 };
    strpool_create(&m.strpool);
    m.pairs = Vec_NaivePair_create();
    return m;
}
void naive_map_free(NaiveMap *m) {
    strpool_destroy(&m->strpool);
    Vec_NaivePair_free(&m->pairs);
    *m = (NaiveMap){0};
}
int *naive_map_get(const NaiveMap *m, strview_t key) {
    for (dyna_foreach_gnu(iter, m->pairs)) {
        strview_t key_str = strpool_get(&m->strpool, iter.ref->key_id);
        if (wstrview_equals(key, key_str)) { return &iter.ref->value; }
    }
    return NULL;
}
int naive_map_upsert(NaiveMap *m, strview_t key, int item) {
    int *saved_item = naive_map_get(m, key);
    if (saved_item) { *saved_item = item; return 0; } // Update.
    ID id = strpool_append(&m->strpool, key);
    if (!ID_valid(id)) { return -1; }
    Vec_NaivePair_append(&m->pairs, (NaivePair){.key_id=id, .value=item});
    return 0;
}
void naive_map_remove(NaiveMap *m, strview_t key) {
    for (dyna_foreach_gnu(iter, m->pairs)) {
        strview_t key_str = strpool_get(&m->strpool, iter.ref->key_id);
        if (wstrview_equals(key, key_str)) {
            int err = strpool_remove(&m->strpool, iter.ref->key_id);
            wassert_live(!err);
            err = Vec_NaivePair_pop_at_preserve_order(&m->pairs, iter.index, NULL);
            wassert_live(!err);
            return;
        }
    }
}
strview_t naive_map_get_key(NaiveMap *m, int index) {
    return strpool_get(&m->strpool, m->pairs.items[index].key_id);
}
/// END [NAIVE MAP]
/// START [BOTH MAPS]
int both_maps_upsert(MapStrInt *a, NaiveMap *b, strview_t key, int item) {
    return MapStrInt_upsert(a, key, item) + naive_map_upsert(b, key, item);
}
void both_maps_remove(MapStrInt *a, NaiveMap *b, strview_t key) {
    MapStrInt_remove(a, key); naive_map_remove(b, key);
}
int *both_maps_get(MapStrInt *a, NaiveMap *b, strview_t key, bool *out_equal) {
    int *result_a = MapStrInt_get(a, key);
    int *result_b = naive_map_get(b, key);
    *out_equal = (result_a == NULL && result_a == result_b) || (*result_a == *result_b);
    return result_a;
}
bool both_maps_compare_order_and_contents(MapStrInt *a, NaiveMap *b) {
    if (MapStrInt_pair_count(a) != b->pairs.size
        || MapStrInt_pair_count(a) != a->values.count
    ) { printferr("Wrong pair count."); return false; }
    // Forward.
    {
        int k = 0;
        MapStrInt_It it = MapStrInt_make_it(a);
        while (MapStrInt_it_next(a, &it)) {
            if (!int_in_range_inclusive(0, b->pairs.size-1, k)) { printferr("Worng pair count.."); return false; }
            strview_t naive_saved_key = strpool_get(&b->strpool, b->pairs.items[k].key_id);
            /*printfd("%d/%d", k, b->pairs.size-1);*/
            /*printfd(ANSI_BLU"["PRIstrw"](%d) ==? ["PRIstrw"](%d)", PRIstrarg(it.key), it.key.size, PRIstrarg(naive_saved_key), naive_saved_key.size);*/
            if (!wstrview_equals(it.key, naive_saved_key)
                || *it.value != b->pairs.items[k].value)
            {
                /*printfd("["PRIstrw"](%d) ==? ["PRIstrw"](%d)", PRIstrarg(it.key), it.key.size, PRIstrarg(naive_saved_key), naive_saved_key.size);*/
                /*printfd("%d ==? %d", *it.value, b->pairs.items[k].value);*/
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
        MapStrInt_It it = MapStrInt_make_it_end(a);
        while (MapStrInt_it_prev(a, &it)) {
            if (!int_in_range_inclusive(0, b->pairs.size-1, k)) { printferr("Worng pair count.."); return false; }
            /*if (it.key != b->pairs.items[k].key*/
                /*|| *it.value != b->pairs.items[k].value)*/
            strview_t naive_saved_key = strpool_get(&b->strpool, b->pairs.items[k].key_id);
            if (!wstrview_equals(it.key, naive_saved_key)
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

IntegrityTest get_integrity_snapshot(MapStrInt *w, Arena *perm) {
    MapStrInt__Table *m = &w->table;
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

        for (int i = 0; i < m->bucket_count; ++i) {
            Vec_Int *bucket_keys;
            {
                Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, perm);
                int id = Vec_Vec_Int_append(&bucket_chains, keys);
                bucket_keys = Vec_Vec_Int_get_safe(&bucket_chains, id);
                wassert(bucket_keys);
            };
            ID node = ID_make(i);
            if (MapStrInt__Table_slot_is_empty(m, node)) { continue; }
            while (ID_valid(node)) {
                Vec_Int_append(bucket_keys, m->keys.items[ID_get(node)]);
                node = m->bucket_next.items[ID_get(node)];
            }
        }
        integrity.bucket_chains = (View_Vec_Int) {bucket_chains.items, bucket_chains.size};
    }
    return integrity;
}

bool check_integrity(MapStrInt *w, IntegrityTest integrity, Arena scratch) {
    MapStrInt__Table *m = &w->table;
    // Check buckets.
    {
        if (integrity.bucket_chains.size != m->bucket_count) {
            printferr("Wrong amount of buckets."); return false;
        }

        for (int i = 0; i < m->bucket_count; ++i) {

            Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, &scratch);

            {
                // Make copy.
                Vec_Int *keys_og = &integrity.bucket_chains.items[i];
                for (dyna_foreach_gnu(iter, *keys_og)) { Vec_Int_append(&keys, *iter.ref); }
            }

            ID node = ID_make(i);
            ID last_valid_node = node;
            if (MapStrInt__Table_slot_is_empty(m, node)) {
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
                        if (ID_equals(keys.items[l], m->keys.items[ID_get(node)])) {
                            found = true; Vec_Int_remove_at(&keys, l); break;
                        }
                    }
                    if (!found) { printferr("Failed to find key"); return false; }
                    node = m->bucket_next.items[ID_get(node)];
                    if (ID_valid(node)) { last_valid_node = node; }
                }
                if (keys.size != 0) {
                    printferr("Couldn't find all.");
                    for (int l = 0; l < keys.size; ++l) { printferr("Missing %d", ID_get(keys.items[i])); }
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
                        if (ID_equals(keys.items[l], m->keys.items[ID_get(node)])) {
                            found = true; Vec_Int_remove_at(&keys, l); break;
                        }
                    }
                    if (!found) { printferr("Failed to find key"); return false; }
                    node = m->bucket_prev.items[ID_get(node)];
                }
                if (keys.size != 0) {
                    printferr("Couldn't find all.");
                    for (int l = 0; l < keys.size; ++l) { printferr("Missing %d", ID_get(keys.items[i])); }
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
            printfd("%d !=? %d", IDget(ordered_keys.items[i]), IDget(m->keys.items[ID_get(node)]));
            if (!ID_equals(ordered_keys.items[i], m->keys.items[ID_get(node)])) { printferr("Wrong order."); return false; }
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
            printfd("%d !=? %d", IDget(ordered_keys.items[i]), IDget(m->keys.items[ID_get(node)]));
            if (!ID_equals(ordered_keys.items[i], m->keys.items[ID_get(node)])) { printferr("Wrong order."); return false; }
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
void shuffle_collision_nodes_same_bucket(MapStrInt *w, const int iterations, Arena arena) {
    MapStrInt__Table *m = &w->table;
    for (int i = 0; i < m->bucket_count; ++i) {
        Arena scratch = arena;
        Vec_Int bucket_nodes = Vec_Int_create_with_allocator(arena_allocator, &scratch);

        const ID bucket = ID_make(i);
        if (MapStrInt__Table_slot_is_empty(m, bucket)) { continue; }
        ID node = bucket;
        while (ID_valid(node)) {
            Vec_Int_append(&bucket_nodes, node);
            node = m->bucket_next.items[ID_get(node)];
        }

        // Now shuffle them all like a maniac.
        for (int k = 0; k < iterations; ++k) {
            ID a = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            ID b = bucket_nodes.items[rand_range(0, bucket_nodes.size-1)];
            MapStrInt__Table__swap_nodes_same_bucket(m, a, b);
        }
    }
}


// START [PRINTING]
void map_print(MapStrInt *w) {
    MapStrInt__Table *m = &w->table;
    printf("Printing map, size %d, buckets %d, pairs %d, cols %d, first %d, last %d",
            m->capacity, m->bucket_count,
            m->pair_count, m->collision_count, ID_get(m->first_node), ID_get(m->last_node));
    printf("\nkeys:    ");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->keys.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    printf("\nvalueid: ");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->val_ids.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    printf("\nbuck_nxt:");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->bucket_next.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    printf("\nbuck_prv:");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->bucket_prev.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    printf("\nvalues:  ");
    for (int i = 0; i < w->values.count; ++i) {
        printf("[%d]=%d,", w->values.itemid_to_userid[i], w->values._items[i]);
    }
    printf("\nnext:    ");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->next.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    printf("\nprev:    ");
    for (int i = 0; i < m->capacity; ++i) {
        printf("%5d|", ID_get(m->prev.items[i]));
        if (i+1 == m->bucket_count) { printf("|"); }
    }
    //printf("\nbuk_limt:");
    //for (int i = 0; i < m->bucket_count; ++i) {
        //printf("%2d,%2d|", i, ID_get(m->bucket_tail.items[i]));
        //if (i+1 == m->bucket_count) { printf("|"); }
    //}
    printf("\n");
}
void map_print_order(MapStrInt *w, bool backwards) {
    MapStrInt__Table *m = &w->table;
    const int MAX_CYCLES = 100;
    int cycles = 0;
    ID node = m->first_node;
    printf("Order Forward:\n");
    while(ID_valid(node)) {
        printf("%d,", ID_get(m->keys.items[ID_get(node)]));
        node = m->next.items[ID_get(node)];
        if (++cycles > MAX_CYCLES) { printf("\n"); return; }
    }
    printf("\n");
    if (!backwards) { return; }
    cycles = 0;
    printf("Order Backwards:\n");
    node = m->last_node;
    while(ID_valid(node)) {
        printf("%d,", IDget(m->keys.items[ID_get(node)]));
        node = m->prev.items[ID_get(node)];
        if (++cycles > MAX_CYCLES) { printf("\n"); return; }
    }
    printf("\n");
}
void map_print_bucket_chains(MapStrInt *w) {
    MapStrInt__Table *m = &w->table;
    const int max_cycles = 100;
    int cycles = 0;
    printf("Forward:\n");
    for (int i = 0; i < m->bucket_count; ++i) {
        ID id = ID_make(i);
        printf("(bucket %d): ", i);
        if (!ID_valid(m->val_ids.items[ID_get(id)])) { printf("\n"); continue; }
        while (ID_valid(id)) {
            //printf("%d"ANSI_GRE"(%d)"ANSI_RESET",", ID_get(id), m->keys.items[ID_get(id)]);
            printf("%d,", IDget(m->keys.items[ID_get(id)]));
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
    MapStrInt _map = { 0 };
    MapStrInt *m = &_map;
    MapStrInt_create(m);

    map_print(m);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("888"), 80);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 1);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("1"), 10);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 2);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("2"), 20);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 3);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("3"), 30);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 4);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("4"), 40);
    map_print(m);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 5);
    printfd("---");
    err = MapStrInt_upsert(m, cstr_SL("5"), 50);
    ASSERT_INT(err, 0);
    ASSERT_INT(MapStrInt_pair_count(m), 6);

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
            int a = rand_range(m->table.bucket_count, (m->table.bucket_count) + m->table.collision_count-1);
            int b = rand_range(m->table.bucket_count, (m->table.bucket_count) + m->table.collision_count-1);
            MapStrInt__Table__swap_nodes_diff_bucket(&m->table, ID_make(a), ID_make(b));
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

    strview_t key;
    key = cstr_SL("888");
    printfd("Removing ["PRIstrw"]", PRIstrarg(key));
    MapStrInt_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = cstr_SL("3");
    printfd("Removing ["PRIstrw"]", PRIstrarg(key));
    MapStrInt_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = cstr_SL("4");
    printfd("Removing ["PRIstrw"]", PRIstrarg(key));
    MapStrInt_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = cstr_SL("5");
    printfd("Removing ["PRIstrw"]", PRIstrarg(key));
    MapStrInt_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);
    key = cstr_SL("1");
    printfd("Removing ["PRIstrw"]", PRIstrarg(key));
    MapStrInt_remove(m, key);
    map_print(m);
    map_print_bucket_chains(m);
    map_print_order(m, false);

    MapStrInt_free(m);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}


strview_t create_random_string(int size, Arena *perm) {
    char *data = arena_new(perm, char, size);
    wassert_live(data);
    for (int i = 0; i < size; ++i) {
        data[i] = (char)rand_range(32, 126);
    }
    return (strview_t) { .data = data, .size = size, };
}


TEST test_auto(void) {
    ArenaRoot arenaroot = ArenaRoot_create(1 << 20);
    Arena arena = ArenaRoot_get_arena(arenaroot);
    int err;

    MapStrInt _map = { 0 };
    MapStrInt *m = &_map;
    MapStrInt_create(m);
    NaiveMap _naive = naive_map_create();
    NaiveMap *nai = &_naive;

    srand(777);

    // Insert NEW pairs.
    {
        for (int i = 0; i < 100; ++i) {
            // @Note: Probably better to get the key from the random number.
            Arena scratch = arena;
            strview_t key = create_random_string(i * 4, &scratch); 
            int value = rand_range(INT_MIN, INT_MAX);
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
            /*strview_t key = strpool_get(&nai->strpool, nai->pairs.items[id].key_id);*/
            strview_t key = naive_map_get_key(nai, id);
            int value = rand_range(INT_MIN, INT_MAX);
            both_maps_upsert(m, nai, key, value);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
        ASSERT(check_integrity(m, integrity, scratch));
    }

    // Remove half pairs.
    {
        for (int i = 0; i < nai->pairs.size/2; ++i) {
            int id = rand_range(0, nai->pairs.size-1);
            /*int key = nai->pairs.items[id].key;*/
            /*strview_t key = strpool_get(&nai->strpool, nai->pairs.items[id].key_id);*/
            strview_t key = naive_map_get_key(nai, id);
            both_maps_remove(m, nai, key);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
    }

    // Insert pairs again.
    {
        for (int i = 0; i < 200; ++i) {
            /*int key = rand_range(INT_MIN, INT_MAX);*/
            /*int value = key / 10;*/
            Arena scratch = arena;
            strview_t key = create_random_string(i * 4, &scratch); 
            int value = rand_range(INT_MIN, INT_MAX);
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
        int i = 0; MapStrInt_It it = MapStrInt_make_it(m);
        while(MapStrInt_it_next(m, &it)) {
            strview_t key = naive_map_get_key(nai, i);
            printfd("%d. ["PRIstrw"]=>%d", i, PRIstrarg(key), nai->pairs.items[i].value);
            printfd("%d. ["PRIstrw"]=>%d", i, PRIstrarg(key), *MapStrInt_get(m, key));
            ASSERT_INT(nai->pairs.items[i].value, *MapStrInt_get(m, key));
            ++i;
        }
    }

    // Remove all pairs.
    {
        Arena scratch = arena;
        Strpool saved_keys; strpool_create_with_allocator(&saved_keys, arena_allocator, &scratch);
        Vec_Int keys = Vec_Int_create_with_allocator(arena_allocator, &scratch);
        MapStrInt_It it = MapStrInt_make_it(m);
        while(MapStrInt_it_next(m, &it)) {
            ID key_id = strpool_append(&saved_keys, it.key);
            ASSERT(ID_valid(key_id));
            Vec_Int_append(&keys, key_id);
        }
        for (dyna_foreach_gnu(iter, keys)) {
            strview_t key = strpool_get(&saved_keys, *iter.ref);
            both_maps_remove(m, nai, key);
        }
        ASSERT(both_maps_compare_order_and_contents(m, nai));
        ASSERT_INT(m->values.count, 0);
    }

    map_print(m); map_print_bucket_chains(m); map_print_order(m, false);

    MapStrInt_free(m);
    naive_map_free(nai);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}


static MapStrInt map_for_fuzzer;
static NaiveMap nm_for_fuzzer;
void LLVMFuzzerCleanup(void) {
    MapStrInt_free(&map_for_fuzzer); naive_map_free(&nm_for_fuzzer);
}
int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc,(void)argv; atexit(LLVMFuzzerCleanup); return 0;
}
strview_t get_str_from_remainder(ArenaDy *perm) {
    int size = (int)(perm->end - perm->beg);
    if (size <= 0) { return STRVIEW_INVALID; }
    const char *data = (char *)arenady_try_get(perm, sizeof(char), _Alignof(char), size);
    if (!data) { return STRVIEW_INVALID; }
    return (strview_t) { data, size };

}
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /*
        @Note: This fuzz tests runs the same operations on WMap and on NaiveMap,
        after each operation it's verified they contain same order and contents.
       */
    static bool setup = false;
    static MapStrInt *m = &map_for_fuzzer;
    static NaiveMap *nm = &nm_for_fuzzer;
    if (!setup) {
        setup = true;
        MapStrInt_create(m);
        nm_for_fuzzer = naive_map_create();
    }

    ArenaDy arena = { .root = (char*)data, .beg = (char*)data, .end = (char*)data + size };
    const int *action = arenady_try_get_one(&arena, int);
    if (!action) { return 0; }

    switch (*action) {
        case 0:
        {
            /*printfd(ANSI_RED"DELME INSERT");*/
            const int *value = arenady_try_get_one(&arena, int);
            if (!value) { break; }
            strview_t key = get_str_from_remainder(&arena);
            if (!wstrview_is_valid(key)) { return 0; }
            int err = both_maps_upsert(m, nm, key, *value);
            wassert_live(err == 0);
            wassert_live(both_maps_compare_order_and_contents(m, nm));
            break;
        }
        case 1:
        {
            /*printfd(ANSI_RED"DELME REMOVE");*/
            strview_t key = get_str_from_remainder(&arena);
            if (!wstrview_is_valid(key)) { return 0; }
            int prev_count_a = m->values.count;
            int prev_count_b = nm->pairs.size;
            both_maps_remove(m, nm, key);
            int new_count_a = m->values.count;
            int new_count_b = nm->pairs.size;
            wassert_live(prev_count_a == prev_count_b);
            wassert_live(new_count_a == new_count_b);
            /*printfd("%d, %d", prev_count_a, new_count_a);*/
            wassert_live(both_maps_compare_order_and_contents(m, nm));
            break;
        }
        case 2:
        {
            /*printfd(ANSI_RED"DELME GET");*/
            strview_t key = get_str_from_remainder(&arena);
            if (!wstrview_is_valid(key)) { return 0; }
            bool equal = false;
            both_maps_get(m, nm, key, &equal);
            wassert_live(equal);
            wassert_live(both_maps_compare_order_and_contents(m, nm));
            break;
        }
        case 3:
        {
            MapStrInt_It it = MapStrInt_make_it(m);
            while(MapStrInt_it_next(m, &it)) { (void)0; }
            break;
        }
        case 4:
        {
            MapStrInt_It it = MapStrInt_make_it_end(m);
            while(MapStrInt_it_prev(m, &it)) { (void)0; }
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


