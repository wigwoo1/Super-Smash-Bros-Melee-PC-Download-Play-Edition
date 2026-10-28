#include "gmscene.h"

#include "gm_1A36.h"
#include "gm_unsplit.h"
#include "gmtitle.h"
#include "gmmain_lib.h"
#include "gmscdata.h"
#include <dolphin/os/OSThread.h>
#include <melee/db/db.h>
#include <melee/if/ifcoget.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/hsd_3924.h>
#include <sysdolphin/baselib/hsd_392C.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/leak.h>
#include <sysdolphin/baselib/perf.h>
#include <sysdolphin/baselib/sobjlib.h>
#ifdef TARGET_PC
#include <pc/udp_input.h>
#include <pc/udp_submission.h>
#include <pc/host_lifecycle.h>
#include <pc/replay_seed.h>
#include <pc/input_retry.h>
#include <aurora/aurora.h>
#include <dolphin/pad.h>
#include <aurora/diagnostics.h>
#include <pc/runtime_timing.h>
#include <pc/frame_clock.h>
#include <pc/tick_clock.h>
#include <aurora/gfx.h>
#include <stdlib.h>
#include <string.h>
extern void MeleePc_SetRenderCallbackTrace(int enabled);
#define PC_FRAME_PHASE(phase) do { if (pc_frame_index <= 2 && getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) OSReport("NATIVE_FRAME_PHASE %s iteration=%u\n", phase, pc_frame_index); } while (0)
#else
#define PC_FRAME_PHASE(phase) ((void)0)
#endif

/* 479D30 */ static HSD_GObjLibInitDataType gobj_init_data;
/* 479D58 */ static struct gm_80479D58_t gm_80479D58;
/* 4D672C */ HSD_GObj* gm_804D672C;
/* 4D6728 */ UNK_T gm_804D6728;
/* 4D6724 */ void (*gm_804D6724)(void);
/* 4D6720 */ struct GameSceneInfo* gm_804D6720;

static u64 gm_803DA888[8] = {
    0, 0x82FFFA, 0, 0x8EFFFA, 0x800FFA, 0x808FFA, 0x800FFA, 0,
};

u64 gm_803DA8C8[2] = { -1, -1 };

#ifdef TARGET_PC
static u64 native_pacing_simulation_ticks;
static u64 native_pacing_scene_draws;
static u64 native_pacing_previous_draws;
static unsigned native_pacing_repeat_render_count;
static unsigned native_pacing_repeat_deferred;
static bool native_gpu_snapshot_repeat;
static bool native_pacing_trace_enabled;
static bool native_pacing_profile_enabled;
/* Opt-in PAD-latch probes used only by bounded native UI verification. */
static bool native_title_start_diagnostic_staged;
static void (*native_menu_start_diagnostic_callback)(void);
static unsigned native_menu_start_diagnostic_frame;
static void (*native_classic_match_diagnostic_callback)(void);
static unsigned native_classic_match_diagnostic_frame;
static unsigned native_congrats_return_menu_frames;
static u64 native_pacing_previous_presented;
static u64 native_pacing_previous_simulation;
static u32 native_pacing_previous_time_ms;
enum NativePacingPhaseId {
    NATIVE_PACING_SCENE_CALLBACK,
    NATIVE_PACING_GOBJ_PROCS,
    NATIVE_PACING_XFB_SLOT_WAIT,
    NATIVE_PACING_HOST_SURFACE_ACQUIRE,
    NATIVE_PACING_RENDER_CALLBACKS,
    NATIVE_PACING_XFB_COPY,
    NATIVE_PACING_HOST_END_FRAME,
    NATIVE_PACING_REPEAT_PRESENT,
    NATIVE_PACING_SNAPSHOT_BEGIN,
    NATIVE_PACING_SNAPSHOT_DRAW,
    NATIVE_PACING_SNAPSHOT_COPY,
    NATIVE_PACING_SNAPSHOT_END,
    NATIVE_PACING_SNAPSHOT_OWNER,
    NATIVE_PACING_DRAW_INVALIDATE,
    NATIVE_PACING_DRAW_START,
    NATIVE_PACING_DRAW_GOBJS,
    NATIVE_PACING_DRAW_FINISH,
    NATIVE_PACING_PHASE_COUNT,
};
static const char* native_pacing_phase_names[NATIVE_PACING_PHASE_COUNT] = {
    "scene_callback", "gobj_procs", "xfb_slot_wait", "host_surface_acquire",
    "render_callbacks", "xfb_copy", "host_end_frame", "repeat_present",
    "snapshot_begin", "snapshot_draw", "snapshot_copy", "snapshot_end", "snapshot_owner",
    "draw_invalidate", "draw_start", "draw_gobjs", "draw_finish",
};
static struct {
    u64 total_ticks;
    OSTime max_ticks;
    u32 calls;
} native_pacing_phases[NATIVE_PACING_PHASE_COUNT];

static void native_pacing_profile_add(enum NativePacingPhaseId phase, OSTime start)
{
    OSTime elapsed;
    if (!native_pacing_profile_enabled || phase >= NATIVE_PACING_PHASE_COUNT) {
        return;
    }
    elapsed = OSGetTime() - start;
    native_pacing_phases[phase].total_ticks += elapsed;
    if (elapsed > native_pacing_phases[phase].max_ticks) {
        native_pacing_phases[phase].max_ticks = elapsed;
    }
    native_pacing_phases[phase].calls++;
}

static void native_pacing_profile_report(void)
{
    unsigned i;
    if (!native_pacing_profile_enabled) {
        return;
    }
    for (i = 0; i < NATIVE_PACING_PHASE_COUNT; i++) {
        if (native_pacing_phases[i].calls != 0) {
            OSReport("NATIVE_PACING_PROFILE phase=%s calls=%u total_ms=%llu max_ms=%llu\n",
                     native_pacing_phase_names[i], native_pacing_phases[i].calls,
                     (unsigned long long) OSTicksToMilliseconds(native_pacing_phases[i].total_ticks),
                     (unsigned long long) OSTicksToMilliseconds(native_pacing_phases[i].max_ticks));
        }
        memset(&native_pacing_phases[i], 0, sizeof(native_pacing_phases[i]));
    }
}

static void native_log_pacing_sample(void)
{
    u64 submitted, completed, failed, presented;
    u32 now_ms;

    if (!native_pacing_trace_enabled) return;
    aurora_diagnostic_gpu_counts(&submitted, &completed, &failed, &presented);
    now_ms = OSTicksToMilliseconds(OSGetTime());
    if (native_pacing_previous_presented == 0) {
        native_pacing_previous_presented = presented;
        native_pacing_previous_simulation = native_pacing_simulation_ticks;
        native_pacing_previous_draws = native_pacing_scene_draws;
        native_pacing_previous_time_ms = now_ms;
        /* Exclude work before the first timing baseline from this interval. */
        memset(native_pacing_phases, 0, sizeof(native_pacing_phases));
        return;
    }
    if (presented < native_pacing_previous_presented ||
        presented - native_pacing_previous_presented < 120) {
        return;
    }
    OSReport("NATIVE_PACING_SAMPLE time_ms=%u presented=%llu simulation_ticks=%llu "
             "delta_ms=%u delta_presented=%llu delta_simulation=%llu "
             "extra_passes=%u failed=%llu draws=%llu delta_draws=%llu deferred=%u\n",
             now_ms, (unsigned long long) presented,
             (unsigned long long) native_pacing_simulation_ticks,
             (u32) (now_ms - native_pacing_previous_time_ms),
             (unsigned long long) (presented - native_pacing_previous_presented),
             (unsigned long long) (native_pacing_simulation_ticks - native_pacing_previous_simulation),
             native_pacing_repeat_render_count,
             (unsigned long long) failed,
             (unsigned long long) native_pacing_scene_draws,
             (unsigned long long) (native_pacing_scene_draws - native_pacing_previous_draws),
             native_pacing_repeat_deferred);
    native_pacing_previous_presented = presented;
    native_pacing_previous_simulation = native_pacing_simulation_ticks;
    native_pacing_previous_draws = native_pacing_scene_draws;
    native_pacing_previous_time_ms = now_ms;
    native_pacing_profile_report();
    (void) submitted;
    (void) completed;
}
#endif

bool gm_GetDbPauseFlag(int bit)
{
    return gm_80479D58.unk_10.x0 & (1ULL << bit);
}

int gm_801A4624(void)
{
    return gm_80479D58.unk_10.x0;
}

void gm_SetDbPauseFlag(int bit)
{
    gm_80479D58.unk_10.x0 |= 1ULL << bit;
}

void gm_ClearDbPauseFlag(int bit)
{
    gm_80479D58.unk_10.x0 &= ~(1ULL << bit);
}

bool gm_801A46B8(int bit)
{
    return gm_80479D58.unk_10.x2 & (1ULL << bit);
}

