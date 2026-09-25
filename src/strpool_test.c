#include "woytest.h"
#include "portable_utils.h"
#include "strpool.h"
#include "arena.h"
#include "arenady.h"

typedef struct { ID first; ID second; } Pair;
#define DYNA__TYPE Pair
#define DYNA__NAMESPACE VecPair
#include "da.h"


#define DYNA__TYPE strview_t
#define DYNA__NAMESPACE VecStrview
#include "da.h"


/// ***** START OF [NAIVE STRING POOL]
#define SLOT__TYPE strview_t
#define SLOT__NAMESPACE SlotStrview
#include "slot.h"
typedef struct {
    SlotStrview strings;
} NaiveStrpool;
NaiveStrpool naivestrpool_create(void) {
    NaiveStrpool n = { 0 };
    SlotStrview_create(&n.strings);
    return n;
}
void naivestrpool_clear(NaiveStrpool *n) {
    for (int i = 0; i < n->strings.count; ++i) {
        strview_t string = n->strings._items[i];
        printfd("Gonna delete "PRIstrw, PRIstrarg(string));
        free((void*)string.data);
    }
    SlotStrview_clear(&n->strings);
}
void naivestrpool_free(NaiveStrpool *n) {
    naivestrpool_clear(n);
    SlotStrview_free(&n->strings);
    *n = (NaiveStrpool) { 0 };
}
// @Returns item id, or -1 on error.
ID naivestrpool_append(NaiveStrpool *n, strview_t view) {
    char *copy = (char*)malloc((size_t)view.size);
    memcpy(copy, view.data, (size_t)(view.size));
    strview_t view_copy = { .data = copy, .size = view.size, };
    int id = SlotStrview_append(&n->strings, view_copy);
    if (id == -1) { free(copy); }
    return ID_make(id);
}
strview_t naivestrpool_get(NaiveStrpool *n, ID id) {
    strview_t *view = SlotStrview_get(&n->strings, IDget(id));
    return view ? *view : STRVIEW_INVALID;
}
int naivestrpool_remove(NaiveStrpool *n, ID id) {
    strview_t *view = SlotStrview_get(&n->strings, IDget(id));
    if (view) { free((void*)view->data); }
    return SlotStrview_pop(&n->strings, IDget(id));
}
/// ***** END OF [NAIVE STRING POOL]
/// ***** START OF [BOTH POOLS]
bool both_pools_append(Strpool *p, NaiveStrpool *n, strview_t view, ID *out_id1, ID *out_id2) {
    ID id_or_err1 = strpool_append(p, view);
    ID id_or_err2 = naivestrpool_append(n, view);
    if (out_id1) { *out_id1 = id_or_err1; }
    if (out_id2) { *out_id2 = id_or_err2; }
    return ID_valid(id_or_err1) && ID_valid(id_or_err2);
}
bool both_pools_remove(Strpool *p, NaiveStrpool *n, ID id1, ID id2) {
    int err = strpool_remove(p, id1);
    int err2 = naivestrpool_remove(n, id2);
    return err == 0 && err2 == 0;
}
bool both_pools_get(Strpool *p, NaiveStrpool *n, ID id1, ID id2, strview_t *out_view) {
    strview_t view1 = strpool_get(p, id1);
    strview_t view2 = naivestrpool_get(n, id2);
    if (out_view) { *out_view = view1; }
    return wstrview_equals(view1, view2);
}
/*void both_pools_clear(Strpool *p, NaiveStrpool *n) {*/
    /*strpool_clear*/
