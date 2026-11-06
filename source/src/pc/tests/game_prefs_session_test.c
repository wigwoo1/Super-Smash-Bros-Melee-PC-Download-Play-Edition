#include <pc/game_prefs_session.h>
#include <melee/gm/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static struct GamePrefs live;
static unsigned checks;
struct GamePrefs* gmMainLib_GetGamePrefs(void) { return &live; }
void OSReport(const char* format, ...) { (void) format; }
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

int main(int argc, char** argv)
{
    MeleePc_GamePrefsTransfer wanted = { 0 }, result, bad;
    unsigned i;
    int ordinary = argc > 1 && !strcmp(argv[1], "ordinary");
    _putenv_s("MELEE_DIAGNOSTIC_GAME_PREFS_TRANSFER", "");
    CHECK(!MeleePc_GamePrefsCapture(&result));
    wanted.item_mask = UINT64_C(0xfedcba9876543210);
    wanted.stage_mask = argc > 2 ? (uint32_t) strtoul(argv[2], NULL, 16) : 0;
    wanted.item_freq = 255; wanted.rumble[0] = wanted.rumble[2] = 1;
    wanted.sound_balance = -100; wanted.deflicker = 0; wanted.saved_language = 1;
    CHECK(!MeleePc_GamePrefsQueue(NULL));
    for (i = 5; i < 255; ++i) {
        bad = wanted; bad.item_freq = (uint8_t) i;
        CHECK(!MeleePc_GamePrefsQueue(&bad));
    }
    for (i = 0; i < 4; ++i) {
        bad = wanted; bad.rumble[i] = 2; CHECK(!MeleePc_GamePrefsQueue(&bad));
    }
    bad = wanted; bad.saved_language = 2; CHECK(!MeleePc_GamePrefsQueue(&bad));
    bad = wanted; bad.deflicker = 2; CHECK(!MeleePc_GamePrefsQueue(&bad));
    bad = wanted; bad.sound_balance = -101; CHECK(!MeleePc_GamePrefsQueue(&bad));
    bad = wanted; bad.sound_balance = 101; CHECK(!MeleePc_GamePrefsQueue(&bad));
    if (!ordinary) {
        CHECK(MeleePc_GamePrefsQueue(&wanted));
        CHECK(!MeleePc_GamePrefsQueue(&wanted));
    }
    /* Model both original resets: neither may receive a premature write. */
    memset(&live, 0x33, sizeof(live));
    CHECK(live.stage_mask == 0x33333333);
    memset(&live, 0, sizeof(live));
    live.item_mask = UINT64_MAX; live.stage_mask = UINT32_MAX;
    live.item_freq = 2; memset(live.rumble_enabled, 1, 4);
    live.deflicker = 1; live.saved_language = 0;
    MeleePc_GamePrefsAfterBootstrap();
    CHECK(MeleePc_GamePrefsCapture(&result));
    if (ordinary) {
        CHECK(result.stage_mask == UINT32_MAX && result.item_mask == UINT64_MAX);
        CHECK(result.item_freq == 2 && result.saved_language == 0 && result.deflicker == 1);
    } else {
        CHECK(!memcmp(&result, &wanted, sizeof(wanted)));
    }
    CHECK(!MeleePc_GamePrefsQueue(&wanted));
    /* A later legitimate reset/change must never replay the old packet. */
    live.stage_mask = 0x12345678; live.sound_balance = 17;
    MeleePc_GamePrefsAfterBootstrap();
    CHECK(live.stage_mask == 0x12345678 && live.sound_balance == 17);
    CHECK(MeleePc_GamePrefsDiagnosticSeed());
    CHECK(live.stage_mask == 0x12345678);
    printf("PASS checks=%u ordinary=%d native_bytes=%u transfer_bytes=%u\n", checks, ordinary,
           (unsigned) sizeof(live), (unsigned) sizeof(wanted));
    return 0;
}
