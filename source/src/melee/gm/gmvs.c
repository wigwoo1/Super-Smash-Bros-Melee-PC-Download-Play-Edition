#include "gmvs.h"
#ifdef TARGET_PC
#include <pc/replay_seed.h>
#include <pc/match_rules_digest.h>
#include <pc/local_match_agreement.h>
#include <sysdolphin/baselib/random.h>
#endif

#include <Runtime/platform.h>

#include <string.h>
#ifdef TARGET_PC
#include <stdlib.h>
#include <melee/ft/ft_0C88.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftMario/forward.h>
#include <melee/ft/kinds/ftKirby/forward.h>
#include <aurora/diagnostics.h>
#include <aurora/aurora.h>
#include <dolphin/gx/GXAurora.h>
#include <melee/ft/types.h>
#include <dolphin/os.h>
extern unsigned int HSD_JObjDiagnosticMatchFrame;
#endif

#include "forward.h"
#include "gm_1884.h"
#include "gm_18A1.h"
#include "gm_unsplit.h"
#include "gmmain_lib.h"
#include "gmmultiman.h"
#include "gmpause.h"
#include "types.h"
#include <dolphin/pad.h>
#include <melee/cm/camera.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/ft/ftdevice.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/fighter.h>
#include <melee/gr/ground.h>
#include <melee/gr/grpstadium.h>
#include <melee/gr/stage.h>
#include <melee/if/if_2F6E.h>
#include <melee/if/ifall.h>
#include <melee/if/ifhazard.h>
#include <melee/if/ifnametag.h>
#include <melee/if/ifstatus.h>
#include <melee/if/ifstock.h>
#include <melee/if/iftime.h>
#include <melee/it/item.h>
#include <melee/it/itspawn.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/lb_0219.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbrefract.h>
#include <melee/lb/lbtime.h>
#include <melee/mn/types.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonus.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobjproc.h>
#ifdef TARGET_PC
#include <pc/target_route_diagnostic.h>
#include <pc/figure_catch_diagnostic.h>
#include <pc/adventure_goomba_diagnostic.h>
#include <pc/adventure_koopa_diagnostic.h>
#endif

static u32 lbl_803D5620[] = {
    0x7C859,
    0x7C85A,
    0x7C85B,
    0x7C85C,
};

static u32 lbl_803D5630[] = {
    U32_MAX, 0x7C858, 0x7C862, 0x7C85F, 0x7C854, 0x7C853,
};

/* 46B6A0 */ static VsSceneController controller;

VsSceneController* gmVs_GetSceneController(void)
{
    return &controller;
}

VsSceneState* gmVs_GetSceneState(void)
{
    return &controller.state;
}

struct StartMeleeRules* gm_GetStartMeleeRules(void)
{
    return &controller.start;
}

struct lbl_8046B488_t* fn_8016AE60(void)
{
    return gm_1601_GetUnkData();
}

bool gm_8016AE80(void)
{
    return gm_GetStartMeleeRules()->item_freq;
}

f32 gm_8016AE94(void)
{
    return gmVs_GetSceneController()->state.unk_34;
}

u64 gm_8016AEA4(void)
{
    return gm_GetStartMeleeRules()->x20;
}

s32 gm_8016AEB8(void)
{
    return gm_GetStartMeleeRules()->x28;
}

bool gm_8016AEC8(void)
{
    return gm_GetStartMeleeRules()->sd_penalty;
}

#ifdef TARGET_PC
static bool gmvs_diagnostic_quiet_runtime(void)
{
    static int quiet = -1;
    if (quiet < 0) {
        quiet = getenv("MELEE_DIAGNOSTIC_QUIET_RUNTIME") != NULL;
    }
    return quiet != 0;
}
#endif

u32 gm_GetFrameCount(void)
{
    return gmVs_GetSceneController()->state.frame_count;
}

u32 gm_8016AEEC(void)
{
    return gmVs_GetSceneController()->state.timer_seconds;
}

u16 gm_8016AEFC(void)
{
    return gmVs_GetSceneController()->state.unk_2C;
}

u16 gm_8016AF0C(void)
{
    int tmp = 99.0F * gmVs_GetSceneController()->state.unk_2C / 59.0F;
    u16 var_r31 = tmp;
    if (!gm_8016B110()) {
        var_r31 = 0x63 - var_r31;
    }
    return var_r31;
}

bool GetMatchTimer(int* arg0)
{
    VsSceneController* data = gmVs_GetSceneController();
    if (arg0 != NULL && data->start.timer_enabled) {
        if (gm_8016B110()) {
            *arg0 = data->start.time_limit - data->state.timer_seconds;
        } else {
            *arg0 = data->state.timer_seconds;
        }
        return true;
    }
    return false;
}

u16 gm_GetStKind(void)
{
    return gm_GetStartMeleeRules()->stkind;
}

bool gm_8016B014(void)
{
    int i;
    PAD_STACK(4);

    if (gm_GetStartMeleeRules()->x4_2 && !gm_8016A1F8()) {
        for (i = 1; i < 6; i++) {
            if (Player_GetPlayerState(i) != 0) {
                i = -1;
                break;
            }
        }
        if (i > 0) {
            return true;
        }
    }
    return false;
}

bool gm_8016B094(void)
{
    return gm_GetStartMeleeRules()->match_kind == 1;
}

bool gm_8016B0B4(void)
{
    return gm_GetStartMeleeRules()->match_kind == 2;
}

bool gm_8016B0D4(void)
{
    return gm_GetStartMeleeRules()->friendly_fire;
}

bool gm_8016B0E8(void)
{
    return gm_GetStartMeleeRules()->x4_2;
}

bool gm_8016B0FC(void)
{
    return gm_GetStartMeleeRules()->single_button;
}

bool gm_8016B110(void)
{
    return gm_GetStartMeleeRules()->timer_counts_up;
}

bool gm_8016B124(void)
{
    return gm_GetStartMeleeRules()->x2_1;
}

void fn_8016B138(void)
{
    gm_GetStartMeleeRules()->is_teams = false;
}

bool gm_8016B14C(void)
{
    return gm_GetStartMeleeRules()->is_teams == false;
}

bool gm_8016B168(void)
{
    return gm_GetStartMeleeRules()->is_teams == true;
}

bool gm_8016B184(void)
{
    if (gmVs_GetSceneController()->state.is_singleplayer == true) {
        return true;
    }
    return false;
}

bool gm_8016B1A8(void)
{
    return gm_GetStartMeleeRules()->x9 == 1;
}

bool gm_8016B1C4(void)
{
    return gm_GetStartMeleeRules()->x5_2;
}

bool gm_8016B1D8(void)
{
    return gm_GetStartMeleeRules()->x5_3;
}

bool gm_8016B1EC(void)
{
    return gmVs_GetSceneController()->state.unk_0 == 0;
}

bool gm_8016B204(void)
{
    StartMeleeRules* rules = gm_GetStartMeleeRules();
    if (rules->time_limit == 0 || !rules->timer_enabled) {
        return true;
    }
    return false;
}

bool gm_8016B238(void)
{
    return gm_GetStartMeleeRules()->x6;
}

f32 gm_8016B248(void)
{
    return gm_GetStartMeleeRules()->x30;
}

bool gm_8016B258(int arg0)
{
    return gmVs_GetSceneController()->state.fighters[arg0].x4.x0.x4_b3;
}

void gm_SetGameSpeed(float speed)
{
    lb_80019880(OSSecondsToTicks(1 / 60.0F / speed));
}

void gm_ResetGameSpeed(void)
{
    lb_80019880(OSSecondsToTicks(1 / 60.0F / controller.start.game_speed));
}

void gm_8016B328(void)
{
    gmVs_GetSceneController()->state.terminate_match = 1;
}

void gm_8016B33C(int arg0)
{
    gmVs_GetSceneController()->state.unk_B = arg0 + 1;
}

void gm_8016B350(int arg0)
{
    gmVs_GetSceneController()->state.unk_10 = arg0 + 1;
}

void gm_8016B364(int arg0)
{
    gmVs_GetSceneController()->state.unk_14 = arg0 + 1;
}

void gm_8016B378(s8 arg0)
{
    gmVs_GetSceneController()->state.unk_18 = arg0;
}

void fn_8016B388(int arg0, int arg1)
{
    gmVs_GetSceneController()->state.fighters[arg0].x6 = arg1;
}

bool gm_8016B3A0(void)
{
    if (gm_GetCurrentGameMode() == GM_SUPER_SUDDEN_DEATH_VS) {
        return true;
    }
    return false;
}

bool gm_8016B3D8(void)
{
    switch (gm_GetCurrentGameMode()) {
    case GM_CLASSIC:
    case GM_ADVENTURE:
    case GM_ALLSTAR:
        return true;
    default:
        return false;
    }
}

bool gm_IsCurrently1PMode_inline(void)
{
    switch (gm_GetCurrentGameMode()) {
    case GM_CLASSIC:
    case GM_ADVENTURE:
    case GM_ALLSTAR:
    case GM_TARGET_TEST:
    case GM_TRAINING:
    case GM_HOME_RUN_CONTEST:
    case GM_10MAN_VS:
    case GM_100MAN_VS:
    case GM_3MIN_VS:
    case GM_15MIN_VS:
    case GM_ENDLESS_VS:
    case GM_CRUEL_VS:
    case GM_EVENT:
        return true;
    default:
        return false;
    }
}

static float get_unk_float(void)
{
    if (gm_IsCurrently1PMode_inline()) {
        return 1.0F;
    } else {
        switch (gm_8016B558()) {
        case 2:
            return 1.0F;
        case 3:
            return 1.1F;
        case 4:
            return 1.2F;
        default:
            return 1.0F;
        }
    }
}

bool gm_IsCurrently1PMode(void)
{
    return gm_Is1PMode(gm_GetCurrentGameMode());
}

bool fn_8016B4BC(void)
{
    PAD_STACK(4);
    {
        s32 spC;
        s32 sp8;

        if (gm_8016B1A8()) {
            Ground_801C1DE4(&sp8, &spC);
            if (sp8 == 0) {
                return true;
            }
        }
        return false;
    }
}

bool fn_8016B510(void)
{
    if (gm_GetStartMeleeRules()->x9 == 2 && Ground_801C1D98()) {
        return true;
    }
    return false;
}

int gm_8016B558(void)
{
    int i;
    int count;
    count = 0;
    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
            count++;
        }
    }
    return count;
}

float fn_8016B5B0(void)
{
    VsSceneController* vsscene = &controller;
    int count;
    int i;
    PAD_STACK(4);

    if (gm_IsCurrently1PMode_inline()) {
        float var_f1 = 3.0F;
        if (controller.state.unk_0 != 0) {
            if (vsscene->state.match_result == OUTCOME_TIMEOUT ||
                vsscene->state.match_result == OUTCOME_1P_GAME_OVER)
            {
                var_f1 *= 1.1F;
            }
        }
        return var_f1;
    }

    count = 0;
    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
            count++;
        }
    }

    switch (count) {
    case 2:
        return 0.9F;
    case 3:
        return 0.8F;
    case 4:
        return 0.7F;
    default:
        return 0.0F;
    }
}

void gm_8016B6E8(int arg0, int arg1)
{
    VsSceneController* tmp = gmVs_GetSceneController();
    if (arg0 == 0) {
        tmp->state.unk_2E = lbTime_8000AF24(tmp->state.unk_2E, arg1);
    }
}

u8 fn_8016B728(void)
{
    return gmVs_GetSceneController()->state.unk_C;
}

void fn_8016B738(int arg0)
{
    VsSceneController* tmp = gmVs_GetSceneController();
    tmp->state.unk_C = lbTime_8000AF74(tmp->state.unk_C, arg0);
}

MatchEnd* gm_8016B774(void)
{
    return &controller.state.x24C;
}

void fn_8016B784(int arg0)
{
    gmVs_GetSceneController()->state.hud_enabled = 1;
    grStadium_801D4150();
}

void fn_8016B7B4(int arg0)
{
    Ground_801C1154();
    if (arg0 == 3) {
        grStadium_801D4084();
    } else {
        grStadium_801D410C();
    }
    un_802FD404();
}

void fn_8016B7F8(int arg0)
{
    VsSceneController* tmp = gmVs_GetSceneController();
    PAD_STACK(4);

    ftLib_EnableAllInput();
    Stage_802252E4(tmp->start.stkind, NULL);
    grStadium_801D4040();
    if (!tmp->start.x1_3) {
        ifStatus_802F6EA4(4, -1, -1, 0, NULL, fn_8016B784);
    } else {
        ifStatus_802F6EA4(8, -1, -1, 0, NULL, fn_8016B784);
    }
    un_802FD428();
}

void fn_8016B88C(int arg0)
{
    Ground_801C1158();
    if (arg0 == 0) {
        grStadium_801D40C8();
    }
    gmVs_GetSceneController()->state.hud_enabled = 0;
}

void gm_8016B8D4(s32 arg0, u8 slot_type)
{
    gmVs_GetSceneController()->state.fighters[arg0].x1 = 0x82;
    gmVs_GetSceneController()->state.fighters[arg0].slot_type = slot_type;
    grStadium_801D3B4C(arg0, slot_type);
}

static inline int fn_8016B918_inline(int var_r31)
{
    int team = Player_GetTeam(var_r31);
    int i;
    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != 3 && var_r31 != i &&
            team == Player_GetTeam(i) && Player_GetStocks(i) > 1)
        {
            return i;
        }
    }
    return 6;
}

void fn_8016B918(void)
{
    HSD_PadStatus* temp_r3_3;
    int temp_r30;
    int temp_r3_2;
    bool var_r0;
    bool var_r0_2;
    int var_r28;
    int var_r31;
    int i;
    StartMeleeRules* rules;
    PAD_STACK(0x10);

    rules = gm_GetStartMeleeRules();
    if (!rules->is_vs) {
        return;
    }
    if (rules->is_teams != true) {
        return;
    }

    for (i = 0; i < GM_MAX_PLAYERS; i++) {
        if (Player_GetPlayerSlotType(i) != Gm_PKind_NA &&
            Player_GetEntity(i) != NULL && Player_8003219C(i) != 0 &&
            Player_GetStocks(i) == 0)
        {
            var_r0 = true;
        } else {
            var_r0 = false;
        }
        if (!var_r0) {
            continue;
        }
        temp_r3_2 = Player_GetPadPort(i);
        if (DbLevel >= DbLKind_DebugRom) {
            temp_r3_3 = &HSD_PadCopyStatus[(u8) temp_r3_2];
            if ((temp_r3_3->trigger & HSD_PAD_DPADUP) &&
                (temp_r3_3->button & HSD_PAD_X))
            {
                var_r0_2 = true;
            } else {
                var_r0_2 = false;
            }
        } else {
            var_r0_2 =
                HSD_PadCopyStatus[(u8) temp_r3_2].trigger & HSD_PAD_START;
        }
        if (var_r0_2 != 0) {
            var_r28 = fn_8016B918_inline(i);
            if (var_r28 != 6 && ifStock_802F7EFC(i, var_r28) == 0) {
                Player_LoseStock(var_r28);
                Player_SetStocks(i, Player_GetStocks(i) + 1);
                gm_GetMatchEndPlayerScore(i);
                fn_8016719C(i, 0);
            }
        }
    }
}

bool gm_AnyControllerPressedStart(void)
{
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        HSD_PadStatus* pad = &HSD_PadMasterStatus[(u8) i];
        if (pad->err == 0 && (pad->trigger & HSD_PAD_START)) {
            return true;
        }
    }
    return false;
}

bool gm_AnyControllerPressedZ(void)
{
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        HSD_PadStatus* pad = &HSD_PadMasterStatus[(u8) i];
        if (pad->err == 0 && (pad->trigger & HSD_PAD_Z)) {
            return true;
        }
    }
    return false;
}

