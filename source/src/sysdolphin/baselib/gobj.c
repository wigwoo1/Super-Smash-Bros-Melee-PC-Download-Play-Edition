#include "gobj.h"

#include "fog.h"
#include "gobjplink.h"
#include "gobjproc.h"
#include "lobj.h"
#ifdef TARGET_PC
#include <dolphin/os.h>
extern char* getenv(const char*);
static int pc_camera_profile = -1;
static struct {
    void (*callback)(HSD_GObj*, int);
    u64 ticks;
    unsigned calls;
} pc_camera_times[64];
static OSTime pc_camera_profile_begin(void)
{
    if (pc_camera_profile < 0) {
        const char* value = getenv("MELEE_DIAGNOSTIC_CAMERA_PROFILE");
        pc_camera_profile = value != NULL && value[0] == '1';
    }
    return pc_camera_profile ? OSGetTime() : 0;
}
static void pc_camera_profile_end(void (*callback)(HSD_GObj*, int), OSTime start)
{
    unsigned i;
    OSTime elapsed;
    if (!pc_camera_profile) return;
    elapsed = OSGetTime() - start;
    for (i = 0; i < 64; i++) {
        if (pc_camera_times[i].callback == NULL ||
            pc_camera_times[i].callback == callback) {
            pc_camera_times[i].callback = callback;
            pc_camera_times[i].ticks += elapsed;
            if (++pc_camera_times[i].calls == 120) {
                OSReport("NATIVE_CAMERA_PROFILE callback=%p calls=120 total_us=%llu\n",
                         (void*)callback, (unsigned long long)
                         OSTicksToMicroseconds(pc_camera_times[i].ticks));
                pc_camera_times[i].calls = 0;
                pc_camera_times[i].ticks = 0;
            }
            return;
        }
    }
}
static unsigned pc_render_trace_count;
static int pc_render_trace_enabled;
void MeleePc_SetRenderCallbackTrace(int enabled){pc_render_trace_enabled=enabled;pc_render_trace_count=0;}
static void pc_render_trace(const char* phase,HSD_GObj* cur,int pass){
 if(pc_render_trace_enabled && pc_render_trace_count++<1024)
  OSReport("NATIVE_RENDER_CALLBACK %s gobj=%p callback=%p gx=%u pass=%d\n",phase,(void*)cur,(void*)cur->render_cb,(unsigned)cur->gx_link,pass);
}
#else
#define pc_render_trace(phase,cur,pass) ((void)0)
#endif

HSD_GObjProc** HSD_GObj_ProcList;
HSD_GObjProc** HSD_GObj_GObjProcHead;
s32 HSD_GObj_804D783C;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
s32 HSD_GObj_CurrentInvokedSLink;
HSD_GObjProc* HSD_GObj_NextInvokedProc;
HSD_GObj** HSD_GObjPLinkHead;
HSD_GObj** plinklow_gobjs;
HSD_GObj** HSD_GObjGXLinkHead;
HSD_GObj** HSD_GObj_804D7820;
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
HSD_GObj* HSD_GObj_804D7818;
HSD_GObj* HSD_GObj_804D7814;
GObjFunc* HSD_GObj_804D7810;

int HSD_GObj_804085F0[] = { 1, 4, 2, 0 };

static inline void GObj_SetFlag1_inline(HSD_GObjProc* proc, u8 value)
{
    while (proc != NULL) {
        proc->flags_1 = value;
        proc = proc->child;
    }
}

static inline void GObj_SetFlag2_inline(HSD_GObjProc* proc, u8 value)
{
    while (proc != NULL) {
        proc->flags_2 = value;
        proc = proc->child;
    }
}

void HSD_GObj_80390C5C(HSD_GObj* gobj)
{
    GObj_SetFlag1_inline(gobj->proc, 1);
}

void HSD_GObj_80390C84(HSD_GObj* gobj)
{
    GObj_SetFlag1_inline(gobj->proc, 0);
}

void HSD_GObj_80390CAC(HSD_GObj* gobj)
{
    GObj_SetFlag2_inline(gobj->proc, 0);
}

void HSD_GObj_80390CD4(HSD_GObj* gobj)
{
    HSD_GObjProc* p = gobj->proc;

    while (p != NULL) {
        p->flags_3 = HSD_GObj_804D783C;
        p = p->child;
    }
}

