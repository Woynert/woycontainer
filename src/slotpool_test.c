#include "stdio.h"
#include "portable_utils.h"
#include "woytest.h"
#include "arena.h"
#include "arenady.h"
#include "slotpool.h"
#include "wstrview.h"

typedef struct {
    ID first;
    ID second;
    int size;
} Pair;

#define DYNA__TYPE Pair
#define DYNA__NAMESPACE Vec_Pair
#include "da.h"

#define ALLOC_PROTOTYPE(x) void* (x) (void* ptr, size_t size, int align, void* user_data)

/// !Start [Naive Implementation].
typedef struct {
    void *data;
    int size;
} Data;
#define SLOT__TYPE Data
#define SLOT__NAMESPACE Slot_Data
#include "slot.h"
typedef struct {
    Slot_Data data_store;
} NaiveSlotpool;
void naiveslotpool_create(NaiveSlotpool *p) {
    *p = (NaiveSlotpool) { 0 };
    Slot_Data_create(&p->data_store);
}
void naiveslotpool_clear(NaiveSlotpool *p) {
    for (int i = 0; i < p->data_store.count; ++i) {
        Data *data = &p->data_store._items[i];
        free(data->data);
    }
    Slot_Data_clear(&p->data_store);
}
void naiveslotpool_free(NaiveSlotpool *p) {
    naiveslotpool_clear(p);
    Slot_Data_free(&p->data_store);
    *p = (NaiveSlotpool) { 0 };
}
ID naiveslotpool__append(NaiveSlotpool *p, int size) {
    if (size <= 0) { return ID_INVALID; }
    Data data = { .data = malloc((size_t)size), .size = size, };
    wassert_live(data.data);
    return ID_make(Slot_Data_append(&p->data_store, data));
}
void *naiveslotpool__get(NaiveSlotpool *p, ID id) {
    Data *saved_data = Slot_Data_get(&p->data_store, ID_get(id));
    if (!saved_data) { return NULL; }
    return saved_data->data;
}
void naiveslotpool_remove(NaiveSlotpool *p, ID id) {
    Data *saved_data = Slot_Data_get(&p->data_store, ID_get(id));
    if (!saved_data) { return; }
    free(saved_data->data);
    Slot_Data_pop(&p->data_store, ID_get(id));
}
#define naiveslotpool_append(TYPE, pool, out) (naiveslotpool__append((pool), (int)sizeof(TYPE), (out)))
#define naiveslotpool_get(TYPE, pool, id) ((TYPE*)naiveslotpool__get((pool), (id)))
/// !End [Naive Implementation].

#define DYNA__TYPE Data
#define DYNA__NAMESPACE Vec_Data
#include "da.h"

bool compare_contents(Slotpool *p, NaiveSlotpool *np, Arena scratch) {
    if (p->views.count != np->data_store.count) { printferr("Wrong item count"); return false; }

    Vec_Data saved_data = Vec_Data_create_with_allocator(arena_allocator, &scratch);
    for (int i = 0; i < np->data_store.count; ++i) {
        Data *data = &np->data_store._items[i];
        Vec_Data_append(&saved_data, *data);
    }

    for (int i = 0; i < p->views.count; ++i) {
        slotpool__view view = p->views._items[i];
        void *data1 = slotpool__get_from_view(p, view);

        if (saved_data.size == 0) { printferr("Found too many."); return false; }

        for (dyna_foreach_gnu(iter, saved_data)) {
            void *data2 = iter.ref->data;
            if (view.bytes == iter.ref->size &&
                0 == memcmp(data1, data2, (size_t)view.bytes)
            ) {
                Vec_Data_remove_at(&saved_data, iter.index);
                printfd("%d/%d Found item of size %d", i, p->views.count, view.bytes);
                print_hex(data1, (size_t)view.bytes);
                print_hex(data2, (size_t)view.bytes);
                break;
            }
        }
    }

    if (saved_data.size != 0) { printferr("Couldn't find all."); return false; }

    return true;
}

typedef struct {
    int birthyear;
    int birthmonth;
    int birthday;
    char name[40];
} Human;

bool Human_compare(Human *a, Human *b) {
    return
        a->birthday == b->birthday &&
        a->birthmonth == b->birthmonth &&
        a->birthday == b->birthday &&
        memcmp(a->name, b->name, sizeof(b->name)) == 0;
}

TEST test_small(ALLOC_PROTOTYPE(*allocator), void *alloc_data) {

    Slotpool p = { 0 };
    slotpool_create_with_allocator(&p, allocator, alloc_data);

    ID gordon_id = { 0 }, marco_id = { 0 }, tarma_id = { 0 };
    Human gordon, marco, tarma;
    // Append.
    {
        gordon = (Human) { 1990, 10, 10, .name = "Gordon Freeman" };
        gordon_id = slotpool_append(&p, Human, gordon);
        marco = (Human) { 1980, 5, 5, .name = "Marco MS3" };
        marco_id = slotpool_append(&p, Human, marco);
        tarma = (Human) { 1970, 3, 3, .name = "Tarma MS3" };
        tarma_id = slotpool_append(&p, Human, tarma);
    }
    // Get.
    {
        Human *ref;
        ref = slotpool_get(&p, Human, gordon_id);
        ASSERT(Human_compare(&gordon, ref));
        ref = slotpool_get(&p, Human, marco_id);
        ASSERT(Human_compare(&marco, ref));
        ref = slotpool_get(&p, Human, tarma_id);
        ASSERT(Human_compare(&tarma, ref));
    }
    // Remove.
    {
        int err = slotpool_remove(&p, tarma_id);
        ASSERT_INT(err, 0);
        err = slotpool_remove(&p, gordon_id);
        ASSERT_INT(err, 0);
    }
    // Get again.
    {
        Human *ref;
        ref = slotpool_get(&p, Human, gordon_id);
        ASSERT(!ref);
        ref = slotpool_get(&p, Human, marco_id);
        ASSERT(Human_compare(&marco, ref));
        ref = slotpool_get(&p, Human, tarma_id);
        ASSERT(!ref);
    }

    slotpool_destroy(&p);
    TEST_PASS;
}


