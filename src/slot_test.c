#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"

typedef struct {
    char name[10];
} Fruit;

#define SLOT__TYPE Fruit
#include "slot.h"


void print_slot(const Fruit_Slot *s) {
    printf("\ncap %d count %d", s->capacity, s->count);
    printf("\nuserid_to_itemid ");
    for (int i = 0; i < s->capacity; ++i) {
        printf("%04d ", s->userid_to_itemid[i]);
    }
    printf("\nitemid_to_userid ");
    for (int i = 0; i < s->capacity; ++i) {
        printf("%04d ", s->itemid_to_userid[i]);
    }
    printf("\nfruits           ");
    for (int i = 0; i < s->count; ++i) {
        printf("%.4s ", s->items[i].name);
    }
    printf("\n");
}


TEST test_general(void) {

    Fruit fruit;

    Fruit_Slot s = { 0 };
    int err = Fruit_Slot_create(&s);
    ASSERT(err == 0);
    print_slot(&s);

    // Appending.

    fruit = (Fruit) { "Apple" };
    int apple_id = Fruit_Slot_append(&s, fruit);
    ASSERT(apple_id >= 0);
    print_slot(&s);
    ASSERT(s.count == 1);

    fruit = (Fruit) { "Orange" };
    int orange_id = Fruit_Slot_append(&s, fruit);
    ASSERT(orange_id >= 0);
    print_slot(&s);
    ASSERT(s.count == 2);

    fruit = (Fruit) { "Kiwi" };
    int kiwi_id = Fruit_Slot_append(&s, fruit);
    ASSERT(kiwi_id >= 0);
    print_slot(&s);
    ASSERT(s.count == 3);

    fruit = (Fruit) { "Coco" };
    int coco_id = Fruit_Slot_append(&s, fruit);
    ASSERT(coco_id >= 0);
    print_slot(&s);
    ASSERT(s.count == 4);

    fruit = (Fruit) { "Frog" };
    int frog_id = Fruit_Slot_append(&s, fruit);
    ASSERT(frog_id >= 0);
    print_slot(&s);
    ASSERT(s.count == 5);

    // Removing.

    printf("\n\nDeleting apple_id %d\n", apple_id);
    ASSERT_INT(Fruit_Slot_pop(&s, apple_id), 0);
    print_slot(&s);
    ASSERT_INT(s.count, 4);
    apple_id = -1;

    printf("\n\nDeleting kiwi_id %d\n", kiwi_id);
    ASSERT_INT(Fruit_Slot_pop(&s, kiwi_id), 0);
    print_slot(&s);
    ASSERT_INT(s.count, 3);
    kiwi_id = -1;

    fruit = (Fruit) { "Banana" };
    int banana_id = Fruit_Slot_append(&s, fruit);
    ASSERT(banana_id >= 0);
    print_slot(&s);
    ASSERT_INT(s.count, 4);

    fruit = (Fruit) { "Guanabana" };
    int guanabana_id = Fruit_Slot_append(&s, fruit);
    ASSERT(guanabana_id >= 0);
    print_slot(&s);
    ASSERT_INT(s.count, 5);

    fruit = (Fruit) { "Mandarina" };
    int mandarina_id = Fruit_Slot_append(&s, fruit);
    ASSERT(mandarina_id >= 0);
    print_slot(&s);
    ASSERT_INT(s.count, 6);

    printf("\n\nDeleting orange_id %d\n", orange_id);
    ASSERT_INT(Fruit_Slot_pop(&s, orange_id), 0);
    print_slot(&s);
    ASSERT_INT(s.count, 5);
    orange_id = -1;

    printf("\nEND\n");

    // Reading back.

    Fruit *f = NULL;

    f = Fruit_Slot_get(&s, banana_id);
    ASSERT(f != NULL);
    printvalnum(banana_id);
    printf("[[[%s]]]\n", f->name);
    ASSERT(strcmp("Banana", f->name) == 0);

    f = Fruit_Slot_get(&s, guanabana_id);
    ASSERT(f != NULL);
    printvalnum(guanabana_id);
    ASSERT(strcmp("Guanabana", f->name) == 0);

    f = Fruit_Slot_get(&s, mandarina_id);
    ASSERT(f != NULL);
    printvalnum(mandarina_id);
    ASSERT(strcmp("Mandarina", f->name) == 0);

    f = Fruit_Slot_get(&s, coco_id);
    ASSERT(f != NULL);
    printvalnum(coco_id);
    ASSERT(strcmp("Coco", f->name) == 0);

    f = Fruit_Slot_get(&s, frog_id);
    ASSERT(f != NULL);
    printvalnum(frog_id);
    ASSERT(strcmp("Frog", f->name) == 0);

    // Removing again.

    printf("Removing "); printvalnum(mandarina_id);
    err = Fruit_Slot_pop(&s, mandarina_id);
    ASSERT_INT(err, 0);
    printf("Removing "); printvalnum(apple_id);
    err = Fruit_Slot_pop(&s, apple_id);
    ASSERT_INT_NEQ(err, 0);
    printf("Removing "); printvalnum(guanabana_id);
    err = Fruit_Slot_pop(&s, guanabana_id);
    ASSERT_INT(err, 0);
    printf("Removing "); printvalnum(orange_id);
    err = Fruit_Slot_pop(&s, orange_id);
    ASSERT_INT_NEQ(err, 0);
    printf("Removing "); printvalnum(banana_id);
    err = Fruit_Slot_pop(&s, banana_id);
    ASSERT_INT(err, 0);
    printf("Removing "); printvalnum(kiwi_id);
    err = Fruit_Slot_pop(&s, kiwi_id);
    ASSERT_INT_NEQ(err, 0);
    printf("Removing "); printvalnum(frog_id);
    err = Fruit_Slot_pop(&s, frog_id);
    ASSERT_INT(err, 0);
    printf("Removing "); printvalnum(coco_id);
    err = Fruit_Slot_pop(&s, coco_id);
    ASSERT_INT(err, 0);
    ASSERT_INT(s.count, 0);

    Fruit_Slot_free(&s);

    TEST_PASS;
}

int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


