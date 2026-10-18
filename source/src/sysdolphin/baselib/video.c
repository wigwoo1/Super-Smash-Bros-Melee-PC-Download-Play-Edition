#include "video.h"

#include "debug.h"
#include "state.h"
#include <dolphin/gx.h>
#include <dolphin/vi.h>
#ifdef TARGET_PC
#include <aurora/aurora.h>
#include <pc/video_config.h>
#include <stdlib.h>
#endif

HSD_VIInfo HSD_VIData;
static u8 garbage[HSD_ANTIALIAS_GARBAGE_SIZE] ATTRIBUTE_ALIGN(32);
#ifdef TARGET_PC
/* A native Present acknowledgement may arrive after a later copy completes.
 * Post must promote only the slot actually selected by this transaction. */
static int native_selected_xfb = -1;
#endif

#define _p ((HSD_VIInfo*) &HSD_VIData)

#ifdef TARGET_PC
/* Owner-thread observations only; draw_done delivers its callback on this
 * thread after GPU completion and CPU readback. This does not time those
 * two asynchronous operations separately or change slot ownership. */
static void native_trace_xfb_event(const char* event, int idx, int new_copy)
{
    static int enabled = -1;
    static unsigned records;
    static unsigned sequence;
    static unsigned copies[HSD_VI_XFB_MAX];
    if (enabled < 0) {
        enabled = getenv("MELEE_DIAGNOSTIC_XFB_SLOT_TRACE") != NULL;
    }
    if (!enabled || records >= 8192) {
        return;
    }
    if (new_copy) {
        copies[idx] = ++sequence;
    }
    OSReport("NATIVE_XFB_EVENT record=%u event=%s slot=%d copy=%u "
             "time_us=%llu retrace=%u status=%d\n",
             ++records, event, idx, copies[idx],
             (unsigned long long) OSTicksToMicroseconds(OSGetTime()),
             VIGetRetraceCount(), _p->xfb[idx].status);
}
#endif

static int HSD_VISearchXFBByStatus(HSD_VIXFBDrawDispStatus status)
{
    int i;

    for (i = 0; i < HSD_VI_XFB_MAX; i++) {
        if (_p->xfb[i].status == status) {
            return i;
        }
    }
    return -1;
}

HSD_VIRetraceCallback HSD_VISetUserPreRetraceCallback(HSD_VIRetraceCallback cb)
{
    bool intr;
    HSD_VIRetraceCallback old = _p->pre_cb;

    intr = OSDisableInterrupts();
    _p->pre_cb = cb;
    OSRestoreInterrupts(intr);

    return old;
}

HSD_VIRetraceCallback
HSD_VISetUserPostRetraceCallback(HSD_VIRetraceCallback cb)
{
    bool intr;
    HSD_VIRetraceCallback old = _p->post_cb;

    intr = OSDisableInterrupts();
    _p->post_cb = cb;
    OSRestoreInterrupts(intr);

    return old;
}

HSD_VIGXDrawDoneCallback
HSD_VISetUserGXDrawDoneCallback(HSD_VIGXDrawDoneCallback cb)
{
    bool intr;
    HSD_VIGXDrawDoneCallback old = _p->drawdone.cb;

    intr = OSDisableInterrupts();
    _p->drawdone.cb = cb;
    OSRestoreInterrupts(intr);

    return old;
}

static void HSD_VIPreRetraceCB(u32 retraceCount)
{
    int idx;
    int flush = 0;
    int renew = 0;
    int stageDropped = 0;

    idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT);
#ifdef TARGET_PC
    native_selected_xfb = idx;
