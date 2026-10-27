#include "gm_1A3F.h"

#include "gm_1A36.h"
#include "gmmain_lib.h"
#include "gmscdata.h"
#include "gmscene.h"
#include "types.h"
#include <dolphin/vi.h>
#include <melee/db/db.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lbmthp.h>
#include <melee/lb/lbsnap.h>
#include <melee/lb/types.h>
#include <melee/ty/toy.h>
#include <melee/ty/tydisplay.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/devcom.h>
#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/video.h>
#ifdef TARGET_PC
#include <aurora/aurora.h>
#include <aurora/diagnostics.h>
#include <dolphin/gx/GXAurora.h>
#include <stdlib.h>
#include <dolphin/pad.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/objalloc.h>
#define PC_OBJALLOC_TRACE(phase) HSD_ObjAllocDiagnosticTrace(phase)
#define PC_SCENE_TRACE(phase,kind) do { \
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) \
        OSReport("NATIVE_SCENE %s kind=%d\n", phase, (int)(kind)); \
    if (getenv("MELEE_DIAGNOSTIC_SCENE_RNG")) \
        OSReport("NATIVE_SCENE_RNG phase=%s kind=%d seed=%u\n", \
                 phase, (int)(kind), *HSD_RandSeedPtr); \
} while (0)
#else
#define PC_OBJALLOC_TRACE(phase) ((void)0)
#define PC_SCENE_TRACE(phase,kind) ((void)0)
#endif

#ifdef TARGET_PC
/* The original 2D/menu and FMV scenes are composed for a 4:3 canvas. The
 * gameplay scene keeps the selected widescreen camera. Switching render-target
 * extents while a VI/XFB ticket is in flight invalidates its source texture, so
 * cross only after a black frame has retired and before opening the new scene. */
static int pc_scene_viewport_policy = -1;

static void pc_prepare_scene_aspect(int scene_kind)
{
    AuroraViewportPolicy current_policy;
    AuroraViewportPolicy desired_policy;

    if (scene_kind == GS_MEMCARD) {
        /* This no-save startup transition is not a user-visible scene. */
        return;
    }
    current_policy = pc_scene_viewport_policy >= 0
                         ? (AuroraViewportPolicy) pc_scene_viewport_policy
                         : (aurora_get_display_preset() == 0
                                ? AURORA_VIEWPORT_FIT
                                : AURORA_VIEWPORT_STRETCH);
    /* These scene handlers all render combat. Sudden Death and Training must
     * retain the selected gameplay canvas rather than inherit a menu canvas. */
    desired_policy = (scene_kind == GS_VS || scene_kind == GS_SUDDEN_DEATH ||
                      scene_kind == GS_TRAINING)
                         ? AURORA_VIEWPORT_STRETCH
                         : AURORA_VIEWPORT_FIT;
    if (desired_policy == current_policy) {
        pc_scene_viewport_policy = current_policy;
        return;
    }
    if (!aurora_get_widescreen_enabled()) {
        pc_scene_viewport_policy = AURORA_VIEWPORT_FIT;
        return;
    }

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    AuroraSetViewportPolicy(desired_policy);
    /* VIWaitForRetrace services Aurora's queued resize with no game frame open;
     * the second black retrace retires the old-size presentation ticket. */
    VIWaitForRetrace();
    VISetBlack(0);
    pc_scene_viewport_policy = desired_policy;
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        OSReport("NATIVE_SCENE_ASPECT scene=%d policy=%s\n", scene_kind,
                 desired_policy == AURORA_VIEWPORT_STRETCH ? "gameplay-stretch"
                                                           : "authored-fit");
    }
}
#endif

struct routingInfo {
    u8 curr_mode;     ///< ::GameModeKind
    u8 pending_mode;  ///< ::GameModeKind
    u8 prev_mode;     ///< ::GameModeKind
    u8 curr_state_id; ///< from ::GameModeState::id
    u8 prev_state_id;
    u8 next_state_id;
};
ASSERT_SIZE(struct routingInfo, 0x6);

struct stateMachine {
    struct routingInfo routing;
    struct routingInfo backup_routing;
    u8 pending_mode_change; ///< ::bool
    u8 (*get_override)(void);
};
ASSERT_SIZE(struct stateMachine, 0x14);

/* 1A3F48 */ static void preloadState(GameModeState*);

