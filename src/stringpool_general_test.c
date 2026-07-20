
#include "woytest.h"
#include "portable_utils.h"
#include "stringpool_general.h"

/*
void strpool_print_debug(strpool *p) {
    printfd("\nAlright gonna print all now.");
    for (int i = 0; i < p->pairs.size; ++i) {
        Stringpool__Pair pair = strpool->pairs.items[i];
        str result = (str) { .data = strpool->buffer->cstr + pair.start, .size = pair.size };
        ASSERT(strview_is_valid(result));
        printfd("%d (%d) [%"PRIstr"]", i, i + strpool->id_base, PRIstrarg(result));
    }
    printfd("FULL BUFFER (size %d)[%"PRIstr"]", strpool->buffer->capacity, PRIstrarg(strbuf_view(&strpool->buffer)));
    printf("\n");
}
*/

#define str strpool__str
#define cstr_SL(sl_arg) ((str){.data=(sl_arg), .size=sizeof(sl_arg)-1})
#define PRIstr ".*s"
#define PRIstrarg(arg) ((arg).size),((arg).data)

bool strview_is_valid(str mystr) {
    return mystr.data != NULL;
}

int strpool__get_free_node_mount(strpool *p) {
    int count = 0;
    int i_node = p->i_first_free_node;
    for (;;) {
        FreeNode *node = strpool__get_node(p, i_node);
        if (node == NULL) { break; }
        ++count;
        i_node = node->i_next_node;
    }
    return count;
}

void strpool_print_debug(strpool *p) {
    // TODO: 2. Print pairs of index->str.
    // TODO: 3. Print binary representation of the whole thing.
    // TODO: 1. Print chain of free nodes.

    
    FreeNode *node;
    int i_node = p->i_first_free_node;

    printf("\n");
    printf("Pool capacity %d\n", p->capacity);
    printf(ANSI_RED"Printing free nodes\n"ANSI_RESET);
    while (i_node >= 0) {
        node = &p->nodes[i_node];
        printf("start %5d end %5d chunks %5d next %5d\n", i_node, i_node + node->free_chunks, node->free_chunks, node->i_next_node);
        i_node = node->i_next_node;
    }

    printf(ANSI_RED"Printing pairs (unordered)\n"ANSI_RESET);
    for (int i = 0; i < p->views.count; ++i) {
        int userid = p->views.itemid_to_userid[i];
        str mystr = strpool_get(p, userid);
        printf("userid %d itemid %d item (offset %d size %d (%d chunks)) {%"PRIstr"}\n",
                userid, i, p->views.items[i].offset, p->views.items[i].size, strpool__div_ceil(p->views.items[i].size, STRPOOL_CHUNK), PRIstrarg(mystr));
    }
    printf("\n");
}

TEST test1(void) {
    strpool pool = { 0 };
    int err = strpool_create(&pool);
    ASSERT(err == 0);
    strpool_print_debug(&pool);

    printvalnum(strpool__div_ceil(0, STRPOOL_CHUNK));

    int view_id;
    str result;

    view_id = strpool_append(&pool, cstr_SL(""));
    ASSERT_INT_GTE(view_id, 0);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);
    int str_id_empty1 = view_id;

    view_id = strpool_append(&pool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);
    int str_id_hello = view_id;

    view_id = strpool_append(&pool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);
    int str_id_empty2 = view_id;

    view_id = strpool_append(&pool, cstr_SL("1234567890"));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);

    view_id = strpool_append(&pool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);
    int str_id_longtext = view_id;

    /*
    view_id = strpool_append(&pool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(*result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(*result));
    strpool_print_debug(&pool);
    */

    view_id = strpool_append(&pool, cstr_SL("Crazy? I was crazy once."));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);
    int str_id_crazy = view_id;

    view_id = strpool_append(&pool, cstr_SL("They locked me in a room."));
    strpool_print_debug(&pool);
    ASSERT(view_id != -1);
    result = strpool_get(&pool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    strpool_print_debug(&pool);

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
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = strpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_hello = view_id;

    view_id = strpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_empty = view_id;

    view_id = strpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_thisme = view_id;

    view_id = strpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = strpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_longtext = view_id;

    strpool_print_debug(&strpool);
    */

    // Testing the remove.

    ASSERT_INT(strpool__get_free_node_mount(&pool), 2);

    printf("\nRemoving id ");
    printvalnum(str_id_longtext);
    err = strpool_remove(&pool, str_id_longtext);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 2);

    printf("\nRemoving id ");
    printvalnum(str_id_hello);
    err = strpool_remove(&pool, str_id_hello);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);

    printf("\nRemoving id ");
    printvalnum(str_id_empty2);
    err = strpool_remove(&pool, str_id_empty2);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);

    printf("\nRemoving id ");
    printvalnum(str_id_empty1);
    err = strpool_remove(&pool, str_id_empty1);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);

    printf("\nRemoving id ");
    printvalnum(str_id_crazy);
    err = strpool_remove(&pool, str_id_crazy);
    ASSERT_INT(err, 0);
    strpool_print_debug(&pool);
    ASSERT_INT(strpool__get_free_node_mount(&pool), 3);

    strpool_destroy(&pool);
    exit(0);
    TEST_PASS;
}

int main (void) {
    TESTS_INIT();
    RUN_TEST(test1);
    TESTS_SHOW_RESULTS();
}

