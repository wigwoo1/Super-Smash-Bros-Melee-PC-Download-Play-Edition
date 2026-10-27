/// @file gm_1A9B.c
/// Game-over "Regend" ending THP playback scene (OnEnter/OnFrame pair).
///
/// This TU is split from gmregenddisp.c: the retail object anchors the THP
/// table loads in gm_Scene_Congrats_OnEnter at the first table literal
/// (gm_803DB640, displacements 0x278/0x5B4/0x8D0), i.e. at this TU's own
/// .data+0. MWCC 1.2.5n always places its merged-data anchor (..data.N) at
/// the object's .data start, so these tables cannot share a TU with the
/// string literals at 0x803DB458-0x803DB640 (which belong to gmregenddisp.c,
/// whose functions anchor at 0x803DB458). Splitting here reproduces the
/// retail anchor exactly; keeping the tables in gmregenddisp.c compiles the
/// same instructions with displacements 0x460/0x79C/0xAB8 instead.
#include "forward.h"
#ifdef TARGET_PC
#include <stdlib.h>
#include <dolphin/os.h>
#include <aurora/diagnostics.h>
#include <sysdolphin/baselib/controller.h>
#endif
#include "gm_1A36.h"
#include "gm_1A3F.h"
#include "gm_1A7A.h"
#include "gmevent.h"
#include "gmregclear.h"
#include "gmregtyfall.h"
#include <dolphin/pad.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbbgflash.h>
#include <melee/lb/lbmthp.h>
#include <melee/mn/inlines.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/sobjlib.h>

static u8 gmRegend_InputDelayTimer;
static u8 gmRegend_ExitTimer;

static char* gmRegend_SimpleThpFiles[] = {
    "GmRegendSimpleCaptain.thp",   "GmRegendSimpleDonkey.thp",
    "GmRegendSimpleFox.thp",       "GmRegendSimpleGamewatch.thp",
    "GmRegendSimpleKirby.thp",     "GmRegendSimpleKoopa.thp",
    "GmRegendSimpleLink.thp",      "GmRegendSimpleLuigi.thp",
    "GmRegendSimpleMario.thp",     "GmRegendSimpleMarth.thp",
    "GmRegendSimpleMewtwo.thp",    "GmRegendSimpleNess.thp",
    "GmRegendSimplePeach.thp",     "GmRegendSimplePikachu.thp",
    "GmRegendSimplePoponana.thp",  "GmRegendSimplePurin.thp",
    "GmRegendSimpleSamus.thp",     "GmRegendSimpleYoshi.thp",
    "GmRegendSimpleZeldaseak.thp", "GmRegendSimpleZeldaseak.thp",
    "GmRegendSimpleFalco.thp",     "GmRegendSimpleClink.thp",
    "GmRegendSimpleDrmario.thp",   "GmRegendSimpleRoy.thp",
    "GmRegendSimplePichu.thp",     "GmRegendSimpleGanon.thp",
};

static char* gmRegend_AdventureThpFiles[] = {
    "GmRegendAdventureCaptain.thp",   "GmRegendAdventureDonkey.thp",
    "GmRegendAdventureFox.thp",       "GmRegendAdventureGamewatch.thp",
    "GmRegendAdventureKirby.thp",     "GmRegendAdventureKoopa.thp",
    "GmRegendAdventureLink.thp",      "GmRegendAdventureLuigi.thp",
    "GmRegendAdventureMario.thp",     "GmRegendAdventureMarth.thp",
    "GmRegendAdventureMewtwo.thp",    "GmRegendAdventureNess.thp",
    "GmRegendAdventurePeach.thp",     "GmRegendAdventurePikachu.thp",
    "GmRegendAdventurePoponana.thp",  "GmRegendAdventurePurin.thp",
    "GmRegendAdventureSamus.thp",     "GmRegendAdventureYoshi.thp",
    "GmRegendAdventureZeldaseak.thp", "GmRegendAdventureZeldaseak.thp",
    "GmRegendAdventureFalco.thp",     "GmRegendAdventureClink.thp",
    "GmRegendAdventureDrmario.thp",   "GmRegendAdventureRoy.thp",
    "GmRegendAdventurePichu.thp",     "GmRegendAdventureGanon.thp",
};

static char* gmRegend_AllstarThpFiles[] = {
    "GmRegendAllstarCaptain.thp",   "GmRegendAllstarDonkey.thp",
    "GmRegendAllstarFox.thp",       "GmRegendAllstarGamewatch.thp",
    "GmRegendAllstarKirby.thp",     "GmRegendAllstarKoopa.thp",
    "GmRegendAllstarLink.thp",      "GmRegendAllstarLuigi.thp",
    "GmRegendAllstarMario.thp",     "GmRegendAllstarMarth.thp",
    "GmRegendAllstarMewtwo.thp",    "GmRegendAllstarNess.thp",
    "GmRegendAllstarPeach.thp",     "GmRegendAllstarPikachu.thp",
    "GmRegendAllstarPoponana.thp",  "GmRegendAllstarPurin.thp",
    "GmRegendAllstarSamus.thp",     "GmRegendAllstarYoshi.thp",
    "GmRegendAllstarZeldaseak.thp", "GmRegendAllstarZeldaseak.thp",
    "GmRegendAllstarFalco.thp",     "GmRegendAllstarClink.thp",
    "GmRegendAllstarDrmario.thp",   "GmRegendAllstarRoy.thp",
    "GmRegendAllstarPichu.thp",     "GmRegendAllstarGanon.thp",
};

