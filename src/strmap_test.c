#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"


typedef struct Age {
    int years;
} Age;


#define STRMAP__TYPE Age
#define STRMAP__NAMESPACE name2age_map
#include "strmap.h"


#define str strmap__view
#define cstr_SL(sl_arg) ((str){.data=(sl_arg), .size=sizeof(sl_arg)-1})
#define PRIstr ".*s"
#define PRIstrarg(arg) ((arg).size),((arg).data)


void map_print(name2age_map *m) {
    printfd("\nPrinting map.");
    int count = 0;
    for (int i = 0; i < m->buckets.size; ++i) {
        name2age_map__Bucket *bucket = &m->buckets.items[i];
        for (int k = 0; k < bucket->pairs.size; ++k) {
            name2age_map__Pair *pair = &bucket->pairs.items[k];
            strpool__str key = strpool_get(&m->strpool, pair->key);
            printf("(\"%"PRIstr"\") = (%d)\n", PRIstrarg(key), pair->value.years);
            ++count;
        }
    }
    printf("Pair count == %d. Printed %d pairs.\n", m->pair_count, count);
}


TEST test_general(void) {
    name2age_map map;
    name2age_map *m = &map;
    int err;


    err = name2age_map_create(m);
    ASSERT_INT(err, 0);
    map_print(m);

    // Addition.

    str name_marco = cstr_SL("Marco");
    Age age = { 20 };
    err = name2age_map_set_pair(m, name_marco, age);
    ASSERT_INT(err, 0);
    map_print(m);
    Age *age_got = name2age_map_get(m, name_marco);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    str name_polo = cstr_SL("Polo");
    age = (Age) { 1025 };
    err = name2age_map_set_pair(m, name_polo, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_polo);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    str name_available = cstr_SL("available Estimation of how much memory is available for starting new applications, without swapping. Unlike the data provided by the cache or free fields, this field takes into account page cache and also that not all reclaimable memory slabs will be reclaimed due to items being in use (MemAvailable in /proc/meminfo, available on kernels 3.14, emulated on kernels 2.6.27+, other‐ wise the same as free)");
    age = (Age) { 990099 };
    err = name2age_map_set_pair(m, name_available, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_available);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    str name_numbers = cstr_SL("one two three four five");
    age = (Age) { 12345 };
    err = name2age_map_set_pair(m, name_numbers, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_numbers);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    // Replacing.

    age = (Age) { 100 };
    err = name2age_map_set_pair(m, name_marco, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_marco);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 200 };
    err = name2age_map_set_pair(m, name_polo, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_polo);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 300 };
    err = name2age_map_set_pair(m, name_available, age);
    ASSERT_INT(err, 0);
    age_got = name2age_map_get(m, name_available);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    age = (Age) { 400 };
    err = name2age_map_set_pair(m, name_numbers, age);
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

    /*str name_numbers = cstr_SL("");*/
    age = (Age) { 12345 };
    err = name2age_map_set_pair(m, name_numbers, age);
    ASSERT_INT(err, 0);
    map_print(m);
    age_got = name2age_map_get(m, name_numbers);
    ASSERT(age_got != NULL);
    ASSERT_INT(age_got->years, age.years);

    // Deletion.

    name2age_map_free(m);

    TEST_PASS;
}

int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