bool fn_801A46F4(void)
{
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        HSD_PadStatus* pad = &HSD_PadMasterStatus[(u8) i];
        if (pad->err == 0 && (pad->trigger & HSD_PAD_DPADUP) &&
            (pad->button & HSD_PAD_X))
        {
            return true;
        }
    }
    return false;
}

bool fn_801A47E4(void)
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

u64 gm_801A48A4(u8 arg0)
{
    int i;
    u64 result = 0;

    for (i = 0; i < ARRAY_SIZE(gm_803DA888); i++) {
        if (arg0 & 1) {
            result |= gm_803DA888[i];
        }
        arg0 >>= 1;
    }

    return result;
}

void gm_801A4970(struct gm_DbPauseInputHandlers* db_input)
{
    HSD_PadStatus* temp_r3;
    s8 var_r26;
    s8* temp_r4;
    u64 temp_ret;
    int i;
    PAD_STACK(8);

    var_r26 = 0;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        temp_r3 = &HSD_PadMasterStatus[(u8) i];
        if ((temp_r3->trigger & HSD_PAD_DPADRIGHT) &&
            (temp_r3->button & HSD_PAD_X))
        {
            lbHeap_80015DF8();
            OSReport("[hsdDumpClassStat] -- Report --\n");
            hsdDumpClassStat(NULL, 0, 1);
            OSReport("\n");
            OSReport("[HSD_ObjDumpStat] -- Report --\n");
            HSD_ObjDumpStat();
            OSReport("\n");
            db_PrintEntityCounts();
            db_PrintThreadInfo();
            HSD_Leak_80387DF8(0);
            if (gm_804D6728 != NULL) {
                gm_801653C8(gm_804D6728);
                gm_804D6728 = NULL;
            } else {
                gm_804D6728 = gm_80165388(0x19, 0x3F, 0, 0xFE);
                if (gm_804D6724 != NULL) {
                    gm_804D6724();
                }
            }
        }
    }

    if (db_input->check_pause != NULL && db_input->check_pause()) {
        if (gm_GetDbPauseFlag(0)) {
            gm_80479D58.unk_10.x0 &= ~1;
        } else {
            gm_80479D58.unk_10.x0 |= 1;
        }
    }
    if (gm_GetDbPauseFlag(0)) {
        if (db_input->check_framestep != NULL && db_input->check_framestep()) {
            gm_80479D58.unk_10.x2 |= 1;
        }
    }
}

void gm_SetDbPauseInputHandlers(Predicate check_db_pause,
                                Predicate check_db_framestep)
{
    gm_80479D58.unk_10.db_input.check_pause = check_db_pause;
    gm_80479D58.unk_10.db_input.check_framestep = check_db_framestep;
}

void gm_801A4B1C(void)
{
    gm_SetDbPauseInputHandlers(fn_801A46F4, fn_801A47E4);
}

void gm_SetPreGObjProcCallback(Event cb)
{
    gm_80479D58.unk_10.pre_gobj_proc = cb;
}

void gm_801A4B50(int arg0)
{
    gm_80479D58.unk_10.unk_34 = arg0;
}

void gm_801A4B60(void)
{
    gm_80479D58.unk_C = 1;
}

void gm_801A4B74(void)
{
    gm_80479D58.unk_C = 2;
}

void gm_801A4B88(struct GameSceneInfo* info)
{
    gm_804D6720 = info;
}

/// @brief returns a pointer to the current scenes enter data
void* gm_GetCurrentSceneEnterData(void)
{
    return gm_804D6720->enter_data;
}

/// @brief returns a pointer to the current scenes exit data
void* gm_GetCurrentSceneExitData(void)
{
    return gm_804D6720->exit_data;
}

u32 gm_801A4BA8(void)
{
    return gm_80479D58.unk_0;
}

u32 gm_801A4BB8(void)
{
    return gm_80479D58.unk_8;
}

HSD_GObj* gm_801A4BC8(void)
{
    return gm_804D672C;
}

void fn_801A4BD0(HSD_GObj* gobj) {}

void gm_801A4BD4(void)
{
    PAD_STACK(0x18);

    gm_SetDbPauseInputHandlers(fn_801A46F4, fn_801A47E4);
    gm_SetPreGObjProcCallback(NULL);
    gm_801A4B50(0);

    lb_80019880(OSSecondsToTicks(1.0F / GM_FPS));
    HSD_GObjSetInitDefaults(&gobj_init_data);
    gobj_init_data.gproc_pri_max = 0x18;
    HSD_SObjLib_804D7960 =
        HSD_GObj_803912A8(&gobj_init_data, &HSD_SObjLib_8040C3A4);
    HSD_SObjLib_803A44A4();
    gobj_init_data.unk_2 = &gm_80479D58.unk_10.unk_28;
    HSD_GObjInit(&gobj_init_data);
    hsd_80392474();
    un_802FF78C();
    gm_804D672C = GObj_Create(14, 0, 0);
    if (gm_804D672C != NULL) {
        HSD_GObj_SetupProc(gm_804D672C, fn_801A4BD0, 0);
    }
    gm_804D6728 = NULL;
    gm_804D6724 = NULL;
    gm_801A3E88();
    lbAudioAx_8002835C();
    lb_80014534();
}

GameScene* gm_FindGameSceneHandler(u8 kind)
{
    GameScene* cur;
    for (cur = gm_GetAllGameScenes(); cur->kind != GS_COUNT; cur++) {
        if (cur->kind == kind) {
            return cur;
        }
    }
    return NULL;
}

static inline u64 maybe_gm_801A48A4(u8 i)
{
    u64 temp_ret = gm_801A48A4(i);
    if (gm_80479D58.unk_10.unk_38_0) {
        return temp_ret;
    } else {
        return -1ULL;
    }
}

#ifdef TARGET_PC
typedef struct NativeUdpCleanup {
    MeleePc_UdpInput* sender;
    MeleePc_UdpInput* receiver;
    MeleePc_InputRetry* retry;
    MeleePc_UdpInput* submission_senders;
    MeleePc_SubmissionWindow* submission_window;
} NativeUdpCleanup;
static void native_udp_cleanup(void* data)
{
    NativeUdpCleanup* session=data;
    MeleePc_UdpClose(session->sender);MeleePc_UdpClose(session->receiver);
    MeleePc_InputRetryEnd(session->retry);
    for(unsigned i=0;i<4;i++)MeleePc_UdpClose(&session->submission_senders[i]);
    MeleePc_SubmissionWindowEnd(session->submission_window);
    HSD_PadEndOrderedInput();
    HSD_ASSERT(1,!session->sender->started&&!session->receiver->started &&
                 !session->sender->input.session&&!session->receiver->input.session);
    OSReport("NATIVE_INPUT_UDP closed=1 buffers_cleared=1 ordered_input_ended=1\n");
}

/* Experimental owner-thread presentation-only pass. This intentionally runs
 * the existing GX render callbacks again without running on_frame(),
 * HSD_GObj_RunProcs(), pad sampling, or any game simulation callback. It is
 * opt-in for runtime validation while the normal owner loop remains at 60 Hz.
 */
static unsigned native_diagnostic_repeat_render_hz(void)
{
    const char* value = getenv("MELEE_DIAGNOSTIC_REPEAT_RENDER_HZ");
    if (value == NULL) {
        return 0;
    }
    if (strcmp(value, "120") == 0) {
        return 120;
    }
    if (strcmp(value, "240") == 0) {
        return 240;
    }
    OSReport("NATIVE_REPEAT_RENDER rejected_hz=%s allowed=120,240\n", value);
    return 0;
}
static unsigned native_gameplay_presentation_hz(unsigned scene_kind,
                                                 unsigned repeat_hz)
{
    const char* value = getenv("MELEE_DIAGNOSTIC_PRESENTATION_HZ");
    if (scene_kind != 2) return 0;
    /* Native gameplay presents its GPU scene directly at60Hz. Original VI
     * remains responsible for bootstrap, menus and genuine VI/black changes.
     * An explicit0 retains the legacy path for compatibility diagnostics. */
    if (!value) {
        return repeat_hz || getenv("MELEE_DIAGNOSTIC_REPEAT_PRESENT_ONLY") ? 0 : 60;
    }
    if (!strcmp(value, "0")) return 0;
    if (!strcmp(value, "60")) return 60;
    if (!strcmp(value, "120")) return 120;
    if (!strcmp(value, "240")) return 240;
    OSReport("NATIVE_PRESENTATION_CLOCK rejected_hz=%s allowed=0,60,120,240\n", value);
    return 0;
}
static void native_poll_presentation_events(void)
{
    HSD_ASSERT(1, aurora_service_frame_events());
}

static bool native_begin_scene_frame(void)
{
    if (!getenv("MELEE_DIAGNOSTIC_PRESENT_PENDING_XFB")) {
        return aurora_begin_frame();
    }
    const u32 simulation_before = gm_80479D58.unk_0;
    const bool acquired = aurora_begin_scene_frame();
    HSD_ASSERT(1, simulation_before == gm_80479D58.unk_0);
    return acquired;
}