/**
 * @brief Runs one game-mode scene transition and invokes the run loop for a
 * given scene
 *
 * Finds the current #GameModeState in @p mode, runs its @c Prep handler, loads
 * the scene via #gm_FindGameSceneHandler / #gm_801A4D34, then advances
 * #GameRouting::curr_scene_idx when the scene loop exits.
 */
/* 1A4014 */ static void gm_801A4014(GameMode*);

/**
 * @brief Loads a game mode, runs it to completion, then unloads it.
 *
 * Loads the data associated with the given #GameModeKind (asset preload and
 * #GameMode::Load), then executes its scene graph via #gm_801A4014 until
 * #GameState::pending is set. When the loop finishes, unloads the mode
 * (#GameMode::Unload) unless the game is resetting, then returns
 * #GameRouting::pending_mode, the next pending #GameModeKind.
 *
 * If #GameState::game_mode_override is defined and returns a mode < #GM_COUNT,
 * a single scene from that game mode will be executed after the current scene
 * exits, skipping typical game mode load/preload routines for the resulting
 * gamemode.  #GameState::pending takes precedence over the override behavior.
 *
 * See also: ::gm_ChangeGameModeAfterCurrentScene, ::gm_SetPendingGameMode,
 * ::gm_SetNewGameModePending
 *
 * @returns The next pending #GameModeKind (#GameRouting::pending_mode).
 */
/* 1A43A0 */ static u8 runGameMode(u8 mode);

/* 479D30 */ static struct stateMachine state_machine;

void preloadState(GameModeState* state)
{
    PreloadedGameModeState* preloaded_state;

    PC_OBJALLOC_TRACE("preloadState.enter");
    lbDvd_80018CF4(state->preload);
    PC_OBJALLOC_TRACE("preloadState.after_dvd");
    switch (state->info.scene_kind) {
    case GS_STAFFROLL:
    case GS_RESULTS:
        HSD_SisLib_803A6048(0xC000);
        break;
    case GS_CSS:
        HSD_SisLib_803A6048(0x2400);
        break;
    default:
        HSD_SisLib_803A6048(0x4800);
        break;
    }
    PC_OBJALLOC_TRACE("preloadState.after_sis");
    preloaded_state = lbDvd_GetPreloadCacheScene();
    if (lbHeap_80015BB8(2) == 0) {
        preloaded_state->is_heap_persistent[0] = true;
    }
    if (lbHeap_80015BB8(3) == 0) {
        preloaded_state->is_heap_persistent[1] = true;
    }
    lbDvd_80018254();
    PC_OBJALLOC_TRACE("preloadState.after_dvd_finish");
    lbCardNew_ForgetMemory();
    PC_OBJALLOC_TRACE("preloadState.after_card_new_reset");
    lbCardGame_Reset();
    PC_OBJALLOC_TRACE("preloadState.after_card_game_reset");
    lbSnap_8001E27C();
    PC_OBJALLOC_TRACE("preloadState.after_snap");
    Toy_803127D4();
    PC_OBJALLOC_TRACE("preloadState.after_toy");
    tyDisplay_8031C8B8();
    PC_OBJALLOC_TRACE("preloadState.after_trophy");
}

static inline u8 firstState(GameModeState* state, u8 next_id)
{
    for (; state->id != (u8) -1; state++) {
        do {
            if (state->id == next_id) {
                break;
            }
        } while (0);
        return state->id;
    }
    return 0;
}

static inline u8 nextState(GameModeState* states)
{
    GameModeState* next = states;
    u8 curr_id = state_machine.routing.curr_state_id;
    int i;
    u8 next_id;
    GameModeState* cur = states;

    for (i = 0; (next_id = next->id) != (u8) -1; i++) {
        if (cur->id > curr_id) {
            return states[i].id;
        }
        cur++;
        next++;
    }

    return firstState(states, next_id);
}

static inline GameModeState* findState(GameModeState* state)
{
    int i, j;
    for (i = state_machine.routing.curr_state_id; i < U8_MAX; i++) {
        for (j = 0; state[j].id != (u8) -1; j++) {
            if (i == state[j].id) {
                return &state[j];
            }
        }
    }
    return NULL;
}

void gm_801A4014(GameMode* mode)
{
    GameScene* scene;
    GameModeState* state;
    struct stateMachine* sm;
    struct GameSceneInfo* info;
    u8 kind;
    uintptr_t zero;
    PAD_STACK(4);

    sm = &state_machine;
    state = findState(mode->states);
    sm->routing.curr_state_id = state->id;
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_CLASSIC_MATCH")) {
        OSReport("NATIVE_CSS_STATE selected mode=%p states=%p state=%p id=%u enter=%p exit=%p\n",
                 (void*) mode, (void*) mode->states, (void*) state,
                 state->id, (void*) state->on_enter, (void*) state->on_exit);
    }