#endif
    if (idx != -1) {
        VISetNextFrameBuffer(_p->xfb[idx].buffer);
#ifdef TARGET_PC
        // Aurora may intentionally retain the last VI image when GXCopyDisp
        // drops this destination after a recoverable CPU readback allocation
        // failure. Do not commit this buffer as DISPLAY unless VI accepted it.
        if (VIGetNextFrameBuffer() != _p->xfb[idx].buffer) {
            native_selected_xfb = -1;
            /* VI rejected this copy (for example, its XFB readback Job could
             * not be allocated). Do not leave the invalid frame at NEXT: the
             * pre-retrace callback would select it forever and repeatedly
             * fail to stage it, preventing later completed XFBs from taking
             * over. Recycle only the selected, rejected NEXT slot. */
            if (_p->xfb[idx].status == HSD_VI_XFB_NEXT) {
                _p->xfb[idx].status = HSD_VI_XFB_FREE;
            }
            idx = -1;
            stageDropped = 1;
        }
#endif
    }
    if (idx != -1) {
#ifdef TARGET_PC
        native_trace_xfb_event("selected", idx, 0);
#endif
        if (_p->xfb[idx].vi_all.chg_flag) {
            VIConfigure(&_p->xfb[idx].vi_all.vi.rmode);
            VISetBlack(_p->xfb[idx].vi_all.vi.black);
        }
        flush = 1;
        renew = 1;
    } else if (!stageDropped && HSD_VIGetNbXFB() == 1 &&
               _p->efb.status == HSD_VI_EFB_DRAWDONE)
    {
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) == -1) {
            idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
            HSD_ASSERT(252, idx != -1);
            VISetNextFrameBuffer(_p->xfb[idx].buffer);
            flush = 1;
        }
        _p->xfb[idx].status = HSD_VI_XFB_COPYEFB;
        if (_p->efb.vi_all.chg_flag) {
            VIConfigure(&_p->efb.vi_all.vi.rmode);
            VISetBlack(_p->efb.vi_all.vi.black);
            flush = 1;
        }
        renew = 1;
    } else if (stageDropped) {
#ifdef TARGET_PC
        /* On the very first XFB failure, no current surface exists to retain.
         * Flush startup black so VIWaitForRetrace can advance while a later
         * successful readback produces the first presentable game image. */
        if (VIGetCurrentFrameBuffer() == NULL) {
            VISetBlack(1);
            flush = 1;
        }
#endif
        renew = 1;
    }

    if (flush) {
        VIFlush();
    }

    {
        static int vr_count = 0;
        static int renew_count = 0;

        if (renew) {
            renew_count++;
        }
        if (++vr_count >= _p->perf.frame_period) {
            _p->perf.frame_renew = renew_count;
            vr_count = renew_count = 0;
        }
    }

    if (_p->pre_cb) {
        _p->pre_cb(retraceCount);
    }
}

#ifdef TARGET_PC
/* Refresh only the source of an unpublished, unchanged VI transaction.
 * Do not replay user pre callbacks, performance accounting, or mode changes. */
static void native_refresh_ready_xfb(void)
{
    int idx;
    if (native_selected_xfb != -1 || _p->nb_xfb < 2) return;
    idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT);
    if (idx == -1 || _p->xfb[idx].vi_all.chg_flag) return;
    VISetNextFrameBuffer(_p->xfb[idx].buffer);
    if (VIGetNextFrameBuffer() != _p->xfb[idx].buffer) return;
    native_selected_xfb = idx;
    VIFlush();
    native_trace_xfb_event("selected", idx, 0);
    {
        static unsigned records;
        if (records < 128) OSReport("NATIVE_LATE_XFB_STAGED record=%u slot=%d\n", ++records, idx);
    }
}
#endif

static void HSD_VIPostRetraceCB(u32 retraceCount)
{
    int idx;
    int next;

#ifdef TARGET_PC
    next = native_selected_xfb;
    native_selected_xfb = -1;
    if (next != -1) {
        HSD_ASSERT(0, _p->xfb[next].status == HSD_VI_XFB_NEXT &&
                      VIGetCurrentFrameBuffer() == _p->xfb[next].buffer);
#else
    if ((next = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) != -1) {
#endif
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) != -1) {
            _p->xfb[idx].status = HSD_VI_XFB_FREE;
#ifdef TARGET_PC
            native_trace_xfb_event("freed", idx, 0);
#endif
        }
        _p->xfb[next].status = HSD_VI_XFB_DISPLAY;
#ifdef TARGET_PC
        native_trace_xfb_event("display", next, 0);
#endif
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) != -1) {
            _p->xfb[idx].status = HSD_VI_XFB_NEXT;
        }
    } else if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_COPYEFB)) != -1) {
        HSD_VICopyEFB2XFBPtr(&_p->efb.vi_all.vi, _p->xfb[idx].buffer,
                             HSD_RP_SCREEN);
        _p->xfb[idx].status = HSD_VI_XFB_DISPLAY;
        _p->efb.status = HSD_VI_EFB_FREE;
    }

    if (_p->post_cb) {
        _p->post_cb(retraceCount);
    }
}