static bool native_present_current_scene_state(void)
{
    bool present_only = getenv("MELEE_DIAGNOSTIC_REPEAT_PRESENT_ONLY") != NULL;
    bool gpu_snapshot = !present_only && native_gpu_snapshot_repeat;
    const bool profile_snapshot = gpu_snapshot && native_pacing_profile_enabled;
    OSTime snapshot_phase = profile_snapshot ? OSGetTime() : 0;
    void* snapshot_destination = NULL;
    /* Re-presenting an already pinned/current XFB does not draw into VI's
     * source, so it need not wait for the next draw-enable retrace. */
    if (!present_only && !gpu_snapshot && HSD_VIGetXFBDrawEnable() == -1) {
        /* An optional redraw must not wait for the 60Hz VI transaction to
         * release a pinned XFB. Try the same owner-thread reservation as the
         * normal wait, without changing any submitted/displayed slot. The
         * ordinary simulation frame still uses HSD_VIWaitXFBDrawEnable. */
        native_pacing_repeat_deferred++;
        return false;
    }
    if (gpu_snapshot) {
        snapshot_destination = aurora_begin_render_snapshot_frame();
        if (!snapshot_destination) {
            if (profile_snapshot) native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_BEGIN, snapshot_phase);
            native_pacing_repeat_deferred++;
            return false;
        }
    } else {
        while (!(present_only ? aurora_begin_frame() : native_begin_scene_frame())) {
            aurora_update();
            lb_800195D0();
        }
    }
    /* Diagnostic fast path: submit another surface presentation of VI's
     * already-accepted current XFB. Do not re-run GX callbacks or allocate a
     * new XFB copy. This is deliberately opt-in until cadence and image
     * lifetime are validated on both renderer backends. */
    if (present_only) {
        aurora_end_frame();
        HSD_ASSERT(1, aurora_service_owner());
        return true;
    }
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_BEGIN, snapshot_phase);
        snapshot_phase = OSGetTime();
    }
    OSTime draw_phase = profile_snapshot ? OSGetTime() : 0;
    GXInvalidateVtxCache();
    GXInvalidateTexAll();
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_DRAW_INVALIDATE, draw_phase);
        draw_phase = OSGetTime();
    }
    HSD_StartRender(HSD_RP_SCREEN);
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_DRAW_START, draw_phase);
        draw_phase = OSGetTime();
    }
    HSD_GObj_80390FC0();
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_DRAW_GOBJS, draw_phase);
        draw_phase = OSGetTime();
    }
    native_pacing_scene_draws++;
    HSD_Init_803755A8();
    HSD_PerfSetDrawTime();
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_DRAW_FINISH, draw_phase);
        native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_DRAW, snapshot_phase);
        snapshot_phase = OSGetTime();
    }
    if (gpu_snapshot) {
        /* Use the same filter/gamma/crop/clear setup without claiming a CPU
         * XFB or requesting its draw-done/VI promotion callbacks. */
        HSD_VICopyEFB2XFBPtr(HSD_VIGetVIStatus(), snapshot_destination, HSD_RP_SCREEN);
    } else {
        HSD_VICopyXFBAsync(HSD_RP_SCREEN);
    }
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_COPY, snapshot_phase);
        snapshot_phase = OSGetTime();
    }
    HSD_ASSERT(1, aurora_service_owner());
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_OWNER, snapshot_phase);
        snapshot_phase = OSGetTime();
    }
    aurora_end_frame();
    if (profile_snapshot) {
        native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_END, snapshot_phase);
        snapshot_phase = OSGetTime();
    }
    HSD_ASSERT(1, aurora_service_owner());
    if (profile_snapshot) native_pacing_profile_add(NATIVE_PACING_SNAPSHOT_OWNER, snapshot_phase);
    return true;
}

/* Service one due diagnostic presentation. Advance from the scheduled
 * deadline and coalesce missed deadlines instead of accumulating a burst of
 * stale presents after a long simulation/render stall. This remains opt-in;
 * normal gameplay scheduling is unchanged until full fixed-step validation. */
static bool native_service_repeat_presentation(unsigned target_hz,
                                               OSTime period,
                                               OSTime* next_deadline)
{
    if (target_hz == 0 || period <= 0 || next_deadline == NULL ||
        *next_deadline == 0) {
        return false;
    }

    const OSTime now = OSGetTime();
    if (now < *next_deadline) {
        return false;
    }

    const OSTime present_start = OSGetTime();
    const u32 simulation_before = gm_80479D58.unk_0;
    const bool rendered = native_present_current_scene_state();
    HSD_ASSERT(1, simulation_before == gm_80479D58.unk_0);
    /* Include time spent submitting and servicing this present when skipping
     * missed deadlines; otherwise a slow present can leave the clock overdue
     * and cause an avoidable burst of immediate repeats. */
    const OSTime completed_at = OSGetTime();
    native_pacing_profile_add(NATIVE_PACING_REPEAT_PRESENT, present_start);
    if (rendered) native_pacing_repeat_render_count++;

    /* Keep the phase anchored to the previous deadline, but skip any missed
     * slots. Multiplication is bounded by elapsed / period and OS time here. */
    const OSTime elapsed = completed_at - *next_deadline;
    const OSTime intervals = elapsed / period + 1;
    *next_deadline += intervals * period;
    native_log_pacing_sample();
    return rendered;
}
static bool native_service_presentation_clock(MeleePc_PresentationClock* clock,
                                              OSTime origin)
{
    u64 elapsed;
    HSD_ASSERT(1, MeleePc_TicksToNanosecondsCeil(OSGetTime() - origin, OS_TIMER_CLOCK, &elapsed));
    HSD_ASSERT(1, MeleePc_PresentationClockPoll(clock, elapsed));
    /* Retained PAD debt owns every simulation step. Draw only once the
     * currently sampled input is consumed; no render callback runs game procs. */
    if (!clock->pending || HSD_PadGetRawQueueCount()) return false;
    const u32 before = gm_80479D58.unk_0;
    const bool rendered = native_present_current_scene_state();
    HSD_ASSERT(1, before == gm_80479D58.unk_0);
    HSD_ASSERT(1, MeleePc_TicksToNanosecondsCeil(OSGetTime() - origin, OS_TIMER_CLOCK, &elapsed));
    HSD_ASSERT(1, MeleePc_PresentationClockFinish(clock, elapsed, rendered));
    if (rendered) native_pacing_repeat_render_count++;
    if (native_pacing_trace_enabled && clock->presented <= 64)
        OSReport("NATIVE_PRESENTATION_DRAW target=%u drawn=%llu opportunities=%llu missed=%llu scene_step=%u input_before=0 elapsed_ns=%llu\n",
                 clock->phase.presentation_hz, clock->presented, clock->opportunities,
                 clock->missed, before, elapsed);
    native_log_pacing_sample();
    return rendered;
}
#endif
void gm_801A4D34(void (*on_frame)(void), UNUSED GameSceneInfo* info)
{
    int pad_queue_count;
    int i;
    struct gm_80479D58_t* temp_r25;

    PAD_STACK(28);
#ifdef TARGET_PC
    unsigned pc_frame_index = 0;
    const bool native_congrats_return_menu = info->scene_kind == GS_MENU &&
        getenv("MELEE_DIAGNOSTIC_CONGRATS_RETURN_ACTIVE") != NULL;
    unsigned native_results_input_frame = 0;
    unsigned native_prize_input_frame = 0;
    unsigned native_gameover_input_frame = 0;
    const bool native_prize_pad_trace = info->scene_kind == 39 &&
        getenv("MELEE_DIAGNOSTIC_PRIZE_PAD_TRACE") != NULL;
    unsigned native_ordered_tick = 0;
    PADStatus native_delayed_ports[4];
    bool native_delayed_pending = false;
    OSTime native_delayed_start = 0;
    unsigned native_delayed_frame = 0;
    unsigned native_delayed_services = 0;
    bool native_ordered_diagnostic = on_frame == gm_Scene_Vs_OnFrame &&
        getenv("MELEE_DIAGNOSTIC_ORDERED_INPUT") != NULL;
    bool native_udp_diagnostic = native_ordered_diagnostic &&
        getenv("MELEE_DIAGNOSTIC_ORDERED_UDP") != NULL;
    bool native_udp_external = native_udp_diagnostic &&
        getenv("MELEE_DIAGNOSTIC_UDP_PEER_PORT") != NULL;
    MeleePc_UdpInput native_udp_sender = {0}, native_udp_receiver = {0};
    MeleePc_InputRetry native_udp_retry={0};
    MeleePc_UdpInput native_submission_senders[4]={{0}};
    MeleePc_UdpPeers native_submission_peers={0};
    MeleePc_SubmissionWindow native_submission_window={0};
    bool native_submission_diagnostic=native_udp_diagnostic&&getenv("MELEE_DIAGNOSTIC_UDP_SUBMISSIONS")!=NULL;
    NativeUdpCleanup native_udp_cleanup_data={&native_udp_sender,&native_udp_receiver,&native_udp_retry,
        native_submission_senders,&native_submission_window};
    bool native_retry_diagnostic=native_udp_external&&getenv("MELEE_DIAGNOSTIC_UDP_RETRY")!=NULL;
    unsigned native_repeat_render_hz = native_diagnostic_repeat_render_hz();
    unsigned native_presentation_hz = native_gameplay_presentation_hz(info->scene_kind, native_repeat_render_hz);
    HSD_ASSERT(1, !native_presentation_hz || !native_repeat_render_hz);
    HSD_ASSERT(1, !native_presentation_hz || !getenv("MELEE_DIAGNOSTIC_REPEAT_PRESENT_ONLY"));
    MeleePc_PresentationClock native_presentation_clock = {0};
    OSTime native_presentation_origin = 0;
    bool native_presentation_ready = false;
    native_gpu_snapshot_repeat = info->scene_kind == 2 &&
        (native_presentation_hz || getenv("MELEE_DIAGNOSTIC_GPU_SNAPSHOT_REPEAT") != NULL);
    native_pacing_trace_enabled =
        (native_presentation_hz && getenv("MELEE_DIAGNOSTIC_PRESENTATION_HZ")) ||
        native_repeat_render_hz != 0 ||
        getenv("MELEE_DIAGNOSTIC_PACING_TRACE") != NULL;
    native_pacing_profile_enabled = native_pacing_trace_enabled &&
        getenv("MELEE_DIAGNOSTIC_PACING_PROFILE") != NULL;
    native_pacing_simulation_ticks = native_pacing_repeat_render_count = 0;
    native_pacing_repeat_deferred = 0;
    native_pacing_scene_draws = native_pacing_previous_draws = 0;
    native_pacing_previous_presented = native_pacing_previous_simulation = 0;
    native_pacing_previous_time_ms = 0;
    memset(native_pacing_phases, 0, sizeof(native_pacing_phases));
    bool native_repeat_render_each_tick = native_repeat_render_hz != 0 &&
        getenv("MELEE_DIAGNOSTIC_REPEAT_RENDER_EACH_TICK") != NULL;
    bool native_repeat_render_during_backlog = native_repeat_render_hz != 0 &&
        getenv("MELEE_DIAGNOSTIC_REPEAT_RENDER_DURING_BACKLOG") != NULL;
    unsigned native_pad_stall_ms = 0;
    if (info->scene_kind == 2 && getenv("MELEE_DIAGNOSTIC_PAD_STALL_MS")) {
        u32 parsed;
        HSD_ASSERT(1, MeleePc_ParseReplaySeed(getenv("MELEE_DIAGNOSTIC_PAD_STALL_MS"), &parsed));
        HSD_ASSERT(1, parsed > 0 && parsed <= 1000);
        native_pad_stall_ms = parsed;
    }
    OSTime native_repeat_render_period = 0;
    OSTime native_next_repeat_render = 0;
    const bool native_vsync_override = native_presentation_hz || native_repeat_render_hz;
    const bool native_previous_vsync = aurora_is_vsync_enabled();
    if (native_vsync_override && native_previous_vsync) aurora_enable_vsync(false);
    if (info->scene_kind == 2) {
        OSReport("NATIVE_GAMEPLAY_PRESENTATION target=%u default=%u simulation_hz=60 previous_vsync=%u\n",
                 native_presentation_hz,
                 !getenv("MELEE_DIAGNOSTIC_PRESENTATION_HZ") && native_presentation_hz==60,
                 native_previous_vsync);
    }
    if (native_repeat_render_hz != 0) {
        native_repeat_render_period = OSSecondsToTicks(1.0F / (f32) native_repeat_render_hz);
        HSD_ASSERT(1, native_repeat_render_period != 0);
        OSReport("NATIVE_REPEAT_RENDER enabled=1 target_hz=%u simulation_hz=60 vsync=0 each_tick=%u during_backlog=%u gpu_snapshot=%u\n",
                 native_repeat_render_hz, native_repeat_render_each_tick,
                 native_repeat_render_during_backlog, native_gpu_snapshot_repeat);
    }
#endif

    temp_r25 = &gm_80479D58;
    PC_FRAME_PHASE("state_init_begin");
    gm_801677C0(&temp_r25->unk_10);
    PC_FRAME_PHASE("state_init_end");
    gm_80479D58.unk_0 = 0;
    gm_80479D58.unk_4 = 0;
    gm_80479D58.unk_8 = 0;
    gm_80479D58.unk_C = 0;
    HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_LEAVE1);
    lbCardGame_InitScene();