#endif

    PC_SCENE_TRACE("preload_begin", state->id);
    preloadState(state);
    PC_SCENE_TRACE("preload_end", state->id);
    if (state->on_enter != NULL) {
        PC_SCENE_TRACE("state_enter_begin", state->id);
        state->on_enter(state);
        PC_SCENE_TRACE("state_enter_end", state->id);
    }
    info = &state->info;
    kind = info->scene_kind;
#ifdef TARGET_PC
    pc_prepare_scene_aspect(kind);
    /* Explicit scene ownership also protects lit 3D menu art; projection type
     * alone cannot distinguish that artwork from combat. The host snapshots
     * effective quality per recorded frame without changing session choices. */
    HSD_ASSERT(1, aurora_set_gameplay_graphics(
        kind == GS_VS || kind == GS_SUDDEN_DEATH || kind == GS_TRAINING ||
        kind == GS_CAMERA_VS));
#endif
    /* The lookup's result has to reach `scene` through an instruction the
     * copy propagator cannot delete, or `scene` loses its own register web and
     * takes the one this function's state pointer needs. `| (zero = 0)` is the
     * only spelling that survives that pass and still folds back to a plain
     * move, and C has no bitwise operator on pointers, hence the round trip.
     */
    scene =
        (GameScene*) ((uintptr_t) gm_FindGameSceneHandler(kind) | (zero = 0));
    gm_801A4BD4();
    gm_801A4B88(info);
    if (scene->on_enter != NULL) {
#ifdef TARGET_PC
        if ((kind == 8 && getenv("MELEE_DIAGNOSTIC_CSS_VS_START")) ||
            (kind == 9 && getenv("MELEE_DIAGNOSTIC_STAGE_SELECT"))) {
            PADStatus neutral = { 0 };
            PADSetVirtualStatus(0, &neutral);
            PADSetVirtualStatus(1, &neutral);
        }
#endif
        PC_SCENE_TRACE("scene_enter_begin", kind);
        scene->on_enter(info->enter_data);
        PC_SCENE_TRACE("scene_enter_end", kind);
    }
#if defined(TARGET_PC) && defined(_WIN32)
    if (kind == GS_MENU &&
        getenv("MELEE_DIAGNOSTIC_CONGRATS_RETURN_PENDING")) {
        char* path = NULL;
        size_t size = 0;
        int configured =
            _putenv_s("MELEE_DIAGNOSTIC_CONGRATS_RETURN_PENDING", "") == 0 &&
            _putenv_s("MELEE_DIAGNOSTIC_CONGRATS_RETURN_ACTIVE", "1") == 0 &&
            _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_COMPOSITED_PRESENT", "1") == 0 &&
            _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_VI", "1") == 0 &&
            _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_GPU_SNAPSHOT", "") == 0 &&
            _putenv_s("AURORA_DIAGNOSTIC_CAPTURE_DELAY", "20") == 0;
        if (_dupenv_s(&path, &size, "MELEE_DIAGNOSTIC_RETURN_CAPTURE") == 0 &&
            path && path[0]) {
            configured = configured &&
                _putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE", path) == 0;
        }
        OSReport("NATIVE_CONGRATS_RETURN_MENU_ENTRY kind=%d mode=%d capture_configured=%d\n",
                 kind, gm_GetCurrentGameMode(), configured);
        free(path);
    }
#endif
    PC_SCENE_TRACE("frame_loop_begin", kind);
#ifdef TARGET_PC
    if(kind==GS_GAMEOVER && getenv("MELEE_DIAGNOSTIC_MASTERHAND_ADVANCE"))
        OSReport("NATIVE_POSTCLEAR_ABORT original_game_over=1 queued=%d\n",
                 aurora_diagnostic_queue_quit());