static void HSD_VIGXDrawDoneCB(void)
{
    _p->drawdone.waiting = 0;

    if (_p->drawdone.cb) {
        _p->drawdone.cb(_p->drawdone.arg);
    }
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
static int HSD_VIGetDrawDoneWaitingFlag(void)
{
    return _p->drawdone.waiting;
}
#ifdef MUST_MATCH
#pragma pop
#endif

int HSD_VIGetXFBDrawEnable(void)
{
    bool intr;
    int idx = -1;

    if (HSD_VIGetNbXFB() < 2) {
        goto ret;
    }

    intr = OSDisableInterrupts();

    if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING)) == -1) {
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE)) != -1) {
            _p->xfb[idx].status = HSD_VI_XFB_DRAWING;
        }
    }

    OSRestoreInterrupts(intr);

ret:
    return idx;
}

int HSD_VIWaitXFBDrawEnable(void)
{
    int idx = -1;
#ifdef TARGET_PC
    static int trace_enabled = -1;
    static unsigned trace_records;
    unsigned waits = 0;
    int initial_status[HSD_VI_XFB_MAX];
    u32 initial_retrace = 0;
    OSTime start = 0;
    if (trace_enabled < 0) {
        trace_enabled = getenv("MELEE_DIAGNOSTIC_XFB_SLOT_TRACE") != NULL;
    }
    if (trace_enabled) {
        int i;
        start = OSGetTime();
        initial_retrace = VIGetRetraceCount();
        for (i = 0; i < HSD_VI_XFB_MAX; i++) {
            initial_status[i] = _p->xfb[i].status;
        }
    }
#endif

    if (HSD_VIGetNbXFB() < 2) {
        goto ret;
    }

    while ((idx = HSD_VIGetXFBDrawEnable()) == -1) {
        VIWaitForRetrace();
#ifdef TARGET_PC
        waits++;
#endif
    }

#ifdef TARGET_PC
    /* Observe blocked reservations only, without changing slot ownership,
     * callback delivery, retrace cadence or the wait itself. Bound log volume. */
    if (trace_enabled && waits != 0 && trace_records < 1024) {
        const OSTime elapsed = OSGetTime() - start;
        OSReport("NATIVE_XFB_SLOT_WAIT record=%u elapsed_us=%llu waits=%u "
                 "retraces=%u buffers=%d before=%d,%d,%d after=%d,%d,%d slot=%d\n",
                 ++trace_records,
                 (unsigned long long) OSTicksToMicroseconds(elapsed), waits,
                 (u32) (VIGetRetraceCount() - initial_retrace), _p->nb_xfb,
                 initial_status[0], initial_status[1], initial_status[2],
                 _p->xfb[0].status, _p->xfb[1].status, _p->xfb[2].status, idx);
    }
#endif

ret:
    return idx;
}

static void HSD_VICopyEFB2XFBHiResoAA(GXRenderModeObj* rmode)
{
    int n_xfb_lines;

    GXSetDispCopySrc(0, 0, rmode->fbWidth,
                     rmode->efbHeight - HSD_ANTIALIAS_OVERLAP);
    n_xfb_lines = GXSetDispCopyYScale(1.0);
    GXSetDispCopyDst(rmode->fbWidth, n_xfb_lines);
}

