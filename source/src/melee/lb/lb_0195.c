#include "lb_0195.h"

#include "lb_0192.h"
#include "lbaudio_ax.h"
#include "lbcardgame.h"
#include "lbcardnew.h"
#include "lbsnap.h"
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <sysdolphin/baselib/controller.h>
#ifdef TARGET_PC
#include <pc/pad_sampling.h>
#include <stdlib.h>
#include <pc/os_alarm.h>
#include <dolphin/ar.h>
#include <sysdolphin/baselib/debug.h>
#include <aurora/audio.h>
#include <aurora/dvd.h>
#include <aurora/aurora.h>
#include <aurora/diagnostics.h>
#include <pc/callback_gate.h>
#include <sysdolphin/baselib/devcom.h>
#endif

struct UnkArrElem {
    /* 0x00 */ s64 x0;
    /* 0x08 */ s64 x8;
    /* 0x10 */ int x10;
};

struct lb_804329F0_t {
    struct UnkArrElem x0[2];
    u32 x4;
    u64 x38;
    OSTime x40;
    int x48;
    OSAlarm alarm;
};

/* 4329F0 */ static struct lb_804329F0_t lb_804329F0;

void lb_8001955C(void)
{
    if (HSD_PadGetResetSwitch()) {
        lbAudioAx_80027DBC();
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
        while (lbCardNew_CompleteNextTask() == 11);
#endif
        VISetPostRetraceCallback(0);
        VISetPreRetraceCallback(0);
        VISetBlack(1);
        VIFlush();
        VIWaitForRetrace();
        VIWaitForRetrace();
        OSResetSystem(0, 0, 0);
    }
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
    lbCardNew_CompleteNextTask();
    lb_8001CC84();
#endif
}

void lb_800195D0(void)
{
#ifdef TARGET_PC
    if(MeleePc_CallbacksEnabled()) HSD_ASSERT(1,aurora_service_wait_events());
    if(MeleePc_CallbacksEnabled() && getenv("MELEE_DIAGNOSTIC_CLOSE_FILE_WAIT") &&
       (HSD_DevComIsBusy(0)||HSD_DevComIsBusy(1)||HSD_DevComIsBusy(2))) {
        OSReport("NATIVE_ORIGINAL_ASSET_WAIT_EXIT dvd_busy=1\n");
        HSD_ASSERT(1,aurora_diagnostic_queue_window_close());
        OSReport("NATIVE_ORIGINAL_ASSET_WAIT_EXIT event=window_close queued=1\n");
        HSD_ASSERT(1,aurora_service_wait_events());
        OSPanic(__FILE__,__LINE__,"Loading close event was not handled");
    }
    /* Console timer interrupts become bounded owner-thread delivery on PC.
     * The scene's pad-queue wait calls this function, so timer callbacks must
     * run here even before a frame can be drawn. */
    HSD_ASSERT(1, MeleePc_DispatchAlarms(NULL, 64) >= 0);
    HSD_ASSERT(1, aurora_arq_service());
    HSD_ASSERT(1, aurora_dvd_service());
    if(aurora_ax_source_rate()) HSD_ASSERT(1, aurora_ax_service());
#endif
    lb_800192A8(lb_8001955C);
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
    lb_8001CC84();
#endif
}

void fn_800195FC(void)
{
    HSD_PadRenewRawStatus(0);
#if !defined(TARGET_PC) || !defined(MELEE_NO_SAVES)
    lb_8001C600();
    lbSnap_8001D2BC();
#endif
}

#ifdef TARGET_PC
static bool native_pad_catch_up;
static u64 native_pad_delivered, native_pad_blocked;
static OSTime native_pad_max_lateness;
static int NativePadCanQueue(void* unused)
{
    (void) unused;
    const bool ready = HSD_PadLibData.qnum != 0 &&
        HSD_PadLibData.qcount < HSD_PadLibData.qnum;
    if (!ready) native_pad_blocked++;
    return ready;
}
static void NativePadSamplingAlarm(OSAlarm* alarm, OSContext* context)
{
    (void) context;
    if (native_pad_catch_up) {
        /* OS adapter has advanced fire by exactly one period. */
        OSTime late = OSGetTime() - (alarm->fire - alarm->period);
        if (late > native_pad_max_lateness) native_pad_max_lateness = late;
        native_pad_delivered++;
        if (native_pad_delivered <= 1024 && getenv("MELEE_DIAGNOSTIC_PAD_CATCHUP_TRACE"))
            OSReport("NATIVE_PAD_CATCHUP_SAMPLE sequence=%llu scheduled=%lld now=%lld queued=%u capacity=%u\n",
                     native_pad_delivered, alarm->fire - alarm->period, OSGetTime(),
                     HSD_PadLibData.qcount, HSD_PadLibData.qnum);
    }
    fn_800195FC();
}

