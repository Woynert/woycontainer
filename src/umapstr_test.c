#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"


typedef struct Age {
    int years;
} Age;


#define UMAPSTR__TYPE Age
#define UMAPSTR__NAMESPACE name2age_map
#include "umapstr.h"


#define Str strview_t


void map_print(name2age_map *m) {
    printfd("\nPrinting map.");
    int count = 0;
    name2age_map_It it = { 0 };
    while (name2age_map_it_next(m, &it)) {
        printf("(\"%"PRIstr"\") = (%d)\n", PRIstrarg(it.key), it.value->years);
        ++count;
    }
    /*
    for (int i = 0; i < m->buckets.size; ++i) {
        name2age_map__Bucket *bucket = &m->buckets.items[i];
        for (int k = 0; k < bucket->pairs.size; ++k) {
            name2age_map__Pair *pair = &bucket->pairs.items[k];
            strview_t key = strpool_get(&m->strpool, pair->key);
            printf("(\"%"PRIstr"\") = (%d)\n", PRIstrarg(key), pair->value.years);
            ++count;
        }
    }*/
    printf("Pair count == %d. Printed %d pairs.\n", name2age_map_pair_count(m), count);
}


TEST test_basic(void) {
    name2age_map map;
    name2age_map *m = &map;
    int err;


    err = name2age_map_create(m);
    ASSERT_INT(err, 0);
    map_print(m);

    // Addition.

    Str name_marco = cstr_SL("Marco");
    Age age = { 20 };
    err = name2age_map_upsert(m, name_marco, age);
    ASSERT_INT(err, 0);
    map_print(m);
    Age *age_got = name2age_map_get(m, name_marco);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    Str name_polo = cstr_SL("Polo");
    age = (Age) { 1025 };
    err = name2age_map_upsert(m, name_polo, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_polo);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    Str name_available = cstr_SL("available Estimation of how much memory is available for starting new applications, without swapping. Unlike the data provided by the cache or free fields, this field takes into account page cache and also that not all reclaimable memory slabs will be reclaimed due to items being in use (MemAvailable in /proc/meminfo, available on kernels 3.14, emulated on kernels 2.6.27+, other‐ wise the same as free)");
    age = (Age) { 990099 };
    err = name2age_map_upsert(m, name_available, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_available);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    Str name_numbers = cstr_SL("one two three four five");
    age = (Age) { 12345 };
    err = name2age_map_upsert(m, name_numbers, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_numbers);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    // Replacing.

    age = (Age) { 100 };
    err = name2age_map_upsert(m, name_marco, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_marco);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 200 };
    err = name2age_map_upsert(m, name_polo, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_polo);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 300 };
    err = name2age_map_upsert(m, name_available, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_available);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 400 };
    err = name2age_map_upsert(m, name_numbers, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_numbers);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    map_print(m);

    // Deletion.

    err = name2age_map_remove(m, name_available);
    map_print(m);
    ASSERT_INT(err, 0);
    err = name2age_map_remove(m, name_marco);
    map_print(m);
    ASSERT_INT(err, 0);
    err = name2age_map_remove(m, name_numbers);
    map_print(m);
    ASSERT_INT(err, 0);
    err = name2age_map_remove(m, name_polo);
    map_print(m);
    ASSERT_INT(err, 0);

    // Last insertion.

    age = (Age) { 12345 };
    err = name2age_map_upsert(m, name_numbers, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_numbers);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    // Deletion.

    name2age_map_free(m);

    TEST_PASS;
}


typedef struct {
    char name[20];
} Fruit;
#define makefruit(name) ((Fruit) { name })

#define UMAPSTR__TYPE Fruit
#define UMAPSTR__NAMESPACE Map_Fruit
#include "umapstr.h"
#define Map Map_Fruit
#define MAP__TYPE Fruit

bool items_are_equal(Fruit a, Fruit b) {
    bool res = wstrview_equals(wcstr(a.name), wcstr(b.name));
    /*printfd("COMPARING %s with %s (%s)", a.name, b.name, PRIbool(res));*/
    return res;
}


typedef struct Item2Find {
    MAP__TYPE item;
    bool found;
} Item2Find;


void map_fruit_print(Map *m) {
    printfd("\nPrinting map.");
    int count = 0;
    Map_Fruit_It it = { 0 };
    while (Map_Fruit_it_next(m, &it)) {
        printf("(\"%"PRIstr"\") = (%s)\n", PRIstrarg(it.key), it.value->name);
        ++count;
    }
    printf("Pair count == %d. Printed %d pairs.\n", Map_Fruit_pair_count(m), count);
}


