#include "stdio.h"
#include "../portable_utils.h"
#include "../woytest.h"
#include "string.h"

typedef struct {
    char name[20];
} Fruit;

#define MAP__TYPE Fruit
#include "npmap_str2.h"


/* CONVENIENCE MACROS. */

#define MAP__TYPE Fruit
#define MAP__TOKCAT_(a, b) a ## b
#define MAP__TOKCAT(a, b) MAP__TOKCAT_(a, b)
#ifndef MAP__NAMESPACE
#define MAP__NAMESPACE MAP__TOKCAT(MAP__TYPE, _strmap)
#endif
#define pub(name) MAP__TOKCAT(MAP__TOKCAT(MAP__NAMESPACE, _), name)
#define pri(name) MAP__TOKCAT(MAP__TOKCAT(MAP__NAMESPACE, __), name)


#define Map MAP__NAMESPACE
#define Str wstrview_t
#define makefruit(name) ((Fruit) { name })



void print_pairs(const Map *m) {
    printfd("\nMap pairs %d, max capacity %d", m->count, (1 << m->size_exp) -1);
    printfd("Printing pairs:");
    for (int i = 0; i < (1 << m->size_exp); ++i) {
        pri(Node) * node = m->hashmap[i];
        if (pri(node_is_empty)(m, node)) { continue; }
        wstrview_t stored_key = strpool_get(&m->strpool, node->key);
        printfd("(i %-4d)(strkey %-4d): %"PRIwstr" = %s", i, node->key, PRIwstrarg(stored_key), node->value.name);
    }
}


bool items_are_equal(Fruit a, Fruit b) {
    bool res = wstrview_equals(wcstr(a.name), wcstr(b.name));
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

    for (int i = 0; i < (1 << m->size_exp); ++i) {
        pri(Node) *node = m->hashmap[i];
        if (pri(node_is_empty)(m, node)) { continue; }
        ++valid_items_found_count;

        // See if it corresponds to one item.
        for (int k = 0; k < item_amount; ++k) {
            Item2Find *item = &items[k];
            if (items_are_equal(item->item, node->value)) {
                item->found = true;
            }
        }
    }

    // All items must have been found.

    for (int k = 0; k < item_amount; ++k) {
        Item2Find *item = &items[k];
        if (!item->found) {
            printfd("WAR: Didn't find item %d (%s)", k, item->item.name);
            return false;
        }
    }

    if (item_amount != valid_items_found_count) {
        printfd("WAR: Expected to find %d items but found %d", item_amount, valid_items_found_count);
        return false;
    }
    return true; // success;
}


TEST test_str(void) {
    ASSERT_FALSE(wstrview_equals(wcstr_SL("banana"), wcstr_SL("BANANA")));
    ASSERT_FALSE(wstrview_equals(wcstr_SL("banana"), wcstr_SL("banana\0"))); 
    ASSERT_FALSE(wstrview_equals(wcstr_SL("banana\0"), wcstr("banana\0")));
    ASSERT_TRUE(wstrview_equals(wcstr("banana"), wcstr("banana\0\0\0\0")));
    ASSERT_FALSE(wstrview_equals(wcstr_SL("banana"), wcstr_SL("banana\0\0\0\0")));
    ASSERT_TRUE(wstrview_equals(wcstr("banana"), wcstr("banana\0")));
    // ↑↑↑ True because wcstr calls strlen. But wcstr_SL calls sizeof.
    TEST_PASS;
}


TEST test_general(void) {
    int err;
    Map map = { 0 };
    Map *m = &map;
    err = pub(create)(m);
    ASSERT_INT(err, 0);

    print_pairs(m);

    Str key;
    Fruit fruit;
    Fruit *result;

    key = wcstr_SL("APPLE");
    fruit = (Fruit) { "Appless" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    ASSERT_INT(m->count, 1);
    /*ASSERT(wstrview_equals(wcstr(m->items[0].value.name), wcstr(fruit.name)));*/
    /* ↑↑↑ Test internal implementation to make sure the FIRST pair
           is filled. Note it is NOT a feature of this map to be able
           to iterate through the items. */

    {
        Fruit to_find[] = { makefruit("Appless") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = wcstr_SL("APPLE");
    fruit = (Fruit) { "Apple" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    ASSERT_INT(m->count, 1);
    Str key_apple = key;
    {
        Fruit to_find[] = { makefruit("Apple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = wcstr_SL("BANANA");
    fruit = (Fruit) { "Banana" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Apple"), makefruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = wcstr_SL("PEAR");
    fruit = (Fruit) { "Pear" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    Str key_pear = key;
    {
        Fruit to_find[] = { makefruit("Apple"), makefruit("Banana"), makefruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Try to gain some space back.

    err = pub(remove)(m, key_apple);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->count, 2);
    print_pairs(m);
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    err = pub(remove)(m, key_apple);
    ASSERT_INT(err, -1);

    err = pub(remove)(m, key_pear);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->count, 1);
    
    printfd("\n---->>> Removed APPLE and Pear, should remain Banana.");
    print_pairs(m);
    {
        Fruit to_find[] = { makefruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Insert now that there is space available.

    key = wcstr_SL("PINEAPPLE");
    fruit = (Fruit) { "Pineapple" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    printfd("\n---->>> Reinserting APPLE.");

    key = wcstr_SL("APPLE");
    fruit = (Fruit) { "Apple" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    result = pub(get)(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple"), makefruit("Apple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = wcstr_SL("MANGO");
    fruit = (Fruit) { "Mango" };
    err = pub(upsert)(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple"), makefruit("Apple"), makefruit("Mango") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Insert 100 things to see what's up.

    int prev_count = m->count;

    for (int i = 0; i < 100; ++i) {
        char key_str[100] = { 0 };
        fruit = (Fruit) { 0 };
        sprintf(fruit.name, "fruit%d", i);
        sprintf(key_str, "FRUIT%d", i);
        err = pub(upsert)(m, wcstr(key_str), fruit);
        ASSERT_INT(err, 0);
    }

    print_pairs(m);
    ASSERT_INT(m->count, prev_count + 100);

    // Remove 100 pairs.

    for (int i = 0; i < (1 << m->size_exp); ++i) {
        pri(Node) * node = m->hashmap[i];
        if (node == NULL) { continue; }
        wstrview_t stored_key = strpool_get(&m->strpool, node->key);
        err = pub(remove)(m, stored_key);
        if (err != 0) {
            printfd(ANSI_RED"Failed to remove: %-4d: %"PRIwstr" = %s", node->key, PRIwstrarg(stored_key), node->value.name);
        } else {
            printfd(ANSI_GRE"Removed: %-4d: %"PRIwstr" = %s", node->key, PRIwstrarg(stored_key), node->value.name);
        }
        ASSERT_INT(err, 0);
    }

    print_pairs(m);
    ASSERT_INT(m->count, 0);

    pub(free)(m);

    TEST_PASS;
}


int main(void) {
    TESTS_INIT();
    RUN_TEST(test_str);
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}