int gm_DefaultVSGetPauser(void)
{
    HSD_PadStatus* mpPadStatus;
    HSD_PadStatus* spPadStatus;
    bool spPausePressed;
    bool mpPausePressed;
    int mpPlayerId;
    int mpPlayerSlot;
    int spPlayerId;
    PAD_STACK(0x18);

    if (gm_IsCurrently1PMode_inline() ||
        gm_GetCurrentGameMode() == GM_CHALLENGER_APPROACH ||
        (gm_GetCurrentGameMode() == GM_VS &&
         gm_GetCurrentSceneIndex() == 0x81))
    {
        spPlayerId = Player_GetPadPort(0);
        spPadStatus = &HSD_PadCopyStatus[(u8) spPlayerId];
        if (spPadStatus->err == 0) {
            if (DbLevel >= DbLKind_DebugRom) {
                if ((spPadStatus->trigger & HSD_PAD_DPADUP) &&
                    (spPadStatus->button & HSD_PAD_X))
                {
                    spPausePressed = 1;
                } else {
                    spPausePressed = 0;
                }
            } else {
                spPausePressed = spPadStatus->trigger & HSD_PAD_START;
            }
            if (spPausePressed != 0) {
                return spPlayerId;
            }
        }
    } else {
        for (mpPlayerId = 0; mpPlayerId < PAD_MAX_CONTROLLERS; mpPlayerId++) {
            mpPadStatus = &HSD_PadCopyStatus[(u8) mpPlayerId];
            if (mpPadStatus->err == 0) {
                if (DbLevel >= DbLKind_DebugRom) {
                    if ((mpPadStatus->trigger & HSD_PAD_DPADUP) &&
                        (mpPadStatus->button & HSD_PAD_X))
                    {
                        mpPausePressed = 1;
                    } else {
                        mpPausePressed = 0;
                    }
                } else {
                    mpPausePressed = mpPadStatus->trigger & HSD_PAD_START;
                }
                if (mpPausePressed) {
                    for (mpPlayerSlot = 0; mpPlayerSlot < 6; mpPlayerSlot++) {
                        if (Player_GetPlayerSlotType(mpPlayerSlot) !=
                                Gm_PKind_NA &&
                            Player_GetEntity(mpPlayerSlot) != NULL &&
                            Player_8003219C(mpPlayerSlot) == 0 &&
                            mpPlayerId == Player_GetPadPort(mpPlayerSlot))
                        {
                            return mpPlayerId;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int gm_CameraModeVSGetPauser(void)
{
    HSD_PadStatus* temp_r3;
    bool var_r0;
    int var_r30;
    int var_r29;
    PAD_STACK(0x10);

    for (var_r30 = 0; var_r30 < PAD_MAX_CONTROLLERS; var_r30++) {
        temp_r3 = &HSD_PadCopyStatus[(u8) var_r30];
        if (temp_r3->err == 0) {
            if (DbLevel >= DbLKind_DebugRom) {
                if ((temp_r3->trigger & HSD_PAD_DPADUP) &&
                    (temp_r3->button & HSD_PAD_X))
                {
                    var_r0 = 1;
                } else {
                    var_r0 = 0;
                }
            } else {
                var_r0 = temp_r3->trigger & HSD_PAD_START;
            }
            if (var_r0 != 0) {
                if (var_r30 == 3) {
                    return var_r30;
                }
                for (var_r29 = 0; var_r29 < 6; var_r29++) {
                    if (var_r30 == Player_GetPadPort(var_r29) &&
                        Player_GetPlayerSlotType(var_r29) != Gm_PKind_NA)
                    {
                        return var_r30;
                    }
                }
            }
        }
    }
    return -1;
}

MatchOutcome gm_GetFFAOutcome(void)
{
    bool isSingleplayer;
    int var_r0_2;
    bool notSingleplayer;
    VsSceneController* tmp = &controller;
    PAD_STACK(0x8);

    if (controller.start.x4_2 && Player_GetP1Stock() == 0) {
        return OUTCOME_1P_GAME_OVER;
    }
    if (controller.state.is_singleplayer == 1) {
        isSingleplayer = true;
    } else {
        isSingleplayer = false;
    }
    if (isSingleplayer && fn_8016A1E4() != 0 && fn_801693A8() == 0) {
        var_r0_2 = 5;
    } else {
        var_r0_2 = 0;
    }
    if (var_r0_2 != 0) {
        return OUTCOME_UNK_1P_HORDE_BATTLE_VICTORY;
    }
    if (controller.state.is_singleplayer == 0) {
        notSingleplayer = 1;
    } else {
        notSingleplayer = 0;
    }
    if (notSingleplayer != 0 && !controller.start.x5_1 &&
        (controller.start.match_kind == 1 || tmp->start.x4_2))
    {
        int i;
        int eliminatedPlayers = 0;
        int playerCount = 0;
        for (i = 0; i < 6; i++) {
            if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
                if (Player_GetStocks(i) == 0) {
                    eliminatedPlayers += 1;
                }
                playerCount += 1;
            }
        }

        if (eliminatedPlayers >= playerCount - 1) {
            return OUTCOME_ELIMINATION;
        }
    }
    return OUTCOME_NONE;
}

MatchOutcome gm_GetTeamBattleOutcome(void)
{
    int i;
    bool isSingleplayer;
    int var_r0_2;
    bool notSinglePlayer;
    bool notSinglePlayer_2;
    int slot;
    int eliminatedTeamCount;
    int enemyStocksCount;
    int teamCount;
    int enemyCharacterCount;
    int slot_2;

    s16 teamStocks[5];
    PAD_STACK(0xC);

    if (controller.start.x4_2) {
        enemyStocksCount = 0;
        enemyCharacterCount = 0;
        if (Player_GetP1Stock() == 0) {
            return OUTCOME_1P_GAME_OVER;
        }
        if (controller.state.is_singleplayer == 1) {
            isSingleplayer = true;
        } else {
            isSingleplayer = false;
        }
        if (isSingleplayer && fn_8016A1E4() != 0 && fn_801693A8() == 0) {
            var_r0_2 = 5;
        } else {
            var_r0_2 = 0;
        }
        if (var_r0_2 != 0) {
            return OUTCOME_UNK_1P_HORDE_BATTLE_VICTORY;
        }
        if (controller.state.is_singleplayer == 0) {
            notSinglePlayer = true;
        } else {
            notSinglePlayer = false;
        }
        if (notSinglePlayer && !controller.start.x5_1) {
            for (slot = 0; slot < 6; slot++) {
                if (Player_GetFlagsBit1(slot) == 0 &&
                    Player_GetPlayerSlotType(slot) != Gm_PKind_NA &&
                    Player_GetEntity(slot) != NULL)
                {
                    if (Player_GetTeam(slot) != Player_GetTeam(0)) {
                        enemyStocksCount += Player_GetStocks(slot);
                        enemyCharacterCount += 1;
                    }
                }
            }
            if (enemyCharacterCount != 0 && enemyStocksCount == 0) {
                return OUTCOME_TEAM_ELIMINATION;
            }
        }
    }
    if (controller.state.is_singleplayer == 0) {
        notSinglePlayer_2 = true;
    } else {
        notSinglePlayer_2 = false;
    }
    if (notSinglePlayer_2 && controller.start.match_kind == 1) {
        teamCount = 0;
        eliminatedTeamCount = 0;
        memset(teamStocks, -1, sizeof(teamStocks));

        for (slot_2 = 0; slot_2 < 6; slot_2++) {
            if (Player_GetPlayerSlotType(slot_2) != Gm_PKind_NA) {
                if (teamStocks[Player_GetTeam(slot_2)] == -1) {
                    teamStocks[Player_GetTeam(slot_2)] =
                        Player_GetStocks(slot_2);
                    teamCount++;
                } else {
                    teamStocks[Player_GetTeam(slot_2)] +=
                        Player_GetStocks(slot_2);
                }
            }
        }

        for (i = 0; i < 5; i++) {
            if (teamStocks[i] == 0) {
                eliminatedTeamCount++;
            }
        }
        if (eliminatedTeamCount >= teamCount - 1) {
            return OUTCOME_TEAM_ELIMINATION;
        }
    }
    return OUTCOME_NONE;
}

MatchOutcome gm_GetMatchOutcome(void)
{
    MatchOutcome team_battle_outcome;
    MatchOutcome ffa_outcome;
    VsSceneController* tmp = &controller;
    PAD_STACK(0x8);

    if (controller.state.match_result != OUTCOME_NONE) {
        return controller.state.match_result;
    }
    if (controller.state.terminate_match == 1) {
        return OUTCOME_TERMINATED;
    }
    if (controller.start.x5_0) {
        return OUTCOME_NONE;
    }
    if (tmp->start.timer_enabled) {
        if (tmp->start.timer_counts_up) {
            if (controller.state.timer_seconds ==
                    controller.start.time_limit &&
                controller.state.unk_2C == 0)
            {
                return OUTCOME_TIMEOUT;
            }
        } else {
            if (controller.state.timer_seconds == 0 &&
                controller.state.unk_2C == 0x3B)
            {
                return OUTCOME_TIMEOUT;
            }
        }
    }
    if (!controller.start.is_teams) {
        ffa_outcome = gm_GetFFAOutcome();
        if (ffa_outcome != OUTCOME_NONE) {
            return ffa_outcome;
        }
    }
    if (controller.start.is_teams == true) {
        team_battle_outcome = gm_GetTeamBattleOutcome();
        if (team_battle_outcome != 0) {
            return team_battle_outcome;
        }
    }
    if (Ground_801C1D84() != 0) {
        return OUTCOME_UNK_1P_BONUS_STAGE_END;
    }
    return OUTCOME_NONE;
}

void fn_8016C46C(int arg0)
{
    if (gmVs_GetSceneController()->state.unk_9 != 0) {
        switch (gm_GetCurrentGameMode()) {
        case GM_CLASSIC:
            fn_8017EE40(arg0);
            return;
        case GM_ADVENTURE:
            fn_8017E8A4(arg0);
            return;
        case GM_ALLSTAR:
            fn_8018A364(arg0);
            break;
        }
    }
}

static inline void fn_8016C46C_dontinline(int arg0)
{
    fn_8016C46C(arg0);
}

void fn_8016C4F4(MatchEnd* arg0)
{
    struct UnkResultPlayerData* x44C = arg0->x44C;
    int i;
    int j;

    for (i = 0; i < 6; i++) {
        if (arg0->player_standings[i].pkind != 3) {
            pl_80039450(i);
            for (j = 0; j < 0x101; j++) {
                x44C[i].x0[j] = fn_80170110(arg0, j, 2, i);
                if (x44C[i].x0[j] != 0) {
                    x44C[i].x104[j] = fn_8016FAD4(arg0, fn_8016F1F0(j), 2, i);
                } else {
                    x44C[i].x104[j] = 0;
                }
            }
        }
    }
}

int gm_8016C5C0(int pl_slot)
{
    MatchEnd* tmp = gm_8016B774();
    PAD_STACK(8);

    if (tmp->x0 != gm_801A4BA8()) {
        tmp->x0 = gm_801A4BA8();
        gm_80166378(tmp);
    }
    if (!gm_GetStartMeleeRules()->is_teams) {
        return tmp->player_standings[pl_slot].is_big_loser;
    }
    return tmp->team_standings[tmp->player_standings[pl_slot].team]
        .is_big_loser;
}

int gm_GetMatchEndPlayerScore(int pl_slot)
{
    MatchEnd* match_end;
    PAD_STACK(8);

    match_end = gm_8016B774();
    if (match_end->x0 != gm_801A4BA8()) {
        match_end->x0 = gm_801A4BA8();
        gm_80166378(match_end);
    }
    return match_end->player_standings[pl_slot].score;
}

s32 gm_8016C6C0(HSD_GObj* arg0)
{
    PAD_STACK(0x10);
    return gm_8016C5C0(ftLib_GetPlayerIndex(arg0));
}

int gm_8016C75C(HSD_GObj* player)
{
    MatchEnd* match_end;
    PAD_STACK(8);

    match_end = gm_8016B774();
    if (match_end->x0 != gm_801A4BA8()) {
        match_end->x0 = gm_801A4BA8();
        gm_80166378(match_end);
    }
    return match_end->player_standings[ftLib_GetPlayerIndex(player)].x20;
}

void fn_8016C7D0(HSD_GObj* gobj)
{
    fn_80171DC4();
}

void fn_8016C7F0(void)
{
    s32 sp24;
    s32 sp20;
    u8 _[0x18];

    u32* temp_r30_2;
    int var_r29;
    u8 var_r29_2;
    int var_r28;
    u8* temp_r29_2;
    UnkAllstarData* temp_r30;

    if (controller.start.on_match_end != NULL) {
        controller.start.on_match_end(
            (MatchOutcome) controller.state.match_result);
    }
    HSD_PadRumbleRemoveAll();
    for (var_r29 = 0; var_r29 < PAD_MAX_CONTROLLERS; var_r29++) {
        HSD_PadRumbleOn(var_r29);
    }
    if (!controller.start.x1_5) {
        lbAudioAx_800236DC();
    }
    lbAudioAx_80023694();
    lbAudioAx_80024C84();
    ifTime_FreeCountdown();
    if (controller.state.match_result == OUTCOME_1P_GAME_OVER &&
        controller.start.x4_3)
    {
        gm_80167858(Player_GetPadPort(0), Player_GetNametagSlotID(0), 0xD, 0);
        Camera_RequestQuake(QuakeKind_Large, NULL);
    }
    if (gm_GetCurrentGameMode() == GM_TARGET_TEST ||
        (gm_GetCurrentGameMode() == GM_CLASSIC &&
         controller.start.stkind >= 0x21 && controller.start.stkind <= 0x3A))
    {
        temp_r29_2 = (u8*) gm_801B6320();
        temp_r30 = gm_GetAllStarData();
        var_r28 = 0;
        if (gm_GetCurrentGameMode() == GM_TARGET_TEST) {
            var_r29_2 = gm_CKindToSelKind(*temp_r29_2);
        } else {
            var_r29_2 = gm_CKindToSelKind(temp_r30->x0.x0.ckind);
        }
        temp_r30_2 = gmMainLib_8015D438(var_r29_2);
        gmMainLib_8015D450(var_r29_2);
        Ground_801C1DE4(&sp24, &sp20);
        if (gmMainLib_8015D48C(var_r29_2) == 0) {
            if (sp24 == 0) {
                var_r28 = 1;
            }
        } else if (sp24 == 0 && *temp_r30_2 > controller.state.frame_count) {
            var_r28 = 1;
        }
        if (var_r28 != 0) {
            controller.state.unk_10 = 0x9C41;
            controller.state.unk_14 = 0x145;
            gm_80167858(Player_GetPadPort(0), Player_GetNametagSlotID(0), 0xD,
                        0x5A);
        } else if (sp24 > 0) {
            controller.state.unk_14 = 0x149;
            controller.state.unk_18 = 0x28;
        }
    }
    if (gm_GetCurrentGameMode() == GM_EVENT &&
        controller.state.unk_10 == 0x9C41)
    {
        gm_80167858(Player_GetPadPort(0), Player_GetNametagSlotID(0), 0xD,
                    0x5A);
    }
    switch (gm_GetCurrentGameMode()) {
    case GM_10MAN_VS:
    case GM_100MAN_VS:
    case GM_3MIN_VS:
    case GM_15MIN_VS:
    case GM_ENDLESS_VS:
    case GM_CRUEL_VS:
        fn_80182B5C();
    }
}

static inline s8 gm_GetSlotByPlayerId(int pauserId)
{
    int slot;
    for (slot = 0; slot < 6; slot++) {
        if (pauserId == Player_GetPadPort(slot) &&
            Player_GetPlayerSlotType(slot) == Gm_PKind_Human)
        {
            return slot;
        }
    }
    return -1;
}

void gm_DoPauseChecksAndRoutine(VsSceneController* arg0, int arg1)
{
    int pauser;
    s8 pauserSlot;
    u8 var_r4;

    if (arg0->state.unpause_timer == 0 && arg0->state.hud_enabled != 0 &&
        !arg0->start.disable_pausing)
    {
        if (arg0->start.check_for_pauser_override != NULL) {
            pauser = arg0->start.check_for_pauser_override();
        } else {
            pauser = gm_DefaultVSGetPauser();
        }
        if (pauser != -1) {
            lbAudioAx_80024E84(1);
            lbAudioAx_80024030(5);
            ifAll_802F3394();
            if (arg0->start.x3_7) {
                ifTime_ShowTimers();
            }
            if (!arg0->start.x3_6) {
                var_r4 = 0;
                if (arg0->start.x3_5) {
                    var_r4 |= 1;
                }
                if (arg0->start.x3_3) {
                    var_r4 |= 2;
                }
                if (arg0->start.x3_1) {
                    var_r4 |= 4;
                }
                gm_801A0FEC(pauser, var_r4);
            }
            gm_SetDbPauseFlag((s64) arg1);
            if (arg0->start.x4_0) {
                pauserSlot = gm_GetSlotByPlayerId(pauser);
                if (arg0->start.on_pause_override != NULL) {
                    arg0->start.on_pause_override(pauserSlot);
                } else {
                    gm_EnablePlayerPauseCamera(pauserSlot, pauser);
                }
            }
            HSD_PadRumblePauseAll();
            arg0->state.pauser = pauser;
            arg0->state.pause_timer = 0xA;
        }
    }
}

/**
 * @brief Returns the player id pressing unpause, or @c -1 if none.
 *
 * In retail builds, checks for a newly pressed Start button. When @c DbLevel
 * >= DbLKind_DebugRom, accepts D-Pad Up (@c HSD_PAD_DPADUP, @c 1 << 3) plus X
 * instead.
 */
static inline int gm_GetPlayerPressingUnpause(void)
{
    HSD_PadStatus* pad;
    bool var_r0;
    int playerId;
    for (playerId = 0; playerId < PAD_MAX_CONTROLLERS; playerId++) {
        pad = &HSD_PadCopyStatus[(u8) playerId];
        if (pad->err == 0) {
            if (DbLevel >= DbLKind_DebugRom) {
                if ((pad->trigger & HSD_PAD_DPADUP) &&
                    (pad->button & HSD_PAD_X))
                {
                    var_r0 = true;
                } else {
                    var_r0 = false;
                }
            } else {
                var_r0 = pad->trigger & HSD_PAD_START;
            }
            if (var_r0) {
                return playerId;
            }
        }
    }
    return -1;
}

void gm_DoUnpauseChecksAndRoutine(VsSceneController* arg0, int arg1)
{
    int i;
    PAD_STACK(0x14);

    if (arg0->state.pause_timer != 0) {
        return;
    }
    if (arg0->state.hud_enabled == 0) {
        return;
    }
    if (arg0->start.disable_pausing) {
        return;
    }

    i = gm_GetPlayerPressingUnpause();

    if (i != -1 && i == arg0->state.pauser) {
        lbAudioAx_80024E84(0);
        gm_ClearDbPauseFlag(arg1);
        ifAll_802F33CC();
        gm_801A10FC(i);
        HSD_PadRumbleUnpauseAll();
        if (arg0->start.x4_0) {
            if (arg0->start.on_unpause_override != NULL) {
                arg0->start.on_unpause_override(i);
            } else {
                Ground_EnableMatchCamera();
            }
        }
        arg0->state.unpause_timer = 0xA;
    }
}

void fn_8016CD98(VsSceneController* scene)
{
    s32 temp_r3_4;
    s32 var_r3;
    u16 temp_r3_2;
    u8 temp_r0_2;
    int i;
    PAD_STACK(4);

    if (scene->state.hud_enabled == 0) {
        return;
    }

    if (gm_801A4624() == 0 || gm_801A46B8(0) != 0 || scene->start.x3_7 == 1) {
        if (scene->state.frame_count < U32_MAX) {
            scene->state.frame_count++;
        }
        if (scene->start.timer_enabled) {
            if (++scene->state.unk_2C >= 60) {
                scene->state.unk_2C = 0;
                if (scene->start.timer_counts_up) {
                    if (scene->state.timer_seconds < U32_MAX) {
                        scene->state.timer_seconds++;
                    }
                } else {
                    if (scene->state.timer_seconds != 0) {
                        if (scene->state.timer_seconds < 6 &&
                            ifTime_IsTimerHidden() == 0)
                        {
                            temp_r3_4 =
                                controller.state
                                    .unk_1C[scene->state.timer_seconds];
                            if (temp_r3_4 >= 0) {
                                lbAudioAx_800237A8(temp_r3_4, 0x7FU, 0x40U);
                            }
                        }
                        scene->state.timer_seconds -= 1;
                    }
                }
            }
        }
        for (i = 0; i < 4; i++) {
            if (scene->state.fighters[i].x1 != 0) {
                scene->state.fighters[i].x1--;
                temp_r0_2 = scene->state.fighters[i].x1;
                if (temp_r0_2 == 0x50) {
                    if (scene->state.fighters[i].slot_type == Gm_PKind_Human) {
                        var_r3 = scene->state.unk_20[i];
                    } else {
                        var_r3 = 0x7C865;
                    }
                    lbAudioAx_80023870(var_r3, 0x7F, 0x40, 0x19);
                } else if (temp_r0_2 == 0) {
                    lbAudioAx_80023870(0x7C852, 0x7F, 0x40, 0x19);
                }
            }
        }
    }
}

void fn_8016CF4C(int slot, MatchOutcome matchResult)
{
    gm_801A10FC(slot);
    controller.state.match_result = matchResult;
    if (matchResult != OUTCOME_RETRY && DbLevel >= DbLKind_DebugRom) {
        gm_ClearDbPauseFlag(1);
        if (controller.start.xD > 1) {
            controller.state.unk_30 = controller.start.xD - 1;
        }
        fn_8016C7F0();
        lbAudioAx_80024D50();
        gm_SetDbPauseFlag(4);
        controller.state.unk_0 = 1;
        return;
    }
    fn_8016C7F0();
    gm_801A4B60();
}

static inline void fn_8016CF4C_dontinline(int slot, MatchOutcome matchResult)
{
    fn_8016CF4C(slot, matchResult);
}

static inline void fn_8016CFE0_inline(void)
{
    int i;
    VsSceneController* tmp;

    if (DbLevel >= DbLKind_DebugRom && gm_801A46B8(0) != 0 &&
        (HSD_PadCopyStatus->button &
         (HSD_PAD_L | HSD_PAD_R | HSD_PAD_A | HSD_PAD_START)) ==
            (HSD_PAD_L | HSD_PAD_R | HSD_PAD_A | HSD_PAD_START))
    {
        fn_8016C7F0();
        gm_801A4B60();
    }

    tmp = &controller;
    for (i = 0; i < 6; i++) {
        if (tmp->state.fighters[i].respawn_timer != 0) {
            if (--tmp->state.fighters[i].respawn_timer == 0) {
                fn_8016719C(i, tmp->state.fighters[i].xB);
            }
        }
    }
}

void fn_8016CFE0(void)
{
    VsSceneController* tmp = &controller;
    int tmp_btns;
    s64 no_contest_buttons;
    int unpauser_slot;
    PAD_STACK(0x10);

    fn_8016CFE0_inline();

    fn_8016A4C8();
    fn_8016758C();
    if (gm_GetDbPauseFlag(1) != 0) {
        unpauser_slot = gm_GetPlayerPressingUnpause();
        if (DbLevel >= DbLKind_DebugRom) {
            tmp_btns = PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_A;
        } else {
            tmp_btns = PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_A |
                       PAD_BUTTON_START;
        }
        no_contest_buttons = tmp_btns;
        if (tmp->start.x3_4 && tmp->state.pause_timer == 0 &&
            tmp->state.unk_3 == 0)
        {
            u64 buttons = gm_GetButtonsTriggered(tmp->state.pauser);
            if ((no_contest_buttons & buttons) != 0) {
                buttons = gm_GetButtonsPressed(tmp->state.pauser);
                {
                    u64 masked_buttons = no_contest_buttons & buttons;
                    if (masked_buttons == no_contest_buttons) {
                        fn_8016CF4C_dontinline(unpauser_slot,
                                               OUTCOME_NO_CONTEST);
                        return;
                    }
                }
            }
        }
        if (tmp->start.x3_2 && tmp->state.pause_timer == 0 &&
            tmp->state.unk_3 == 0)
        {
            if ((gm_GetButtonsTriggered(tmp->state.pauser) & PAD_TRIGGER_Z) !=
                0)
            {
                fn_8016CF4C_dontinline(unpauser_slot, OUTCOME_RETRY);
                return;
            }
        }
        gm_DoUnpauseChecksAndRoutine(tmp, 1);
        if (tmp->state.pause_timer != 0) {
            tmp->state.pause_timer--;
        }
        goto block_50;
    } else {
        tmp->state.match_result = gm_GetMatchOutcome();
        if (tmp->state.match_result == OUTCOME_NONE) {
            gm_DoPauseChecksAndRoutine(tmp, 1);
            if (tmp->state.unpause_timer != 0) {
                tmp->state.unpause_timer--;
            }
        } else {
            goto block_51;
        }
    block_50:
        fn_8016B918();
        tmp->state.match_result = gm_GetMatchOutcome();
    }
    if (tmp->state.match_result != OUTCOME_NONE) {
    block_51:
        fn_8016C7F0();
        ifStatus_802F7034(fn_8016B88C);
        lbAudioAx_80024D50();
        gm_SetDbPauseFlag(4);
        tmp->state.unk_0 = 1;
    } else {
        fn_8016CD98(tmp);
    }
}

static inline int fn_8016CBE8_inline(void)
{
    HSD_PadStatus* pad;
    bool var_r0;
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        pad = &HSD_PadCopyStatus[(u8) i];
        if (pad->err == 0) {
            if (DbLevel >= DbLKind_DebugRom) {
                if ((pad->trigger & HSD_PAD_DPADUP) &&
                    (pad->button & HSD_PAD_X))
                {
                    var_r0 = true;
                } else {
                    var_r0 = false;
                }
            } else {
                var_r0 = pad->trigger & HSD_PAD_START;
            }
            if (var_r0) {
                return i;
            }
        }
    }
    return -1;
}

void gm_Scene_Training_OnFrame(void)
{
    int i;
    VsSceneController* tmp = &controller;
    PAD_STACK(0x28);

    fn_8016CFE0_inline();

    fn_8016758C();
    if (gm_GetDbPauseFlag(2) != 0) {
        fn_8016CBE8_inline();
        gm_DoUnpauseChecksAndRoutine(tmp, 2);
        if (tmp->state.pause_timer != 0) {
            tmp->state.pause_timer--;
        }
    } else {
        gm_DoPauseChecksAndRoutine(tmp, 2);
        if (tmp->state.unpause_timer != 0) {
            tmp->state.unpause_timer--;
        }
    }
    fn_8016CD98(tmp);
    tmp->state.match_result = gm_GetMatchOutcome();
    if (tmp->state.match_result != OUTCOME_NONE) {
        fn_8016C7F0();
        gm_801A4B60();
    }
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_TRAINING_MANUAL_INPUT")) {
        static unsigned calls;
        static int prior_pause = -1;
        int paused = gm_GetDbPauseFlag(2);
        ++calls;
        /* Passive trace: no PAD staging, auto-quit or game-state assignment. */
        if (calls % 60 == 0 || HSD_PadCopyStatus[0].trigger || paused != prior_pause) {
            Vec3 position = { 0 };
            HSD_GObj* fighter = Player_GetEntity(0);
            if (fighter) ftLib_GetPos(fighter, &position);
            OSReport("NATIVE_TRAINING_OS_INPUT call=%u frame=%u button=%u trigger=%u stick=%d,%d paused=%d menu=%u speed=%d x=%f y=%f\n",
                     calls, controller.state.frame_count,
                     HSD_PadCopyStatus[0].button, HSD_PadCopyStatus[0].trigger,
                     HSD_PadCopyStatus[0].stickX, HSD_PadCopyStatus[0].stickY,
                     paused, gm_80473814.x00, gm_80473814.menu_values[0],
                     position.x, position.y);
        }
        prior_pause = paused;
    } else if (getenv("MELEE_DIAGNOSTIC_TRAINING")) {
        static unsigned calls;
        PADStatus input = { 0 };
        HSD_GObj* fighter = Player_GetEntity(0);
        ++calls;
        if (calls >= 120 && calls < 140) input.stickX = 50;
        /* Allow the original turnaround animation before measuring left walk. */
        if (calls >= 180 && calls < 220) input.stickX = -50;
        if (!HSD_PadStageDiagnosticInput(0, &input))
            OSReport("NATIVE_TRAINING_PROBE input_refused=1 call=%u\n", calls);
        if (calls % 10 == 0 && fighter) {
            Vec3 position;
            ftLib_GetPos(fighter, &position);
            OSReport("NATIVE_TRAINING_PROBE call=%u frame=%u requested_x=%d pad_x=%d paused=%d motion=%d x=%f y=%f\n",
                     calls, controller.state.frame_count, input.stickX,
                     HSD_PadCopyStatus[0].stickX, gm_GetDbPauseFlag(2),
                     ftLib_GetMotionId(fighter), position.x, position.y);
        }
        if (calls == 60) {
            u32 width, height;
            AuroraGetRenderSize(&width, &height);
            OSReport("NATIVE_TRAINING_RENDER width=%u height=%u preset=%u\n",
                     width, height, aurora_get_display_preset());
        }
        if (calls == 300) {
            if (!aurora_diagnostic_queue_quit())
                OSPanic(__FILE__, __LINE__, "Training diagnostic Quit queue failed");
            OSReport("NATIVE_TRAINING_CLOSE call=%u frame=%u owner=1\n",
                     calls, controller.state.frame_count);
        }
    }
#endif
}

bool fn_8016D538(void)
{
    bool result = true;
    PAD_STACK(4);

    if (gm_8016B3D8()) {
        if (fn_8017DF90() == 0 && gm_GetCurrentSceneIndex() == 0x49) {
            result = true;
        } else {
            if (gm_8016B184() && fn_801693A8() != 0) {
                result = false;
            } else if (controller.start.x5_1) {
                result = false;
            }
        }
        if (fn_8017DF90() == 1 && (fn_8017DEC8(1)->x8 & 0x20)) {
            result = false;
        }
    }
    return result;
}

void fn_8016D634(void)
{
    VsSceneController* tmp = &controller;
    MatchEnd* dst;
    MatchEnd* copied_dst;

    PAD_STACK(8);

    if (controller.state.unk_30++ <= controller.start.xD) {
        return;
    }
    if (gm_8016B3D8()) {
        pl_80040688(0, controller.state.unk_D, fn_8016D538());
    }
    gm_801A4B1C();
    if (controller.start.x4_4 &&
        controller.state.match_result != OUTCOME_TIMEOUT &&
        (controller.start.x9 == 1 || Player_GetStocks(0) > 0))
    {
        un_802FD668();
        ifAll_802F3A64();
        gm_ClearDbPauseFlag(4);
        gm_SetDbPauseFlag(5);
        controller.state.unk_9 = 1;
        dst = &tmp->state.x24C;
        copied_dst = dst;
        if (controller.state.match_over == 0) {
            *copied_dst = tmp->state.x24C;
            copied_dst->is_teams = controller.start.is_teams;
            copied_dst->outcome = tmp->state.match_result;
            gm_80166378(copied_dst);
            fn_8016C46C_dontinline((int) copied_dst);
            if (tmp->state.match_result != OUTCOME_NO_CONTEST &&
                tmp->state.match_result != OUTCOME_RETRY)
            {
                fn_8016C4F4(copied_dst);
            }
            tmp->state.match_over = 1;
        } else {
            *copied_dst = tmp->state.x24C;
        }
        fn_80180630(tmp->start.x18, 0, tmp->start.x9, tmp->start.x4_5,
                    copied_dst);
        controller.state.unk_0 = 2;
    } else {
        controller.state.unk_0 = 3;
    }
}

#ifdef TARGET_PC
static unsigned diagnostic_vs_entries;
#endif

void gm_Scene_Vs_OnFrame(void)
{
    PAD_STACK(8);
#ifdef TARGET_PC
    /*
     * Result-screen renderer diagnostics should not wait through a tied
     * time-up and Sudden Death.  This opt-in harness hook drives the ordinary
     * game timeout condition after a bounded number of simulation frames.
     * It is deliberately gated by the native startup diagnostic flag and is
     * never active during normal play.
     */
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        getenv("MELEE_DIAGNOSTIC_FORCE_TIMEOUT") &&
        controller.start.timer_enabled && controller.state.unk_0 == 0)
    {
        static int timeout_forced;
        static unsigned force_frame;
        if (!force_frame) {
            const char* text = getenv("MELEE_DIAGNOSTIC_FORCE_TIMEOUT_FRAME");
            char* end = NULL;
            unsigned long requested = text ? strtoul(text, &end, 10) : 360;
            force_frame = text && end != text && !*end && requested >= 360 && requested <= 7200
                              ? (unsigned) requested : 360;
        }
        if (!timeout_forced && controller.state.frame_count >= force_frame &&
            controller.state.match_result == OUTCOME_NONE)
        {
            controller.state.timer_seconds = 0;
            controller.state.unk_2C = 0x3B;
            timeout_forced = 1;
            OSReport("NATIVE_DIAGNOSTIC_FORCE_TIMEOUT frame=%u outcome=timeout\n",
                     controller.state.frame_count);
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_PROFILE_LIFECYCLE") &&
        (controller.state.frame_count == 1 || controller.state.frame_count == 180 ||
         controller.state.frame_count == 360 || controller.state.frame_count == 600))
        OSReport("NATIVE_TEXTURE_PROFILE match_frame=%u pack=%u quality=%u pending=%d\n",
                 controller.state.frame_count, aurora_get_texture_pack(), aurora_get_texture_quality(),
                 aurora_is_texture_pack_pending());
    if (getenv("MELEE_DIAGNOSTIC_TEXTURE_QUALITY")) {
        static unsigned phase;
        static const unsigned frames[3]={300,450,600},quality[3]={50,25,100};
        if(phase<3 && controller.state.frame_count>=frames[phase]) {
            int accepted=aurora_set_texture_quality(quality[phase]);
            OSReport("NATIVE_TEXTURE_QUALITY frame=%u requested=%u actual=%u accepted=%d\n",
                     controller.state.frame_count,quality[phase],aurora_get_texture_quality(),accepted);
            if(accepted)++phase;
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_SHADOW_RESOLUTION")) {
        static unsigned phase;
        static const unsigned frames[3]={300,450,600},sizes[3]={64,128,256};
        if(phase<3 && controller.state.frame_count>=frames[phase]) {
            int accepted=aurora_set_fighter_shadow_resolution(sizes[phase]);
            OSReport("NATIVE_SHADOW_RESOLUTION frame=%u requested=%u actual=%u accepted=%d\n",
                     controller.state.frame_count,sizes[phase],aurora_get_fighter_shadow_resolution(),accepted);
            if(accepted)++phase;
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_SHADOW_TOGGLE")) {
        static unsigned phase;
        if ((phase==0 && controller.state.frame_count>=400) ||
            (phase==1 && controller.state.frame_count>=600)) {
            int enabled=phase==1;
            int accepted=aurora_set_fighter_shadows(enabled);
            OSReport("NATIVE_SHADOW_SETTING frame=%u enabled=%d accepted=%d\n",
                     controller.state.frame_count,enabled,accepted);
            if(accepted)++phase;
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_REFRACTION_CLOAK")) {
        static int applied;
        if (!applied && controller.state.frame_count >= 400) {
            Fighter_GObj* fighter=Player_GetEntity(0);
            if (fighter) {
                ftCo_800C88D4(fighter,600,0);
                applied=1;
                OSReport("NATIVE_REFRACTION_CLOAK applied=1 frame=%u slot=0\n",controller.state.frame_count);
#if defined(_WIN32)
                if(getenv("MELEE_DIAGNOSTIC_CLOAK_UNIFORM_UPLOAD") &&
                   _putenv_s("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD","1")==0) {
                    aurora_diagnostic_refresh_command_flags();
                    OSReport("NATIVE_REFRACTION_UNIFORM_ARMED frame=%u\n",controller.state.frame_count);
                }
#endif
            }
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_CLASSIC_COMBAT") &&
        gm_GetCurrentGameMode() == GM_CLASSIC) {
        PADStatus input = { 0 };
        unsigned frame = controller.state.frame_count, phase = frame % 60;
        HSD_GObj* self = Player_GetEntity(0);
        HSD_GObj* target = NULL;
        int target_slot = -1;
        static unsigned clear_calls, stalled, previous_entry;
        static f32 prior_x;
        if (previous_entry != diagnostic_vs_entries) {
            clear_calls = stalled = 0; previous_entry = diagnostic_vs_entries;
        }
        if (controller.state.unk_0 == 0) clear_calls = 0;
        /* Original Classic assigns allies before opponents. Choose a live
         * opposing team by read-only player metadata, never a fixed slot. */
        if (self && controller.state.unk_0 == 0) {
            Vec3 a; f32 nearest = 1.0e30f;
            ftLib_GetPos(self, &a);
            for (int slot = 1; slot < 6; ++slot) {
                HSD_GObj* candidate = Player_GetEntity(slot);
                Vec3 b; f32 dx, dy, distance;
                if (!candidate || Player_GetStocks(slot) <= 0 ||
                    (controller.start.is_teams &&
                     Player_GetTeam(slot) == Player_GetTeam(0))) continue;
                ftLib_GetPos(candidate, &b);
                dx = b.x-a.x; dy = b.y-a.y;
                distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                if (distance < nearest) {
                    nearest = distance; target = candidate; target_slot = slot;
                }
            }
        }
        if (controller.state.unk_0 == 0 && self && target) {
            Vec3 a, b;
            ftLib_GetPos(self, &a); ftLib_GetPos(target, &b);
            f32 dx = b.x-a.x, dy = b.y-a.y;
            f32 distance = dx < 0 ? -dx : dx;
            int direction = dx >= 0 ? 1 : -1;
            if (frame >= 180) {
                bool grounded = ftLib_GetGroundAir(self) == GA_Ground;
                if (distance > 18) input.stickX = direction*55;
                /* Only movement blocked on the ground counts as an obstacle.
                 * Separate jump edges from A/stick attack edges. */
                if (grounded && input.stickX &&
                    a.x > prior_x-0.01f && a.x < prior_x+0.01f) ++stalled;
                else stalled = 0;
                if (phase < 4 && grounded && (dy > 15 || stalled > 60))
                    input.button = PAD_BUTTON_X;
                else if (phase >= 30 && phase < 34 && !grounded && dy > 25)
                    input.button = PAD_BUTTON_X;
                /* Neutral samples precede the original A + stick-flick path. */
                if (distance <= 35 && dy < 28 && dy > -28) {
                    if ((phase >= 6 && phase < 12) ||
                        (phase >= 36 && phase < 42)) input.stickX = 0;
                    if ((phase >= 12 && phase < 15) ||
                        (phase >= 42 && phase < 45)) {
                        input.stickX = direction*80;
                        input.button = PAD_BUTTON_A;
                    }
                }
                if (!grounded && a.y < -15 && phase >= 20 && phase < 24) {
                    input.button = PAD_BUTTON_B; input.stickY = 80;
                }
            }
            prior_x = a.x;
            if (frame % 30 == 0)
                OSReport("NATIVE_CLASSIC_COMBAT frame=%u state=%u result=%d self_stocks=%d target_stocks=%d self_damage=%d target_damage=%d self_motion=%d target_motion=%d x=%f y=%f target_x=%f target_y=%f requested_x=%d cstick_x=%d button=%u entry=%u target_slot=%d teams=%d self_team=%d target_team=%d\n",
                         frame, controller.state.unk_0, controller.state.match_result,
                         Player_GetStocks(0), Player_GetStocks(target_slot), Player_GetDamage(0), Player_GetDamage(target_slot),
                         ftLib_GetMotionId(self), ftLib_GetMotionId(target), a.x,a.y,b.x,b.y,
                         input.stickX,input.substickX,input.button, diagnostic_vs_entries, target_slot,
                         controller.start.is_teams, Player_GetTeam(0), Player_GetTeam(target_slot));
#ifdef TARGET_PC
            if (frame >= 500 && frame <= 560 && frame % 5 == 0) {
                Fighter* cpu_state = target ? target->user_data : NULL;
                if (cpu_state && cpu_state->player_idx == 1)
                    OSReport("NATIVE_CLASSIC_CPU_TRACE frame=%u entry=%u kind=%d motion=%d level=%d cmd=%d cmd_prev=%d cmd_duration=%u x80=%u seed=%08x\n",
                             frame, diagnostic_vs_entries, cpu_state->kind,
                             cpu_state->motion_id, cpu_state->cpu.level,
                             cpu_state->cpu.x18, cpu_state->cpu.x1C,
                             cpu_state->cpu.command_duration, cpu_state->cpu.x80,
                             *HSD_RandSeedPtr);
            }
#endif
        } else if (controller.state.unk_0 == 2) {
            ++clear_calls;
            if (clear_calls >= 240 && clear_calls % 60 < 4)
                input.button = PAD_BUTTON_START;
            if (clear_calls % 60 == 0)
                OSReport("NATIVE_CLASSIC_CLEAR_INPUT calls=%u button=%u result=%d stocks=%d\n",
                         clear_calls,input.button,controller.state.match_result,Player_GetStocks(0));
        }
        if (controller.state.unk_0 == 0 && !target && frame % 60 == 0)
            OSReport("NATIVE_CLASSIC_COMBAT_NO_TARGET entry=%u frame=%u rules_mode=%u\n",
                     diagnostic_vs_entries, frame, controller.start.x9);
        if (diagnostic_vs_entries == 3 && controller.state.unk_0 == 0 && self) {
            static int target_capture_armed;
            static unsigned target_recovery_stable;
            static unsigned target_second_phase, target_second_begin, target_second_stable;
            static int target_second_air;
            int target_recovery_mode=getenv("MELEE_DIAGNOSTIC_TARGET_RECOVERY")!=NULL;
            int target_second_mode=target_recovery_mode &&
                getenv("MELEE_DIAGNOSTIC_TARGET_SECOND")!=NULL &&
                stage_info.grkind==Gr_Kind_TDrmario && ftLib_GetKind(self)==Ft_Kind_DrMario;
            if (getenv("MELEE_DIAGNOSTIC_TARGET_HIT")) {
                static Vec3 desired, start, edge;
                static int point = -1, drop_direction, teeter_ready, initial_stocks;
                static unsigned attack_frame, hit_frame, air_frame, cliff_frame, prehit_jump_frame;
                Fighter* target_input_state=self->user_data;
                Vec3 position; ftLib_GetPos(self,&position);
                if (frame >= 180 && point == -1) {
                    f32 nearest = 1.0e30f; start = position;
                    initial_stocks=Player_GetStocks(0);
                    for (int id = 199; id < 220; ++id) {
                        Vec3 value;
                        if (Ground_801C2D24(id,&value)) {
                            f32 dx=value.x-position.x,dy=position.y-value.y;
                            f32 score=(dx<0?-dx:dx)+dy*0.25f;
                            OSReport("NATIVE_TARGET_POINT id=%d x=%f y=%f\n",id,value.x,value.y);
                            if (dy>8 && dy<100 && score<nearest) {
                                nearest=score;desired=value;point=id;
                            }
                        }
                    }
                    if (point>=0) {
                        drop_direction=desired.x>=position.x?1:-1;
                        OSReport("NATIVE_TARGET_ROUTE point=%d target_x=%f target_y=%f start_x=%f start_y=%f direction=%d\n",
                                 point,desired.x,desired.y,start.x,start.y,drop_direction);
                    }
                }
                if (stage_info.x6D4<stage_info.x6D2 && !hit_frame) hit_frame=frame;
                if (point>=0 && frame>=180 && !hit_frame) {
                    f32 dx=desired.x-position.x,dy=position.y-desired.y;
                    if (ftLib_GetGroundAir(self)==GA_Ground) {
                        /* Let original Ottotto stop walking momentum before
                         * crossing the edge; do not carry a long run-up. */
                        int motion=ftLib_GetMotionId(self);
                        if (motion==ftCo_MS_Ottotto || motion==ftCo_MS_OttottoWait) {
                            if (!teeter_ready) edge=position;
                            teeter_ready=1;
                        }
                        /* Original scale80:62/80=.775 exceeds teeter .75,
                         * while staying below dash .8. No rule changes. */
                        input.stickX=drop_direction*(teeter_ready?62:55);
                        air_frame=0;
                    } else {
                        if (!air_frame) air_frame=frame;
                        if (target_recovery_mode) {
                            /* Original floor line6 has no ledge-grab flag.
                             * Jump into the target attack instead of waiting
                             * out the whole aerial while falling below it. */
                            f32 outside_dx=edge.x+drop_direction*3-position.x;
                            input.stickX=outside_dx>0.5f?55:outside_dx<-0.5f?-55:
                                target_input_state->self_vel.x>0?-35:
                                target_input_state->self_vel.x<0?35:0;
                            if (!prehit_jump_frame && dy<=-10) prehit_jump_frame=frame;
                            if (prehit_jump_frame &&
                                target_input_state->x1968_jumpsUsed<target_input_state->co_attrs.max_jumps) {
                                input.stickX=0;
                                if ((frame-prehit_jump_frame)%2==0) input.button=PAD_BUTTON_X;
                            } else if (prehit_jump_frame && !attack_frame) attack_frame=frame;
                            if (attack_frame) {
                                input.stickX=-drop_direction*80;
                                if (frame-attack_frame<3) input.button=PAD_BUTTON_A;
                            }
                            if (frame%5==0 || input.button)
                                OSReport("NATIVE_TARGET_RECOVERY_PREHIT frame=%u motion=%d x=%f y=%f vy=%f jumps=%d jump_frame=%u attack_frame=%u button=%u stick_x=%d\n",
                                         frame,ftLib_GetMotionId(self),position.x,position.y,
                                         target_input_state->self_vel.y,target_input_state->x1968_jumpsUsed,
                                         prehit_jump_frame,attack_frame,input.button,input.stickX);
                            if (ftLib_GetMotionId(self)==ftCo_MS_AttackAirB)
                                for (int i=0;i<4;++i) {
                                    HitCapsule* hit=&target_input_state->x914[i];
                                    if (hit->state==HitCapsule_Unk2 || hit->state==HitCapsule_Unk3)
                                        OSReport("NATIVE_TARGET_HITBOX_SAMPLE frame=%u box=%d state=%d damage=%f radius=%f x=%f y=%f z=%f previous_x=%f previous_y=%f previous_z=%f\n",
                                                 frame,i,hit->state,hit->damage,hit->scale,
                                                 hit->x4C.x,hit->x4C.y,hit->x4C.z,
                                                 hit->x58.x,hit->x58.y,hit->x58.z);
                                }
                        } else {
                            /* Preserve the verified hit-only input sequence. */
                            input.stickX=dx>2?80:dx<-2?-80:
                                target_input_state->self_vel.x>0?-55:
                                target_input_state->self_vel.x<0?55:0;
                            if (!attack_frame && frame-air_frame>=6 && dx<8 && dx>-8 && dy<28 && dy>-5)
                                attack_frame=frame;
                            if (attack_frame) {
                                input.stickY=-80;
                                if (frame-attack_frame<3) input.button=PAD_BUTTON_A;
                            }
                        }
                    }
                } else if (hit_frame && target_recovery_mode) {
                    int motion=ftLib_GetMotionId(self);
                    unsigned recovery_phase=(frame-hit_frame)%2;
                    int grounded=ftLib_GetGroundAir(self)==GA_Ground;
                    if (grounded && position.y>=edge.y-1 &&
                        position.x>start.x-25 && position.x<start.x+25 &&
                        Player_GetStocks(0)==initial_stocks) {
                        ++target_recovery_stable;
                    } else target_recovery_stable=0;
                    if (!grounded) {
                        f32 goal_x=position.y>edge.y+5?start.x:edge.x+drop_direction*8;
                        f32 dx=goal_x-position.x;
                        input.stickX=dx>2?55:dx<-2?-55:
                            target_input_state->self_vel.x>0?-35:
                            target_input_state->self_vel.x<0?35:0;
                        if (motion==ftCo_MS_CliffWait) {
                            if (!cliff_frame) cliff_frame=frame;
                            input.stickX=0;
                            if (frame-cliff_frame>=6) input.stickY=80;
                        } else {
                            cliff_frame=0;
                            /* Original aerial attack IASA gates jumping.
                             * Repeated press/release edges do not force it. */
                            if (target_input_state->x1968_jumpsUsed<target_input_state->co_attrs.max_jumps) {
                                if (recovery_phase==0) input.button=PAD_BUTTON_X;
                            } else if (target_input_state->self_vel.y<=0) {
                                input.stickY=80;
                                if (recovery_phase==0) input.button=PAD_BUTTON_B;
                            }
                        }
                    }
                    if (frame%5==0 || input.button || target_recovery_stable==60)
                        OSReport("NATIVE_TARGET_RECOVERY frame=%u motion=%d ground=%d x=%f y=%f vx=%f vy=%f interrupt=%d anim=%f jumps=%d max_jumps=%d stocks=%d edge_x=%f edge_y=%f stable=%u button=%u stick_x=%d stick_y=%d\n",
                                 frame,motion,grounded?0:1,position.x,position.y,
                                 target_input_state->self_vel.x,target_input_state->self_vel.y,
                                 target_input_state->allow_interrupt,target_input_state->cur_anim_frame,
                                 target_input_state->x1968_jumpsUsed,target_input_state->co_attrs.max_jumps,
                                 Player_GetStocks(0),edge.x,edge.y,target_recovery_stable,
                                 input.button,input.stickX,input.stickY);
                    if (target_recovery_stable==60)
                        OSReport("NATIVE_TARGET_RECOVERY_COMPLETE frame=%u remaining=%d stable=%u stocks=%d x=%f y=%f\n",
                                 frame,stage_info.x6D4,target_recovery_stable,Player_GetStocks(0),position.x,position.y);
                } else if (hit_frame && ftLib_GetGroundAir(self)==GA_Air) {
                    input.stickX=position.x>start.x?-40:40;
                    if (phase<4){input.stickY=80;input.button=PAD_BUTTON_B;}
                }
                if (target_second_mode && hit_frame) {
                    int grounded=ftLib_GetGroundAir(self)==GA_Ground;
                    int motion=ftLib_GetMotionId(self);
                    Vec3 second_target={0};
                    Vec3 waypoint_left,waypoint_right;
                    mpFloorGetLeft(7,&waypoint_left);mpFloorGetRight(7,&waypoint_right);
                    f32 waypoint_x=(waypoint_left.x+waypoint_right.x)*0.5f;
                    f32 waypoint_y=(waypoint_left.y+waypoint_right.y)*0.5f;
                    int has_target=Ground_801C2D24(204,&second_target);
                    if (!target_second_phase && target_recovery_stable>=60) {
                        target_second_phase=1;target_second_begin=frame;
                        target_second_stable=0;target_second_air=0;
                        OSReport("NATIVE_TARGET_SECOND_BEGIN frame=%u point=204 target_x=%f target_y=%f stocks=%d\n",
                                 frame,second_target.x,second_target.y,Player_GetStocks(0));
                    }
                    if (target_second_phase) {
                        /* This route owns only the opt-in test PAD input.
                         * Stage/player state and outcomes remain original. */
                        memset(&input,0,sizeof(input));
                        if (stage_info.x6D4<=8 && target_second_phase<3) {
                            target_second_phase=3;target_second_stable=0;
                        }
                        if (target_second_phase==1 || target_second_phase==2) {
                            unsigned elapsed=frame-target_second_begin;
                            if (!grounded) target_second_air=1;
                            if (grounded && target_second_air) {
                                int waypoint=target_input_state->coll_data.floor.index==7 &&
                                    position.x>waypoint_left.x && position.x<waypoint_right.x;
                                if (target_second_phase==1 && waypoint) {
                                    ++target_second_stable;
                                    if (target_second_stable==30) {
                                        OSReport("NATIVE_TARGET_SECOND_WAYPOINT frame=%u stable=30 stocks=%d x=%f y=%f\n",
                                                 frame,Player_GetStocks(0),position.x,position.y);
                                        target_second_phase=2;target_second_begin=frame;
                                        target_second_stable=0;target_second_air=0;
                                    }
                                } else {
                                    int queued=aurora_diagnostic_queue_quit();
                                    OSReport("NATIVE_TARGET_SECOND_ABORT frame=%u phase=%u reason=route_landed_without_goal remaining=%d queued=%d\n",
                                             frame,target_second_phase,stage_info.x6D4,queued);
                                    target_second_phase=99;
                                }
                            } else if (elapsed<8) {
                                /* Hold through original KneeBend for a full
                                 * ground jump, then release before air jump. */
                                input.button=PAD_BUTTON_X;
                            } else if (!grounded) {
                                if (motion==ftMr_MS_SpecialAirHi) {
                                    if (target_second_phase==1) {
                                        f32 dx=waypoint_x-position.x;
                                        input.stickX=position.y<waypoint_y+5?35:dx>2?55:dx<-2?-55:0;
                                    } else input.stickX=second_target.x>position.x?35:-35;
                                } else if (target_input_state->self_vel.y<=0) {
                                    if (target_second_phase==2 &&
                                        target_input_state->x1968_jumpsUsed<target_input_state->co_attrs.max_jumps) {
                                        if (elapsed%2==0) input.button=PAD_BUTTON_X;
                                    } else {
                                        input.stickY=80;
                                        input.stickX=target_second_phase==1?35:second_target.x>position.x?35:-35;
                                        if (elapsed%2==0) input.button=PAD_BUTTON_B;
                                    }
                                }
                            }
                        } else if (target_second_phase==3) {
                            int safe_floor=target_input_state->coll_data.floor.index==6 ||
                                           target_input_state->coll_data.floor.index==7;
                            if (grounded && safe_floor && Player_GetStocks(0)==initial_stocks)
                                ++target_second_stable;
                            else target_second_stable=0;
                            if (!grounded) {
                                f32 goal=position.y>waypoint_y+5?waypoint_x:start.x,dx=goal-position.x;
                                input.stickX=dx>2?55:dx<-2?-55:
                                    target_input_state->self_vel.x>0?-35:
                                    target_input_state->self_vel.x<0?35:0;
                            }
                            if (target_second_stable==60)
                                OSReport("NATIVE_TARGET_SECOND_COMPLETE frame=%u remaining=%d stable=60 stocks=%d x=%f y=%f\n",
                                         frame,stage_info.x6D4,Player_GetStocks(0),position.x,position.y);
                            if (target_second_stable==100 && !getenv("MELEE_DIAGNOSTIC_TARGET_SURVEY") &&
                                !getenv("MELEE_DIAGNOSTIC_TARGET_ALL")) {
                                int queued=aurora_diagnostic_queue_quit();
                                OSReport("NATIVE_TARGET_SECOND_CLOSE frame=%u remaining=%d stable=100 queued=%d\n",
                                         frame,stage_info.x6D4,queued);
                            }
                        }
                        if (frame%5==0 || input.button)
                            OSReport("NATIVE_TARGET_SECOND frame=%u phase=%u ground=%d motion=%d x=%f y=%f vx=%f vy=%f jumps=%d stocks=%d remaining=%d stable=%u target_valid=%d target_x=%f target_y=%f button=%u stick_x=%d stick_y=%d floor=%d waypoint_x=%f waypoint_y=%f\n",
                                     frame,target_second_phase,grounded?0:1,motion,position.x,position.y,
                                     target_input_state->self_vel.x,target_input_state->self_vel.y,
                                     target_input_state->x1968_jumpsUsed,Player_GetStocks(0),stage_info.x6D4,
                                     target_second_stable,has_target,second_target.x,second_target.y,
                                     input.button,input.stickX,input.stickY,target_input_state->coll_data.floor.index,
                                     waypoint_x,waypoint_y);
                    }
                }
                if (getenv("MELEE_DIAGNOSTIC_TARGET_SURVEY"))
                    MeleePc_TargetRouteSurvey(frame,&input,target_second_stable>=60);
                if (getenv("MELEE_DIAGNOSTIC_TARGET_ALL"))
                    MeleePc_TargetRouteAll(frame,self,&input,target_second_stable>=60);
                if ((frame%10==0 || input.button) && frame>=180)
                    OSReport("NATIVE_TARGET_INPUT frame=%u motion=%d ground=%d x=%f y=%f point=%d remaining=%d stick_x=%d stick_y=%d button=%u attack_frame=%u hit_frame=%u facing=%f latched_x=%f teeter=%f walk=%f dash=%f stick_scale=%d velocity_x=%f teeter_ready=%d\n",
                             frame,ftLib_GetMotionId(self),ftLib_GetGroundAir(self),position.x,position.y,point,
                             stage_info.x6D4,input.stickX,input.stickY,input.button,attack_frame,hit_frame,
                             target_input_state->facing_dir,target_input_state->input.lstick[0].x,
                             p_ftCommonData->teeter_walk_threshold,p_ftCommonData->walk_stick_threshold,
                             p_ftCommonData->dash_smash_stick_threshold,HSD_PadLibData.scale_stick,
                             target_input_state->self_vel.x,teeter_ready);
                /* Allow the20-present GPU capture to finish, then leave
                 * before this deliberately off-stage hit probe falls out. */
                if (hit_frame && !target_second_mode && ((!target_recovery_mode && frame-hit_frame>=24) ||
                                  (target_recovery_mode && target_recovery_stable>=100))) {
                    static int close_queued;
                    if (!close_queued) {
                        close_queued=aurora_diagnostic_queue_quit();
                        if (target_recovery_mode)
                            OSReport("NATIVE_TARGET_RECOVERY_CLOSE frame=%u remaining=%d stable=%u queued=%d\n",frame,stage_info.x6D4,target_recovery_stable,close_queued);
                        else OSReport("NATIVE_TARGET_HIT_CLOSE frame=%u remaining=%d queued=%d\n",frame,stage_info.x6D4,close_queued);
                    }
                }
            }
            if (frame % 60 == 0)
                OSReport("NATIVE_TARGET_STAGE frame=%u stage_kind=%d initial=%d remaining=%d\n",
                         frame,stage_info.grkind,stage_info.x6D2,stage_info.x6D4);
#if defined(_WIN32)
            if (!getenv("MELEE_DIAGNOSTIC_TARGET_ALL") && !target_capture_armed && frame >= 180 &&
                (!getenv("MELEE_DIAGNOSTIC_TARGET_HIT") ||
                 (stage_info.x6D4<stage_info.x6D2 &&
                  (target_second_mode?(stage_info.x6D4==8 && target_second_stable>=60):
                   (!target_recovery_mode || target_recovery_stable>=60))))) {
                char* path = NULL; size_t size = 0;
                if (_dupenv_s(&path,&size,"MELEE_DIAGNOSTIC_THIRD_STAGE_CAPTURE") == 0 &&
                    path && path[0]) {
                    target_capture_armed = _putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE",path) == 0;
                    OSReport("NATIVE_TARGET_STAGE capture_armed=%d frame=%u\n",target_capture_armed,frame);
                }
                free(path);
            }
#endif
        }
        if (getenv("MELEE_DIAGNOSTIC_TARGET_SECOND") && diagnostic_vs_entries==3 &&
            controller.state.unk_0==2) {
            static int target_second_end_queued;
            if (getenv("MELEE_DIAGNOSTIC_TARGET_ALL") && stage_info.x6D4==0) {
                if (clear_calls==1)
                    OSReport("NATIVE_TARGET_ALL_ORIGINAL_END frame=%u remaining=0 outcome=%d stocks=%d\n",
                             frame,controller.state.match_result,Player_GetStocks(0));
            } else if (!target_second_end_queued) {
                target_second_end_queued=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_TARGET_SECOND_ABORT reason=original_stage_end remaining=%d outcome=%d queued=%d\n",
                         stage_info.x6D4,controller.state.match_result,target_second_end_queued);
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_TARGET_NEXT_COMBAT") && diagnostic_vs_entries==4) {
            static int next_combat_closed;
            if (controller.state.unk_0==2 && clear_calls==1)
                OSReport("NATIVE_CLASSIC_PROGRESS_END frame=%u outcome=%d stocks=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0));
            if (!next_combat_closed &&
                ((controller.state.unk_0==2 && controller.state.match_result==OUTCOME_1P_GAME_OVER) ||
                 (controller.state.unk_0==0 && frame>=3600))) {
                next_combat_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_CLASSIC_PROGRESS_ABORT frame=%u outcome=%d stocks=%d queued=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0),next_combat_closed);
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_TARGET_ALL") &&
            diagnostic_vs_entries==(getenv("MELEE_DIAGNOSTIC_TARGET_NEXT_COMBAT")?5:4) &&
            controller.state.unk_0==0) {
            memset(&input,0,sizeof(input));
            if (getenv("MELEE_DIAGNOSTIC_TARGET_NEXT_INPUT") && self) {
                static unsigned next_attack_frame;
                Fighter* fp=self->user_data;
                if (frame>=180 && frame<210) input.stickX=45;
                if (frame>=240 && frame<270) input.stickX=-45;
                if (frame>=360 && frame<368) input.button=PAD_BUTTON_X;
                if (!next_attack_frame && frame>=540 && frame<850 &&
                    (frame<700 || frame>=760) &&
                    ftLib_GetGroundAir(self)==GA_Ground &&
                    ftLib_GetMotionId(self)==ftCo_MS_Wait &&
                    !(fp->input.held_buttons[0]&PAD_BUTTON_A)) {
                    next_attack_frame=frame;
                    OSReport("NATIVE_TARGET_NEXT_ATTACK_REQUEST frame=%u motion=%d ground=1 held=%u stocks=%d\n",
                             frame,ftLib_GetMotionId(self),fp->input.held_buttons[0],Player_GetStocks(0));
                }
                if (next_attack_frame && frame-next_attack_frame<4)
                    input.button=PAD_BUTTON_A;
                if (frame>=720 && frame<724) input.button=PAD_BUTTON_B;
                if (frame>=175 && frame<=890 &&
                    (frame%5==0 || (next_attack_frame && frame-next_attack_frame<=40))) {
                    OSReport("NATIVE_TARGET_NEXT_INPUT frame=%u requested_x=%d requested_button=%u held=%u pressed=%u latched_x=%f motion=%d ground=%d x=%f y=%f stocks=%d\n",
                             frame,input.stickX,input.button,fp->input.held_buttons[0],
                             fp->input.pressed_buttons,fp->input.lstick[0].x,
                             ftLib_GetMotionId(self),ftLib_GetGroundAir(self)==GA_Ground,
                             fp->cur_pos.x,fp->cur_pos.y,Player_GetStocks(0));
                }
            }
#if defined(_WIN32)
            static int next_capture;
            if (frame>=180 && !next_capture && !getenv("MELEE_DIAGNOSTIC_FIGURE_GET")) {
                char* path=NULL;size_t size=0;
                if (_dupenv_s(&path,&size,"MELEE_DIAGNOSTIC_THIRD_STAGE_CAPTURE")==0 && path && path[0]) {
                    next_capture=_putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE",path)==0;
                    OSReport("NATIVE_TARGET_NEXT_STAGE capture_armed=%d frame=%u stage_kind=%d stocks=%d\n",
                             next_capture,frame,stage_info.grkind,Player_GetStocks(0));
                }
                free(path);
            }
#endif
        }
        /* Optional Figure Get proof uses normal PAD edges and read-only state. */
        if (getenv("MELEE_DIAGNOSTIC_FIGURE_GET") && diagnostic_vs_entries==6 &&
            stage_info.grkind==Gr_Kind_FigureGet) {
            static int figure_capture,figure_closed;
            memset(&input,0,sizeof(input));
            if (controller.state.unk_0==0 && self) {
                Fighter* fp=self->user_data;
                if (frame>=180 && frame<210)input.stickX=45;
                if (frame>=240 && frame<270)input.stickX=-45;
                if (frame>=360 && frame<364)input.button=PAD_BUTTON_X;
                if (frame>=540 && frame<544)input.button=PAD_BUTTON_A;
                if(getenv("MELEE_DIAGNOSTIC_FIGURE_CATCH"))MeleePc_FigureCatchInput(self,&input,frame);
                if (frame%5==0) {
                    /* ground.c constructs these in link5/classSTAGE. Walk
                     * the existing objects; Ground_GetStageGObj allocates. */
                    Ground* gp=NULL;
                    for(HSD_GObj* g=HSD_GObjPLinkHead[5];g;g=g->next){
                        if(g->classifier==HSD_GOBJ_CLASS_STAGE && g->user_data){
                            Ground* candidate=g->user_data;
                            if(candidate->map_id==1 && candidate->gobj==g){gp=candidate;break;}
                        }
                    }
                    OSReport("NATIVE_FIGURE_PLAY frame=%u motion=%d ground=%d x=%f y=%f stocks=%d requested_x=%d requested_button=%u held=%u pressed=%u latched_x=%f spawned=%d caught=%d lost=%d\n",
                             frame,ftLib_GetMotionId(self),ftLib_GetGroundAir(self)==GA_Ground,
                             fp->cur_pos.x,fp->cur_pos.y,Player_GetStocks(0),input.stickX,input.button,
                             fp->input.held_buttons[0],fp->input.pressed_buttons,fp->input.lstick[0].x,
                             gp?gp->u.figureget.x4:-1,gp?gp->u.figureget.x8:-1,gp?gp->u.figureget.xC:-1);
                    for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next){
                        Item* item=g->user_data;
                        if(item && item->kind==It_Kind_Coin)
                            OSReport("NATIVE_FIGURE_ITEM frame=%u item=%p motion=%d x=%f y=%f\n",
                                     frame,g,item->msid,item->pos.x,item->pos.y);
                    }
                }
#if defined(_WIN32)
                if(frame>=180 && !figure_capture && !getenv("MELEE_DIAGNOSTIC_FIGURE_NEXT")){
                    char* path=NULL;size_t size=0;
                    if(_dupenv_s(&path,&size,"MELEE_DIAGNOSTIC_THIRD_STAGE_CAPTURE")==0 && path && path[0]){
                        figure_capture=_putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE",path)==0;
                        OSReport("NATIVE_FIGURE_CAPTURE armed=%d frame=%u stage_kind=%d\n",figure_capture,frame,stage_info.grkind);
                    }
                    free(path);
                }
#endif
            }
            if(!figure_closed && ((controller.state.unk_0!=0 && controller.state.match_result!=OUTCOME_NONE) ||
                                  (controller.state.unk_0==0 && frame>=1800))){
                if(getenv("MELEE_DIAGNOSTIC_FIGURE_NEXT") && controller.state.unk_0!=0){
                    figure_closed=1;
                    OSReport("NATIVE_FIGURE_END frame=%u state=%u outcome=%d stocks=%d queued=0 continue=1\n",
                             frame,controller.state.unk_0,controller.state.match_result,Player_GetStocks(0));
                }else{
                    figure_closed=aurora_diagnostic_queue_quit();
                    OSReport("NATIVE_FIGURE_END frame=%u state=%u outcome=%d stocks=%d queued=%d\n",
                             frame,controller.state.unk_0,controller.state.match_result,Player_GetStocks(0),figure_closed);
                }
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_CLASSIC_SEVENTH_COMBAT") && diagnostic_vs_entries==7){
            static int seventh_closed;
            if(controller.state.unk_0==2 && clear_calls==1)
                OSReport("NATIVE_CLASSIC_SEVENTH_END frame=%u outcome=%d stocks=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0));
            if(!seventh_closed &&
               ((controller.state.unk_0!=0 && controller.state.match_result==OUTCOME_1P_GAME_OVER) ||
                (controller.state.unk_0==0 && frame>=3600))){
                seventh_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_CLASSIC_SEVENTH_ABORT frame=%u outcome=%d stocks=%d queued=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0),seventh_closed);
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_CLASSIC_EIGHTH_COMBAT") && diagnostic_vs_entries==8){
            static int eighth_closed;
            if(controller.state.unk_0==2 && clear_calls==1)
                OSReport("NATIVE_CLASSIC_EIGHTH_END frame=%u outcome=%d stocks=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0));
            if(!eighth_closed &&
               ((controller.state.unk_0!=0 && controller.state.match_result==OUTCOME_1P_GAME_OVER) ||
                (controller.state.unk_0==0 && frame>=7200))){
                eighth_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_CLASSIC_EIGHTH_ABORT frame=%u outcome=%d stocks=%d queued=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0),eighth_closed);
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_PUSHON_DOOR_EXIT") && diagnostic_vs_entries==9){
            static int door_closed;
            if(controller.state.unk_0==2 && clear_calls==1)
                OSReport("NATIVE_PUSHON_EXIT_END frame=%u outcome=%d stocks=%d flags=%u point=%d\n",
                    frame,controller.state.match_result,Player_GetStocks(0),Ground_801C1D84(),Ground_801C1DD4());
            if(!door_closed && controller.state.unk_0==0 && frame>=1800){
                door_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_PUSHON_EXIT_ABORT frame=%u queued=%d\n",frame,door_closed);
            }
        }
        /* The tenth match reuses the normal Classic combat PAD policy above.
         * Observe its original result, then allow the original clear/Start path
         * to advance; never assign stocks, damage, or an outcome here. */
        if(getenv("MELEE_DIAGNOSTIC_CLASSIC_TENTH_COMBAT") && diagnostic_vs_entries==10){
            static int tenth_closed;
            if(controller.state.unk_0==2 && clear_calls==1)
                OSReport("NATIVE_CLASSIC_TENTH_END frame=%u outcome=%d stocks=%d\n",
                    frame,controller.state.match_result,Player_GetStocks(0));
            if(!tenth_closed &&
               ((controller.state.unk_0!=0 && controller.state.match_result==OUTCOME_1P_GAME_OVER) ||
                (controller.state.unk_0==0 && frame>=7200))){
                tenth_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_CLASSIC_TENTH_ABORT frame=%u outcome=%d stocks=%d queued=%d\n",
                    frame,controller.state.match_result,Player_GetStocks(0),tenth_closed);
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_MASTERHAND_DEFEAT") && diagnostic_vs_entries==11 &&
           controller.state.unk_0==2){
            static int boss_clear_capture,boss_clear_closed;
            if(clear_calls==1)
                OSReport("NATIVE_MASTERHAND_END frame=%u outcome=%d stocks=%d\n",
                         frame,controller.state.match_result,Player_GetStocks(0));
            if(getenv("MELEE_DIAGNOSTIC_MASTERHAND_ADVANCE") && clear_calls==240)
                OSReport("NATIVE_MASTERHAND_START_ADVANCE calls=%u button=%u outcome=%d\n",
                         clear_calls,input.button,controller.state.match_result);
#if defined(_WIN32)
            /* The clear overlay may change the end-graphic selector. Observe
             * the original boss result and persistent HP instead. */
            if(controller.state.match_result==OUTCOME_TERMINATED &&
               Player_GetRemainingHP(2)==0 && !boss_clear_capture){
                char* path=NULL;size_t size=0;
                if(_dupenv_s(&path,&size,"MELEE_DIAGNOSTIC_BOSS_CLEAR_CAPTURE")==0 && path && path[0]){
                    boss_clear_capture=_putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE",path)==0;
                    OSReport("NATIVE_MASTERHAND_CLEAR capture_armed=%d calls=%u hp=%d end_graphic=%u\n",
                             boss_clear_capture,clear_calls,Player_GetRemainingHP(2),controller.state.unk_B);
                }
                free(path);
            }
#endif
            if(!boss_clear_closed && !getenv("MELEE_DIAGNOSTIC_MASTERHAND_ADVANCE") && (clear_calls>=180 ||
               controller.state.match_result==OUTCOME_1P_GAME_OVER)){
                boss_clear_closed=aurora_diagnostic_queue_quit();
                OSReport("NATIVE_MASTERHAND_CLEAR_CLOSE calls=%u outcome=%d queued=%d\n",
                         clear_calls,controller.state.match_result,boss_clear_closed);
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_FIGURE_NEXT") &&
           (diagnostic_vs_entries==(getenv("MELEE_DIAGNOSTIC_CLASSIC_EIGHTH_COMBAT")?9:
                                    getenv("MELEE_DIAGNOSTIC_CLASSIC_SEVENTH_COMBAT")?8:7) ||
            (getenv("MELEE_DIAGNOSTIC_PUSHON_DOOR_EXIT") && diagnostic_vs_entries==10 &&
             !getenv("MELEE_DIAGNOSTIC_CLASSIC_TENTH_COMBAT")) ||
            (getenv("MELEE_DIAGNOSTIC_CLASSIC_TENTH_COMBAT") && diagnostic_vs_entries==11)) &&
           controller.state.unk_0==0 && self){
            static int next_capture;
            static unsigned attack_frame;
            static unsigned route_stable,route_complete,route_stalled;
            static unsigned second_stable,second_complete,second_stalled;
            static float second_previous_x;
            static float route_previous_x;
            Fighter* fp=self->user_data;
            static unsigned next_input_entry;
            if(next_input_entry!=diagnostic_vs_entries){next_input_entry=diagnostic_vs_entries;attack_frame=0;next_capture=0;}
            memset(&input,0,sizeof(input));
            if(frame>=180 && frame<210)input.stickX=45;
            if(frame>=240 && frame<270)input.stickX=-45;
            if(frame>=360 && frame<364)input.button=PAD_BUTTON_X;
            if(!attack_frame && frame>=540 && frame<850 &&
               ftLib_GetGroundAir(self)==GA_Ground && ftLib_GetMotionId(self)==ftCo_MS_Wait &&
               !(fp->input.held_buttons[0]&PAD_BUTTON_A))attack_frame=frame;
            if(attack_frame && frame-attack_frame<4)input.button=PAD_BUTTON_A;
            if(getenv("MELEE_DIAGNOSTIC_MASTERHAND_COMBAT") && diagnostic_vs_entries==11){
                HSD_GObj* boss=NULL;
                int slot;
                memset(&input,0,sizeof(input));
                for(slot=1;slot<6;++slot){
                    HSD_GObj* candidate=Player_GetEntity(slot);
                    if(candidate && ((Fighter*)candidate->user_data)->kind==Ft_Kind_MasterH){boss=candidate;break;}
                }
                if(boss){
                    Fighter* hand=boss->user_data;
                    f32 goal=hand->cur_pos.x;
                    unsigned cycle=frame%100;
                    if(goal>50)goal=50;
                    if(goal< -50)goal= -50;
                    if(frame>=180){
                        if(fp->cur_pos.x<goal-5)input.stickX=55;
                        else if(fp->cur_pos.x>goal+5)input.stickX=-55;
                        if(cycle<4 && ftLib_GetGroundAir(self)==GA_Ground)input.button=PAD_BUTTON_X;
                        if(cycle>=12 && cycle<16){input.button=PAD_BUTTON_A;input.stickY=80;}
                    }
                    /* Optional passive trace for reproducing nondeterministic
                     * Master Hand route failures. This observes the fighter,
                     * stock/damage counters, and the stage's real blast-zone
                     * limits without changing player input or match state. */
                    if(getenv("MELEE_DIAGNOSTIC_MASTERHAND_FRAME_TRACE") &&
                       frame>=3818 && frame<=3828){
                        OSReport("NATIVE_MASTERHAND_FRAME_TRACE frame=%u stocks=%d damage=%d "
                                 "x=%f y=%f vx=%f vy=%f motion=%d grounded=%d jumps=%u "
                                 "outside_blast=%d blast_l=%f blast_r=%f blast_b=%f blast_t=%f "
                                 "boss_hp=%d result=%d state=%d input_x=%d input_y=%d input_buttons=%u\n",
                            frame,Player_GetStocks(0),Player_GetDamage(0),
                            fp->cur_pos.x,fp->cur_pos.y,fp->self_vel.x,fp->self_vel.y,
                            ftLib_GetMotionId(self),ftLib_GetGroundAir(self)==GA_Ground,
                            fp->x1968_jumpsUsed,Stage_IsOutsideBlastZone(&fp->cur_pos),
                            Stage_GetBlastZoneLeftOffset(),Stage_GetBlastZoneRightOffset(),
                            Stage_GetBlastZoneBottomOffset(),Stage_GetBlastZoneTopOffset(),
                            Player_GetRemainingHP(slot),controller.state.match_result,
                            controller.state.unk_0,input.stickX,input.stickY,input.button);
                    }
                    if(frame%15==0)
                        OSReport("NATIVE_MASTERHAND_COMBAT frame=%u self_x=%f self_y=%f motion=%d boss_slot=%d boss_x=%f boss_y=%f boss_motion=%d hp=%d percent=%f requested_x=%d requested_y=%d button=%u\n",
                            frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,slot,
                            hand->cur_pos.x,hand->cur_pos.y,hand->motion_id,
                            Player_GetRemainingHP(slot),hand->dmg.x1830_percent,input.stickX,input.stickY,input.button);
                }
            }
            if(getenv("MELEE_DIAGNOSTIC_PUSHON_FIRST_ROUTE") && diagnostic_vs_entries==9 && stage_info.grkind==Gr_Kind_Pushon){
                bool grounded=ftLib_GetGroundAir(self)==GA_Ground;
                memset(&input,0,sizeof(input));
                if(frame>=180 && !route_complete){
                    if(fp->cur_pos.x<104.0f)input.stickX=55;
                    else if(fp->cur_pos.x>112.0f)input.stickX=-45;
                    if(grounded && input.stickX && fp->cur_pos.x>route_previous_x-0.01f && fp->cur_pos.x<route_previous_x+0.01f)++route_stalled;
                    else route_stalled=0;
                    if(grounded && route_stalled>=30 && frame%120<4)input.button=PAD_BUTTON_X;
                    if(grounded && fp->cur_pos.x>=95.0f && fp->cur_pos.x<=118.0f &&
                       fp->cur_pos.y>=625.0f && fp->cur_pos.y<=665.0f && Player_GetStocks(0)>0)++route_stable;
                    else route_stable=0;
                    if(route_stable>=30){
                        route_complete=frame;memset(&input,0,sizeof(input));
                        OSReport("NATIVE_PUSHON_FIRST_COMPLETE frame=%u stable=%u x=%f y=%f floor=%d stocks=%d damage=%d\n",
                                 frame,route_stable,fp->cur_pos.x,fp->cur_pos.y,fp->coll_data.floor.index,Player_GetStocks(0),Player_GetDamage(0));
                    }
                }
                if(getenv("MELEE_DIAGNOSTIC_PUSHON_SECOND_ROUTE") && route_complete){
                    if(frame>=route_complete+30 && !second_complete){
                        if(fp->cur_pos.x<296.0f)input.stickX=65;
                        else if(fp->cur_pos.x>304.0f)input.stickX=-45;
                        if(grounded && input.stickX && fp->cur_pos.x>second_previous_x-0.01f && fp->cur_pos.x<second_previous_x+0.01f)++second_stalled;
                        else second_stalled=0;
                        if(grounded && second_stalled>=20 && frame%120<4)input.button=PAD_BUTTON_X;
                        if(grounded && fp->cur_pos.x>=290.0f && fp->cur_pos.x<=312.0f &&
                           fp->cur_pos.y>=620.0f && fp->cur_pos.y<=660.0f && Player_GetStocks(0)>0)++second_stable;
                        else second_stable=0;
                        if(second_stable>=30){
                            second_complete=frame;memset(&input,0,sizeof(input));
                            OSReport("NATIVE_PUSHON_SECOND_COMPLETE frame=%u stable=%u x=%f y=%f floor=%d stocks=%d damage=%d\n",
                                frame,second_stable,fp->cur_pos.x,fp->cur_pos.y,fp->coll_data.floor.index,Player_GetStocks(0),Player_GetDamage(0));
                        }
                    }
                    second_previous_x=fp->cur_pos.x;
                    if(frame%5==0)OSReport("NATIVE_PUSHON_SECOND_INPUT frame=%u motion=%d ground=%d x=%f y=%f floor=%d stocks=%d damage=%d stable=%u complete=%u requested_x=%d requested_button=%u held=%u pressed=%u latched_x=%f\n",
                        frame,fp->motion_id,grounded,fp->cur_pos.x,fp->cur_pos.y,fp->coll_data.floor.index,
                        Player_GetStocks(0),Player_GetDamage(0),second_stable,second_complete,input.stickX,input.button,
                        fp->input.held_buttons[0],fp->input.pressed_buttons,fp->input.lstick[0].x);
                }
                route_previous_x=fp->cur_pos.x;
                if(frame%5==0)OSReport("NATIVE_PUSHON_FIRST_INPUT frame=%u motion=%d ground=%d x=%f y=%f floor=%d stocks=%d damage=%d stable=%u complete=%u requested_x=%d requested_button=%u held=%u pressed=%u latched_x=%f\n",
                    frame,fp->motion_id,grounded,fp->cur_pos.x,fp->cur_pos.y,fp->coll_data.floor.index,
                    Player_GetStocks(0),Player_GetDamage(0),route_stable,route_complete,input.stickX,input.button,
                    fp->input.held_buttons[0],fp->input.pressed_buttons,fp->input.lstick[0].x);
            }
            if(frame%5==0 || (attack_frame && frame-attack_frame<12))
                OSReport("NATIVE_FIGURE_NEXT_INPUT frame=%u motion=%d ground=%d x=%f y=%f stocks=%d requested_x=%d requested_button=%u held=%u pressed=%u latched_x=%f attack_frame=%u\n",
                         frame,fp->motion_id,ftLib_GetGroundAir(self)==GA_Ground,fp->cur_pos.x,fp->cur_pos.y,
                         Player_GetStocks(0),input.stickX,input.button,fp->input.held_buttons[0],fp->input.pressed_buttons,
                         fp->input.lstick[0].x,attack_frame);
#if defined(_WIN32)
            if(frame>=180 && !next_capture &&
               (!getenv("MELEE_DIAGNOSTIC_PUSHON_DOOR_EXIT") ||
                diagnostic_vs_entries==(getenv("MELEE_DIAGNOSTIC_CLASSIC_TENTH_COMBAT")?11:10)) &&
               (diagnostic_vs_entries==(getenv("MELEE_DIAGNOSTIC_CLASSIC_TENTH_COMBAT")?11:10) || !getenv("MELEE_DIAGNOSTIC_PUSHON_FIRST_ROUTE") ||
                (getenv("MELEE_DIAGNOSTIC_PUSHON_SECOND_ROUTE")?second_complete:route_complete))){
                char* path=NULL;size_t size=0;
                if(_dupenv_s(&path,&size,"MELEE_DIAGNOSTIC_THIRD_STAGE_CAPTURE")==0 && path && path[0]){
                    next_capture=_putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE",path)==0;
                    OSReport("NATIVE_FIGURE_NEXT_STAGE capture_armed=%d frame=%u stage_kind=%d entry=%u\n",
                             next_capture,frame,stage_info.grkind,diagnostic_vs_entries);
                }
                free(path);
            }
#endif
        }
        if (!HSD_PadStageDiagnosticInput(0, &input))
            OSReport("NATIVE_CLASSIC_COMBAT input_refused=1\n");
    }
    if ((getenv("MELEE_DIAGNOSTIC_CLASSIC_DEFEAT") &&
         gm_GetCurrentGameMode() == GM_CLASSIC) ||
        (getenv("MELEE_DIAGNOSTIC_STAGE_NAVIGATION") &&
         gm_GetCurrentGameMode() == GM_VS)) {
        PADStatus input = { 0 };
        unsigned frame = controller.state.frame_count;
        int stocks = Player_GetStocks(0);
        HSD_GObj* entity = Player_GetEntity(0);
        static int previous_stocks = -1;
        static unsigned stalled_ticks, obstacle_jump_frame;
        static f32 previous_x;
        /* Natural off-stage movement only; original KO/stock/Game Over rules
         * choose the outcome. No damage, stocks, outcome or route writes. */
        if (frame >= 180) input.stickX = -80;
        if (entity && frame >= 180) {
            Vec3 position;
            ftLib_GetPos(entity, &position);
            if (stocks != previous_stocks) {
                stalled_ticks = obstacle_jump_frame = 0;
                previous_x = position.x;
            }
            if (ftLib_GetGroundAir(entity) == GA_Ground &&
                position.x > previous_x - 0.01f && position.x < previous_x + 0.01f)
                ++stalled_ticks;
            else stalled_ticks = 0;
            previous_x = position.x;
            /* Original solid blocks can stop a held direction (Green Greens).
             * Cross them with normal jump/release/double-jump input only. */
            if (stalled_ticks >= 90 &&
                !getenv("MELEE_DIAGNOSTIC_STAGE_NAV_BASELINE")) {
                obstacle_jump_frame = frame;
                stalled_ticks = 0;
                OSReport("NATIVE_CLASSIC_DEFEAT obstacle_jump=1 frame=%u x=%f y=%f\n",
                         frame, position.x, position.y);
            }
            if (obstacle_jump_frame) {
                unsigned elapsed = frame - obstacle_jump_frame;
                if (elapsed < 4 || (elapsed >= 30 && elapsed < 34))
                    input.button = PAD_BUTTON_X;
            }
        }
        if (entity && ftLib_GetMotionId(entity) == ftCo_MS_CliffWait) {
            /* Original CliffWait requires a neutral sample before a drop.
             * Keep Down out of grounded movement so it cannot crouch. */
            input.stickX = 0;
            input.button = 0;
            if (frame % 30 >= 6) input.stickY = -80;
        }
        if (!HSD_PadStageDiagnosticInput(Player_GetPadPort(0), &input))
            OSReport("NATIVE_CLASSIC_DEFEAT input_refused=1 frame=%u\n", frame);
        if (getenv("MELEE_DIAGNOSTIC_STAGE_NAVIGATION") && entity &&
            (frame % 10 == 0 || input.button ||
             (HSD_PadCopyStatus[Player_GetPadPort(0)].trigger & PAD_BUTTON_X))) {
            Vec3 position;
            Fighter* state = entity->user_data;
            ftLib_GetPos(entity, &position);
            OSReport("NATIVE_STAGE_NAV frame=%u motion=%d ground=%d x=%f y=%f collision=%x requested_x=%d button=%u pad_x=%d trigger=%u stalled=%u\n",
                     frame, ftLib_GetMotionId(entity), ftLib_GetGroundAir(entity),
                     position.x, position.y, (unsigned) state->coll_data.env_flags,
                     input.stickX, input.button,
                     HSD_PadCopyStatus[Player_GetPadPort(0)].stickX,
                     HSD_PadCopyStatus[Player_GetPadPort(0)].trigger, stalled_ticks);
        }
        if (stocks != previous_stocks || frame % 120 == 0) {
            Vec3 position = { 0 };
            if (entity) ftLib_GetPos(entity, &position);
            OSReport("NATIVE_CLASSIC_DEFEAT frame=%u stocks=%d result=%d motion=%d x=%f y=%f\n",
                     frame, stocks, controller.state.match_result,
                     entity ? ftLib_GetMotionId(entity) : -1,
                     position.x, position.y);
            previous_stocks = stocks;
        }
    }
    /* Test only: exercise the original pause/no-contest input path without
     * assigning match outcome, damage, stocks or scene routing. Use calls for
     * the release delay because simulation frame_count stops while paused. */
    if (getenv("MELEE_DIAGNOSTIC_CLASSIC_PAUSE_EXIT") &&
        gm_GetCurrentGameMode() == GM_CLASSIC) {
        static unsigned calls, start_call;
        PADStatus input = { 0 };
        unsigned elapsed = 0;
        ++calls;
        if (getenv("MELEE_DIAGNOSTIC_CLASSIC_CONTINUE_PLAY") && !start_call) {
            unsigned frame = controller.state.frame_count;
            if (frame >= 180 && frame < 196) input.stickX = 35;
            if (frame >= 240 && frame < 256) input.stickX = -35;
            if (frame >= 300 && frame < 308) input.button = PAD_BUTTON_X;
        }
        if (!start_call && controller.state.frame_count >= 360) start_call = calls;
        if (start_call) {
            elapsed = calls - start_call;
            if (elapsed < 4)
                input.button = DbLevel >= DbLKind_DebugRom ?
                    PAD_BUTTON_UP | PAD_BUTTON_X : PAD_BUTTON_START;
            if (elapsed >= 120 && elapsed < 124)
                input.button = PAD_TRIGGER_L | PAD_TRIGGER_R |
                               PAD_BUTTON_A | PAD_BUTTON_START;
            if (elapsed <= 150 && elapsed % 15 == 0)
                OSReport("NATIVE_CLASSIC_PAUSE_EXIT elapsed=%u frame=%u paused=%d requested=%u outcome=%d\n",
                         elapsed, controller.state.frame_count, gm_GetDbPauseFlag(1),
                         input.button, controller.state.match_result);
        }
        if (!HSD_PadStageDiagnosticInput(0, &input))
            OSReport("NATIVE_CLASSIC_PAUSE_EXIT input_refused=1\n");
        if (getenv("MELEE_DIAGNOSTIC_CLASSIC_CONTINUE_PLAY")) {
            unsigned frame = controller.state.frame_count;
            if (frame % 60 == 0 || (frame >= 178 && frame <= 198) ||
                (frame >= 238 && frame <= 258) || (frame >= 298 && frame <= 310)) {
                Vec3 position = { 0 };
                HSD_GObj* entity = Player_GetEntity(0);
                if (entity) ftLib_GetPos(entity, &position);
                OSReport("NATIVE_CLASSIC_RESUME calls=%u frame=%u stocks=%d result=%d paused=%d motion=%d x=%f y=%f requested_x=%d pad_x=%d button=%u trigger=%u\n",
                         calls, frame, Player_GetStocks(0), controller.state.match_result,
                         gm_GetDbPauseFlag(1), entity ? ftLib_GetMotionId(entity) : -1,
                         position.x, position.y, input.stickX,
                         HSD_PadCopyStatus[0].stickX, HSD_PadCopyStatus[0].button,
                         HSD_PadCopyStatus[0].trigger);
            }
        }
    }
    /* This callback also owns the original in-match clear screen (state 2).
     * Script only active gameplay; physical confirmation must survive there. */
    if (getenv("MELEE_DIAGNOSTIC_MATCH_INPUT") && controller.state.unk_0 == 0) {
        PADStatus input = { 0 };
        unsigned frame = controller.state.frame_count;
        HSD_JObjDiagnosticMatchFrame = frame;
        if (getenv("MELEE_DIAGNOSTIC_MATCH_PAUSE")) {
            static unsigned calls, start_call;
            ++calls;
            if (!start_call && frame >= 720) start_call = calls;
            if (start_call) {
                unsigned elapsed = calls - start_call;
                if (elapsed < 4 || (elapsed >= 120 && elapsed < 124))
                    input.button = DbLevel >= DbLKind_DebugRom ?
                        PAD_BUTTON_UP | PAD_BUTTON_X : PAD_BUTTON_START;
                if (!gmvs_diagnostic_quiet_runtime() && elapsed <= 180 && elapsed % 15 == 0)
                    OSReport("NATIVE_MATCH_PAUSE calls=%u elapsed=%u frames=%u seconds=%u timer_frames=%u paused=%d requested=%u pad=%u trigger=%u\n",
                             calls, elapsed, frame, controller.state.timer_seconds,
                             controller.state.unk_2C, gm_GetDbPauseFlag(1),
                             input.button, HSD_PadCopyStatus[0].button,
                             HSD_PadCopyStatus[0].trigger);
            }
        }
        if (frame >= 180 && frame < 240) input.stickX = 45;
        if (frame >= 300 && frame < 360) input.stickX = -45;
        if (frame >= 420 && frame < 426) input.button |= PAD_BUTTON_A;
        if (frame >= 450 && frame < 456) input.button |= PAD_BUTTON_B;
        if (getenv("MELEE_DIAGNOSTIC_PEACH_PARASOL") && frame >= 780 && frame < 788) {
            input.button = PAD_BUTTON_B;
            input.stickY = 80;
        }
        if (getenv("MELEE_DIAGNOSTIC_PEACH_SIDE_SPECIAL") && frame >= 780 && frame < 788) {
            input.button = PAD_BUTTON_B;
            input.stickX = 80;
        }
        if (getenv("MELEE_DIAGNOSTIC_PEACH_FLOAT") && frame >= 780 &&
            frame < (getenv("MELEE_DIAGNOSTIC_PEACH_FLOAT_EXHAUST") ? 1100 :
                     getenv("MELEE_DIAGNOSTIC_PEACH_FLOAT_ATTACK") ? 930 : 870)) {
            input.button = PAD_BUTTON_X;
            if (frame >= 800) input.stickY = -80;
            if (getenv("MELEE_DIAGNOSTIC_PEACH_FLOAT_ATTACK") && frame >= 820) {
                input.stickY = 0;
                if (frame < 828) input.button |= PAD_BUTTON_A;
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_MATCH_ACTIONS")) {
            if (frame >= 420 && frame < 428) input.button = PAD_BUTTON_X;
            if (frame >= 600 && frame < 608) input.button = PAD_BUTTON_A;
            if (getenv("MELEE_DIAGNOSTIC_MATCH_SPECIALS")) {
                if (frame >= 780 && frame < 788) input.button = PAD_BUTTON_B;
                if (frame >= 960 && frame < 980) {
                    input.button = PAD_BUTTON_B;
                    input.stickY = -80;
                }
                if (getenv("MELEE_DIAGNOSTIC_PEACH_TURNIP_THROW") &&
                    frame >= 1080 && frame < 1088) {
                    input.button = PAD_BUTTON_A;
                    input.stickX = 80;
                }
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_MATCH_EXTRA_SPECIALS")) {
            /* Purin Rest recovery ends after the generic side-B window.
             * Shift diagnostic pad timing only; original move logic is unchanged. */
            unsigned side_frame = getenv("MELEE_DIAGNOSTIC_PURIN_EXTRA_SPECIALS") ? 1260 : 1200;
            if (frame >= side_frame && frame < side_frame + 8) {
                input.button = PAD_BUTTON_B;
                input.stickX = 80;
            }
            if (frame >= 1440 && frame < 1448) {
                input.button = PAD_BUTTON_B;
                input.stickY = 80;
            }
        }
        /* Ordinary B edge releases Mewtwo charge before the later down-B test. */
        if (getenv("MELEE_DIAGNOSTIC_MEWTWO_CHARGE_RELEASE") &&
            frame >= 870 && frame < 878)
            input.button = PAD_BUTTON_B;
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_SHOT") && frame >= 870 && frame < 878)
            input.button = PAD_BUTTON_B;
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_STORE")) {
            if (frame >= 870 && frame < 878) input.button = PAD_TRIGGER_L;
            if (frame >= 960 && frame < 980) input.stickY = 0;
            if (frame >= 1080 && frame < 1088) input.button = PAD_BUTTON_B;
        }
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_MISSILES") && frame >= 750) {
            memset(&input, 0, sizeof(input));
            if (frame < 788) input.stickX = -60;
            if (frame >= 780 && frame < 788) input.button = PAD_BUTTON_B;
            if (frame >= 960 && frame < 968) {
                input.stickX = -80;
                input.button = PAD_BUTTON_B;
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_SCREW_ATTACK") && frame >= 750) {
            memset(&input, 0, sizeof(input));
            if (frame >= 780 && frame < 788) {
                input.button = PAD_BUTTON_B;
                input.stickY = 80;
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_GRAPPLE") && frame >= 750) {
            memset(&input, 0, sizeof(input));
            if (frame >= 780 && frame < 788) input.button = PAD_BUTTON_X;
            if (frame >= 800 && frame < 808) input.button = PAD_TRIGGER_Z;
        }
        /* Diagnostic only: ordinary two-player pad input, no fighter-state writes. */
        if (getenv("MELEE_DIAGNOSTIC_KIRBY_MARIO_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_LINK_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_FOX_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_SAMUS_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_FALCO_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_CAPTAIN_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_KOOPA_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_DONKEY_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_PIKACHU_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_NESS_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_PEACH_COPY_INPUT") ||
            getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_COPY_INPUT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Fighter* kirby = first->user_data;
                Fighter* mario = second->user_data;
                if (kirby->kind == Ft_Kind_Kirby &&
                    ((mario->kind == Ft_Kind_Mario &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_MARIO_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Mewtwo &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Link &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_LINK_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Fox &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_FOX_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Samus &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_SAMUS_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Falco &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_FALCO_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Captain &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_CAPTAIN_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Koopa &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_KOOPA_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Donkey &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_DONKEY_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Yoshi &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Purin &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Pikachu &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_PIKACHU_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_GameWatch &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_GAMEWATCH_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Popo &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Peach &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_PEACH_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Ness &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_NESS_COPY_INPUT")) ||
                     (mario->kind == Ft_Kind_Luigi &&
                      getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_COPY_INPUT")))) {
                    Vec3 a, b;
                    /* The same flag also enables PAD readback diagnostics.
                     * Clear scripted P1 input only for the actual copy test. */
                    memset(&input, 0, sizeof(input));
                    ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                    /* Readback is the previously consumed sample, not proof
                     * that the request issued later this frame was consumed. */
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT") &&
                        frame % 30 == 0) {
                        unsigned port = mario->pad_port;
                        OSReport("NATIVE_KIRBY_MEWTWO_PAD frame=%u port=%u player_port=%u fighter_x=%f fighter_y=%f\n",
                                 frame, port, Player_GetPadPort(1),
                                 mario->input.lstick[0].x,
                                 mario->input.lstick[0].y);
                        if (port < 4) {
                            OSReport("NATIVE_KIRBY_MEWTWO_PAD_STAGES frame=%u port=%u master_err=%d master_x=%d copy_err=%d copy_x=%d game_err=%d game_x=%d game_normal_x=%f\n",
                                     frame, port,
                                     HSD_PadMasterStatus[port].err,
                                     HSD_PadMasterStatus[port].stickX,
                                     HSD_PadCopyStatus[port].err,
                                     HSD_PadCopyStatus[port].stickX,
                                     HSD_PadGameStatus[port].err,
                                     HSD_PadGameStatus[port].stickX,
                                     HSD_PadGameStatus[port].nml_stickX);
                        }
                    }
                    /* This paired test starts on opposite Yoshi platforms.
                     * Drop through using ordinary pad input before approaching. */
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_PEACH_COPY_INPUT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_GAMEWATCH_COPY_INPUT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_COPY_INPUT")) &&
                        frame >= 120 && frame < 128) {
                        input.stickY = -80;
                        other.stickY = -80;
                    }
                    if (frame >= 180 && frame < 420) {
                        float front = getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT")
                                          ? kirby->facing_dir * 14.0f : 14.0f;
                        float dx = a.x + front - b.x;
                        if (dx > 2.0f) other.stickX = 35;
                        if (dx < -2.0f) other.stickX = -35;
                        if (b.y > a.y + 8.0f) other.stickY = -80;
                    }
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_MARIO_FIREBALL_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_FIREBALL_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_LINK_ARROW_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_FOX_LASER_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_FALCO_LASER_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_PIKACHU_JOLT_HIT") ||
                         (getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT") &&
                          getenv("MELEE_DIAGNOSTIC_MEWTWO_SHADOWBALL_HIT"))) &&
                        frame >= 700 && frame < 860) {
                        if (frame < 708) other.button = PAD_BUTTON_X;
                        float dx = a.x + 40.0f - b.x;
                        if (dx > 2.0f) other.stickX = 35;
                        if (dx < -2.0f) other.stickX = -35;
                        if ((getenv("MELEE_DIAGNOSTIC_KIRBY_FALCO_LASER_HIT") ||
                             getenv("MELEE_DIAGNOSTIC_KIRBY_PIKACHU_JOLT_HIT")) &&
                            b.y > a.y + 8.0f) other.stickY = -80;
                    }
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_CAPTAIN_PUNCH_HIT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_KOOPA_FLAME_HIT")) &&
                        frame >= 700 && frame < 860) {
                        if (frame < 708) other.button = PAD_BUTTON_X;
                        float dx = a.x + 18.0f - b.x;
                        if (dx > 2.0f) other.stickX = 35;
                        if (dx < -2.0f) other.stickX = -35;
                        if (frame >= 708 && b.y > a.y + 8.0f) other.stickY = -80;
                    }
                    if (frame >= 420 && frame < 700) input.button = PAD_BUTTON_B;
                    /* Extend copy coverage using ordinary opponent movement.
                     * No hat, position, move or damage state is written. */
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT")) &&
                        frame >= 700 && frame < 860) {
                        float front = getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT")
                                          ? 18.0f : 40.0f;
                        float dx = a.x + kirby->facing_dir * front - b.x;
                        if (frame < 708) other.button = PAD_BUTTON_X;
                        if (dx > 2.0f) other.stickX = 35;
                        if (dx < -2.0f) other.stickX = -35;
                        if (frame >= 708 && b.y > a.y + 8.0f)
                            other.stickY = -80;
                    }
                    /* Ordinary movement places Popo in front after the late
                     * partner-assisted inhale, before the copied shot at1200. */
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_ICE_HIT") &&
                        kirby->u.kb.hat.kind == Ft_Kind_Popo &&
                        frame >= 1000 && frame < 1170) {
                        float dx = a.x + kirby->facing_dir * 35.0f - b.x;
                        /* Jump over Kirby instead of pushing his collision
                         * capsule along the ground from behind. */
                        if (frame < 1008) other.button = PAD_BUTTON_X;
                        if (dx > 2.0f) other.stickX = 35;
                        if (dx < -2.0f) other.stickX = -35;
                        if (frame >= 1040 && b.y > a.y + 8.0f)
                            other.stickY = -80;
                    }
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_NESS_PKFLASH_HIT") &&
                        frame >= 860 && frame < 1320) {
                        float dx = a.x - b.x;
                        if (dx > 1.0f) other.stickX = 25;
                        if (dx < -1.0f) other.stickX = -25;
                        if ((frame >= 920 && frame < 928) ||
                            (frame >= 1220 && frame < 1228))
                            other.button = PAD_BUTTON_X;
                    }
                    if (frame >= 550 && frame < 558) input.stickY = -80;
                    /* Copy tests with a partner must swallow when the inhale
                     * actually finishes, not at a fixed frame that can miss it. */
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_COPY_INPUT") &&
                        kirby->victim_gobj != NULL &&
                        (kirby->motion_id == ftKb_MS_EatWait ||
                         kirby->motion_id == ftKb_MS_Eat))
                        input.stickY = -80;
                    if ((frame >= 900 && frame < 908) ||
                        (frame >= 1200 && frame < 1208)) input.button = PAD_BUTTON_B;
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT") &&
                        frame >= 900 && frame < 1020)
                        input.button = PAD_BUTTON_B;
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_LINK_ARROW_HIT") &&
                        ((frame >= 900 && frame < 960) ||
                         (frame >= 1200 && frame < 1260))) input.button = PAD_BUTTON_B;
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_KOOPA_COPY_INPUT") &&
                        ((frame >= 900 && frame < 1020) ||
                         (frame >= 1200 && frame < 1320))) input.button = PAD_BUTTON_B;
                    if (frame % 30 == 0)
                        OSReport("NATIVE_KIRBY_COPY_INPUT frame=%u hat=%d motion=%d x=%f y=%f opponent_motion=%d opponent_x=%f opponent_y=%f button=%u stick_y=%d other_x=%d other_y=%d\n",
                                 frame, kirby->u.kb.hat.kind, ftLib_GetMotionId(first),
                                 a.x, a.y, ftLib_GetMotionId(second), b.x, b.y,
                                 input.button, input.stickY, other.stickX, other.stickY);
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT")) &&
                        frame % 10 == 0)
                        OSReport("NATIVE_KIRBY_COPY_USE frame=%u hat=%d motion=%d opponent_motion=%d opponent_damage=%d\n",
                                 frame, kirby->u.kb.hat.kind, kirby->motion_id,
                                 mario->motion_id, Player_GetDamage(1));
                    if ((getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT") ||
                         getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT")) &&
                        frame % 30 == 0)
                        OSReport("NATIVE_KIRBY_COPY_PAD frame=%u requested_x=%d consumed_x=%f consumed_y=%f\n",
                                 frame, other.stickX, mario->input.lstick[0].x,
                                 mario->input.lstick[0].y);
                    if (getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT") &&
                        (frame % 30 == 0 || (frame >= 1200 && frame <= 1225 && frame % 5 == 0)))
                        OSReport("NATIVE_KIRBY_MEWTWO_STATE frame=%u motion=%d held=%p charge=%d cmd0=%d cmd1=%d cmd2=%d cmd3=%d\n",
                                 frame, kirby->motion_id, kirby->u.kb.x98,
                                 kirby->u.kb.x9C, kirby->cmd_vars[0],
                                 kirby->cmd_vars[1], kirby->cmd_vars[2],kirby->cmd_vars[3]);
                }
            }
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_FIREBALL_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_LUIGI_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_POPO_ICE_HIT") && frame % 10 == 0)
                OSReport("NATIVE_KIRBY_ICE_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_MARIO_FIREBALL_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_MARIO_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_NESS_PKFLASH_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_NESS_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_MEWTWO_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_LINK_ARROW_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_LINK_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_FOX_LASER_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_FOX_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_FALCO_LASER_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_FALCO_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_CAPTAIN_PUNCH_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_CAPTAIN_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_KOOPA_FLAME_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_KOOPA_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_DONKEY_PUNCH_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_DONKEY_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_PIKACHU_JOLT_HIT") && frame % 30 == 0)
                OSReport("NATIVE_KIRBY_PIKACHU_DAMAGE frame=%u opponent_damage=%d\n",
                         frame, Player_GetDamage(1));
            if (getenv("MELEE_DIAGNOSTIC_KIRBY_YOSHI_COPY_INPUT") ||
                getenv("MELEE_DIAGNOSTIC_KIRBY_PURIN_COPY_INPUT")) {
                /* Match P1's next-latch staging, avoiding asynchronous SDL
                 * virtual-pad updates for these two deterministic probes. */
                if (!HSD_PadStageDiagnosticInput(1, &other))
                    OSReport("NATIVE_KIRBY_COPY_OTHER_STAGE_FAILED frame=%u\n", frame);
            } else {
                PADSetVirtualStatus(1, &other);
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_PEACH_SIDE_CONTACT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                if (frame >= 650 && frame < 780) {
                    float dx = a.x + 20.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame >= 780 && frame <= 1000 && frame % 5 == 0)
                    OSReport("NATIVE_PEACH_SIDE_CONTACT frame=%u damage=%d motion=%d x=%f y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_PEACH_TURNIP_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                if (frame >= 900 && frame < 1080) {
                    float dx = a.x + 18.0f - b.x;
                    if (dx > 2.0f) other.stickX = 25;
                    if (dx < -2.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (!gmvs_diagnostic_quiet_runtime() && frame % 30 == 0)
                    OSReport("NATIVE_PEACH_THROW_TARGET frame=%u damage=%d x=%f y=%f slot_type=%d pad_port=%d cpu_kind=%d\n",
                             frame, Player_GetDamage(1), b.x, b.y,
                             Player_GetPlayerSlotType(1), Player_GetPadPort(1),
                             Player_GetCpuType(1));
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_PEACH_TOAD_COUNTER")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                if (frame >= 650 && frame < 750) {
                    float dx = a.x + 7.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                /* Ordinary walk inputs turn each fighter toward the other. */
                if (frame >= 750 && frame < 754) input.stickX = 45;
                if (frame >= 758 && frame < 762) other.stickX = -45;
                if (frame >= 800 && frame < 808) other.button = PAD_BUTTON_A;
                if (frame % 30 == 0 || (frame >= 780 && frame <= 850 && frame % 5 == 0)) {
                    Fighter* peach = first->user_data;
                    Fighter* attacker = second->user_data;
                    OSReport("NATIVE_PEACH_COUNTER_TARGET frame=%u motion=%d damage=%d x=%f y=%f\n",
                             frame, ftLib_GetMotionId(second), Player_GetDamage(1), b.x, b.y);
                    OSReport("NATIVE_PEACH_COUNTER_WINDOW frame=%u peach_motion=%d cmd1=%d active=%d facing=%f attacker_facing=%f dx=%f\n",
                             frame, peach->motion_id, peach->cmd_vars[1],
                             peach->x221B.x221B_b0, peach->facing_dir,
                             attacker->facing_dir, b.x-a.x);
                }
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_LUIGI_FIREBALL_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Luigi) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Approach the projectile path through ordinary controller input. */
                if (frame >= 650 && frame < 780) {
                    float dx = a.x - 20.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame % 30 == 0)
                    OSReport("NATIVE_LUIGI_FIREBALL_TARGET frame=%u damage=%d motion=%d x=%f y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_ICE_SHOT_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Popo) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Walk into the projectile path; do not reposition fighters. */
                if (frame >= 650 && frame < 780) {
                    float dx = a.x + 30.0f - b.x;
                    if (dx > 3.0f) other.stickX = 45;
                    if (dx < -3.0f) other.stickX = -45;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                /* Face the actual opponent after the approach, on either side. */
                if (frame >= 750 && frame < 754)
                    input.stickX = b.x > a.x ? 25 : -25;
                if (frame % 30 == 0)
                    OSReport("NATIVE_ICE_SHOT_TARGET frame=%u damage=%d motion=%d x=%f y=%f leader_x=%f facing=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y, a.x, ((Fighter*) first->user_data)->facing_dir);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_BLIZZARD_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Popo) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Walk into the projectile path; do not reposition fighters. */
                if (frame >= 840 && frame < 960) {
                    float dx = a.x + ((Fighter*) first->user_data)->facing_dir * 15.0f - b.x;
                    if (dx > 3.0f) other.stickX = 45;
                    if (dx < -3.0f) other.stickX = -45;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                /* Face the actual opponent after the approach, on either side. */
                if (frame >= 930 && frame < 934)
                    input.stickX = b.x > a.x ? 25 : -25;
                if (frame % 30 == 0)
                    OSReport("NATIVE_BLIZZARD_TARGET frame=%u damage=%d motion=%d x=%f y=%f leader_x=%f facing=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y, a.x, ((Fighter*) first->user_data)->facing_dir);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_YOSHI_TONGUE")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Yoshi) {
                Fighter* yoshi = first->user_data;
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Approach the tongue through normal pad input only. */
                if (frame >= 650 && frame < 780) {
                    float dx = a.x + yoshi->facing_dir * 6.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (!gmvs_diagnostic_quiet_runtime() && frame >= 650 && frame % 5 == 0)
                    OSReport("NATIVE_YOSHI_TONGUE_TARGET frame=%u damage=%d motion=%d x=%f y=%f attacker_x=%f attacker_y=%f facing=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second),
                             b.x, b.y, a.x, a.y, yoshi->facing_dir);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_PURIN_COMBAT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Purin) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Approach through normal pad input; never reposition fighters. */
                if ((frame >= 1150 && frame < 1260) ||
                    (frame >= 1350 && frame < 1440)) {
                    float dx = a.x + 6.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame >= 1150 && frame % 5 == 0)
                    OSReport("NATIVE_PURIN_COMBAT_TARGET frame=%u damage=%d motion=%d x=%f y=%f attacker_x=%f attacker_y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second),
                             b.x, b.y, a.x, a.y);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_MEWTWO_SHADOWBALL_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second && ((Fighter*) first->user_data)->kind == Ft_Kind_Mewtwo) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                /* Approach the projectile path through ordinary controller input. */
                if (frame >= 650 && frame < 860) {
                    float dx = a.x - 20.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame % 30 == 0)
                    OSReport("NATIVE_MEWTWO_SHADOWBALL_TARGET frame=%u damage=%d motion=%d x=%f y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y);
                /* The hit flag also enables shared item logging for Kirby's
                 * copied projectile. Only Mewtwo's own test owns this pad. */
                PADSetVirtualStatus(1, &other);
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_SHOT_HIT") ||
            getenv("MELEE_DIAGNOSTIC_SAMUS_MISSILE_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                if (frame >= 650 && frame < 780) {
                    float dx = a.x - 20.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame % 30 == 0)
                    OSReport(getenv("MELEE_DIAGNOSTIC_SAMUS_MISSILE_HIT") ?
                             "NATIVE_SAMUS_MISSILE_TARGET frame=%u damage=%d motion=%d x=%f y=%f\n" :
                             "NATIVE_SAMUS_SHOT_TARGET frame=%u damage=%d motion=%d x=%f y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_SAMUS_BOMB_HIT")) {
            PADStatus other = { 0 };
            HSD_GObj* first = Player_GetEntity(0);
            HSD_GObj* second = Player_GetEntity(1);
            if (first && second) {
                Vec3 a, b;
                ftLib_GetPos(first, &a); ftLib_GetPos(second, &b);
                if (frame >= 650 && frame < 940) {
                    float dx = a.x + 6.0f - b.x;
                    if (dx > 1.0f) other.stickX = 25;
                    if (dx < -1.0f) other.stickX = -25;
                    if (b.y > a.y + 8.0f) other.stickY = -80;
                }
                if (frame % 30 == 0)
                    OSReport("NATIVE_SAMUS_BOMB_TARGET frame=%u damage=%d motion=%d x=%f y=%f\n",
                             frame, Player_GetDamage(1), ftLib_GetMotionId(second), b.x, b.y);
            }
            PADSetVirtualStatus(1, &other);
        }
        if (getenv("MELEE_DIAGNOSTIC_ONETT_ROAD") && frame >= 600) {
            HSD_GObj* fighter = Player_GetEntity(0);
            input = (PADStatus) { 0 };
            if (fighter != NULL) {
                Vec3 position;
                ftLib_GetPos(fighter, &position);
                /* Walk off the starting roof, then wait on the road. All
                 * movement and damage still go through original gameplay. */
                if (position.y > 1.0F) {
                    input.stickX = 80;
                    input.stickY = -80;
                }
                if (frame % 60 == 0) {
                    OSReport("NATIVE_ONETT_ROAD frame=%u requested_x=%d requested_y=%d ground_air=%d damage=%d motion=%d x=%f y=%f\n",
                             frame, input.stickX, input.stickY,
                             ftLib_GetGroundAir(fighter), Player_GetDamage(0),
                             ftLib_GetMotionId(fighter), position.x, position.y);
                }
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_GARDEN_WATER") && frame >= 600) {
            HSD_GObj* fighter = Player_GetEntity(0);
            input = (PADStatus) { 0 };
            if (fighter != NULL) {
                Vec3 position;
                ftLib_GetPos(fighter, &position);
                /* Normal input only: leave the roof and continue left into
                 * the water. Never overwrite physics or fighter position. */
                input.stickX = -80;
                input.stickY = -80;
                if (frame % 60 == 0)
                    OSReport("NATIVE_GARDEN_WATER_INPUT frame=%u x=%f y=%f\n",
                             frame, position.x, position.y);
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_YORSTER_BLOCKS") && frame >= 600) {
            HSD_GObj* fighter = Player_GetEntity(0);
            input = (PADStatus) { 0 };
            if (fighter) {
                Vec3 position;
                ftLib_GetPos(fighter, &position);
                if (position.x < -2.0F) input.stickX = 40;
                if (position.x > 2.0F) input.stickX = -40;
                if (frame % 120 < 2) input.button = PAD_BUTTON_A;
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_NANA_COOP")) {
            PADStatus quiet = { 0 };
            input = quiet;
            if (frame >= 960 && frame < 980) {
                input.button = PAD_BUTTON_B;
                input.stickY = -80;
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
            getenv("MELEE_DIAGNOSTIC_LINK_GRAB_INPUT")) {
            /* Drive ordinary grab input, not a direct motion-state change.
             * Suppress the generic movement/attack samples in this probe. */
            PADStatus quiet = { 0 };
            input = quiet;
            if (frame >= 418 && frame < 428) input.button = PAD_TRIGGER_R;
            if (frame >= 420 && frame < 428) input.button |= PAD_BUTTON_A;
            if (frame >= 660 && frame < 680) input.stickX = 80;
            if (frame >= 664 && frame < 674)
                input.button = PAD_TRIGGER_R | PAD_BUTTON_A;
            if ((frame >= 410 && frame <= 450) ||
                (frame >= 655 && frame <= 700)) {
                if (frame % 5 == 0) {
                    HSD_GObj* entity = Player_GetEntity(0);
                    Fighter* state = entity ? entity->user_data : NULL;
                    OSReport("NATIVE_LINK_GRAB_INPUT frame=%u buttons=%u stick=%d kind=%d motion=%d\n",
                             frame, input.button, input.stickX,
                             state ? state->kind : -1,
                             state ? state->motion_id : -1);
                }
            }
        }
        if (getenv("MELEE_DIAGNOSTIC_ZELDA_RESULTS_TRANSFORM")) {
            static unsigned previous_frame, pulse_until;
            static int observed;
            HSD_GObj* entity = Player_GetEntity(0);
            Fighter* state = entity ? entity->user_data : NULL;
            if (frame < previous_frame) {
                pulse_until = 0;
                observed = 0;
            }
            previous_frame = frame;
            /* Feedback selects ordinary PAD input, never fighter state. Keep
             * the fighter still until grounded idle so the B edge cannot be
             * consumed during the generic probe's landing animation. */
            memset(&input, 0, sizeof(input));
            if (!pulse_until && frame >= 120 && state &&
                state->kind == Ft_Kind_Zelda &&
                state->motion_id == ftCo_MS_Wait &&
                ftLib_GetGroundAir(entity) == GA_Ground) {
                pulse_until = frame + 12;
                OSReport("NATIVE_ZELDA_RESULTS_TRANSFORM pulse_frame=%u grounded_idle=1\n", frame);
            }
            if (pulse_until && frame < pulse_until) {
                input.button = PAD_BUTTON_B;
                input.stickY = -80;
            }
            if (state && state->kind == Ft_Kind_Seak) {
                if (!observed) {
                    observed = 1;
                    OSReport("NATIVE_ZELDA_RESULTS_TRANSFORM observed_kind=%d frame=%u\n", state->kind, frame);
                }
                /* Retain a short ordinary walk-off attempt after the move;
                 * let original KO/timer/Results logic determine the outcome. */
                if (frame >= 300 && frame < 360) input.stickX = -45;
            }
        }
        if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_GOOMBA"))
            MeleePc_AdventureGoombaInput(Player_GetEntity(0),&input,frame);
        if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_KOOPA"))
            MeleePc_AdventureKoopaInput(Player_GetEntity(0),&input,frame);
        /* This opt-in probe runs after the current PAD latch. Use the SDL
         * virtual-device path when requested; otherwise stage the sample for
         * the next native PAD latch. Retail input remains unchanged. */
        if (getenv("AURORA_DIAGNOSTIC_SDL_GAMEPAD")) {
            /* Route the optional test sample through SDL's virtual gamepad,
             * so the next PADRead consumes the same mapping path used by a
             * connected controller. This remains opt-in diagnostic input. */
            PADSetVirtualStatus(0, &input);
        } else if (!HSD_PadStageDiagnosticInput(0, &input)) {
            OSReport("NATIVE_MATCH_INPUT_STAGE_FAILED frame=%u\n", frame);
        }
        if (!gmvs_diagnostic_quiet_runtime() &&
            (frame % 30 == 0 ||
             (getenv("MELEE_DIAGNOSTIC_MATCH_INPUT") && frame >= 410 &&
              frame <= 470 && frame % 5 == 0) ||
             (getenv("MELEE_DIAGNOSTIC_MATCH_ACTIONS") && frame >= 415 &&
              frame <= (getenv("MELEE_DIAGNOSTIC_MATCH_EXTRA_SPECIALS") ? 1560 : 1080) && frame % 5 == 0))) {
            HSD_GObj* fighter = Player_GetEntity(0);
            if (fighter) {
                Vec3 position;
                float stick_x, stick_y;
                ftLib_GetPos(fighter, &position);
                ftLib_GetLStick(fighter, &stick_x, &stick_y);
                if (getenv("MELEE_DIAGNOSTIC_ICE_CLIMBERS_PAIR") &&
                    ((Fighter*) fighter->user_data)->kind == Ft_Kind_Popo) {
                    HSD_GObj* companion = Player_GetEntityAtIndex(0, 1);
                    if (companion) {
                        Vec3 companion_position;
                        ftLib_GetPos(companion, &companion_position);
                        OSReport("NATIVE_ICE_CLIMBERS_PAIR frame=%u leader_kind=%d companion_kind=%d leader_motion=%d companion_motion=%d leader_x=%f companion_x=%f companion_y=%f\n",
                                 frame, ((Fighter*) fighter->user_data)->kind,
                                 ((Fighter*) companion->user_data)->kind,
                                 ftLib_GetMotionId(fighter), ftLib_GetMotionId(companion),
                                 position.x, companion_position.x, companion_position.y);
                        if (getenv("MELEE_DIAGNOSTIC_NANA_INPUT")) {
                            Fighter* leader_fp = fighter->user_data;
                            Fighter* nana_fp = companion->user_data;
                            OSReport("NATIVE_NANA_INPUT frame=%u cpu_kind=%d cpu_ticks=%u following=%d leader_buttons=%u companion_buttons=%u cpu_buttons=%u cpu_stick_y=%d companion_stick_y=%f\n",
                                     frame, nana_fp->cpu.kind, nana_fp->cpu.x7C,
                                     nana_fp->cpu.xFA_b7,
                                     leader_fp->input.held_buttons[0],
                                     nana_fp->input.held_buttons[0],
                                     nana_fp->cpu.buttons, nana_fp->cpu.lstick.y,
                                     nana_fp->input.lstick[0].y);
                        }
                    } else {
                        OSReport("NATIVE_ICE_CLIMBERS_PAIR frame=%u companion_missing=1\n", frame);
                    }
                }
                HSD_GObj* opponent = Player_GetEntity(1);
                Fighter* fighter_state = fighter->user_data;
                Fighter* opponent_state = opponent ? opponent->user_data : NULL;
                OSReport("NATIVE_MATCH_INPUT frame=%u requested=%d requested_button=%u pad=%d pad_button=%u pad_trigger=%u fighter_stick=%f motion=%d x=%f y=%f p1_kind=%d p2_kind=%d\n",
                         frame, input.stickX, input.button,
                         HSD_PadCopyStatus[0].stickX,
                         HSD_PadCopyStatus[0].button,
                         HSD_PadCopyStatus[0].trigger,
                         stick_x, ftLib_GetMotionId(fighter), position.x, position.y,
                         fighter_state->kind,
                         opponent_state ? opponent_state->kind : -1);
                if (getenv("MELEE_DIAGNOSTIC_SAMUS_STORE")) {
                    Fighter* fp = fighter->user_data;
                    if (fp->kind == Ft_Kind_Samus)
                        OSReport("NATIVE_SAMUS_CHARGE_STATE frame=%u motion=%d charge=%d attached=%d\n",
                                 frame, fp->motion_id, fp->u.ss.x2230,
                                 fp->u.ss.x222C != NULL);
                }
                if (getenv("MELEE_DIAGNOSTIC_MATCH_ACTIONS"))
                    OSReport("NATIVE_MATCH_ACTION frame=%u requested=%u pad=%u trigger=%u motion=%d ground_air=%d x=%f y=%f\n",
                             frame, input.button, HSD_PadCopyStatus[0].button,
                             HSD_PadCopyStatus[0].trigger, ftLib_GetMotionId(fighter),
                             ftLib_GetGroundAir(fighter), position.x, position.y);
            }
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_CLOSE_DURING_MATCH")) {
        static int close_queued;
        static unsigned close_frame, close_entry;
        if (!close_entry) {
            const char* text = getenv("MELEE_DIAGNOSTIC_MATCH_CLOSE_ENTRY");
            char* end = NULL;
            unsigned long requested = text ? strtoul(text, &end, 10) : 2;
            close_entry = text && end != text && !*end && requested >= 2 && requested <= 16
                              ? (unsigned) requested : 2;
        }
        if (!close_frame) {
            const char* text = getenv("MELEE_DIAGNOSTIC_MATCH_CLOSE_FRAME");
            char* end = NULL;
            unsigned long requested = text ? strtoul(text, &end, 10) : 360;
            close_frame = text && end != text && !*end && requested >= 360 && requested <= 7200
                              ? (unsigned) requested : 360;
        }
        if (!close_queued && controller.state.frame_count >= close_frame &&
            (!getenv("MELEE_DIAGNOSTIC_CLOSE_SECOND_MATCH") || diagnostic_vs_entries >= close_entry)) {
            if (!aurora_diagnostic_queue_quit())
                OSPanic(__FILE__, __LINE__, "Match diagnostic Quit queue failed");
            close_queued = 1;
            OSReport("NATIVE_MATCH_CLOSE_QUEUED frame=%u owner=1\n",
                     controller.state.frame_count);
            if (getenv("MELEE_DIAGNOSTIC_CLOSE_SECOND_MATCH")) {
                OSReport("NATIVE_SECOND_MATCH_CLOSE entries=%u frame=%u\n",
                         diagnostic_vs_entries, controller.state.frame_count);
                OSReport("NATIVE_MATCH_CLOSE_ENTRY entries=%u frame=%u\n",
                         diagnostic_vs_entries, controller.state.frame_count);
            }
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_MATCH_STATE") && !gmvs_diagnostic_quiet_runtime()) {
        static unsigned diagnostic_calls;
        static OSTime active_start;
        static unsigned active_first_frame;
        if (getenv("MELEE_DIAGNOSTIC_MATCH_TIMING") && controller.state.frame_count) {
            if (!active_start) {
                active_start = OSGetTime();
                active_first_frame = controller.state.frame_count;
            }
            if (diagnostic_calls % 120 == 0)
                OSReport("NATIVE_MATCH_TIMING frames=%u elapsed_ms=%llu\n",
                         controller.state.frame_count - active_first_frame,
                         (unsigned long long) OSTicksToMilliseconds(OSGetTime() - active_start));
        }
        if (++diagnostic_calls == 1 || diagnostic_calls % 120 == 0)
            OSReport("NATIVE_MATCH_STATE calls=%u state=%u frames=%u seconds=%u timer_frames=%u pause=%d intro=%d result=%u\n",
                     diagnostic_calls, controller.state.unk_0,
                     controller.state.frame_count, controller.state.timer_seconds,
                     controller.state.unk_2C, gm_GetDbPauseFlag(1),
                     gm_GetDbPauseFlag(0), controller.state.match_result);
    }
#endif
    if (controller.start.on_frame_start != NULL) {
        controller.start.on_frame_start();
    }
    switch (controller.state.unk_0) {
    case 0:
        fn_8016CFE0();
        break;
    case 1:
        fn_8016D634();
        break;
    case 2:
        if (fn_80180AC0() != 0) {
            controller.state.unk_0 = 3;
        }
        break;
    case 3:
        gm_801A4B60();
        break;
    }
    if (controller.start.on_frame_end != NULL) {
        controller.start.on_frame_end();
    }
}

void fn_8016D8AC(int arg0, struct PlayerInitData* arg1)
{
    VsSceneController* tmp = gmVs_GetSceneController();
    PAD_STACK(4);

    Player_SetSlottype(arg0, arg1->slot_type);
    Player_SetPlayerCharacter(arg0, arg1->ckind);

    tmp->state.fighters[arg0].x0 = arg1->ckind;

    Player_SetStocks(arg0, arg1->stocks);
    Player_SetCostumeId(arg0, arg1->color);
    Player_SetSubColor(arg0, arg1->sub_color);
    if (arg1->slot == 0) {
        Player_SetPadPort(arg0, arg0);
    } else {
        Player_SetPadPort(arg0, arg1->slot - 1);
    }
    if (arg1->slot_type == Gm_PKind_Human &&
        (HSD_PadCopyStatus[(u8) Player_GetPadPort(arg0)].button & HSD_PAD_A) &&
        (Player_GetPlayerCharacter(arg0) == CKind_Zelda ||
         Player_GetPlayerCharacter(arg0) == CKind_Seak))
    {
        if (Player_GetPlayerCharacter(arg0) == CKind_Zelda) {
            Player_SetPlayerCharacter(arg0, CKind_Seak);
        } else {
            Player_SetPlayerCharacter(arg0, CKind_Zelda);
        }
        tmp->state.fighters[arg0].x4.x0.x4_b4 = true;
    }

    tmp->state.fighters[arg0].spawn_point = arg1->spawn_pos;

    Player_SetHandicap(arg0, arg1->handicap);
    Player_SetTeam(arg0, arg1->team);
    Player_SetFlagsBit0(arg0, arg1->rumble_enabled);
    Player_SetNametagSlotID(arg0, arg1->nametag);
    if (arg1->xC_b1) {
        tmp->state.unk_A += 5;
        Player_SetFlagsBit3(arg0, 1);
        Player_SetUnk4C(arg0, tmp->state.unk_A);
    }
    Player_SetPlayerAndEntityCpuType(arg0, arg1->cpu_kind);
    Player_SetPlayerAndEntityCpuLevel(arg0, arg1->cpu_level);
    if (arg1->damage != 0) {
        Player_SetHUDDamage(arg0, arg1->damage);
    } else {
        Player_SetHUDDamage(arg0, arg1->damage1);
    }

    tmp->state.fighters[arg0].x6 = arg1->damage1;

    if (arg1->xC_b7) {
        Player_SetMoreFlagsBit2(arg0, 1);
    } else {
        Player_SetMoreFlagsBit2(arg0, 0);
    }
    Player_SetOtherStamina(arg0, arg1->hp);
    Player_SetModelScale(arg0, arg1->model_scale);
    Player_SetAttackRatio(arg0, arg1->attack_ratio);
    Player_SetDefenseRatio(arg0, arg1->defense_ratio);
    if (arg1->xC_b6) {
        Player_SetMoreFlagsBit1(arg0, 1);
    } else {
        Player_SetMoreFlagsBit1(arg0, 0);
    }

    tmp->state.fighters[arg0].x4.x0.x4_b1 = arg1->xC_b3;
    tmp->state.fighters[arg0].x4.x0.x4_b0 = arg1->vs_metal;
    if (arg1->vs_metal) {
        Player_SetFlagsBit5(arg0, true);
    } else {
        Player_SetFlagsBit5(arg0, false);
    }
    tmp->state.fighters[arg0].x4.x0.x4_b2 = arg1->vs_invisible;
    if (arg1->vs_invisible) {
        Player_SetFlagsBit6(arg0, 1);
        Player_SetFlagsBit7(arg0, 0);
    } else {
        Player_SetFlagsBit6(arg0, 0);
        Player_SetFlagsBit7(arg0, 0);
    }
    if (arg1->xC_b5) {
        Player_SetFlagsAEBit0(arg0, 1);
    } else {
        Player_SetFlagsAEBit0(arg0, 0);
    }
    if (!arg1->xD_b2) {
        Player_SetMoreFlagsBit3(arg0, 1);
    } else {
        Player_SetMoreFlagsBit3(arg0, 0);
    }
    Player_SetMoreFlagsBit4(arg0, arg1->xD_b3);
    if (arg1->xD_b4) {
        Player_SetMoreFlagsBit5(arg0, 1);
    } else {
        Player_SetMoreFlagsBit5(arg0, 0);
    }
    tmp->state.fighters[arg0].x4.x0.x4_b3 = arg1->xD_b1;
    Player_SetMoreFlagsBit6(arg0, arg1->xB);
    Player_SetFacingDirection(arg0, arg1->spawn_dir);
    tmp->state.fighters[arg0].x4.x0.x4_b5 = arg1->xD_b0;
}

void fn_8016DCC0(StartMeleeData* arg0)
{
    VsSceneController* tmp = &controller;
    int i;

    memzero(&controller, sizeof(VsSceneState));
    controller.state.unk_D = 6;
    controller.state.unk_1C = (void*) &lbl_803D5630;
    controller.state.unk_20 = (void*) &lbl_803D5620;
    controller.state.unk_0 = 0;

    controller.start = arg0->rules;

    controller.state.x24C.match_kind = arg0->rules.match_kind;
    controller.state.x24C.is_teams = arg0->rules.is_teams;
    if (arg0->rules.x7 != 0) {
        controller.state.unk_B = arg0->rules.x7;
    }
    controller.state.x24C.sd_penalty = arg0->rules.sd_penalty;
    Camera_SetQuakeScale(arg0->rules.x2C);

    if (tmp->start.timer_enabled) {
        if (tmp->start.timer_counts_up) {
            if (arg0->rules.x14 != 0) {
                controller.state.unk_2C = arg0->rules.x14 - 1;
            } else {
                controller.state.unk_2C = 0;
            }
            controller.start.time_limit = -1;
            controller.state.timer_seconds = arg0->rules.time_limit;
        } else {
            if (arg0->rules.time_limit == 0) {
                controller.state.timer_seconds = -1;
            } else {
                controller.state.timer_seconds = arg0->rules.time_limit;
            }
            if (arg0->rules.x14 != 0) {
                controller.state.unk_2C = arg0->rules.x14 - 1;
            } else {
                controller.state.unk_2C = 0x3B;
            }
        }
    } else if (controller.start.x1_0) {
        controller.state.timer_seconds = arg0->rules.time_limit;
        if (arg0->rules.x14 != 0) {
            controller.state.unk_2C = arg0->rules.x14;
        } else {
            controller.state.unk_2C = 0;
        }
    }
    if (arg0->rules.x54 != NULL) {
        controller.state.is_singleplayer = 1;
    } else {
        controller.state.is_singleplayer = 0;
    }
    Player_InitAllPlayers();

    switch (controller.state.is_singleplayer) {
    case 0:
    case 2:
    case 3:
        for (i = 0; i < 6; i++) {
            fn_8016D8AC(i, &arg0->players[i]);
        }
        break;
    case 1:
        fn_8016D8AC(0, &arg0->players[0]);
        for (i = 1; i < 6; i++) {
            Player_SetSlottype(i, Gm_PKind_NA);
        }
        if (arg0->rules.x54->x10_b1) {
            fn_8016A09C();
        }
    }
}

static float direction(float x)
{
    if (x >= 0.0F) {
        return -1.0F;
    } else {
        return +1.0F;
    }
}

static inline void getSpawnPoint(int i, Vec3* v)
{
    VsSceneController* tmp = &controller;
    if (tmp->state.fighters[i].spawn_point == -1) {
        Stage_80224E64(i, v);
    } else {
        Stage_80224E64(tmp->state.fighters[i].spawn_point, v);
    }
}

static inline void setPlayerUnk45(int i)
{
    bool is_teams = controller.start.is_teams == true;
    Player_SetUnk45(
        i, fn_80160840(gm_80160854(Player_GetPadPort(i), Player_GetTeam(i),
                                   is_teams, Player_GetPlayerSlotType(i))));
}

void fn_8016DEEC(void)
{
    VsSceneController* tmp = &controller;
    VsSceneController* tmp2 = &controller;
    float sp18[6];
    Vec3 spC;
    float temp_f0;
    float var_f1;
    float var_f26;
    float var_f25;
    int var_r25;
    int i;
    int j;
    int var_r23;

    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
            getSpawnPoint(i, &spC);
            sp18[i] = spC.x;
        }
    }
    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != 3 &&
            Player_GetFacingDirection(i) == 0.0F)
        {
            var_r23 = -1;

            for (j = 0; j < 6; j++) {
                if (j != i && Player_GetPlayerSlotType(j) != Gm_PKind_NA &&
                    (tmp->start.is_teams != true ||
                     Player_GetTeam(i) != Player_GetTeam(j)))
                {
                    if (var_r23 == -1 ||
                        var_f25 < (var_f26 = ABS(sp18[i] - sp18[j])))
                    {
                        var_r23 = j;
                        var_f25 = var_f26;
                    }
                }
            }
            if (var_r23 == -1) {
                var_f1 = 1.0F;
            } else {
                temp_f0 = sp18[var_r23] - sp18[i];
                if (temp_f0 < -5.0F) {
                    var_f1 = -1.0F;
                } else if (temp_f0 > 5.0F) {
                    var_f1 = +1.0F;
                } else if (Player_GetFacingDirection(var_r23) == +1.0F) {
                    var_f1 = -1.0F;
                } else if (Player_GetFacingDirection(var_r23) == -1.0F) {
                    var_f1 = +1.0F;
                } else {
                    var_f1 = +1.0F;
                }
            }
            Player_SetFacingDirection(i, var_f1);
        }
    }
}

void fn_8016E124(void)
{
    int var_r27 = 0;
    int i;
    int j;
    int temp_r3;

    int sp10[7];

    for (i = 0; i < 7; i++) {
        sp10[i] = -1;
    }

    for (i = 0; i < 6; i++) {
        if (Player_GetPlayerSlotType(i) != 3 &&
            Player_GetPlayerCharacter(i) == CKind_Kirby)
        {
            s8 temp_r3 = Player_GetCostumeId(i);
            bool var_r5 = false;
            for (j = 0; j < var_r27; j++) {
                if (temp_r3 == sp10[j]) {
                    var_r5 = true;
                    break;
                }
            }
            if (!var_r5) {
                sp10[var_r27] = temp_r3;
                var_r27++;
            }
        }
    }
    if (var_r27 > 0) {
        for (i = 0; i < 6; i++) {
            if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
                temp_r3 = Player_80032610(i, 0);
                if (temp_r3 != -1 && temp_r3 != 4) {
                    for (j = 0; j < var_r27; j++) {
                        Player_80031DA8(temp_r3, sp10[j]);
                    }
                }
                temp_r3 = Player_80032610(i, 1);
                if (temp_r3 != -1 && temp_r3 != 4) {
                    for (j = 0; j < var_r27; j++) {
                        Player_80031DA8(temp_r3, sp10[j]);
                    }
                }
            }
        }
    }
}

/// @todo The loop's SFX flag load uses a different equivalent address form.
void fn_8016E2BC(void)
{
    UNUSED u8 pad[8];
    Vec3 sp24;
    Vec3 sp18;
    float var_f1_2;
    bool var_r0;
    int i;
    bool single_is_teams;
    bool is_teams;
    VsSceneController* tmp;
    PAD_STACK(0xC);

    Player_80036DA4();
    if (controller.state.is_singleplayer == 1) {
        var_r0 = true;
    } else {
        var_r0 = false;
    }
    if (var_r0) {
        getSpawnPoint(0, &sp24);
        if (Player_GetFacingDirection(0) == 0.0F) {
            if (Stage_80224DC8(controller.start.stkind) != 0) {
                Player_SetFacingDirection(0, 1.0F);
            } else {
                Player_SetFacingDirection(0, direction(sp24.x));
            }
        }
        Player_80032768(0, &sp24);
        single_is_teams = controller.start.is_teams == true;
        Player_SetUnk45(0, fn_80160840(gm_80160854(
                               Player_GetPadPort(0), Player_GetTeam(0),
                               single_is_teams, Player_GetPlayerSlotType(0))));
        Player_80031AD0(0);
        if (controller.state.fighters[0].x4.x0.x4_b4) {
            lbAudioAx_800237A8(0x41F4E, 0x7F, 0x40);
        }
        fn_80169C54(Player_GetPlayerCharacter(0), Player_GetCostumeId(0));
        fn_80169F50(Player_GetPlayerCharacter(0), Player_GetCostumeId(0));
    } else {
        fn_8016DEEC();
        for (i = 0; i < 6; i++) {
            if (Player_GetPlayerSlotType(i) != Gm_PKind_NA) {
                getSpawnPoint(i, &sp18);
                tmp = &controller;
                if (Player_GetFacingDirection(i) == 0.0F) {
                    if (Stage_80224DC8(controller.start.stkind) != 0) {
                        Player_SetFacingDirection(i, 1.0F);
                    } else {
                        if (sp18.x >= 0.0F) {
                            var_f1_2 = -1.0F;
                        } else {
                            var_f1_2 = +1.0F;
                        }
                        Player_SetFacingDirection(i, var_f1_2);
                    }
                }
                Player_80032768(i, &sp18);
                is_teams = controller.start.is_teams == true;
                Player_SetUnk45(
                    i, fn_80160840(gm_80160854(Player_GetPadPort(i),
                                               Player_GetTeam(i), is_teams,
                                               Player_GetPlayerSlotType(i))));
                Player_80031AD0(i);
                if (tmp->state.fighters[i].x4.x0.x4_b4) {
                    lbAudioAx_800237A8(0x41F4E, 0x7F, 0x40);
                }
            }
        }
        fn_8016E124();
    }
}

bool fn_8016E5C0(StartMeleeData* arg0)
{
    bool var_r4;
    bool var_r6;
    int i;

    if (arg0->rules.xA != 0) {
        return arg0->rules.xA;
    }
    var_r4 = false;
    var_r6 = false;
    for (i = 0; i < 6; i++) {
        if (arg0->players[i].slot_type == Gm_PKind_Human) {
            var_r4 = true;
            if (!(HSD_PadCopyStatus[(u8) i].button & (HSD_PAD_L | HSD_PAD_R)))
            {
                var_r6 = true;
                break;
            }
        }
    }

    if (var_r4 && !var_r6) {
        return true;
    }
    return false;
}

void fn_8016E730(StartMeleeData* arg0)
{
    HSD_GObj* temp_r30;
    VsSceneController* r30;

    db_Setup();
    gm_SetDbPauseInputHandlers(gm_AnyControllerPressedStart,
                               gm_AnyControllerPressedZ);
    gm_SetPreGObjProcCallback(db_RunEveryFrame);
    gm_801A4B50(1);
    lb_80019880(OSSecondsToTicks(1.0F / GM_FPS / arg0->rules.game_speed));
    Camera_Init(70);
    Camera_Create();
    fn_8016DCC0(arg0);
    lbRefract_800222A4();
    lb_8000FCDC();
    efLib_Init();
    efAsync_LoadSync(0);
    efAsync_LoadSync(0x1F);
    Player_80036DD8();
    ftCo_800C06C0();
    mpColl_80041C78();
    Ground_801C0378(0x40);
    Stage_802251E8(arg0->rules.stkind, NULL);

    r30 = &controller;

    r30->state.unk_34 = get_unk_float();
    Item_80266F70();
    Item_80266FCC();
    it_8026D018();
    lbAudioAx_8002785C();
    Stage_8022524C();
    Camera_80030730(Ground_801C20D0());
    fn_8016E2BC();
    Stage_80225298();
    Ground_EnableMatchCamera();
    Camera_8002F3AC();
    fn_801A1134();
    sfx_setupCrowdSFX();
    if (controller.start.on_unpause_override != NULL) {
        controller.start.on_unpause_override(0);
        Camera_8002F3AC();
    }
    ifAll_802F390C();
    lbBgFlash_Init(0xFF);
    if (arg0->rules.on_match_start != NULL) {
        arg0->rules.on_match_start();
    }
    temp_r30 = gm_801A4BC8();
    fn_80171AD4();
    if (temp_r30 != NULL) {
        HSD_GObj_SetupProc(temp_r30, fn_8016C7D0, 0x14);
    }
    if (!arg0->rules.x1_4) {
        Stage_80225074(fn_8016E5C0(arg0));
    }
}

void gm_Scene_Vs_OnEnter(void* arg0)
{
#ifdef TARGET_PC
    const char* replay_seed = getenv("MELEE_DIAGNOSTIC_MATCH_SEED");
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") && replay_seed) {
        uint32_t seed;
        if (!MeleePc_ParseReplaySeed(replay_seed, &seed))
            OSPanic(__FILE__, __LINE__, "Invalid diagnostic match seed");
        *HSD_RandSeedPtr = seed;
        OSReport("NATIVE_MATCH_REPLAY seed=%u before_initialization=1\n", seed);
    }
    if (getenv("MELEE_DIAGNOSTIC_CLOSE_SECOND_MATCH")) {
        ++diagnostic_vs_entries;
        OSReport("NATIVE_VS_ENTRY number=%u\n", diagnostic_vs_entries);
    }
#endif
    StartMeleeData* tmp = arg0;
#ifdef TARGET_PC
    /* Shorten only the first diagnostic VS countdown. The timer itself still
     * expires through the ordinary simulation path, allowing Sudden Death to
     * be investigated without forcing an outcome. */
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        getenv("MELEE_DIAGNOSTIC_MATCH_TIME_LIMIT") && !tmp->rules.x6 &&
        tmp->rules.timer_enabled)
    {
        static int time_limit_applied;
        if (!time_limit_applied) {
            const char* text = getenv("MELEE_DIAGNOSTIC_MATCH_TIME_LIMIT");
            char* end = NULL;
            unsigned long requested = strtoul(text, &end, 10);
            if (end != text && !*end && requested >= 2 && requested <= 600) {
                tmp->rules.time_limit = (u32) requested;
                time_limit_applied = 1;
                OSReport("NATIVE_DIAGNOSTIC_MATCH_TIME_LIMIT seconds=%u x6=%u\n",
                         (unsigned) requested, tmp->rules.x6);
            } else {
                OSReport("NATIVE_DIAGNOSTIC_MATCH_TIME_LIMIT rejected=1\n");
            }
        }
    }
    /* Apply diagnostic configuration to the one input consumed by every
     * initialization reader, before taking its compatibility digest. */
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        getenv("MELEE_DIAGNOSTIC_MATCH_NO_ITEMS")) {
        tmp->rules.item_freq = (s8) -1;
        OSReport("NATIVE_MATCH_RULES diagnostic_no_items=1 item_freq=%d\n",
                 (s8) tmp->rules.item_freq);
    }
    if (getenv("MELEE_DIAGNOSTIC_RULES_DIGEST")) {
        unsigned char digest[32],bytes[MELEE_PC_RULES_CAPACITY];
        const uint32_t ids[7]={0};
        unsigned length=MeleePc_EncodeMatchRules(bytes,sizeof(bytes),tmp,ids);
        if (length && MeleePc_VersusRulesDigest(tmp,digest)) {
            unsigned i;
            OSReport("NATIVE_VERSUS_RULES_BYTES ");
            for (i=0;i<length;i++) OSReport("%02x",bytes[i]);
            OSReport("\nNATIVE_VERSUS_RULES_SHA256 ");
            for (i=0;i<sizeof(digest);i++) OSReport("%02x",digest[i]);
            OSReport("\n");
        } else {
            OSReport("NATIVE_VERSUS_RULES_SHA256 unsupported\n");
        }
    }
    if (getenv("MELEE_DIAGNOSTIC_LOCAL_AGREEMENT")) {
        unsigned char packet[MELEE_PC_MATCH_AGREEMENT_SIZE];
        const unsigned char owners[4]={0,1,2,3};
        unsigned i;
        if (!MeleePc_LocalMatchAgreement(packet,tmp,42,0,*HSD_RandSeedPtr,owners)) {
            OSPanic(__FILE__,__LINE__,"Cannot construct diagnostic local agreement");
        }
        OSReport("NATIVE_LOCAL_MATCH_AGREEMENT ");
        for (i=0;i<sizeof(packet);i++) OSReport("%02x",packet[i]);
        OSReport("\n");
    }
#endif
    fn_8016E730(tmp);
    if (tmp->rules.x1_2) {
        ifStatus_802F6EA4(8, -1, -1, 0, fn_8016B7B4, fn_8016B7F8);
    } else {
        ifStatus_802F6EA4(3, -1, -1, 0, fn_8016B7B4, fn_8016B7F8);
    }
    ifTime_CreateTimers();
    ifStatus_802F665C(tmp->rules.x0_3);
}

struct EndMeleeData {
    /* 00 */ u32 x0;
    /* 04 */ u32 x4;
    /* 08 */ u32 x8;
    /* 0C */ MatchEnd xC;
};

static inline bool gm_8016E9C8_inline(void)
{
    switch (gm_GetCurrentGameMode()) {
    case GM_SUPER_SUDDEN_DEATH_VS:
    case GM_INVISIBLE_VS:
    case GM_SLOMO_VS:
    case GM_LIGHTNING_VS:
    case GM_TINY_VS:
    case GM_GIANT_VS:
    case GM_STAMINA_VS:
    case GM_CAMERA_VS:
    case GM_SINGLE_BUTTON_VS:
        return true;
    default:
        return false;
    }
}

void gm_Scene_Vs_OnExit(void* user_data)
{
    struct EndMeleeData* data = user_data;
    VsSceneController* tmp = &controller;
    int i;

    if (controller.state.match_over == 0) {
        data->xC = controller.state.x24C;
        data->xC.is_teams = controller.start.is_teams;
        data->xC.outcome = controller.state.match_result;
        gm_80166378(&data->xC);
        fn_8016C46C_dontinline((int) &data->xC);
        if (tmp->state.match_result != OUTCOME_NO_CONTEST &&
            tmp->state.match_result != OUTCOME_RETRY)
        {
            fn_8016C4F4(&data->xC);
        }
        tmp->state.match_over = 1;
    } else {
        data->xC = controller.state.x24C;
    }
    if (controller.start.x4_4) {
        data->x0 = fn_8017F294();
    } else {
        data->x0 = controller.start.x18;
    }
    data->x4 = controller.state.timer_seconds;
    data->x8 = Ground_801C5ABC();

    if (gm_8016B3D8() || gm_8016E9C8_inline() ||
        gm_GetCurrentGameMode() == GM_VS)
    {
        if (controller.state.match_result != OUTCOME_NO_CONTEST &&
            controller.state.match_result != OUTCOME_RETRY)
        {
            for (i = 0; i < 6; i++) {
                if (Player_GetPlayerSlotType(i) == Gm_PKind_Human) {
                    gmMainLib_8015D00C(
                        gm_CKindToSelKind(Player_GetPlayerCharacter(i)));
                }
            }
        }
    }
}

void gm_Scene_SuddenDeath_OnEnter(void* user_data)
{
    StartMeleeData* data = user_data;
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        getenv("MELEE_DIAGNOSTIC_SUDDEN_DEATH_STATE"))
        OSReport("NATIVE_DIAGNOSTIC_SUDDEN_DEATH_ENTER timer_enabled=%u time_limit=%u x6=%u\n",
                 data->rules.timer_enabled, data->rules.time_limit, data->rules.x6);
#endif
    data->rules.x6 = true;
    fn_8016E730(data);
    ifStatus_802F6EA4(1, -1, -1, 0, fn_8016B7B4, fn_8016B7F8);
    ifTime_CreateTimers();
    ifStatus_802F665C(data->rules.x0_3);
}

void gm_Scene_Training_OnEnter(void* user_data)
{
    StartMeleeData* data = user_data;
    struct VsSceneController* tmp2;
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_TRAINING")) {
        /* Do not carry the CSS probe's held Start into the Training pause UI. */
        PADClearAllVirtualStatus();
    }
    if (getenv("MELEE_DIAGNOSTIC_TRAINING_MANUAL_INPUT") &&
        getenv("MELEE_DIAGNOSTIC_TRAINING_UI_TRACE") &&
        getenv("MELEE_DIAGNOSTIC_TRAINING_UI_KEYS")) {
        /* Session-only binding test using the existing validated PAD API.
         * SDL public letter scancodes: T=23,G=10,F=9,R=21. This avoids the
         * Windows test helper's known non-extended arrow-key injection issue;
         * it neither synthesizes input nor changes normal/default bindings. */
        const PADKeyButtonBinding keys[] = {
            {23, PAD_BUTTON_UP}, {10, PAD_BUTTON_DOWN},
            {9, PAD_BUTTON_LEFT}, {21, PAD_BUTTON_RIGHT}
        };
        for (unsigned i=0; i<sizeof(keys)/sizeof(keys[0]); ++i)
            HSD_ASSERT(1, PADTrySetKeyButtonBinding(0, keys[i]));
        OSReport("NATIVE_TRAINING_UI_KEYS session_only=1 T=Up G=Down F=Left R=Right\n");
    }
#endif

    fn_8016E730(data);
    Ground_801C1154();
    grStadium_801D410C();
    un_802FD404();
    tmp2 = &controller;
    ftLib_EnableAllInput();
    Stage_802252E4(tmp2->start.stkind, NULL);
    grStadium_801D4040();
    if (!controller.start.x1_3) {
        ifStatus_802F6EA4(4, -1, -1, 0, 0, fn_8016B784);
    } else {
        ifStatus_802F6EA4(8, -1, -1, 0, 0, fn_8016B784);
    }
    un_802FD428();
    ifStatus_802F665C(data->rules.x0_3);
    fn_8018A000();
}

float gm_8016ECE8(void)
{
    float var_f29;
    int i;
    int count;

    count = 0;
    var_f29 = 0.0F;
    for (i = 0; i < 6; i++) {
        CharacterKind kind = Player_GetPlayerCharacter(i);
        if (kind == CKind_MasterH || kind == CKind_CrezyH) {
            var_f29 +=
                (float) Player_GetRemainingHP(i) / Player_GetOtherStamina(i);
            count++;
        }
    }
    if (count > 0) {
        var_f29 /= count;
    }
    return var_f29;
}

bool gm_8016EDDC(int arg0, PlayerInitData* arg1)
{
    VsSceneController* tmp = &controller;
    Vec3 sp18;
    float var_f1;
    PAD_STACK(8);

    if (controller.state.is_singleplayer == 0 &&
        Player_GetPlayerSlotType(arg0) == Gm_PKind_NA &&
        Player_GetEntity(arg0) == NULL)
    {
        Player_80036D24(arg0);
        fn_8016D8AC(arg0, arg1);

        getSpawnPoint(arg0, &sp18);

        if (Player_GetFacingDirection(arg0) == 0.0F) {
            if (Stage_80224DC8(controller.start.stkind) != 0) {
                Player_SetFacingDirection(arg0, 1.0F);
            } else {
                if (sp18.x >= 0.0F) {
                    var_f1 = -1.0F;
                } else {
                    var_f1 = +1.0F;
                }
                Player_SetFacingDirection(arg0, var_f1);
            }
        }
        Player_80032768(arg0, &sp18);
        setPlayerUnk45(arg0);
        Player_80031AD0(arg0);
        if (controller.state.fighters[arg0].x4.x0.x4_b4) {
            lbAudioAx_800237A8(0x41F4E, 0x7F, 0x40);
        }
        fn_8016E124();
        Player_80031848(arg0);
        ifStatus_802F6508(arg0);
        un_802FD28C(arg0);
        return true;
    }
    return false;
}

bool fn_8016EF98(int arg0)
{
    if (controller.state.is_singleplayer == 0 &&
        Player_GetEntity(arg0) != NULL)
    {
        ifStatus_802F6788(arg0);
        Player_80031EBC(arg0);
        Player_SetSlottype(arg0, Gm_PKind_NA);
        return 1;
    }
    return 0;
}

void gm_8016F00C(int arg0)
{
    fn_8016719C(arg0, 0);
}

void fn_8016F030(StartMeleeData* arg0)
{
    int i;
    gm_SetupRulesDefaults(&arg0->rules);
    for (i = 0; i < 6; i++) {
        gm_SetupPlayerDefaults(&arg0->players[i]);
    }
}

int getPort(PlayerInitData* player, int i)
{
    int slot = player->slot;
    if (slot == 0) {
        return i;
    } else {
        return slot - 1;
    }
}

void gm_LoadRumbleEnabled(StartMeleeData* start)
{
    ssize_t i;

    for (i = 0; i < GM_MAX_PLAYERS; i++) {
        if (start->players[i].slot_type == Gm_PKind_Human &&
            i < PAD_MAX_CONTROLLERS)
        {
            start->players[i].rumble_enabled = gm_RumbleEnabledForPlayer(
                getPort(&start->players[i], i), start->players[i].nametag);
        } else {
            start->players[i].rumble_enabled = false;
        }
    }
}

int gm_8016F120(int arg0)
{
    return gmMainLib_8015DADC(arg0);
}

void fn_8016F140(int arg0)
{
    gmMainLib_8015DAB4(arg0);
}

int fn_8016F160(int arg0, int arg1)
{
    return pl_80039418(arg0, arg1);
}