void HSD_VICopyEFB2XFBPtr(HSD_VIStatus* vi, void* buffer, HSD_RenderPass rpass)
{
    GXRenderModeObj* rmode = &vi->rmode;
    int n_xfb_lines;
    u16 lines;
    u32 offset;

    GXSetCopyFilter(rmode->aa, rmode->sample_pattern, vi->vf, rmode->vfilter);
    GXSetDispCopyGamma(vi->gamma);

    HSD_StateSetColorUpdate(vi->update_clr);
    HSD_StateSetAlphaUpdate(vi->update_alpha);
    HSD_StateSetZMode(vi->update_z, GX_LEQUAL, GX_TRUE);

    GXSetCopyClear(vi->clear_clr, vi->clear_z);

    switch (rpass) {
    case HSD_RP_SCREEN:
        GXSetCopyClamp((GXFBClamp) (GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
        GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
        n_xfb_lines = GXSetDispCopyYScale((f32) (rmode->xfbHeight) /
                                          (f32) (rmode->efbHeight));
        GXSetDispCopyDst(rmode->fbWidth, n_xfb_lines);
        GXCopyDisp(buffer, GX_TRUE);
        break;

    case HSD_RP_TOPHALF:
        HSD_VICopyEFB2XFBHiResoAA(rmode);
        GXSetCopyClamp(GX_CLAMP_TOP);
        lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
        GXSetDispCopySrc(0, 0, rmode->fbWidth, lines);
        GXCopyDisp(buffer, GX_TRUE);
        GXPixModeSync();
        return;

    case HSD_RP_BOTTOMHALF:
        HSD_VICopyEFB2XFBHiResoAA(rmode);
        GXSetCopyClamp(GX_CLAMP_BOTTOM);
        lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
        GXSetDispCopySrc(0, HSD_ANTIALIAS_OVERLAP, rmode->fbWidth, lines);
        offset = (VIPadFrameBufferWidth(rmode->fbWidth) * lines *
                  (u32) VI_DISPLAY_PIX_SZ);
        GXCopyDisp((void*) ((u32) buffer + offset), GX_TRUE);
        GXSetDispCopySrc(0, 0, rmode->fbWidth, HSD_ANTIALIAS_OVERLAP);
        GXSetCopyClamp((GXFBClamp) (GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
        GXCopyDisp((void*) garbage, GX_TRUE);
        break;

    default:
        HSD_Panic(__FILE__, 0x207, "unexpected type of render pass.\n");
    }

    GXPixModeSync();
}

void HSD_VIGXSetDrawDone(int arg)
{
    while (HSD_VIGetDrawDoneWaitingFlag()) {
        GXWaitDrawDone();
    }
    _p->drawdone.waiting = 1;
    _p->drawdone.arg = arg;
    GXSetDrawDone();
}

void HSD_VISetXFBWaitDone(int idx)
{
    bool intr;

    intr = OSDisableInterrupts();

    HSD_ASSERT(590, _p->xfb[idx].status == HSD_VI_XFB_DRAWING);

    _p->xfb[idx].status = HSD_VI_XFB_WAITDONE;
    _p->xfb[idx].vi_all = _p->current;
    _p->current.chg_flag = 0;

    OSRestoreInterrupts(intr);
}

void HSD_VICopyXFBAsync(HSD_RenderPass rpass)
{
    int idx;

    if (HSD_VIGetNbXFB() < 2) {
        return;
    }

    idx = HSD_VIWaitXFBDrawEnable();
#ifdef TARGET_PC
    native_trace_xfb_event("copy_begin", idx, 1);
#endif
    HSD_VICopyEFB2XFBPtr(HSD_VIGetVIStatus(), HSD_VIGetXFBPtr(idx), rpass);
    HSD_VISetXFBWaitDone(idx);

    HSD_VIGXSetDrawDone(idx);
#ifdef TARGET_PC
    native_trace_xfb_event("queued", idx, 0);
#endif
}

void HSD_VIDrawDoneXFB(int idx)
{
    bool intr;

    intr = OSDisableInterrupts();

    HSD_ASSERT(722, _p->xfb[idx].status == HSD_VI_XFB_WAITDONE);
#ifdef TARGET_PC
    native_trace_xfb_event("draw_done", idx, 0);
#endif

    _p->xfb[idx].status = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1
                              ? HSD_VI_XFB_DRAWDONE
                              : HSD_VI_XFB_NEXT;

    OSRestoreInterrupts(intr);
}

static int HSD_VIWaitXFBFlush_sub(void)
{
    bool intr;
    int val;

    intr = OSDisableInterrupts();

    val = (HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE) != -1 ||
           HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE) != -1 ||
           HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1)
              ? 1
              : 0;

    OSRestoreInterrupts(intr);

    return val;
}

void HSD_VIWaitXFBFlush(void)
{
    if (HSD_VIGetNbXFB() < 2) {
        return;
    }

    while (HSD_VIWaitXFBFlush_sub()) {
        VIWaitForRetrace();
    }
}

void HSD_VIWaitXFBFlushNoYield(void)
{
    if (HSD_VIGetNbXFB() < 2) {
        return;
    }

    while (HSD_VIWaitXFBFlush_sub()) {
#ifdef TARGET_PC
        /* Native completions are delivered on this owner thread. Busy-spinning
         * would prevent the very callbacks this wait requires. */
        VIWaitForRetrace();
#endif
    }
}

int HSD_VIGetXFBLastDrawDone(void)
{
    bool intr;
    int idx = -1;

    intr = OSDisableInterrupts();

    if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE)) == -1) {
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) == -1) {
            if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) == -1) {
                idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY);
            }
        }
    }

    OSRestoreInterrupts(intr);

    return idx;
}