#ifdef TARGET_PC
    if (native_ordered_diagnostic) HSD_ASSERT(1,HSD_PadBeginOrderedInput(42,0));
    if (native_udp_diagnostic) {
        HSD_ASSERT(1,!native_submission_diagnostic||!native_udp_external);
        HSD_ASSERT(1, MeleePc_UdpOpen(&native_udp_sender,"127.0.0.1",0,42,0));
        HSD_ASSERT(1, MeleePc_UdpOpen(&native_udp_receiver,"127.0.0.1",0,42,0));
        HSD_ASSERT(1, MeleePc_UdpSetPeer(&native_udp_sender,"127.0.0.1",native_udp_receiver.port));
        if (native_udp_external) {
            u32 port;
            HSD_ASSERT(1,MeleePc_ParseReplaySeed(getenv("MELEE_DIAGNOSTIC_UDP_PEER_PORT"),&port));
            HSD_ASSERT(1,port>0&&port<=65535);
            HSD_ASSERT(1,MeleePc_UdpSetPeer(&native_udp_receiver,"127.0.0.1",port));
            OSReport("NATIVE_INPUT_UDP external_peer=1 peer_port=%u native_pid=%u\n",port,(unsigned)GetCurrentProcessId());
        } else {
            HSD_ASSERT(1, MeleePc_UdpSetPeer(&native_udp_receiver,"127.0.0.1",native_udp_sender.port));
        }
        HSD_ASSERT(1, MeleePc_RegisterSessionCleanup(native_udp_cleanup,&native_udp_cleanup_data));
        if(native_submission_diagnostic){
            unsigned char owners[4]={0,1,2,3};
            HSD_ASSERT(1,MeleePc_SubmissionWindowBegin(&native_submission_window,42,0,owners));
            for(unsigned port=0;port<4;port++){
                HSD_ASSERT(1,MeleePc_UdpOpen(&native_submission_senders[port],"127.0.0.1",0,42,0));
                HSD_ASSERT(1,MeleePc_UdpSetPeer(&native_submission_senders[port],"127.0.0.1",native_udp_receiver.port));
                HSD_ASSERT(1,MeleePc_UdpAssignPeer(&native_submission_peers,port,"127.0.0.1",native_submission_senders[port].port));
            }
            OSReport("NATIVE_INPUT_SUBMISSIONS opened=1 endpoints=4 diagnostic=1\n");
        }
        OSReport("NATIVE_INPUT_UDP opened=1 loopback=1\n");
    }
    if (native_prize_pad_trace) PADSetDiagnosticPrizeTrace(true);
#endif
        PC_FRAME_PHASE("session_init_end");
#ifdef TARGET_PC
    MeleePc_SetScenePadCatchUp(info->scene_kind == 2);