static struct {
    Slotpool slotpool;
    NaiveSlotpool naiveslotpool;
    ArenaRoot arenaroot;
    Vec_Pair id_pairs;
} fuzzer_state;
void LLVMFuzzerCleanup(void) {
    slotpool_destroy(&fuzzer_state.slotpool);
    naiveslotpool_free(&fuzzer_state.naiveslotpool);
    ArenaRoot_free(&fuzzer_state.arenaroot);
    Vec_Pair_free(&fuzzer_state.id_pairs);
}
int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc,(void)argv; atexit(LLVMFuzzerCleanup); return 0;
}
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t data_size) {
    /*
        @Note: This fuzz tests runs the same operations on both
        the real implementation and the naive implementation,
        after each operation it's verified their contents match.
       */
    static bool setup = false;
    static Slotpool *p = &fuzzer_state.slotpool;
    static NaiveSlotpool *np = &fuzzer_state.naiveslotpool;
    static ArenaRoot *arenaroot = &fuzzer_state.arenaroot;
    static Vec_Pair *id_pairs = &fuzzer_state.id_pairs;
    if (!setup) {
        setup = true;
        slotpool_create(p);
        naiveslotpool_create(np);
        *arenaroot = ArenaRoot_create(1 << 20);
        *id_pairs = Vec_Pair_create();
    }

    Arena arena = ArenaRoot_get_arena(*arenaroot);
    ArenaDy arenady = { .root = (char*)data, .beg = (char*)data, .end = (char*)data + data_size };
    const int *action = arenady_try_get_one(&arenady, int);
    if (!action) { return 0; }

    enum {
        CASE_INSERT,
        CASE_REMOVE,
        CASE_GET,
        CASE_IT_FORWARDS,
        CASE_IT_BACKWARDS,
        CASE_CLEAR,
    };

    switch (*action) {
        case CASE_INSERT:
        {
            const int str_size = (int)(arenady.end - arenady.beg);
            if (str_size <= 0) { break; }
            const char *str_data = (char*)arenady_try_get(&arenady, sizeof(char), alignof(char), str_size);
            wassert_live(str_data);

            ID id1, id2;
            char *data1, *data2;
            {
                id1 = slotpool__append(p, str_data, str_size, (void**)&data1);
                wassert_live(ID_valid(id1));
            }
            {
                id2 = naiveslotpool__append(np, str_size);
                wassert_live(ID_valid(id2));
                data2 = (char*)naiveslotpool__get(np, id2);
                memcpy(data2, str_data, (size_t)str_size);
            }
            wassert_live(compare_contents(p, np, arena));
            Vec_Pair_append(id_pairs, (Pair) { id1, id2, str_size });
            break;
        }
        case CASE_REMOVE:
        {
            if (id_pairs->size <= 0) { break; }
            Pair pair;
            {
                const int *index_raw = arenady_try_get_one(&arenady, int);
                if (!index_raw) { break; }
                const int index = true_modulo(*index_raw, id_pairs->size);
                pair = id_pairs->items[index];
                Vec_Pair_remove_at(id_pairs, index);
            }
            slotpool_remove(p, pair.first);
            naiveslotpool_remove(np, pair.second);
            wassert_live(compare_contents(p, np, arena));
            break;
        }
        case CASE_GET:
        {
            if (id_pairs->size <= 0) {
                wassert_live(p->views.count == 0);
                wassert_live(np->data_store.count == 0);
                break;
            }
            Pair pair;
            {
                const int *index_raw = arenady_try_get_one(&arenady, int);
                if (!index_raw) { break; }
                const int index = true_modulo(*index_raw, id_pairs->size);
                pair = id_pairs->items[index];
            }
            void *data1 = slotpool__get(p, pair.first, pair.size);
            void *data2 = naiveslotpool__get(np, pair.second);
            wassert_live(0 == memcmp(data1, data2, (size_t)pair.size));
            break;
        }
        case CASE_CLEAR:
        {
            wassert_live(compare_contents(p, np, arena));
            slotpool_clear(p);
            naiveslotpool_clear(np);
            Vec_Pair_clear_preserving(id_pairs);
            wassert_live(compare_contents(p, np, arena));
            break;
        }
        default: break;
    }
    return 0;
}


#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
int main(void) {
    TESTS_INIT();
    RUN_TEST(test_small, NULL, NULL);
    TESTS_SHOW_RESULTS();
}
#endif