void gm_Scene_Congrats_OnEnter(UNK_T unused)
{
    HSD_SObj* thp_disp;
    s32 ckind;
    HSD_GObj* gobj;
    HSD_GObj* sobj_gobj;
    const char* thpfile;
    s32 level;
    int gover_kind;

    gmRegend_InputDelayTimer = 0x1E;
    gmRegend_ExitTimer = 0;
    sobj_gobj = GObj_Create(0x13, 0x14, 0);
    HSD_SObjLib_803A55DC(sobj_gobj, 0x280, 0x1E0, 8);
    sobj_gobj->gxlink_prios = 0x800;
    lbAudioAx_80026F2C(0x12);
    lbAudioAx_8002702C(2, 0x10);
    lbAudioAx_80027168();
    lbAudioAx_80027648();
    lbBgFlash_800209F4();
    gobj = GObj_Create(0xE, 0xF, 0);
    HSD_GObjObject_80390A70(gobj, HSD_SObjLib_804D7960, NULL);
    GObj_SetupGXLink(gobj, lbMthp8001F928, 0xB, 0);
    ckind = gm_801BEFB0();
    gover_kind = gm_GetCurrentGameMode();
    if (gover_kind == GM_DEBUG_GOVER) {
        gover_kind = gm_801BF050();
    }
    switch (gover_kind) {
    case GM_CLASSIC_GOVER:
        thpfile = gmRegend_SimpleThpFiles[ckind];
        break;
    case GM_ADVENTURE_GOVER:
        thpfile = gmRegend_AdventureThpFiles[ckind];
        break;
    default:
        thpfile = gmRegend_AllstarThpFiles[ckind];
        break;
    }
    lbMthp8001FAA0(thpfile, 0x230, 0x1A0);
    thp_disp = lbMthp8001F890(gobj);
    thp_disp->x10 = 320.0F;
    thp_disp->x14 = 240.0F;
    thp_disp->x40 |= 2;
    switch (gm_GetCurrentGameMode()) {
    case GM_CLASSIC_GOVER:
        level = gm_8017DFF4(1);
        break;
    case GM_ADVENTURE_GOVER:
        level = gm_8017DFF4(0);
        break;
    default:
        level = gm_8017DFF4(2);
        break;
    }
    if (level == 4) {
        lbAudioAx_800237A8(0x9C45, 0x7F, 0x40);
    } else {
        lbAudioAx_800237A8(0x9C41, 0x7F, 0x40);
    }
}

void gm_Scene_Congrats_OnFrame(void)
{
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_CONGRATS")) {
        static unsigned calls;
        PADStatus neutral = { 0 };
        const bool test_return = getenv("MELEE_DIAGNOSTIC_CONGRATS_RETURN") != NULL;
        ++calls;
        /* Exercise the original post-lock Start path once, then release it.
         * The production input-delay, timer, and scene routing remain intact. */
        if (test_return && calls >= 120 && calls < 124) {
            neutral.button = PAD_BUTTON_START;
        }
        HSD_PadStageDiagnosticInput(0, &neutral);
#if defined(_WIN32)
        if (calls == 1 && !test_return) {
            char* path = NULL;
            size_t size = 0;
            int configured =
                _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_COMPOSITED_PRESENT", "1") == 0 &&
                _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_VI", "1") == 0 &&
                _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_GPU_SNAPSHOT", "") == 0;
            OSReport("NATIVE_CONGRATS_CAPTURE_CONFIG scene_entry_only=%d\n", configured);
            if (_dupenv_s(&path, &size, "MELEE_DIAGNOSTIC_POSTCLEAR_CAPTURE") == 0 && path && path[0]) {
                OSReport("NATIVE_CONGRATS capture_armed=%d\n",
                         _putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE", path) == 0);
            }
            free(path);
        }
#endif
        if (calls == 1 || calls % 60 == 0) {
            OSReport("NATIVE_CONGRATS frame=%u input_delay=%u exit_timer=%u\n",
                     calls, gmRegend_InputDelayTimer, gmRegend_ExitTimer);
        }
        if (test_return && calls == 120) {
            OSReport("NATIVE_CONGRATS_RETURN_START_STAGED frame=%u button=%u\n",
                     calls, neutral.button);
        }
        if (test_return && calls > 120 && gmRegend_ExitTimer != 0) {
            static bool start_accepted;
            if (!start_accepted) {
                start_accepted = true;
                OSReport("NATIVE_CONGRATS_RETURN_START_ACCEPTED frame=%u exit_timer=%u\n",
                         calls, gmRegend_ExitTimer);
            }
        }
        if (calls == 300 && !test_return) {
            OSReport("NATIVE_CONGRATS_CLOSE frame=%u queued=%d\n", calls, aurora_diagnostic_queue_quit());
        }
    }
#endif
    if (gmRegend_InputDelayTimer != 0) {
        gmRegend_InputDelayTimer--;
    } else if (gmRegend_ExitTimer != 0) {
        gmRegend_ExitTimer--;
        if (gmRegend_ExitTimer == 0) {
            if (gm_GetCurrentGameMode() == GM_DEBUG_GOVER) {
                gm_801A6630(6);
            } else {
#if defined(TARGET_PC) && defined(_WIN32)
                if (getenv("MELEE_DIAGNOSTIC_CONGRATS_RETURN")) {
                    _putenv_s("MELEE_DIAGNOSTIC_CONGRATS_RETURN_PENDING", "1");
                    OSReport("NATIVE_CONGRATS_ORIGINAL_MENU_REQUEST mode=%d exit_timer=0\n",
                             gm_GetCurrentGameMode());
                }
#endif
                gm_801A6630(1);
            }
        }
    } else {
        if (gm_GetButtonsTriggered(gm_801BF010()) &
            (PAD_BUTTON_A | PAD_BUTTON_START))
        {
            lbBgFlash_8002063C(0x3C);
            gmRegend_ExitTimer = 0x3C;
            lbAudioAx_80023694();
            sfxForward();
        }
    }
}
