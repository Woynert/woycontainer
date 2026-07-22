#include "stdio.h"
#include "../portable_utils.h"
#include "../woytest.h"

typedef struct {
    char name[20];
} Fruit;

#define MAP__TYPE Fruit
#include "npmap0.h"


void print_pairs(const Map *m) {
    printfd("\nMap pairs %d, max capacity %d", m->pair_count, (1 << SIZE_EXP) -1);
    printfd("Printing pairs:");
    for (int i = 0; i < (1 << SIZE_EXP); ++i) {
        const Node *node = &m->items[i];
        if (!node->used) { continue; }
        printfd("%d = %s", node->key, node->value.name);
    }
}


TEST test_general(void) {
    Map map = create();
    Map *m = &map;
    int err;

    print_pairs(m);

    int key;
    Fruit fruit;

    key = 10;
    fruit = (Fruit) { "Appless" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);

    key = 10;
    fruit = (Fruit) { "Apple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);

    key = 20;
    fruit = (Fruit) { "Banana" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    int key_banana = key;

    key = 101;
    fruit = (Fruit) { "Pear" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);
    int key_pear = key;

    // At this point it should be full. (1 << SIZE_EXP) -1.

    key = 102;
    fruit = (Fruit) { "Pineapple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, -1);
    print_pairs(m);

    // Try to gain some space back.

    err = map_remove(m, key_banana);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, (1 << SIZE_EXP) -2);
    print_pairs(m);

    err = map_remove(m, key_banana);
    ASSERT_INT(err, -1);

    err = map_remove(m, key_pear);
    ASSERT_INT(err, 0);
    ASSERT_INT(m->pair_count, (1 << SIZE_EXP) -3);
    print_pairs(m);

    // Insert now that there is space available.

    key = 102;
    fruit = (Fruit) { "Pineapple" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);


    key = 103;
    fruit = (Fruit) { "Kiwi" };
    err = upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    print_pairs(m);


    TEST_PASS;
}

int main(void) {
    /*int a =sizeof(Node);*/
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