#endif
#if defined(TARGET_PC) && defined(_WIN32)
    if (kind == 2 && getenv("MELEE_DIAGNOSTIC_MATCH_DRAW_UV")) {
        if (_putenv_s("AURORA_DIAGNOSTIC_DRAW_UV", "1") == 0) {
            aurora_diagnostic_refresh_command_flags();
            OSReport("NATIVE_SCENE draw_uv_armed kind=2\n");
        }
    }
    if (kind == 2 && getenv("MELEE_DIAGNOSTIC_MATCH_TEXTURE_MATRICES")) {
        if (_putenv_s("AURORA_DIAGNOSTIC_TEXTURE_MATRICES", "1") == 0) {
            aurora_diagnostic_refresh_command_flags();
            OSReport("NATIVE_SCENE texture_matrices_armed kind=2\n");
        }
    }
    if ((kind == 0 || kind == 2 || kind == 4 || kind == 1 || kind == 8 || kind == 9 || kind == 5 || kind == 39 || kind == GS_GAMEOVER) && getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        const char* capture = getenv(kind == 0 ? "MELEE_DIAGNOSTIC_TITLE_SCENE_CAPTURE" : kind == GS_GAMEOVER ? "MELEE_DIAGNOSTIC_GAMEOVER_SCENE_CAPTURE" : kind == 39 ? "MELEE_DIAGNOSTIC_PRIZE_SCENE_CAPTURE" : kind == 5 ? "MELEE_DIAGNOSTIC_RESULTS_SCENE_CAPTURE" : kind == 9 ? "MELEE_DIAGNOSTIC_STAGE_SCENE_CAPTURE" : kind == 8 ? "MELEE_DIAGNOSTIC_CSS_SCENE_CAPTURE" : kind == 1 ? "MELEE_DIAGNOSTIC_MENU_SCENE_CAPTURE" : "MELEE_DIAGNOSTIC_GAME_SCENE_CAPTURE");
        if (capture && capture[0]) {
            /* Arm the existing GPU readback only after original game entry. */
            if (_putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE", capture) == 0) {
                OSReport("NATIVE_SCENE color_capture_armed kind=%d\n", kind);
            }
        }
    }
#endif
#ifdef TARGET_PC
    if (kind == 2 && getenv("MELEE_DIAGNOSTIC_GRAPHICS_MENU_TRACE")) {
        OSReport("NATIVE_PC_GRAPHICS_MATCH aspect=%u render_height=%u shadows=%d shadow_size=%u texture_quality=%u lighting=%d low_poly=%d\n",
                 aurora_get_display_preset(), aurora_get_render_height(),
                 aurora_get_fighter_shadows(), aurora_get_fighter_shadow_resolution(),
                 aurora_get_texture_quality(), aurora_get_lighting_enabled(),
                 aurora_get_low_poly_fighters());
    }
    if (getenv("MELEE_DIAGNOSTIC_PROFILE_LIFECYCLE")) {
        char phase[16];
        snprintf(phase, sizeof(phase), "%d", kind);
        _putenv_s("MELEE_DIAGNOSTIC_TEXTURE_TRACE_PHASE", phase);
        OSReport("NATIVE_TEXTURE_PROFILE scene_entry kind=%d pack=%u quality=%u pending=%d\n",
                 kind, aurora_get_texture_pack(), aurora_get_texture_quality(),
                 aurora_is_texture_pack_pending());
    }
#endif
    gm_801A4D34(scene->on_frame, info);
#ifdef TARGET_PC
    if (kind == 39 && getenv("MELEE_DIAGNOSTIC_PRIZE_CONFIRM")) {
        PADClearVirtualStatus(0);
        OSReport("NATIVE_PRIZE_INPUT scene_exit_virtual_pad_cleared=1\n");
    }
    if (kind == 5 && getenv("MELEE_DIAGNOSTIC_RESULTS_EXIT")) {
        PADClearVirtualStatus(0);
        PADClearVirtualStatus(1);
        OSReport("NATIVE_RESULTS_INPUT scene_exit_virtual_pads_cleared=1\n");
    }
    if (kind == 2 && getenv("MELEE_DIAGNOSTIC_MATCH_INPUT")) {
        PADClearVirtualStatus(0);
        OSReport("NATIVE_MATCH_INPUT scene_exit_virtual_pad_cleared=1\n");
    }
    if (kind == 9 && getenv("MELEE_DIAGNOSTIC_STAGE_SELECT")) {
        PADClearVirtualStatus(0);
        PADClearVirtualStatus(1);
        OSReport("NATIVE_STAGE_INPUT scene_exit_virtual_pads_cleared=1\n");
    }
    if (kind == 8 && getenv("MELEE_DIAGNOSTIC_CSS_VS_START")) {
        PADClearVirtualStatus(0);
        PADClearVirtualStatus(1);
        OSReport("NATIVE_CSS_INPUT scene_exit_virtual_pads_cleared=1\n");
    }
