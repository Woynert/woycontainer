#include "portable_utils.h"
#include "stringpool.h"

void print_pairs_and_views(Stringpool *strpool) {
    printfd("\nAlright gonna print all now.");
    for (int i = 0; i < strpool->pairs.size; ++i) {
        Stringpool__Pair pair = strpool->pairs.items[i];
        strview_t result = (strview_t) { .data = strpool->buffer->cstr + pair.start, .size = pair.size };
        ASSERT(strview_is_valid(result));
        printfd("%d (%d) [%"PRIstr"]", i, i + strpool->id_base, PRIstrarg(result));
    }
    printfd("FULL BUFFER (size %d)[%"PRIstr"]", strpool->buffer->capacity, PRIstrarg(strbuf_view(&strpool->buffer)));
    printf("\n");
}

void test1(void) {
    Stringpool strpool = { 0 };
    Stringpool_create(&strpool);

    int view_id;
    strview_t result;

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    print_pairs_and_views(&strpool);

    view_id = Stringpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    print_pairs_and_views(&strpool);

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    print_pairs_and_views(&strpool);

    view_id = Stringpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    print_pairs_and_views(&strpool);

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    print_pairs_and_views(&strpool);

    view_id = Stringpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    print_pairs_and_views(&strpool);

    printfd("CLEARING EVERYTHING...");
    Stringpool_clear_preserving(&strpool);
    printfd("CONTINUING...");

    print_pairs_and_views(&strpool);


    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_hello = view_id;

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_empty = view_id;

    view_id = Stringpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_thisme = view_id;

    view_id = Stringpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));
    int str_id_longtext = view_id;

    print_pairs_and_views(&strpool);

    // Testing the remove
    int err;

    printfd("\nRemoving id %d...", str_id_longtext);
    err = Stringpool_remove(&strpool, str_id_longtext);
    assert(err == 0);
    print_pairs_and_views(&strpool);

    printfd("\nRemoving id %d...", str_id_hello);
    err = Stringpool_remove(&strpool, str_id_hello);
    assert(err == 0);
    print_pairs_and_views(&strpool);

    printfd("\nRemoving id %d...", str_id_empty);
    err = Stringpool_remove(&strpool, str_id_empty);
    assert(err == 0);
    print_pairs_and_views(&strpool);

    printfd("\nRemoving id %d...", str_id_thisme);
    err = Stringpool_remove(&strpool, str_id_thisme);
    assert(err == 0);
    print_pairs_and_views(&strpool);


    Stringpool_destroy(&strpool);
    exit(0);
}

int main (void) {
	test1();
}
