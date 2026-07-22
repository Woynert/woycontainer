#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"


#define ARRAY__TYPE int
#include "array.h"


void print_array(const int_Array* a) {
    printf("Contents: "ANSI_BLU);
    for (int i = 0; i < a->size; ++i) {
        printf("%d, ", a->items[i]);
    }
    printf(ANSI_RESET"\n");
}


TEST test_general(void) {
    int_Array a = int_Array_create();

    int err = int_Array_resize(&a, 10);
    ASSERT_INT(err, 0);
    ASSERT_INT(a.size, 10);

    // Simple insert.

    for (int i = 0, k = 0; i < 10; ++i, k += 10) {
        err = int_Array_set_safe(&a, i, k);
        ASSERT_INT(err, 0);
    }
    for (int i = 0, k = 0; i < 10; ++i, k += 10) {
        ASSERT_INT(a.items[i], k);
    }

    // Resizing.

    err = int_Array_resize(&a, 20);
    ASSERT_INT(err, 0);
    ASSERT_INT(a.size, 20);

    for (int i = 0, k = 0; i < 10; ++i, k += 10) {
        ASSERT_INT(a.items[i], k);
    }

    err = int_Array_resize(&a, 0); // Will fail to resize to 0.
    ASSERT_INT_NEQ(err, 0);

    int_Array_destroy(&a);
    ASSERT_INT(a.size, 0);

    TEST_PASS;
}

int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}


