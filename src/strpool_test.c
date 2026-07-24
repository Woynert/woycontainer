
#include "woytest.h"
#include "portable_utils.h"
#include "strpool.h"


#define Str wstrview_t
#define STRPOOL__CHUNK ((int)sizeof(strpool__Node))


bool strview_is_valid(Str mystr) {
    return mystr.data != NULL;
}

int strpool__get_free_node_mount(strpool *p) {
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

void strpool_print_debug(strpool *p) {
    // TODO: 2. Print pairs of index->Str.
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
        int userid = p->views.itemid_to_userid[i];
        Str mystr = strpool_get(p, userid);
        printf("userid %d itemid %d item (offset %d size %d (%d chunks)) {%"PRIwstr"}\n",
                userid, i, p->views.items[i].offset, p->views.items[i].size, strpool__div_ceil(p->views.items[i].size, STRPOOL__CHUNK), PRIwstrarg(mystr));
    }
    printf("\n");
}

TEST test_general(void) {
    strpool pool = { 0 };
    int err = strpool_create(&pool);
    ASSERT(err == 0);
    strpool_print_debug(&pool);

    printvalnum(strpool__div_ceil(0, STRPOOL__CHUNK));

    int view_id;
    Str result;

    view_id = strpool_append(&pool, wcstr_SL(""));
    ASSERT_INT_GTE(view_id, 0);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_empty1 = view_id;

    view_id = strpool_append(&pool, wcstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_hello = view_id;

    view_id = strpool_append(&pool, wcstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_empty2 = view_id;

    view_id = strpool_append(&pool, wcstr_SL("1234567890"));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_numbers = view_id;

    view_id = strpool_append(&pool, wcstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_longtext = view_id;

    /*
    view_id = strpool_append(&pool, wcstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(*result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(*result));
    strpool_print_debug(&pool);
    */

    view_id = strpool_append(&pool, wcstr_SL("Crazy? I was crazy once."));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_crazy = view_id;

    view_id = strpool_append(&pool, wcstr_SL("They locked me in a room."));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    int str_id_locked = view_id;

    /*strpool_print_debug(&pool);*/

    /*
    printfd("CLEARING EVERYTHING...");
    Stringpool_clear_preserving(&strpool);
    printfd("CONTINUING...");
    */

    /*strpool_print_debug(&pool);*/


    /*
    view_id = strpool_append(&strpool, wcstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));

    view_id = strpool_append(&strpool, wcstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    int str_id_hello = view_id;

    view_id = strpool_append(&strpool, wcstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    int str_id_empty = view_id;

    view_id = strpool_append(&strpool, wcstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    int str_id_thisme = view_id;

    view_id = strpool_append(&strpool, wcstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    int str_id_longtext = view_id;

    strpool_print_debug(&strpool);
    */

    // Testing the remove.

    ASSERT_INT(strpool__get_free_node_mount(&pool), 1);

    printf("\nRemoving id ");
    printvalnum(str_id_longtext);
    err = strpool_remove(&pool, str_id_longtext);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 2);
    str_id_longtext = -1;

    printf("\nRemoving id ");
    printvalnum(str_id_hello);
    err = strpool_remove(&pool, str_id_hello);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_hello = -1;

    printf("\nRemoving id ");
    printvalnum(str_id_empty2);
    err = strpool_remove(&pool, str_id_empty2);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_empty2 = -1;

    printf("\nRemoving id ");
    printvalnum(str_id_empty1);
    err = strpool_remove(&pool, str_id_empty1);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_empty1 = -1;

    printf("\nRemoving id ");
    printvalnum(str_id_crazy);
    err = strpool_remove(&pool, str_id_crazy);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);
    str_id_crazy = -1;

    // Adding.

    view_id = strpool_append(&pool, wcstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIwstr"]", view_id, PRIwstrarg(result));
    strpool_print_debug(&pool);
    str_id_hello = view_id;

    // Remove all.
    err = strpool_remove(&pool, str_id_hello);
    ASSERT_INT(err, 0);
    err = strpool_remove(&pool, str_id_locked);
    ASSERT_INT(err, 0);
    err = strpool_remove(&pool, str_id_numbers);
    ASSERT_INT(err, 0);
    str_id_hello = -1;
    str_id_locked = -1;
    str_id_numbers = -1;

    printfd("Removed all.");
    strpool_print_debug(&pool);

    ASSERT_INT_NEQ(pool.i_first_free_node, -1);
    ASSERT_INT(pool.nodes[0].i_next_node, -1);
    ASSERT_INT(pool.nodes[0].free_chunks, pool.capacity);
    ASSERT_INT(pool.nodes[0].free_chunks, 2048);

    strpool_destroy(&pool);
    TEST_PASS;
}

int main (void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}

