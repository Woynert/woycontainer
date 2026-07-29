
#include "stdio.h"
#include "arena.h"
#include "woytest.h"
#include "wstrview.h"


typedef struct {
    char name[20];
} Fruit;
#define makefruit(name) ((Fruit) { name })

#define LIST__TYPE Fruit
#define LIST__NAMESPACE List_Fruit
#include "list_simple.h"


typedef struct Item2Find {
    Fruit item;
    bool found;
} Item2Find;


bool items_are_equal(Fruit a, Fruit b) {
    bool res = wstrview_equals(wcstr(a.name), wcstr(b.name));
    /*printfd("COMPARING %s with %s (%s)", a.name, b.name, PRIbool(res));*/
    return res;
}

bool should_find_these(const List_Fruit *l, Fruit *p_items, const int item_amount) {

    int valid_items_found_count = 0;
    Item2Find items[item_amount];

    for (int i = 0; i < item_amount; ++i) {
        items[i] = (Item2Find) { 0 };
        items[i].found = false;
        items[i].item = p_items[i];
    }

    for (List_Fruit__Node *node = l->root; node != NULL; node = node->next) {
        ++valid_items_found_count;
        // See if it corresponds to one item.
        for (int k = 0; k < item_amount; ++k) {
            Item2Find *item = &items[k];
            if (items_are_equal(item->item, node->item)) {
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


TEST test_general(void) {

    Fruit fruit = { 0 };
    List_Fruit list = List_Fruit_create();
    List_Fruit *l = &list;

    fruit = makefruit("Apple");
    List_Fruit_append(l, fruit);
    {   Fruit to_find[] = { makefruit("Apple") };
        ASSERT(should_find_these(l, to_find, countof(to_find))); }

    fruit = makefruit("Banana");
    List_Fruit_append(l, fruit);
    {   Fruit to_find[] = { makefruit("Apple"), makefruit("Banana") };
        ASSERT(should_find_these(l, to_find, countof(to_find))); }

    fruit = makefruit("Kiwi");
    List_Fruit_append(l, fruit);
    {   Fruit to_find[] = { makefruit("Apple"), makefruit("Banana"), makefruit("Kiwi") };
        ASSERT(should_find_these(l, to_find, countof(to_find))); }

    for (int i = 0; i < (1 << 10); ++i) {
        char key_str[100] = { 0 };
        fruit = (Fruit) { 0 };
        sprintf(fruit.name, "fruit%d", i);
        sprintf(key_str, "FRUIT%d", i);
        int err = List_Fruit_append(l, fruit);
        ASSERT_INT(err, 0);
    }

    // TODO: An easy way to iterate through it.

    int count = 0;
    for (List_Fruit__Node *node = l->root; node != NULL; node = node->next) {
        /*printfd("List has item [%s]", node->item.name);*/
        ++count;
    }
    ASSERT_INT(count, 3 + (1 << 10));

    count = 0;
    List_Fruit_It it = { 0 };
    while(List_Fruit_it_next(l, &it)) {
        printfd(ANSI_BLU"List has item [%s]", it.item->name);
        ++count;
    }
    ASSERT_INT(count, 3 + (1 << 10));

    List_Fruit_free(l);

    TEST_PASS;
}

int main(void) {

    TESTS_INIT();

    RUN_TEST(test_general);

    TESTS_SHOW_RESULTS();
}