void MeleePc_SetScenePadCatchUp(bool enabled)
{
    if (native_pad_catch_up == enabled) return;
    if (native_pad_catch_up && getenv("MELEE_DIAGNOSTIC_PAD_CATCHUP_TRACE"))
        OSReport("NATIVE_PAD_CATCHUP enabled=0 delivered=%llu blocked=%llu max_late_ticks=%lld queued=%u capacity=%u\n",
                 native_pad_delivered, native_pad_blocked, native_pad_max_lateness,
                 HSD_PadLibData.qcount, HSD_PadLibData.qnum);
    native_pad_catch_up = enabled;
    native_pad_delivered = native_pad_blocked = 0;
    native_pad_max_lateness = 0;
    if (lb_804329F0.x48) {
        /* Rebase at scene boundaries; subsequent gameplay keeps every tick. */
        OSSetPeriodicAlarm(&lb_804329F0.alarm, lb_804329F0.x40,
                          lb_804329F0.x40, NativePadSamplingAlarm);
        if (enabled)
            HSD_ASSERT(1, MeleePc_SetAlarmCatchUp(&lb_804329F0.alarm, NativePadCanQueue, NULL));
    }
    if (enabled && getenv("MELEE_DIAGNOSTIC_PAD_CATCHUP_TRACE"))
        OSReport("NATIVE_PAD_CATCHUP enabled=1 period=%lld queued=%u capacity=%u\n",
                 lb_804329F0.x40, HSD_PadLibData.qcount, HSD_PadLibData.qnum);
}
#endif

void lb_80019628(void)
{
    int i;
    OSTime period;
    OSTime new_val = lb_804329F0.x38;
    struct UnkArrElem* tmp = &lb_804329F0.x0[0];

    if (new_val == tmp->x0) {
        return;
    }

    tmp->x0 = new_val;

    if (tmp->x8 >= tmp->x0) {
        tmp->x8 = 0;
    }

    period = (f32) OSSecondsToTicks(1);

    for (i = 0; i < 2; i++) {
        if (lb_804329F0.x0[i].x0 < period) {
            period = lb_804329F0.x0[i].x0;
        }
    }

    if (period >= (OSTime) OSSecondsToTicks(1.0F / 60)) {
        period = OSSecondsToTicks(1.0F / 60);
    }

    if (lb_804329F0.x40 == period) {
        return;
    }

    lb_804329F0.x40 = period;

    {
        u32 rate = OSTicksToMilliseconds(lb_804329F0.x40);
        if (rate > 11) {
            rate = 11;
        }
        if (lb_804329F0.x4 != rate) {
            PADSetSamplingRate(rate);
            lb_804329F0.x4 = rate;
        }
    }

    if (lb_804329F0.x48 != 0) {
        OSCancelAlarm(&lb_804329F0.alarm);
    }
    OSCreateAlarm(&lb_804329F0.alarm);
    OSSetPeriodicAlarm(&lb_804329F0.alarm, lb_804329F0.x40, lb_804329F0.x40,
#ifdef TARGET_PC
                       NativePadSamplingAlarm);
#else
                       (OSAlarmHandler) fn_800195FC);
#endif
#ifdef TARGET_PC
    if (native_pad_catch_up)
        HSD_ASSERT(1, MeleePc_SetAlarmCatchUp(&lb_804329F0.alarm, NativePadCanQueue, NULL));
#endif
    lb_804329F0.x48 = 1;
}

void lb_80019880(u64 arg0)
{
    lb_804329F0.x38 = arg0;
}

u8 lb_80019894(void)
{
    u8 count;
    int enabled = OSDisableInterrupts();
    count = HSD_PadGetRawQueueCount();
    lb_80019628();
    OSRestoreInterrupts(enabled);
    return count;
}

void lb_800198E0(void)
{
#ifdef TARGET_PC
    unsigned queued_before = HSD_PadGetRawQueueCount();
#endif
    HSD_PadRenewMasterStatus();
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_PAD_LATCH")) {
        static unsigned sample;
        if (sample < 2000) {
            OSReport("NATIVE_PAD_LATCH sample=%u queued_before=%u queued_after=%u "
                     "buttons=%u stick_x=%d stick_y=%d\n", ++sample,
                     queued_before, HSD_PadGetRawQueueCount(),
                     HSD_PadMasterStatus[0].button,
                     HSD_PadMasterStatus[0].stickX,
                     HSD_PadMasterStatus[0].stickY);
        }
    }
#endif
}

void lb_80019900(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        lb_804329F0.x0[i].x8 += lb_804329F0.x40;
        if (lb_804329F0.x0[i].x8 >= lb_804329F0.x0[i].x0) {
            lb_804329F0.x0[i].x8 -= lb_804329F0.x0[i].x0;
            lb_804329F0.x0[i].x10 = true;
        } else {
            lb_804329F0.x0[i].x10 = false;
        }
    }

    if (lb_80019A30(0)) {
        HSD_PadRenewGameStatus();
    }
    if (lb_80019A30(0)) {
        HSD_PadRenewCopyStatus();
    }
}

bool lb_80019A30(int index)
{
    return lb_804329F0.x0[index].x10;
}

void lb_80019A48(void)
{
    int enabled = OSDisableInterrupts();

    if (lb_804329F0.x48) {
        OSCancelAlarm(&lb_804329F0.alarm);
        lb_804329F0.x48 = 0;
    }
    OSRestoreInterrupts(enabled);
}

void lb_80019AAC(Event arg0)
{
    int i;

    arg0();

    for (i = 0; i < 2; i++) {
        struct UnkArrElem* cur = &lb_804329F0.x0[i];
        cur->x0 = OSSecondsToTicks(1.0F / 60);
        cur->x8 = 0;
        cur->x10 = 0;
    }

    lb_804329F0.x4 = 0;
    lb_804329F0.x48 = 0;
    lb_804329F0.x0[0].x0 = 0; // huh? overwritten?
    lb_804329F0.x40 = 0;
    lb_804329F0.x38 = OSSecondsToTicks(1.0F / 60);

    lb_80019628();
}