#endif

    while (temp_r25->unk_C == 0) {
#ifdef TARGET_PC
        if (pc_frame_index < 3) ++pc_frame_index;
#endif
        PC_FRAME_PHASE("iteration_begin");
        PC_FRAME_PHASE("development_poll_begin");
    hsd_80392E80();
    PC_FRAME_PHASE("development_poll_end");
        gmMainLib_8046B0F0.xC = false;

        while ((pad_queue_count = lb_80019894()) == 0) {
            lb_800195D0();
#ifdef TARGET_PC
            if (native_presentation_ready) {
                native_poll_presentation_events();
                if (aurora_set_render_snapshot_scene(true) &&
                    native_service_presentation_clock(&native_presentation_clock, native_presentation_origin) &&
                    temp_r25->unk_4 != -2U)
                    temp_r25->unk_4++;
            }
            if (native_repeat_render_hz != 0 && native_next_repeat_render != 0 &&
                OSGetTime() >= native_next_repeat_render) {
                if (native_service_repeat_presentation(
                        native_repeat_render_hz, native_repeat_render_period,
                        &native_next_repeat_render) && temp_r25->unk_4 != -2U) {
                    temp_r25->unk_4++;
                }
            }
#endif
        }
        lb_800195D0();

        if (HSD_PadGetResetSwitch()) {
            gmMainLib_8046B0F0.resetting = true;
            break;
        }

        for (i = 0; i < pad_queue_count; i++) {
            HSD_PerfSetStartTime();
#ifdef TARGET_PC
            /* Give queued 60Hz input/simulation samples priority. Rendering
             * between them can starve the game loop and change gameplay
             * speed; keep that behavior available only as an explicit
             * diagnostic comparison. The default services extra redraws only
             * in the empty-queue wait above. */
            if (native_repeat_render_during_backlog &&
                !native_repeat_render_each_tick &&
                native_repeat_render_hz != 0 &&
                native_next_repeat_render != 0 &&
                native_service_repeat_presentation(native_repeat_render_hz,
                                                   native_repeat_render_period,
                                                   &native_next_repeat_render) &&
                temp_r25->unk_4 != -2U) {
                temp_r25->unk_4++;
            }
#include "gmvs.h"
            if (info->scene_kind == GS_GAMEOVER && getenv("MELEE_DIAGNOSTIC_GAMEOVER_EXIT")) {
                PADStatus pad = { 0 };
                unsigned frame = ++native_gameover_input_frame;
                /* Original Continue defaults to Yes. A Right edge selects No;
                 * a later A edge confirms, after a stable screen capture.
                 * Stage through the same next-tick pad route as gameplay. */
                if (frame >= 220 && frame < 228 &&
                    !getenv("MELEE_DIAGNOSTIC_GAMEOVER_CONTINUE")) pad.stickX = 80;
                if (frame >= 300 && frame < 308) pad.button = PAD_BUTTON_A;
                HSD_ASSERT(1, HSD_PadStageDiagnosticInput(0, &pad));
                if (frame == 1 || frame == 220 || frame == 228 ||
                    frame == 300 || frame == 308)
                    OSReport("NATIVE_GAMEOVER_INPUT frame=%u button=%u stick_x=%d port=0\n",
                             frame, pad.button, pad.stickX);
            }
            if (info->scene_kind == 39 && getenv("MELEE_DIAGNOSTIC_PRIZE_CONFIRM")) {
                PADStatus pad = { 0 };
                unsigned frame = ++native_prize_input_frame;
                unsigned phase = frame % 120;
                if (frame >= 120 && phase < 4) pad.button = PAD_BUTTON_A;
                PADSetVirtualStatus(0, &pad);
                if (frame >= 120 && (phase == 0 || phase == 4))
                    OSReport("NATIVE_PRIZE_INPUT frame=%u confirm=%u port=0\n",
                             frame, pad.button != 0);
            }
            if (info->scene_kind == 5 && getenv("MELEE_DIAGNOSTIC_RESULTS_EXIT")) {
                PADStatus pad = { 0 };
                unsigned frame = ++native_results_input_frame;
                /* Repeat ordinary press/release after the intro. Holding for
                 * 30 ticks crosses native pad sampling; 90 released ticks
                 * permit a fresh edge when the results state becomes ready. */
                bool browse = getenv("MELEE_DIAGNOSTIC_RESULTS_BROWSE") != NULL;
                unsigned exit_frame = browse ? 760 : getenv("MELEE_DIAGNOSTIC_RESULTS_STATS") ? 600 : 300;
                unsigned phase = frame >= exit_frame ? (frame - exit_frame) % 120 : 120;
                if (getenv("MELEE_DIAGNOSTIC_RESULTS_STATS")) {
                    if (frame >= 270 && frame < 300) pad.button = PAD_BUTTON_A;
                    /* First A opens the statistics phase; a fresh edge navigates. */
                    if (frame >= 340 && frame < 370) pad.button = PAD_BUTTON_A;
                    if (!browse && frame >= 400 && frame < 430) pad.button = PAD_BUTTON_B;
                    if (browse) {
                        if (frame >= 380 && frame < 430) pad.stickY = -80;
                        if (frame >= 450 && frame < 510) pad.stickY = 80;
                        if (frame >= 520 && frame < 550) pad.button = PAD_BUTTON_A;
                        if ((frame >= 580 && frame < 610) ||
                            (frame >= 640 && frame < 670)) pad.button = PAD_BUTTON_B;
                    }
                    if (frame == 270 || frame == 300 || frame == 340 ||
                        frame == 370 || frame == 400 || frame == 430)
                        OSReport("NATIVE_RESULTS_STATS_INPUT frame=%u button=%u ports=0,1\n", frame, pad.button);
                    if (browse && (frame == 380 || frame == 430 ||
                                   frame == 450 || frame == 510 ||
                                   frame == 520 || frame == 550 ||
                                   frame == 580 || frame == 610 ||
                                   frame == 640 || frame == 670))
                        OSReport("NATIVE_RESULTS_BROWSE_INPUT frame=%u button=%u stick_y=%d ports=0,1\n",
                                 frame, pad.button, pad.stickY);
                }
                if (phase < 30)
                    pad.button = PAD_BUTTON_START;
                PADSetVirtualStatus(0, &pad);
                PADSetVirtualStatus(1, &pad);
                if (phase == 0 || phase == 30)
                    OSReport("NATIVE_RESULTS_INPUT frame=%u start=%u ports=0,1\n",
                             frame, pad.button != 0);
            }
#endif
#ifdef TARGET_PC
            if (on_frame == gm_Scene_Vs_OnFrame &&
                getenv("MELEE_DIAGNOSTIC_STAGED_BASIC_INPUT")) {
                PADStatus input = { 0 };
                unsigned frame = gm_GetFrameCount() + 1;
                if (frame >= 180 && frame < 240) input.stickX = 45;
                if (frame >= 300 && frame < 360) input.stickX = -45;
                if (frame >= 420 && frame < 428) input.button = PAD_BUTTON_X;
                if (frame >= 600 && frame < 608) input.button = PAD_BUTTON_A;
                if (frame >= 780 && frame < 788) input.button = PAD_BUTTON_B;
                if (frame >= 960 && frame < 980) {
                    input.button = PAD_BUTTON_B;
                    input.stickY = -80;
                }
                if (getenv("MELEE_DIAGNOSTIC_MATCH_EXTRA_SPECIALS")) {
                    if (frame >= 1200 && frame < 1208) {
                        input.button = PAD_BUTTON_B;
                        input.stickX = 80;
                    }
                    if (frame >= 1440 && frame < 1448) {
                        input.button = PAD_BUTTON_B;
                        input.stickY = 80;
                    }
                }
                if (getenv("MELEE_DIAGNOSTIC_FOUR_PORT_INPUT")) {
                    PADStatus ports[4] = { 0 };
                    ports[0] = input;
                    if (native_ordered_diagnostic && getenv("MELEE_DIAGNOSTIC_ORDERED_PAUSE")) {
                        if ((native_ordered_tick >= 200 && native_ordered_tick < 208) ||
                            (native_ordered_tick >= 300 && native_ordered_tick < 308))
                            ports[0].button = PAD_BUTTON_START;
                    }
                    /* Inactive ports exercise transport without adding players. */
                    if (frame >= 180 && frame < 200) {
                        ports[2].button = PAD_BUTTON_A;
                        ports[3].stickX = -30;
                    }
                    if (native_ordered_diagnostic) {
                        if(native_submission_diagnostic){
                            for(unsigned port=4;port>0;port--)
                                HSD_ASSERT(1,MeleePc_UdpSendSubmission(&native_submission_senders[port-1],native_ordered_tick,1u<<(port-1),ports));
                        } else if (native_udp_diagnostic) {
                            unsigned char packet[MELEE_PC_INPUT_PACKET_SIZE];
                            HSD_ASSERT(1, MeleePc_EncodeInputPacket(packet,sizeof(packet),42,native_ordered_tick,ports));
                            if(native_retry_diagnostic)
                                HSD_ASSERT(1,MeleePc_InputRetryBegin(&native_udp_retry,packet,sizeof(packet),42,
                                    OSTicksToMilliseconds(OSGetTime()),20,2000,100));
                            HSD_ASSERT(1, MeleePc_UdpSend(native_udp_external?&native_udp_receiver:&native_udp_sender,packet,sizeof(packet)));
                            if(native_retry_diagnostic)
                                HSD_ASSERT(1,MeleePc_InputRetrySent(&native_udp_retry,OSTicksToMilliseconds(OSGetTime())));
                        } else if (native_ordered_tick == 180 &&
                            getenv("MELEE_DIAGNOSTIC_ORDERED_DELAY")) {
                            memcpy(native_delayed_ports, ports, sizeof(ports));
                            native_delayed_pending = true;
                            native_delayed_start = OSGetTime();
                            native_delayed_frame = gm_GetFrameCount();
                            OSReport("NATIVE_ORDERED_DELAY begin=1 tick=%u gameplay_frame=%u\n",
                                     native_ordered_tick, native_delayed_frame);
                        } else if (!(native_ordered_tick == 180 &&
                              getenv("MELEE_DIAGNOSTIC_ORDERED_WAIT_CLOSE")))
                            HSD_ASSERT(1, HSD_PadStageOrderedInput(native_ordered_tick, ports));
                    } else {
                        HSD_ASSERT(1, HSD_PadStageFrameInput(ports));
                    }
                } else {
                    HSD_PadStageDiagnosticInput(0, &input);
                }
                if (frame >= 779 && frame <= 792)
                    OSReport("NATIVE_STAGED_INPUT frame=%u button=%u\n", frame, input.button);
            }
#endif
#ifdef TARGET_PC
            /* Missing ordered input must stop the entire owner iteration,
             * including procedures which also run during paused ticks. */
            while (!HSD_PadOrderedInputReady()) {
                if(native_submission_diagnostic){
                    HSD_ASSERT(1,MeleePc_UdpPumpSubmissionWindow(&native_udp_receiver,&native_submission_peers,&native_submission_window)>=0);
                    MeleePc_InputGather* front=MeleePc_SubmissionWindowPeek(&native_submission_window);
                    if(front&&HSD_PadStageGatheredInput(front)){
                        HSD_ASSERT(1,MeleePc_SubmissionWindowDrop(&native_submission_window));
                        OSReport("NATIVE_INPUT_SUBMISSIONS tick=%u accepted=%u rejected=%u duplicates=%u next=%llu\n",
                            native_ordered_tick,native_udp_receiver.accepted,native_udp_receiver.rejected,
                            native_udp_receiver.duplicates,(unsigned long long)native_submission_window.next_tick);
                    }
                    if(HSD_PadOrderedInputReady())break;
                }else if (native_udp_diagnostic) {
                    HSD_ASSERT(1, MeleePc_UdpPump(&native_udp_receiver) >= 0);
                    unsigned char confirmation[MELEE_PC_INPUT_PACKET_SIZE];
                    const unsigned char* front=MeleePc_InputBufferPeek(&native_udp_receiver.input);
                    if(front&&native_retry_diagnostic)memcpy(confirmation,front,sizeof(confirmation));
                    if(HSD_PadStageBufferedInput(&native_udp_receiver.input)&&native_retry_diagnostic) {
                        unsigned attempts=native_udp_retry.attempts;
                        HSD_ASSERT(1,MeleePc_InputRetryConfirm(&native_udp_retry,confirmation,sizeof(confirmation)));
                        OSReport("NATIVE_INPUT_RETRY tick=%u confirmed=1 attempts=%u\n",native_ordered_tick,attempts);
                    }
                    if (HSD_PadOrderedInputReady()) break;
                    if(native_retry_diagnostic) {
                        u64 now=OSTicksToMilliseconds(OSGetTime());
                        int due=MeleePc_InputRetryDue(&native_udp_retry,now);
                        HSD_ASSERT(1,due>=0);
                        if(due==1&&MeleePc_UdpSend(&native_udp_receiver,native_udp_retry.packet,MELEE_PC_INPUT_PACKET_SIZE))
                            HSD_ASSERT(1,MeleePc_InputRetrySent(&native_udp_retry,now));
                    }
                }
                if (native_ordered_diagnostic &&
                    getenv("MELEE_DIAGNOSTIC_ORDERED_WAIT_CLOSE")) {
                    OSReport("NATIVE_ORDERED_WAIT tick=%u gameplay_frame=%u close_queued=1\n",
                             native_ordered_tick, gm_GetFrameCount());
                    HSD_ASSERT(1, aurora_diagnostic_queue_quit());
                }
                lb_800195D0();
                HSD_ASSERT(1, aurora_service_input_wait());
                if (native_delayed_pending) {
                    ++native_delayed_services;
                    HSD_ASSERT(1, gm_GetFrameCount() == native_delayed_frame);
                    if (OSGetTime() - native_delayed_start >= OSMillisecondsToTicks(100)) {
                        HSD_ASSERT(1, HSD_PadStageOrderedInput(native_ordered_tick, native_delayed_ports));
                        native_delayed_pending = false;
                        OSReport("NATIVE_ORDERED_DELAY delivered=1 tick=%u gameplay_frame=%u services=%u elapsed_ticks=%llu\n",
                                 native_ordered_tick, gm_GetFrameCount(), native_delayed_services,
                                 (unsigned long long)(OSGetTime()-native_delayed_start));
                    }
                }
            }
#endif
#ifdef TARGET_PC
            if (on_frame == gm_Scene_Title_OnFrame &&
                getenv("MELEE_DIAGNOSTIC_TITLE_START")) {
                static unsigned native_title_pad_frame;
                PADStatus title_input = { 0 };
                native_title_pad_frame++;
                if (native_title_pad_frame >= 120 &&
                    native_title_pad_frame < 150) {
                    title_input.button = PAD_BUTTON_START;
                    if (native_title_pad_frame == 120) {
                        native_title_start_diagnostic_staged = true;
                    }
                }
                HSD_ASSERT(1, HSD_PadStageDiagnosticInput(0, &title_input));
                if (native_title_pad_frame == 120 ||
                    native_title_pad_frame == 150) {
                    OSReport("NATIVE_TITLE_PAD_STAGE frame=%u button=%u before_latch=1\n",
                             native_title_pad_frame, title_input.button);
                }
            }
            if (native_title_start_diagnostic_staged &&
                getenv("MELEE_DIAGNOSTIC_MENU_START")) {
                if (native_menu_start_diagnostic_callback == NULL &&
                    on_frame != NULL && on_frame != gm_Scene_Title_OnFrame) {
                    native_menu_start_diagnostic_callback = on_frame;
                    OSReport("NATIVE_MENU_PAD_TARGET selected=1\n");
                }
                if (native_menu_start_diagnostic_callback != NULL &&
                    on_frame == native_menu_start_diagnostic_callback) {
                    PADStatus menu_input = { 0 };
                    native_menu_start_diagnostic_frame++;
                    if (native_menu_start_diagnostic_frame == 30 ||
                        native_menu_start_diagnostic_frame == 90 ||
                        native_menu_start_diagnostic_frame == 150) {
                        menu_input.button = PAD_BUTTON_START;
                    }
                    HSD_ASSERT(1, HSD_PadStageDiagnosticInput(0, &menu_input));
                    if (native_menu_start_diagnostic_frame == 30 ||
                        native_menu_start_diagnostic_frame == 31 ||
                        native_menu_start_diagnostic_frame == 90 ||
                        native_menu_start_diagnostic_frame == 91 ||
                        native_menu_start_diagnostic_frame == 150 ||
                        native_menu_start_diagnostic_frame == 151) {
                        OSReport("NATIVE_MENU_PAD_STAGE frame=%u button=%u before_latch=1\n",
                                 native_menu_start_diagnostic_frame,
                                 menu_input.button);
                    }
                }
            }
            if (native_title_start_diagnostic_staged &&
                getenv("MELEE_DIAGNOSTIC_CLASSIC_MATCH") &&
                gm_GetCurrentGameMode() == GM_CLASSIC && on_frame != NULL &&
                on_frame != native_menu_start_diagnostic_callback) {
                if (native_classic_match_diagnostic_callback == NULL) {
                    native_classic_match_diagnostic_callback = on_frame;
                    OSReport("NATIVE_CLASSIC_PAD_TARGET selected=1\n");
                }
                if (on_frame == native_classic_match_diagnostic_callback) {
                    PADStatus classic_input = { 0 };
                    native_classic_match_diagnostic_frame++;
                    if (native_classic_match_diagnostic_frame >= 32 &&
                        native_classic_match_diagnostic_frame < 64) {
                        /* Grab the P1 token at the lower-left spawn, then move
                         * it up to the first roster row before dropping it. */
                        classic_input.stickY = 127;
                    }
                    /* Grab the P1 tag, drag it to the first roster row, then
                     * press A again to drop/select the character. */
                    if (native_classic_match_diagnostic_frame == 30 ||
                        native_classic_match_diagnostic_frame == 110) {
                        classic_input.button = PAD_BUTTON_A;
                    }
                    switch (native_classic_match_diagnostic_frame) {
                    case 160:
                        classic_input.button = PAD_BUTTON_START;
                        break;
                    default:
                        break;
                    }
                    /* A targeted CSS probe stages its live cursor input from
                     * mncharsel later in this iteration. Do not replace that
                     * pending sample with this fixed route before the latch. */
                    if (!getenv("MELEE_DIAGNOSTIC_CSS_TARGET0") &&
                        !getenv("MELEE_DIAGNOSTIC_CSS_TARGET1")) {
                        HSD_ASSERT(1, HSD_PadStageDiagnosticInput(0,
                                                                  &classic_input));
                    }
                    if (native_classic_match_diagnostic_frame == 30 ||
                        native_classic_match_diagnostic_frame == 31 ||
                        native_classic_match_diagnostic_frame == 32 ||
                        native_classic_match_diagnostic_frame == 61 ||
                        native_classic_match_diagnostic_frame == 62 ||
                        native_classic_match_diagnostic_frame == 63 ||
                        native_classic_match_diagnostic_frame == 64 ||
                        native_classic_match_diagnostic_frame == 109 ||
                        native_classic_match_diagnostic_frame == 110 ||
                        native_classic_match_diagnostic_frame == 111 ||
                        native_classic_match_diagnostic_frame == 160 ||
                        native_classic_match_diagnostic_frame == 161) {
                        OSReport("NATIVE_CLASSIC_PAD_STAGE frame=%u button=%u stickY=%d before_latch=1\n",
                                 native_classic_match_diagnostic_frame,
                                 classic_input.button,
                                 classic_input.stickY);
                    }
                }
            }
#endif
#ifdef TARGET_PC
            unsigned native_raw_before = 0;
            if (info->scene_kind == 2 && getenv("MELEE_DIAGNOSTIC_PAD_CATCHUP_TRACE"))
                native_raw_before = HSD_PadGetRawQueueCount();
#endif
            lb_800198E0();
#ifdef TARGET_PC
            if (info->scene_kind == 2 && getenv("MELEE_DIAGNOSTIC_PAD_CATCHUP_TRACE"))
                OSReport("NATIVE_PAD_CATCHUP_CONSUMED step=%u before=%u after=%u\n",
                         temp_r25->unk_0, native_raw_before, HSD_PadGetRawQueueCount());
#endif
#ifdef TARGET_PC
            if (native_title_start_diagnostic_staged &&
                getenv("MELEE_DIAGNOSTIC_CLASSIC_MATCH") &&
                gm_GetCurrentGameMode() == GM_CLASSIC &&
                on_frame == native_classic_match_diagnostic_callback &&
                (native_classic_match_diagnostic_frame == 30 ||
                 native_classic_match_diagnostic_frame == 31 ||
                 native_classic_match_diagnostic_frame == 32 ||
                 native_classic_match_diagnostic_frame == 65 ||
                 native_classic_match_diagnostic_frame == 66 ||
                 native_classic_match_diagnostic_frame == 67 ||
                 native_classic_match_diagnostic_frame == 68 ||
                 native_classic_match_diagnostic_frame == 109 ||
                 native_classic_match_diagnostic_frame == 110 ||
                 native_classic_match_diagnostic_frame == 111 ||
                 native_classic_match_diagnostic_frame == 160 ||
                 native_classic_match_diagnostic_frame == 161)) {
                OSReport("NATIVE_CLASSIC_PAD_LATCH frame=%u button=%u trigger=%u stickX=%d stickY=%d err=%u\n",
                         native_classic_match_diagnostic_frame,
                         HSD_PadCopyStatus[0].button,
                         HSD_PadCopyStatus[0].trigger,
                         HSD_PadCopyStatus[0].stickX,
                         HSD_PadCopyStatus[0].stickY,
                         HSD_PadCopyStatus[0].err);
            }
#endif
#ifdef TARGET_PC
            if (native_ordered_diagnostic) {
                OSReport("NATIVE_ORDERED_INPUT tick=%u gameplay_frame=%u ready_after=%u\n",
                         native_ordered_tick++, gm_GetFrameCount(),
                         HSD_PadOrderedInputReady());
                if (native_udp_diagnostic)
                    OSReport("NATIVE_INPUT_UDP consumed_tick=%u accepted=%u rejected=%u duplicates=%u next_buffer_tick=%llu\n",
                             native_ordered_tick-1,native_udp_receiver.accepted,native_udp_receiver.rejected,
                             native_udp_receiver.duplicates,(unsigned long long)native_udp_receiver.input.next_tick);
            }
#endif
#ifdef TARGET_PC
            if (on_frame == gm_Scene_Vs_OnFrame &&
                getenv("MELEE_DIAGNOSTIC_FOUR_PORT_INPUT")) {
                OSReport("NATIVE_FOUR_PORT_CONSUMED frame=%u b0=%u x0=%d b1=%u x1=%d b2=%u x2=%d b3=%u x3=%d\n",
                         gm_GetFrameCount()+1,
                         HSD_PadMasterStatus[0].button,HSD_PadMasterStatus[0].stickX,
                         HSD_PadMasterStatus[1].button,HSD_PadMasterStatus[1].stickX,
                         HSD_PadMasterStatus[2].button,HSD_PadMasterStatus[2].stickX,
                         HSD_PadMasterStatus[3].button,HSD_PadMasterStatus[3].stickX);
            }
#endif
#ifdef TARGET_PC
            if (getenv("MELEE_DIAGNOSTIC_INPUT_TICKS")) {
                OSReport("NATIVE_INPUT_TICK phase=consumed frame=%u queue_remaining=%u button0=%u stick_x0=%d stick_y0=%d button1=%u stick_x1=%d stick_y1=%d\n",
                         gm_GetFrameCount(), HSD_PadGetRawQueueCount(),
                         HSD_PadMasterStatus[0].button,
                         HSD_PadMasterStatus[0].stickX,
                         HSD_PadMasterStatus[0].stickY,
                         HSD_PadMasterStatus[1].button,
                         HSD_PadMasterStatus[1].stickX,
                         HSD_PadMasterStatus[1].stickY);
            }
#endif
            if (DbLevel >= DbLKind_DebugRom) {
                gm_801A4970(&temp_r25->unk_10.db_input);
            }
            if (gm_801A46B8(0) || !gm_GetDbPauseFlag(0)) {
                temp_r25->unk_10.unk_38_0 = true;
            } else {
                temp_r25->unk_10.unk_38_0 = false;
            }
            if (gm_80479D58.unk_10.unk_38_0) {
                lb_80019900();
                if (lb_80019A30(0)) {
                    gm_EvaluateAllControllerInputs();
                }
                if (lb_80019A30(0) && on_frame != NULL) {
#ifdef TARGET_PC
                    OSTime native_scene_callback_start = OSGetTime();
#endif
                    on_frame();
#if defined(TARGET_PC) && defined(_WIN32)
                    if (native_congrats_return_menu) {
                        PADStatus neutral = { 0 };
                        ++native_congrats_return_menu_frames;
                        HSD_ASSERT(1, HSD_PadStageDiagnosticInput(0, &neutral));
                        if (native_congrats_return_menu_frames == 1 ||
                            native_congrats_return_menu_frames % 60 == 0) {
                            OSReport("NATIVE_CONGRATS_RETURN_MENU_FRAME frames=%u mode=%d\n",
                                     native_congrats_return_menu_frames,
                                     gm_GetCurrentGameMode());
                        }
                        if (native_congrats_return_menu_frames == 300) {
                            OSReport("NATIVE_CONGRATS_RETURN_MENU_COMPLETE frames=%u queued=%d\n",
                                     native_congrats_return_menu_frames,
                                     aurora_diagnostic_queue_quit());
                        }
                    }
#endif
#ifdef TARGET_PC
                    if (native_pacing_trace_enabled) {
                        native_pacing_simulation_ticks++;
                    }
                    native_pacing_profile_add(NATIVE_PACING_SCENE_CALLBACK,
                                              native_scene_callback_start);
#endif
#ifdef TARGET_PC
                    if (getenv("MELEE_DIAGNOSTIC_INPUT_TICKS")) {
                        OSReport("NATIVE_INPUT_TICK phase=before_procs frame=%u button0=%u trigger0=%u button1=%u trigger1=%u\n",
                                 gm_GetFrameCount(), HSD_PadCopyStatus[0].button,
                                 HSD_PadCopyStatus[0].trigger,
                                 HSD_PadCopyStatus[1].button,
                                 HSD_PadCopyStatus[1].trigger);
                    }
#endif
                }
            }
            if (gm_80479D58.unk_10.x0 != gm_80479D58.unk_10.x1 ||
                temp_r25->unk_10.x2 != temp_r25->unk_10.x3)
            {
                temp_r25->unk_10.unk_20 =
                    maybe_gm_801A48A4(temp_r25->unk_10.x0);
                temp_r25->unk_10.x1 = temp_r25->unk_10.x0;
                temp_r25->unk_10.x3 = temp_r25->unk_10.x2;
                temp_r25->unk_10.x2 = 0;
            }
            temp_r25->unk_10.unk_28 = temp_r25->unk_10.unk_20;
            if (!lb_80019A30(0)) {
                temp_r25->unk_10.unk_28 |=
                    gm_803DA8C8[temp_r25->unk_10.unk_34];
            }
            if (!lb_80019A30(1)) {
                temp_r25->unk_10.unk_28 |=
                    ~gm_803DA8C8[temp_r25->unk_10.unk_34];
            }
            if (DbLevel >= DbLKind_DebugRom) {
                db_CheckScreenshot();
            }
            lbAudioAx_80027DF8();
            if (temp_r25->unk_10.pre_gobj_proc != NULL) {
                temp_r25->unk_10.pre_gobj_proc();
            }
#ifdef TARGET_PC
            OSTime native_proc_start=OSGetTime();
#endif
            PC_FRAME_PHASE("gobj_procs_begin");
#ifdef TARGET_PC
            if (on_frame == gm_Scene_Training_OnFrame && getenv("MELEE_DIAGNOSTIC_TRAINING_UI_TRACE")) {
                static unsigned calls;
                if (++calls % 60 == 0)
                    OSReport("NATIVE_TRAINING_PROC_MASK call=%u frame=%u flags=%u active=%llu base=%llu enabled=%u input_ready=%d render_ready=%d\n",
                             calls, gm_GetFrameCount(), gm_80479D58.unk_10.x0,
                             temp_r25->unk_10.unk_28, temp_r25->unk_10.unk_20,
                             temp_r25->unk_10.unk_38_0, lb_80019A30(0), lb_80019A30(1));
            }
#endif
    HSD_GObj_RunProcs();
    PC_FRAME_PHASE("gobj_procs_end");
#ifdef TARGET_PC
            native_pacing_profile_add(NATIVE_PACING_GOBJ_PROCS, native_proc_start);
            if (native_ordered_diagnostic && getenv("MELEE_DIAGNOSTIC_ORDERED_PAUSE"))
                OSReport("NATIVE_ORDERED_PAUSE tick=%u gameplay_frame=%u paused=%u button=%u trigger=%u\n",
                         native_ordered_tick-1, gm_GetFrameCount(), gm_GetDbPauseFlag(1),
                         HSD_PadCopyStatus[0].button, HSD_PadCopyStatus[0].trigger);
            MeleePc_ReportSlowPhase("simulation_procs",native_proc_start);
            /* Native decode/procedures can consume most of an audio service
             * interval. Retire owner callbacks before drawing or catch-up
             * simulation adds another long phase. This never advances game
             * simulation and respects the host interrupt gate. */
            HSD_ASSERT(1,aurora_service_owner());
#endif
            if (temp_r25->unk_0 != -2) {
                temp_r25->unk_0++;
            }
#ifdef TARGET_PC
            if (native_pad_stall_ms && temp_r25->unk_0 == 120) {
                OSReport("NATIVE_PAD_STALL begin frame=120 ms=%u ticks=%lld\n", native_pad_stall_ms, OSGetTime());
                Sleep(native_pad_stall_ms);
                OSReport("NATIVE_PAD_STALL end frame=120 ticks=%lld\n", OSGetTime());
                native_pad_stall_ms = 0;
            }
#endif
            if (gm_80479D58.unk_10.unk_38_0 && lb_80019A30(0)) {
                if (temp_r25->unk_8 != -2) {
                    temp_r25->unk_8++;
                }
            }
            HSD_PerfSetCPUTime();
            if (DbLevel >= DbLKind_DebugRom) {
                OSCheckActiveThreads();
            }
            gmMainLib_8046B0F0.xC = false;
            if (temp_r25->unk_C != 0) {
                break;
            }
        }
        if (temp_r25->unk_C == 2) {
            break;
        }

        lb_800195D0();
#ifdef TARGET_PC
        if (native_presentation_ready) {
            native_poll_presentation_events();
            if (aurora_set_render_snapshot_scene(true)) {
                if (native_service_presentation_clock(&native_presentation_clock, native_presentation_origin) &&
                    temp_r25->unk_4 != -2U)
                    temp_r25->unk_4++;
                continue;
            }
            /* A real VI source/black change has ended GPU clock ownership.
             * Preserve its normal copy/presentation path until drained. */
        }
        /* Reserve the next XFB before opening a host recording. The original
         * copy routine waits for an available slot; doing that inside an open
         * recording prevents VI from submitting the previous presentation. */
        OSTime native_xfb_slot_wait_start = OSGetTime();
        HSD_VIWaitXFBDrawEnable();
        native_pacing_profile_add(NATIVE_PACING_XFB_SLOT_WAIT,
                                  native_xfb_slot_wait_start);
        PC_FRAME_PHASE("xfb_ready");
        /* Retain this simulation result while waiting for a usable host
         * surface; acquiring a frame must not run extra game updates. */
        OSTime native_host_surface_acquire_start = OSGetTime();
        while (!native_begin_scene_frame()) {
            /* Aurora updates device state here. Host exit handling remains
             * part of the unfinished native lifecycle integration. */
            aurora_update();
            lb_800195D0();
        }
        native_pacing_profile_add(NATIVE_PACING_HOST_SURFACE_ACQUIRE,
                                  native_host_surface_acquire_start);
#endif
        GXInvalidateVtxCache();
        PC_FRAME_PHASE("host_frame_acquired");
#ifdef TARGET_PC
        OSTime native_draw_start=OSGetTime();
#endif
        GXInvalidateTexAll();
        HSD_StartRender(HSD_RP_SCREEN);
        PC_FRAME_PHASE("render_callbacks_begin");
    #ifdef TARGET_PC
        MeleePc_SetRenderCallbackTrace(pc_frame_index<=2 && getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")!=NULL);
#endif
        HSD_GObj_80390FC0();
#ifdef TARGET_PC
        native_pacing_scene_draws++;
        MeleePc_SetRenderCallbackTrace(0);
#endif
    PC_FRAME_PHASE("render_callbacks_end");
#ifdef TARGET_PC
        native_pacing_profile_add(NATIVE_PACING_RENDER_CALLBACKS, native_draw_start);
        MeleePc_ReportSlowPhase("original_render_callbacks",native_draw_start);
        HSD_ASSERT(1,aurora_service_owner());
        OSTime native_copy_start=OSGetTime();
#endif
        HSD_Init_803755A8();
        HSD_PerfSetDrawTime();
        HSD_VICopyXFBAsync(HSD_RP_SCREEN);
#ifdef TARGET_PC
        native_pacing_profile_add(NATIVE_PACING_XFB_COPY, native_copy_start);
        MeleePc_ReportSlowPhase("original_xfb_copy",native_copy_start);
        MeleePc_ReportSlowPhase("original_draw_copy",native_draw_start);
        HSD_ASSERT(1,aurora_service_owner());
        OSTime native_end_start=OSGetTime();
        aurora_end_frame();
        native_pacing_profile_add(NATIVE_PACING_HOST_END_FRAME, native_end_start);
        MeleePc_ReportSlowPhase("host_end_frame",native_end_start);
        if (native_presentation_hz && !native_presentation_ready) {
            /* The bootstrap frame applies HSD VI configuration and genuinely
             * presents its CPU XFB before the independent GPU-scene handoff. */
            HSD_VIWaitXFBFlush();
            native_presentation_origin = OSGetTime();
            HSD_ASSERT(1, MeleePc_PresentationClockInit(&native_presentation_clock, native_presentation_hz));
            native_presentation_ready = true;
            /* Start timing at the same boundary as the independent clock.
             * Otherwise its first sample can include bootstrap work. */
            native_pacing_previous_presented = 0;
            native_log_pacing_sample();
            OSReport("NATIVE_PRESENTATION_CLOCK initialized=1 target=%u simulation_hz=60 bootstrap_drained=1\n", native_presentation_hz);
        }
        if (native_repeat_render_hz != 0) {
            if (native_repeat_render_each_tick) {
                const u32 simulation_before = gm_80479D58.unk_0;
                const bool rendered = native_present_current_scene_state();
                const u32 simulation_after = gm_80479D58.unk_0;
                HSD_ASSERT(1, simulation_before == simulation_after);
                if (rendered) native_pacing_repeat_render_count++;
                if (rendered && temp_r25->unk_4 != -2U) {
                    temp_r25->unk_4++;
                }
                native_log_pacing_sample();
            } else {
                if (native_next_repeat_render == 0) {
                    native_next_repeat_render = OSGetTime() + native_repeat_render_period;
                }
            }
        }
#endif
        if (temp_r25->unk_4 != -2U) {
            temp_r25->unk_4++;
        }
#ifdef TARGET_PC
        if (native_pacing_trace_enabled) {
            native_log_pacing_sample();
        }
        if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") && temp_r25->unk_4 % 60 == 0) {
            uint64_t submitted, completed, failed, presented;
            aurora_diagnostic_gpu_counts(&submitted, &completed, &failed, &presented);
            OSReport("NATIVE_FRAME simulation=%u render=%u submitted=%llu completed=%llu failed=%llu presented=%llu\n",
                     temp_r25->unk_0, temp_r25->unk_4, submitted, completed, failed, presented);
        }
#endif
        db_TakeScreenshotIfPending();
        HSD_PerfSetTotalTime();
        HSD_PerfInitStat();
    }
#ifdef TARGET_PC
    if (native_prize_pad_trace) PADSetDiagnosticPrizeTrace(false);
    if (native_udp_diagnostic) {
        HSD_ASSERT(1,MeleePc_UnregisterSessionCleanup(native_udp_cleanup,&native_udp_cleanup_data));
        native_udp_cleanup(&native_udp_cleanup_data);
    }
    HSD_PadEndOrderedInput();
    if (native_presentation_hz) {
        HSD_ASSERT(1, aurora_set_render_snapshot_scene(false));
        OSReport("NATIVE_PRESENTATION_CLOCK scene_exit=1 target=%u drawn=%llu missed=%llu pending=%llu\n",
                 native_presentation_hz, native_presentation_clock.presented,
                 native_presentation_clock.missed, native_presentation_clock.pending);
    }
    MeleePc_SetScenePadCatchUp(false);
    if (native_vsync_override) {
        if (aurora_is_vsync_enabled() != native_previous_vsync)
            aurora_enable_vsync(native_previous_vsync);
        OSReport("NATIVE_GAMEPLAY_VSYNC_RESTORED requested=%u actual=%u\n",
                 native_previous_vsync,aurora_is_vsync_enabled());
    }
#endif
    HSD_VIWaitXFBFlush();
}