bool should_find_these(const Map *m, MAP__TYPE *p_items, const int item_amount) {

    int valid_items_found_count = 0;
    Item2Find items[item_amount];

    for (int i = 0; i < item_amount; ++i) {
        items[i] = (Item2Find) { 0 };
        items[i].found = false;
        items[i].item = p_items[i];
    }

    Map_Fruit_It it = { 0 };
    while (Map_Fruit_it_next(m, &it)) {

        ++valid_items_found_count;

        // See if it corresponds to one item.
        for (int k = 0; k < item_amount; ++k) {
            Item2Find *item = &items[k];
            if (items_are_equal(item->item, *it.value)) {
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
    ASSERT_FALSE(wstrview_equals(cstr_SL("banana"), cstr_SL("BANANA")));
    ASSERT_FALSE(wstrview_equals(cstr_SL("banana"), cstr_SL("banana\0"))); 
    ASSERT_FALSE(wstrview_equals(cstr_SL("banana\0"), wcstr("banana\0")));
    ASSERT_TRUE(wstrview_equals(wcstr("banana"), wcstr("banana\0\0\0\0")));
    ASSERT_FALSE(wstrview_equals(cstr_SL("banana"), cstr_SL("banana\0\0\0\0")));
    ASSERT_TRUE(wstrview_equals(wcstr("banana"), wcstr("banana\0")));
    // ↑↑↑ True because cstr calls strlen. But cstr_SL calls sizeof.
    TEST_PASS;
}


TEST test_general(void) {
    int err;
    Map map = { 0 };
    Map *m = &map;
    err = Map_Fruit_create(&map);
    ASSERT_INT(err, 0);

    map_fruit_print(m);

    Str key;
    Fruit fruit;
    Fruit *result;

    key = cstr_SL("APPLE");
    fruit = (Fruit) { "Appless" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    ASSERT_INT(Map_Fruit_pair_count(m), 1);
    /*ASSERT(wstrview_equals(cstr(m->items[0].value.name), cstr(fruit.name)));*/
    /* ↑↑↑ Test internal implementation to make sure the FIRST pair
           is filled. Note it is NOT a feature of this map to be able
           to iterate through the items. */

    {
        Fruit to_find[] = { makefruit("Appless") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("APPLE");
    fruit = (Fruit) { "Apple" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    ASSERT_INT(Map_Fruit_pair_count(m), 1);
    Str key_apple = key;
    {
        Fruit to_find[] = { makefruit("Apple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("BANANA");
    fruit = (Fruit) { "Banana" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Apple"), makefruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("PEAR");
    fruit = (Fruit) { "Pear" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    Str key_pear = key;
    {
        Fruit to_find[] = { makefruit("Apple"), makefruit("Banana"), makefruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Try to gain some space back.

    err = Map_Fruit_remove(m, key_apple);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Fruit_pair_count(m), 2);
    map_fruit_print(m);
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pear") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    err = Map_Fruit_remove(m, key_apple);
    ASSERT_INT(err, -1);

    err = Map_Fruit_remove(m, key_pear);
    ASSERT_INT(err, 0);
    ASSERT_INT(Map_Fruit_pair_count(m), 1);
    
    printfd("\n---->>> Removed APPLE and Pear, should remain Banana.");
    map_fruit_print(m);
    {
        Fruit to_find[] = { makefruit("Banana") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Insert now that there is space available.

    key = cstr_SL("PINEAPPLE");
    fruit = (Fruit) { "Pineapple" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    printfd("\n---->>> Reinserting APPLE.");

    key = cstr_SL("APPLE");
    fruit = (Fruit) { "Apple" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    result = Map_Fruit_get(m, key); ASSERT(result != NULL); ASSERT(wstrview_equals(wcstr(result->name), wcstr(fruit.name)));
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple"), makefruit("Apple") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    key = cstr_SL("MANGO");
    fruit = (Fruit) { "Mango" };
    err = Map_Fruit_upsert(m, key, fruit);
    ASSERT_INT(err, 0);
    map_fruit_print(m);
    {
        Fruit to_find[] = { makefruit("Banana"), makefruit("Pineapple"), makefruit("Apple"), makefruit("Mango") };
        ASSERT(should_find_these(m, to_find, countof(to_find)));
    }

    // Insert 100 things to see what's up.

    int prev_count = Map_Fruit_pair_count(m);

    for (int i = 0; i < 100; ++i) {
        char key_str[100] = { 0 };
        fruit = (Fruit) { 0 };
        sprintf(fruit.name, "fruit%d", i);
        sprintf(key_str, "FRUIT%d", i);
        err = Map_Fruit_upsert(m, wcstr(key_str), fruit);
        ASSERT_INT(err, 0);
    }

    map_fruit_print(m);
    ASSERT_INT(Map_Fruit_pair_count(m), prev_count + 100);

    // Remove all pairs one by one.
    // @Note. This is not a good way to clear the map, it's slow.

    while (Map_Fruit_pair_count(m)) {
        Map_Fruit_It it = { 0 };
        if (!Map_Fruit_it_next(m, &it)) { break; }

        err = Map_Fruit_remove(m, it.key);
        if (err != 0) {
            printfd(ANSI_RED"Failed to remove: %"PRIstr" = %s", PRIstrarg(it.key), it.value->name);
        } else {
            printfd(ANSI_GRE"Removed: %"PRIstr" = %s", PRIstrarg(it.key), it.value->name);
        }
        ASSERT_INT(err, 0);
    }

    map_fruit_print(m);
    ASSERT_INT(Map_Fruit_pair_count(m), 0);

    Map_Fruit_free(m);

    TEST_PASS;
}



int main(void) {
    TESTS_INIT();
    RUN_TEST(test_basic);
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