/*}*/
bool both_pools_compare_contents(Strpool *p, NaiveStrpool *n, Arena scratch) {
    if (p->views.count != n->strings.count) { printferr("Wrong number of strings"); return false; }
    VecStrview views_a = VecStrview_create_with_allocator(arena_allocator, &scratch);
    VecStrview views_b = VecStrview_create_with_allocator(arena_allocator, &scratch);
    for (int i = 0; i < p->views.count; ++i) {
        strpool__view poolview = p->views._items[i];
        strview_t view = strpool_get_from_view(p, poolview);
        VecStrview_append(&views_a, view);
    }
    for (int k = 0; k < n->strings.count; ++k) {
        strview_t view = n->strings._items[k];
        VecStrview_append(&views_b, view);
    }
    int found_n_views = 0;
    for (dyna_foreach_gnu(iter, views_a)) {
        strview_t view_a = *iter.ref;
        bool found = false;
        for (dyna_foreach_gnu(kter, views_b)) {
            strview_t view_b = *kter.ref;
            if (wstrview_equals(view_a, view_b)) {
                found = true;
                ++found_n_views;
                VecStrview_remove_at(&views_b, kter.index);
                break;
            }
        }
        if (!found) { printferr("Couldn't find string"); return false; }
    }
    if (found_n_views != p->views.count) { printferr("Couldn't find all views"); return false; }
    return true;
}
/// ***** END OF [BOTH POOLS]


#define STRPOOL__CHUNK ((int)sizeof(strpool__Node))


int strpool__get_free_node_mount(Strpool *p) {
    int count = 0;
    int i_node = p->i_first_free_node;
    for (;;) {
        strpool__Node *node = strpool__get_node(p, i_node);
        if (node == NULL) { break; }
        ++count;
        i_node = node->i_next_node;
    }
    return count;
}

void strpool_print_debug(Strpool *p) {
    // TODO: 2. Print pairs of index->strview_t.
    // TODO: 3. Print binary representation of the whole thing.
    // TODO: 1. Print chain of free nodes.

    strpool__Node *node;
    int i_node = p->i_first_free_node;

    printf("\n");
    printf("Pool capacity %d\n", p->capacity);
    printf(ANSI_GRE"Printing free nodes\n"ANSI_RESET);
    while (i_node >= 0) {
        node = &p->nodes[i_node];
        printf("chunks %-5d start %-5d end %-5d next %-5d\n", node->free_chunks, i_node, i_node + node->free_chunks, node->i_next_node);
        i_node = node->i_next_node;
    }

    printf(ANSI_GRE"Printing pairs (unordered)\n"ANSI_RESET);
    for (int i = 0; i < p->views.count; ++i) {
        ID userid = IDmake(p->views.itemid_to_userid[i]);
        strview_t mystr = strpool_get(p, userid);
        printf("userid %d itemid %d item (offset %d size %d (%d chunks)) {%"PRIstr"}\n",
                IDget(userid), i, p->views._items[i].offset, p->views._items[i].size, strpool__div_ceil(p->views._items[i].size, STRPOOL__CHUNK), PRIstrarg(mystr));
    }
    printf("\n");
}