#endif
#ifdef TARGET_PC
    if (kind == 1 && getenv("MELEE_DIAGNOSTIC_MENU_VS")) {
        PADClearVirtualStatus(0);
        OSReport("NATIVE_MENU_NAV scene_exit_virtual_pad_cleared=1\n");
    }
#endif
    if (!gmMainLib_8046B0F0.resetting && scene->on_exit != NULL) {
        PC_SCENE_TRACE("scene_exit_begin", kind);
        scene->on_exit(info->exit_data);
        PC_SCENE_TRACE("scene_exit_end", kind);
    }
    if (!gmMainLib_8046B0F0.resetting) {
        if (state->on_exit != NULL) {
            PC_SCENE_TRACE("state_exit_begin", kind);
#ifdef TARGET_PC
            if (kind == 8 && getenv("MELEE_DIAGNOSTIC_CLASSIC_MATCH")) {
                OSReport("NATIVE_CSS_STATE_EXIT mode=%p states=%p state=%p callback=%p state_id=%u mode_id=%u prev=%u next=%u pending_mode=%u enter_data=%p exit_data=%p\n",
                         (void*) mode, (void*) mode->states, (void*) state,
                         (void*) state->on_exit, state->id,
                         state_machine.routing.curr_mode,
                         state_machine.routing.prev_state_id,
                         state_machine.routing.next_state_id,
                         state_machine.routing.pending_mode,
                         state->info.enter_data, state->info.exit_data);
            }
#endif
            state->on_exit(state);
#ifdef TARGET_PC
            if (kind == 8 && getenv("MELEE_DIAGNOSTIC_CLASSIC_MATCH"))
                OSReport("NATIVE_CSS_STATE_EXIT returned=1 state_id=%u\n",
                         state->id);
#endif
            PC_SCENE_TRACE("state_exit_end", kind);
        }

        state_machine.routing.prev_state_id = sm->routing.curr_state_id;

        if (sm->routing.next_state_id) {
            sm->routing.curr_state_id = sm->routing.next_state_id - 1;
            sm->routing.next_state_id = 0;
        } else {
            sm->routing.curr_state_id = nextState(mode->states);
        }
    }
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
    lb_8001CDB4();
    lbCardNew_CompleteAllTasks(11);
#endif
    lbMthp_8001F800();
    if (gmMainLib_8046B0F0.resetting) {
        lbAudioAx_80027DBC();
        HSD_PadReset();
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
        while (lbCardNew_CompleteNextTask() == 11);
#endif
        if (DVDCheckDisk() == 0) {
            OSResetSystem(1, 0, 0);
        }
        lbMthp_8001F800();
        while (HSD_DevComIsBusy(1));
        gmMainLib_8015FBA4();
        gm_GetAllGameModes();
        memzero(&state_machine, sizeof(state_machine));
        gm_801A3EF4();
        gmMainLib_8046B0F0.skip_intro = true;
        gm_ChangeGameModeAfterCurrentScene(GM_BOOT);
        HSD_VISetBlack(0);
    }
}

void* gm_GetGameModeStateEnterData(GameModeState* scene)
{
    return scene->info.enter_data;
}

void* gm_GetGameModeStateExitData(GameModeState* state)
{
    return state->info.exit_data;
}

void gm_SetGameModeStateId(u8 id)
{
    state_machine.routing.curr_state_id = id;
    state_machine.routing.prev_state_id = id;
}

/// @note Actually sets the next scene to the scene following the input
void gm_SetNextGameModeStateId(u8 curr_id)
{
    state_machine.routing.next_state_id = curr_id + 1;
}

u8 gm_GetPreviousSceneIndex(void)
{
    return state_machine.routing.prev_state_id;
}

u8 gm_GetCurrentSceneIndex(void)
{
    return state_machine.routing.curr_state_id;
}

void gm_SetNewGameModePending(void)
{
    state_machine.pending_mode_change = true;
}

void gm_SetPendingGameMode(u8 pending_mode)
{
    state_machine.routing.pending_mode = pending_mode;
}

void gm_ChangeGameModeAfterCurrentScene(int pending_mode)
{
    state_machine.routing.pending_mode = pending_mode;
    state_machine.pending_mode_change = true;
}

u8 gm_GetCurrentGameMode(void)
{
    return state_machine.routing.curr_mode;
}

u8 gm_GetPreviousGameMode(void)
{
    return state_machine.routing.prev_mode;
}

void gm_SetGameModeOverride(u8 (*mode)(void))
{
    state_machine.get_override = mode;
}

