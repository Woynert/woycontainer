#include "mapstrobj.h"
#include "woytest.h"

typedef struct {
    int year, month, day, hour, minute, seconds;
} Date;

typedef struct {
    double x, y, z, health, defense, damage, attack_delay;
} Monster;

bool Date_equal(Date a, Date b) {
    return a.year == b.year &&
    a.month == b.month &&
    a.day == b.day &&
    a.hour == b.hour &&
    a.minute == b.minute &&
    a.seconds == b.seconds;
}

bool Monster_equal(Monster a, Monster b) {
    return a.x == b.x &&
    a.y == b.y &&
    a.z == b.z &&
    a.health == b.health &&
    a.defense == b.defense &&
    a.damage == b.damage &&
    a.attack_delay == b.attack_delay;
}

static_assert(sizeof(Date) != sizeof(Monster), "");

TEST test_small(void) {
    MapStrObj m;
    mapstrobj_create(&m);

    // Insert.

    strview_t today_key = cstr_SL("today");
    Date today = { 2026, 9, 29, 6, 56, 4 };
    int err = mapstrobj_upsert(&m, today_key, &today, sizeof(today));
    ASSERT(!err);

    strview_t tomorrow_key = cstr_SL("tomorrow");
    Date tomorrow = { 2026, 9, 30, 12, 49, 50 };
    err = mapstrobj_upsert(&m, tomorrow_key, blob_arg(tomorrow));
    ASSERT(!err);

    strview_t bigfoot_key = cstr_SL("bigfoot has big feet");
    Monster bigfoot = { 10.6, 30.5, 50.2, 100, 200, 20, 10 };
    err = mapstrobj_upsert(&m, bigfoot_key, blob_arg(bigfoot));
    ASSERT(!err);

    strview_t mothman_key = cstr_SL("mothman, moth and man, and moth.");
    Monster mothman = { 50.6, 20.5, 10.2, 200, 300, 25, 9 };
    err = mapstrobj_upsert(&m, mothman_key, blob_arg(mothman));
    ASSERT(!err);

    // Get.
    {
    Date *date;

    date = (Date*)mapstrobj_get(&m, today_key, sizeofi(Date));
    ASSERT(Date_equal(*date, today));

    date = (Date*)mapstrobj_get(&m, tomorrow_key, sizeofi(Date));
    ASSERT(Date_equal(*date, tomorrow));
    }

    {
    Monster *monster;
    monster = (Monster*)mapstrobj_get(&m, bigfoot_key, sizeofi(Monster));
    ASSERT(Monster_equal(*monster, bigfoot));

    monster = (Monster*)mapstrobj_get(&m, mothman_key, sizeofi(Monster));
    ASSERT(Monster_equal(*monster, mothman));
    }

    // Remove.
    {
    mapstrobj_remove(&m, bigfoot_key);
    Monster *monster = (Monster*)mapstrobj_get(&m, bigfoot_key, sizeofi(Monster));
    ASSERT(!monster);
    }

    mapstrobj_free(&m);

    TEST_PASS;
}

int main(void) {
    TESTS_INIT();
    RUN_TEST(test_small);
    TESTS_SHOW_RESULTS();
}


