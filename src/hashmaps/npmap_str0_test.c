#include "stdio.h"
#include "../portable_utils.h"
#include "../woytest.h"
#include "string.h"

typedef struct {
    char name[20];
} Fruit;

#define MAP__TYPE Fruit
#include "npmap_str0.h"


#define str strpool__str
#define PRIstr ".*s"
#define PRIstrarg(arg) ((arg).size),((arg).data)
#define STRVIEW_INVALID ((strpool__str){.data = NULL, .size = 0})
#define cstr_SL(sl_arg) ((str){.data=(sl_arg), .size=sizeof(sl_arg)-1})
strpool__str cstr(const char* c_str) {
    return c_str ? (strpool__str) { .data = c_str, .size = (int)strlen(c_str) } : STRVIEW_INVALID;
}


void print_pairs(const Map *m) {
    printfd("\nMap pairs %d, max capacity %d", m->size, (1 << SIZE_EXP) -1);
    printfd("Printing pairs:");
    for (int i = 0; i < (1 << SIZE_EXP); ++i) {
        Node * const * node = &m->nodes[i];
        if (*node == NULL) { continue; }
        printfd("%d = %s", (*node)->key, (*node)->value.name);
    }
}

bool items_are_equal(Fruit a, Fruit b) {
    bool res = str_equals(cstr(a.name), cstr(b.name));
    /*printfd("COMPARING %s with %s (%s)", a.name, b.name, PRIbool(res));*/
    return res;
}

typedef struct Item2Find {
    MAP__TYPE item;
    bool found;
} Item2Find;


bool should_find_these(const Map *m, MAP__TYPE *p_items, const int item_amount) {

    int valid_items_found_count = 0;
    Item2Find items[item_amount];

    for (int i = 0; i < item_amount; ++i) {
        items[i] = (Item2Find) { 0 };
        items[i].found = false;
        items[i].item = p_items[i];
    }

    for (int i = 0; i < (1 << SIZE_EXP); ++i) {
        Node * const * node = &m->nodes[i];
        if (*node == NULL) { continue; }
        ++valid_items_found_count;

        // See if it corresponds to one item.
        for (int k = 0; k < item_amount; ++k) {
            Item2Find *item = &items[k];
            if (items_are_equal(item->item, (*node)->value)) {
                item->found = true;
            }
        }
    }

    // All items must have been found.

    for (int k = 0; k < item_amount; ++k) {
        Item2Find *item = &items[k];
        if (!item->found) {
            printfd("WAR: Didn't find item %d", k);
            return false;
        }
    }

    if (item_amount != valid_items_found_count) {
        printfd("WAR: Expected to find %d items but found %d", item_amount, valid_items_found_count);
        return false;
    }
    return true; // success;
}

#define quickfruit(name) ((Fruit) { name })

TEST test_general(void) {
    Map map = create();
    Map *m = &map;
    int err;

    print_pairs(m);

    str key;
    Fruit fruit;
    Fruit *result;

    key = cstr_SL("APPLE");
    fruit = (Fruit) { "Appless" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    ASSERT_INT(m->size, 1);
    /*ASSERT(str_equals(cstr(m->items[0].value.name), cstr(fruit.name)));*/
    /* ↑↑↑ Test internal implementation to make sure the FIRST pair
           is filled. Note it is NOT a feature of this map to be able
           to iterate through the items. */

    {
        Fruit to_find[] = { quickfruit("Appless") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("APPLE");
    fruit = (Fruit) { "Apple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    ASSERT_INT(m->size, 1);
    str key_apple = key;
    {
        Fruit to_find[] = { quickfruit("Apple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("BANANA");
    fruit = (Fruit) { "Banana" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    {
        Fruit to_find[] = { quickfruit("Apple"), quickfruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("PEAR");
    fruit = (Fruit) { "Pear" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    str key_pear = key;
    {
        Fruit to_find[] = { quickfruit("Apple"), quickfruit("Banana"), quickfruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // At this point it should be full. (1 << SIZE_EXP) -1.

    key = cstr_SL("PINEAPPLE");
    fruit = (Fruit) { "Pineapple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, -1);
    print_pairs(m);

    // Try to gain some space back.

    err = map_remove(m, key_apple);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->size, (1 << SIZE_EXP) -2);
    print_pairs(m);
    {
        Fruit to_find[] = { quickfruit("Banana"), quickfruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    err = map_remove(m, key_apple);
    ASSERT_INT(err, -1);

    err = map_remove(m, key_pear);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->size, (1 << SIZE_EXP) -3);

    printfd("\n---->>> Removed APPLE and Pear, should remain Banana.");
    print_pairs(m);
    {
        Fruit to_find[] = { quickfruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Insert now that there is space available.

    key = cstr_SL("PINEAPPLE");
    fruit = (Fruit) { "Pineapple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    {
        Fruit to_find[] = { quickfruit("Banana"), quickfruit("Pineapple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }


    key = cstr_SL("KIWI");
    fruit = (Fruit) { "Kiwi" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = get(m, key); ASSERT(result != NULL); ASSERT(str_equals(cstr(result->name), cstr(fruit.name)));
    {
        Fruit to_find[] = { quickfruit("Banana"), quickfruit("Pineapple"), quickfruit("Kiwi") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    ASSERT_FALSE(str_equals(cstr_SL("banana"), cstr_SL("BANANA")));
    ASSERT_FALSE(str_equals(cstr_SL("banana"), cstr_SL("banana\0"))); 
    ASSERT_FALSE(str_equals(cstr_SL("banana\0"), cstr("banana\0")));
    ASSERT_TRUE(str_equals(cstr("banana"), cstr("banana\0\0\0\0")));
    ASSERT_FALSE(str_equals(cstr_SL("banana"), cstr_SL("banana\0\0\0\0")));
    ASSERT_TRUE(str_equals(cstr("banana"), cstr("banana\0")));
    // ↑↑↑ True because cstr calls strlen. But cstr_SL calls sizeof.

    map_free(m);

    TEST_PASS;
}

int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