bool gm_Is1PMode(u8 mode)
{
    switch (mode) {
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
    }
    return false;
}

static inline GameMode* findMode(u8 kind)
{
    GameMode* cur;
    for (cur = gm_GetAllGameModes(); cur->kind != GM_COUNT; cur++) {
        if (cur->kind == kind) {
            return cur;
        }
    }
    return NULL;
}

u8 runGameMode(u8 mode_kind)
{
    u8 override;
    GameMode* mode;
    struct stateMachine* sm = &state_machine;
    PAD_STACK(2 * 4);

    mode = findMode(mode_kind);

    state_machine.pending_mode_change = false;
    state_machine.routing.curr_state_id = 0;
    state_machine.routing.prev_state_id = 0;
    state_machine.routing.next_state_id = 0;
    PC_OBJALLOC_TRACE("runGameMode.enter");
    PC_SCENE_TRACE("mode_preload_begin", mode_kind);
    lbDvd_80018F58(mode->preloaded);
    PC_OBJALLOC_TRACE("runGameMode.after_mode_preload");
    PC_SCENE_TRACE("mode_preload_end", mode_kind);
    if (mode->on_load != NULL) {
        PC_SCENE_TRACE("load_begin", mode_kind);
        PC_OBJALLOC_TRACE("runGameMode.before_mode_load");
        mode->on_load();
        PC_OBJALLOC_TRACE("runGameMode.after_mode_load");
        PC_SCENE_TRACE("load_end", mode_kind);
    }
    while (!sm->pending_mode_change) {
        if (state_machine.get_override != NULL &&
            (override = state_machine.get_override(), override != GM_COUNT))
        {
            state_machine.backup_routing = state_machine.routing;
            sm->pending_mode_change = false;
            sm->routing.curr_state_id = 0;
            sm->routing.prev_state_id = 0;
            sm->routing.next_state_id = 0;

            gm_801A4014(findMode(override));
            if (!gmMainLib_8046B0F0.resetting) {
                state_machine.routing = state_machine.backup_routing;
            }
        } else {
            gm_801A4014(mode);
        }
    }
    if (!gmMainLib_8046B0F0.resetting && mode->on_unload != NULL) {
        PC_OBJALLOC_TRACE("runGameMode.before_mode_unload");
        mode->on_unload();
        PC_OBJALLOC_TRACE("runGameMode.after_mode_unload");
    }
    return state_machine.routing.pending_mode;
}

/// UnclePunch: Scene_Main
void gm_801A4510(void)
{
    GameMode* modes;
    struct stateMachine* gamestate = &state_machine;
    int i;
    PAD_STACK(2 * 4);

    gm_GetAllGameModes();
    memzero(&state_machine, sizeof(struct stateMachine));
    modes = gm_GetAllGameModes();
    for (i = 0; modes[i].kind != GM_COUNT; i++) {
        if (modes[i].on_init != NULL) {
            PC_SCENE_TRACE("init_begin", modes[i].kind);
            modes[i].on_init();
            PC_SCENE_TRACE("init_end", modes[i].kind);
        }
    }
#ifdef TARGET_PC
    // The console cable/progressive prompt does not apply to a host display.
    // Host renderer/resolution preferences belong to the native main menu.
    state_machine.routing.curr_mode = GM_BOOT;
#else
    if (VIGetDTVStatus() != 0 &&
        (db_gameLaunchButtonState & HSD_PAD_B || OSGetProgressiveMode() == 1))
    {
        state_machine.routing.curr_mode = GM_PROGRESSIVE_SCAN;
    } else {
        state_machine.routing.curr_mode = GM_BOOT;
    }
#endif
    state_machine.routing.prev_mode = GM_COUNT;

    while (true) {
        u8 current_mode = state_machine.routing.curr_mode;
        u8 next_mode = runGameMode(current_mode);
#ifdef TARGET_PC
        static unsigned pc_mode_trace_count;
        if (getenv("MELEE_PC_TRACE_GAME_MODES") &&
            next_mode != current_mode && pc_mode_trace_count < 128) {
            OSReport("NATIVE_GAME_MODE from=%u to=%u\n", current_mode,
                     next_mode);
            pc_mode_trace_count++;
        }
#endif
        if (gmMainLib_8046B0F0.resetting) {
            gmMainLib_8046B0F0.resetting = false;
        }
        gamestate->routing.prev_mode = gamestate->routing.curr_mode;
        gamestate->routing.curr_mode = next_mode;
    }
}