/// GObj_RunProcs
void HSD_GObj_RunProcs(void)
{
    s32 i;
    HSD_GObjProc* proc;
    HSD_GObj* gobj;

    u64 var_r31 =
        HSD_GObjLibInitData.unk_2 != NULL ? *HSD_GObjLibInitData.unk_2 : 0;
    HSD_GObj_804D783C += 1;
    if (HSD_GObj_804D783C > 2) {
        HSD_GObj_804D783C = 0;
    }

    for (i = 0; i <= HSD_GObjLibInitData.gproc_pri_max; i++) {
        HSD_GObj_CurrentInvokedSLink = i;
        proc = HSD_GObj_GObjProcHead[i];
        while (proc != NULL) {
            HSD_GObj_NextInvokedProc = proc->next;
            if (proc->flags_3 != HSD_GObj_804D783C) {
                proc->flags_3 = HSD_GObj_804D783C;
                gobj = proc->gobj;
                if (!(var_r31 & (1LL << gobj->p_link)) && !(proc->flags_1) &&
                    !(proc->flags_2))
                {
                    HSD_GObj_CurrentInvokedProcGObj = gobj;
                    HSD_GObj_CurrentInvokedProc = proc;
                    proc->on_invoke(proc->gobj);
                    HSD_GObj_NextInvokedProc = proc->next;
                    if (HSD_GObj_DelayedProcInfo.x0.flags != 0) {
                        HSD_GObj_DelayedProcInfo.x0.x0.in_delayed_proc = 1;
                        if (HSD_GObj_DelayedProcInfo.x0.x0.delay_remove_gobj) {
                            HSD_GObjFree(proc->gobj);
                        } else {
                            if (HSD_GObj_DelayedProcInfo.x0.x0
                                    .delay_change_gobj_pri)
                            {
                                HSD_GObjPLink_ChangeGObjPri_Unk(
                                    HSD_GObj_DelayedProcInfo.type, proc->gobj,
                                    HSD_GObj_DelayedProcInfo.p_link,
                                    HSD_GObj_DelayedProcInfo.p_prio,
                                    HSD_GObj_DelayedProcInfo.gobj);
                            }
                            if (HSD_GObj_DelayedProcInfo.x0.x0
                                    .delay_remove_proc)
                            {
                                HSD_GObjProc_RemoveProc(proc);
                            }
                        }
                        HSD_GObj_DelayedProcInfo.x0.flags = 0;
                    }
                    HSD_GObj_CurrentInvokedProcGObj = NULL;
                    HSD_GObj_CurrentInvokedProc = NULL;
                }
            }
            proc = HSD_GObj_NextInvokedProc;
        }
    }
}

/// GObj_GetFlagFromArray
u32 HSD_GObj_80390EB8(s32 i)
{
    return HSD_GObj_804085F0[i];
}

static inline void render_gobj(HSD_GObj* cur, int i)
{
    HSD_GObj* saved = HSD_GObj_804D7814;
    HSD_GObj_804D7814 = cur;
    pc_render_trace("nested_begin",cur,i);
    cur->render_cb(cur, i);
    pc_render_trace("nested_end",cur,i);
    HSD_GObj_804D7814 = saved;
}

/// GObj_SetTextureCamera
void HSD_GObj_80390ED0(HSD_GObj* gobj, u32 mask)
{
    s32 i = 0;
    while (mask) {
        if (mask & 1) {
            u64 prios = gobj->gxlink_prios;
            s32 j = 0;
            while (prios) {
                if (prios & 1) {
                    HSD_GObj* cur;
                    for (cur = HSD_GObjGXLinkHead[j]; cur != NULL;
                         cur = cur->next_gx)
                    {
                        if (cur->render_cb != NULL) {
                            render_gobj(cur, i);
                        }
                    }
                }
                j++;
                prios >>= 1;
            }
        }
        i++;
        mask >>= 1;
    }
}

/// GObj_RunGXLinkMaxCallbacks
void HSD_GObj_80390FC0(void)
{
    HSD_GObj* saved;
    HSD_GObj* cur = HSD_GObjGXLinkHead[HSD_GObjLibInitData.gx_link_max + 1];
    while (cur != NULL) {
        if (cur->render_cb != NULL) {
            saved = HSD_GObj_804D7818;
            HSD_GObj_804D7818 = cur;
            pc_render_trace("camera_begin",cur,0);
#ifdef TARGET_PC
            void (*profile_callback)(HSD_GObj*, int) = cur->render_cb;
            OSTime profile_start = pc_camera_profile_begin();
#endif
            cur->render_cb(cur, 0);
#ifdef TARGET_PC
            pc_camera_profile_end(profile_callback, profile_start);
#endif
            pc_render_trace("camera_end",cur,0);
            HSD_GObj_804D7818 = saved;
        }
        cur = cur->next_gx;
    }
}

struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
HSD_ObjAllocData gobjproc_alloc_data;
HSD_ObjAllocData gobj_alloc_data;
HSD_GObjLibInitDataType HSD_GObjLibInitData;
