#include "stdio.h"
#include "strbuf.h"
#include "strview.h"
#include "../strbuf_extra.h"
#include "../portable_utils.h"

typedef struct Human {
    strbuf_t *name;
    int age;
} Human;

void Human_init(Human *human) {
    human->name = strbuf_create(0, NULL);
}

#define FIFORING_TYPE Human
/*#define FIFORING_PREFIX CustomName*/
#define FIFORING_DISABLE_COMPARISONS
#include "fiforing.h"
#undef FIFORING_TYPE
#undef FIFORING_PREFIX
#undef FIFORING_DISABLE_COMPARISONS

#define RING_SIZE 16

Human_FIFORing ring;


void create_human(strview_t name, int age) {
    Human_FIFORing_extend_head(&ring);
    Human *human = Human_FIFORing_get_head(&ring);
    ASSERT(human != NULL);

    strbuf_assign(&human->name, name);
    human->age = age;
}

void print_humans(void) {
    printf("vvvv\n");
    printf("[cap %d size %d tail %d head %d]\n", ring._capacity, ring.size, ring._tail, ring._head);
    for (uint i = 0; i < ring.size; ++i) {
        Human *human = Human_FIFORing_get_relative_to_tail(&ring, i);

        printf("%d -> name: %"PRIstr", age: %d.\n",
            i, PRIstrarg(strview_of_buf(human->name)), human->age);
    }
}

void print_all(void) {
    printf("vvvv RAW vvvv\n");
    printf("[cap %d size %d tail %d head %d]\n", ring._capacity, ring.size, ring._tail, ring._head);
    for (uint i = 0; i < ring._capacity; ++i) {
        Human *human = &ring.buffer[i];

        printf("%d -> name: %"PRIstr", age: %d.\n",
            i, PRIstrarg(strview_of_buf(human->name)), human->age);
    }
}

int main(void) {

    ring = Human_FIFORing_create(RING_SIZE);
    for (int i = 0; i < RING_SIZE; ++i) {
        Human_init(&ring.buffer[i]);
    }

    int err;

    create_human(cstr("Woynert"), 22);
    create_human(cstr("Daniela"), 23);
    create_human(cstr("Marco"), 24);
    create_human(cstr("Eliany"), 24);
    create_human(cstr("Mimo"), 50);
    print_humans();

    Human_FIFORing_pop_tail(&ring);
    Human_FIFORing_pop_tail(&ring);
    print_humans();

    /*Human_FIFORing_pop_head(&ring);*/
    /*print_humans();*/

    /*Human_FIFORing_pop_head(&ring);*/
    /*print_humans();*/

    /*Human_FIFORing_pop_head(&ring);*/
    /*print_humans();*/

    /*err = Human_FIFORing_remove_relative_to_tail(&ring, ring.size-2);*/
    /*ASSERT(err == 0);*/
    /*print_humans();*/

    print_all();
    printf("OK\n");
}