void HSD_VISetConfigure(GXRenderModeObj* rmode)
{
    _p->current.vi.rmode = *rmode;
    _p->current.chg_flag = 1;
}

void HSD_VISetBlack(bool black)
{
    _p->current.vi.black = black;
    _p->current.chg_flag = 1;
}

void HSD_VIInit(HSD_VIStatus* vi, void* xfb0, void* xfb1, void* xfb2)
{
    int i, fbnum, idx;

#ifdef TARGET_PC
    /* Native startup currently supports full progressive frames with rotating
     * XFBs. A displayed buffer stays pinned until successful Present handoff. */
    HSD_ASSERT(0, vi && xfb0 && xfb1 && !vi->rmode.aa &&
                  vi->rmode.efbHeight == vi->rmode.xfbHeight);
    HSD_ASSERT(0, MeleePc_ConfigureProgressiveVideo(vi));
    void* native_buffers[3] = { xfb0, xfb1, xfb2 };
    for (i = 0; i < 3; ++i) {
        if (native_buffers[i]) {
            HSD_ASSERT(0, aurora_register_xfb(native_buffers[i],
                (size_t) vi->rmode.fbWidth * vi->rmode.xfbHeight * 2,
                vi->rmode.fbWidth, vi->rmode.xfbHeight));
        }
    }
#endif
    VIInit();

    _p->current.vi = *vi;
    _p->current.chg_flag = 0;
    _p->xfb[0].buffer = xfb0;
    _p->xfb[1].buffer = xfb1;
    _p->xfb[2].buffer = xfb2;

    for (i = 0, fbnum = 0; i < HSD_VI_XFB_MAX; i++) {
        _p->xfb[i].vi_all = _p->current;
        if (_p->xfb[i].buffer) {
            fbnum++;
            _p->xfb[i].status = HSD_VI_XFB_FREE;
        } else {
            _p->xfb[i].status = HSD_VI_XFB_NONE;
        }
    }

    _p->nb_xfb = fbnum;

    _p->efb.status = HSD_VI_EFB_FREE;
    _p->efb.vi_all = _p->current;

    VISetPreRetraceCallback(HSD_VIPreRetraceCB);
    VISetPostRetraceCallback(HSD_VIPostRetraceCB);
#ifdef TARGET_PC
    HSD_ASSERT(0, aurora_set_ready_xfb_refresh(
        getenv("MELEE_DIAGNOSTIC_LATE_XFB") ? native_refresh_ready_xfb : NULL));
#endif

    _p->pre_cb = NULL;
    _p->post_cb = NULL;

    _p->drawdone.waiting = 0;
    _p->drawdone.arg = 0;

    GXSetDrawDoneCallback(HSD_VIGXDrawDoneCB);
    _p->drawdone.cb = NULL;
#ifdef TARGET_PC
    _p->drawdone.cb = HSD_VIDrawDoneXFB;
#endif

    _p->perf.frame_period = VIGetTvFormat() == VI_NTSC ? 60 : 50;
    _p->perf.frame_renew = 0;

    VIConfigure(&_p->current.vi.rmode);
#ifdef TARGET_PC
    /* Keep startup black until the initial CPU XFB copy really completes. */
    VISetBlack(1);
    _p->current.chg_flag = 1;
#else
    VISetBlack(_p->current.vi.black);
#endif
    VIFlush();

    idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
    HSD_VICopyEFB2XFBPtr(HSD_VIGetVIStatus(), HSD_VIGetXFBPtr(idx),
                         HSD_RP_SCREEN);
#ifdef TARGET_PC
    _p->xfb[idx].status = HSD_VI_XFB_DRAWING;
    HSD_VISetXFBWaitDone(idx);
    HSD_VIGXSetDrawDone(idx);
#endif
}