TEST test_general(void) {
    Strpool pool = { 0 };
    int err = strpool_create(&pool);
    ASSERT(err == 0);
    strpool_print_debug(&pool);

    printvalnum(strpool__div_ceil(0, STRPOOL__CHUNK));

    ID view_id;
    strview_t result;

    result = strpool_get(&pool, ID_make(0));
    ASSERT(!wstrview_is_valid(result));

    view_id = strpool_append(&pool, cstr_SL("MIMOS"));
    ASSERT_INT_GTE(ID_get(view_id), 0);
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_empty1 = view_id;

    view_id = strpool_append(&pool, cstr_SL("Hello"));
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_hello = view_id;

    view_id = strpool_append(&pool, cstr_SL(""));
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_empty2 = view_id;

    view_id = strpool_append(&pool, cstr_SL("1234567890"));
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_numbers = view_id;

    view_id = strpool_append(&pool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    strpool_print_debug(&pool);
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_longtext = view_id;

    /*
    view_id = strpool_append(&pool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(*result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(*result));
    strpool_print_debug(&pool);
    */

    view_id = strpool_append(&pool, cstr_SL("Crazy? I was crazy once."));
    strpool_print_debug(&pool);
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_crazy = view_id;

    view_id = strpool_append(&pool, cstr_SL("They locked me in a room."));
    strpool_print_debug(&pool);
    ASSERT(ID_valid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    ID str_id_locked = view_id;

    /*strpool_print_debug(&pool);*/

    /*
    printfd("CLEARING EVERYTHING...");
    Stringpool_clear_preserving(&strpool);
    printfd("CONTINUING...");
    */

    /*strpool_print_debug(&pool);*/


    /*
    view_id = strpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = strpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_hello = view_id;

    view_id = strpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_empty = view_id;

    view_id = strpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_thisme = view_id;

    view_id = strpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_longtext = view_id;

    strpool_print_debug(&strpool);
    */

    // Testing the remove.

    ASSERT_INT(strpool__get_free_node_mount(&pool), 1);

    printf("\nRemoving id ");
    printvalnum(IDget(str_id_longtext));
    err = strpool_remove(&pool, str_id_longtext);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 2);
    str_id_longtext = ID_INVALID;

    printf("\nRemoving id ");
    printvalnum(IDget(str_id_hello));
    err = strpool_remove(&pool, str_id_hello);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_hello = ID_INVALID;

    printf("\nRemoving id ");
    printvalnum(IDget(str_id_empty2));
    err = strpool_remove(&pool, str_id_empty2);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_empty2 = ID_INVALID;

    printf("\nRemoving id ");
    printvalnum(IDget(str_id_empty1));
    err = strpool_remove(&pool, str_id_empty1);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_empty1 = ID_INVALID;

    printf("\nRemoving id ");
    printvalnum(IDget(str_id_crazy));
    err = strpool_remove(&pool, str_id_crazy);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_crazy = ID_INVALID;

    // Adding.

    view_id = strpool_append(&pool, cstr_SL("Hello"));
    ASSERT(IDvalid(view_id));
    result = strpool_get(&pool, view_id);
    ASSERT(wstrview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", IDget(view_id), PRIstrarg(result));
    strpool_print_debug(&pool);
    str_id_hello = view_id;

    // Remove all.
    err = strpool_remove(&pool, str_id_hello);
    ASSERT_INT(err, 0);
    err = strpool_remove(&pool, str_id_locked);
    ASSERT_INT(err, 0);
    err = strpool_remove(&pool, str_id_numbers);
    ASSERT_INT(err, 0);
    str_id_hello = ID_INVALID;
    str_id_locked = ID_INVALID;
    str_id_numbers = ID_INVALID;

    printfd("Removed all.");
    strpool_print_debug(&pool);

    ASSERT_INT_NEQ(pool.i_first_free_node, -1);
    ASSERT_INT(pool.nodes[0].i_next_node, -1);
    ASSERT_INT(pool.nodes[0].free_chunks, pool.capacity);
    ASSERT_INT(pool.nodes[0].free_chunks, 2048);

    strpool_destroy(&pool);
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
    enum { CYCLES = 50, };

    ArenaRoot arenaroot = ArenaRoot_create(1 << 20);
    Arena arena = ArenaRoot_get_arena(arenaroot);

    Strpool strpool;
    Strpool *p = &strpool;
    NaiveStrpool naivestrpool;
    NaiveStrpool *n = &naivestrpool;
    VecPair saved_ids;

    saved_ids = VecPair_create();
    naivestrpool = naivestrpool_create();
    int err = strpool_create(p);
    ASSERT(!err);

    // Insertion + get.

    for (int i = 0; i < CYCLES; ++i) {
        Arena scratch = arena;
        strview_t view = create_random_string(i * 8, &scratch);
        ID id1, id2;
        ASSERT(both_pools_append(p, n, view, &id1, &id2));
        ASSERT(both_pools_get(p, n, id1, id2, NULL));
        VecPair_append(&saved_ids, (Pair){id1, id2});
    }
    ASSERT(both_pools_compare_contents(p, n, arena));

    // Deletion.

    for (dyna_foreach_reverse(Pair, iter, saved_ids)) {
        ASSERT(both_pools_remove(p, n, iter.ref->first, iter.ref->second));
        VecPair_remove_at(&saved_ids, iter.index);
    }

    ASSERT_INT(saved_ids.size, 0);
    ASSERT_INT(p->views.count, 0);
    ASSERT_INT(n->strings.count, 0);
    ASSERT(both_pools_compare_contents(p, n, arena));

    // Insert again.

    for (int i = 0; i < CYCLES; ++i) {
        Arena scratch = arena;
        strview_t view = create_random_string(i * 8, &scratch);
        ID id1, id2;
        ASSERT(both_pools_append(p, n, view, &id1, &id2));
        ASSERT(both_pools_get(p, n, id1, id2, NULL));
        VecPair_append(&saved_ids, (Pair){id1, id2});
    }
    ASSERT(both_pools_compare_contents(p, n, arena));

    // Clear.

    strpool_clear(p);
    naivestrpool_clear(n);
    ASSERT(both_pools_compare_contents(p, n, arena));

    // Cleanup.

    VecPair_free(&saved_ids);
    naivestrpool_free(n);
    strpool_destroy(p);
    ArenaRoot_free(&arenaroot);
    TEST_PASS;
}


static Strpool static_strpool;
static NaiveStrpool static_naivestrpool;
static VecPair static_saved_ids;
static ArenaRoot static_arena_root;

void LLVMFuzzerCleanup(void) {
    strpool_destroy(&static_strpool);
    naivestrpool_free(&static_naivestrpool);
    VecPair_free(&static_saved_ids);
    ArenaRoot_free(&static_arena_root);
}
int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc,(void)argv; atexit(LLVMFuzzerCleanup); return 0;
}
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    static bool setup = false;
    static Strpool *p = &static_strpool;
    static NaiveStrpool *n = &static_naivestrpool;
    if (!setup) {
        setup = true;
        strpool_create(p);
        *n = naivestrpool_create();
        static_saved_ids = VecPair_create();
        static_arena_root = ArenaRoot_create(1 << 20);
    }

    Arena scratch = ArenaRoot_get_arena(static_arena_root);
    ArenaDy arenady = { .root = (char*)data, .beg = (char*)data, .end = (char*)data + size };
    const int *action = arenady_try_get_one(&arenady, int);
    if (!action) { return 0; }

    enum {
        APPEND,
        GET,
        REMOVE
    };

    bool touched = false;
    if (*action == APPEND) {
        int str_size = (int)(arenady.end - arenady.beg);
        if (str_size <= 0) { return 0; }
        char *str_data = (char*)arenady_try_get(&arenady, sizeof(char), alignof(char), str_size);
        wassert_live(str_data);
        ID id1, id2;
        strview_t view = {str_data, str_size};
        wassert_live(both_pools_append(p, n, view, &id1, &id2));
        strview_t out_view;
        wassert_live(both_pools_get(p, n, id1, id2, &out_view));
        wassert_live(wstrview_equals(view, out_view));
        VecPair_append(&static_saved_ids, (Pair){id1, id2});
        touched = true;
    }
    else if (*action == GET) {
        const ID *id1 = arenady_try_get_one(&arenady, ID);
        const ID *id2 = arenady_try_get_one(&arenady, ID);
        if (!id1 || !id2) { return 0; }
        both_pools_get(p, n, *id1, *id2, NULL);
    }
    else if (*action == REMOVE) {
        const int *id = arenady_try_get_one(&arenady, int);
        if (!id) { return 0; }
        if (static_saved_ids.size <= 0) { return 0; }
        int pair_id = true_modulo(*id, static_saved_ids.size);
        Pair pair = static_saved_ids.items[pair_id];
        wassert_live(both_pools_remove(p, n, pair.first, pair.second));
        VecPair_remove_at(&static_saved_ids, pair_id);
        touched = true;
    }
    if (touched) {
        wassert_live(both_pools_compare_contents(p, n, scratch));
    }

    return 0;
}


#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
int main (void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    RUN_TEST(test_auto);
    TESTS_SHOW_RESULTS();
}
#endif
