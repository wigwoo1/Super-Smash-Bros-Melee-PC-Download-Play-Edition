#include "game_prefs_session.h"

#include <melee/gm/gmmain_lib.h>
#include <melee/gm/types.h>
#include <dolphin/os.h>
#include <string.h>
#include <stdlib.h>

static MeleePc_GamePrefsTransfer queued;
static int pending, ready;

int MeleePc_GamePrefsDiagnosticSeed(void)
{
    const char* fixture = getenv("MELEE_DIAGNOSTIC_GAME_PREFS_TRANSFER");
    struct GamePrefs* prefs;
    if (!fixture) return 1;
    if (!ready || strcmp(fixture, "1") ||
        _putenv_s("MELEE_DIAGNOSTIC_GAME_PREFS_TRANSFER", "")) return 0;
    prefs = gmMainLib_GetGamePrefs();
    prefs->stage_mask = 0x5a5aa55aU;
    prefs->item_mask = UINT64_C(0x0123456789abcdef);
    prefs->item_freq = 255;
    prefs->rumble_enabled[0] = prefs->rumble_enabled[2] = 0;
    prefs->rumble_enabled[1] = prefs->rumble_enabled[3] = 1;
    prefs->sound_balance = -37;
    prefs->deflicker = 0;
    prefs->saved_language = 1;
    OSReport("NATIVE_RENDERER_GAME_PREFS fixture=parent_session_only consumed_before_spawn=1 stage_mask=%08x\n",
             (unsigned) prefs->stage_mask);
    return 1;
}

static int valid(const MeleePc_GamePrefsTransfer* input)
{
    unsigned i;
    /* The original item menu represents None as unsigned -1, and the other
     * five frequencies as 0..4. Original sound balance is -100..100. */
    if (!input || (input->item_freq > 4 && input->item_freq != 255) ||
        input->sound_balance < -100 || input->sound_balance > 100 ||
        input->deflicker > 1 || input->saved_language > 1) return 0;
    for (i = 0; i < 4; ++i) if (input->rumble[i] > 1) return 0;
    return 1;
}

int MeleePc_GamePrefsCapture(MeleePc_GamePrefsTransfer* output)
{
    struct GamePrefs* prefs;
    if (!ready || !output) return 0;
    prefs = gmMainLib_GetGamePrefs();
    memset(output, 0, sizeof(*output));
    output->item_mask = prefs->item_mask;
    output->stage_mask = prefs->stage_mask;
    output->item_freq = prefs->item_freq;
    memcpy(output->rumble, prefs->rumble_enabled, sizeof(output->rumble));
    output->sound_balance = prefs->sound_balance;
    output->deflicker = prefs->deflicker;
    output->saved_language = prefs->saved_language;
    return valid(output);
}

int MeleePc_GamePrefsQueue(const MeleePc_GamePrefsTransfer* input)
{
    /* Transfer is accepted only before original startup. A malformed packet
     * cannot overwrite either live preferences or an already queued packet. */
    if (ready || pending || !valid(input)) return 0;
    queued = *input;
    pending = 1;
    return 1;
}

void MeleePc_GamePrefsAfterBootstrap(void)
{
    struct GamePrefs* prefs = gmMainLib_GetGamePrefs();
    ready = 1;
    if (!pending) return;
    prefs->item_mask = queued.item_mask;
    prefs->stage_mask = queued.stage_mask;
    prefs->item_freq = queued.item_freq;
    memcpy(prefs->rumble_enabled, queued.rumble, sizeof(queued.rumble));
    prefs->sound_balance = queued.sound_balance;
    prefs->deflicker = queued.deflicker;
    prefs->saved_language = queued.saved_language;
    pending = 0;
    OSReport("NATIVE_RENDERER_GAME_PREFS restored=1 stage_mask=%08x item_mask=%016llx item_freq=%u rumble=%u%u%u%u sound_balance=%d deflicker=%u saved_language=%u game_prefs_bytes=%u transfer_bytes=%u persistent=0\n",
             (unsigned) prefs->stage_mask, (unsigned long long) prefs->item_mask,
             prefs->item_freq, prefs->rumble_enabled[0], prefs->rumble_enabled[1],
             prefs->rumble_enabled[2], prefs->rumble_enabled[3],
             prefs->sound_balance, prefs->deflicker, prefs->saved_language,
             (unsigned) sizeof(*prefs), (unsigned) sizeof(queued));
    memset(&queued, 0, sizeof(queued));
}
