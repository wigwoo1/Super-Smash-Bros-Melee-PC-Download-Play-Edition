#ifndef GALE01_258940
#define GALE01_258940

#include <Runtime/platform.h>

#include <sysdolphin/baselib/forward.h>

#include <sysdolphin/baselib/gobj.h>

/* 258940 */ void mnGallery_80258940(void);
/* 25896C */ void mnGallery_8025896C(HSD_GObj* gobj, intptr_t render_pass);
/* 258A08 */ void mnGallery_80258A08(HSD_GObj* gobj, u16 width, u16 height,
                                     u32 priority);
/* 258ED0 */ void fn_80258ED0(HSD_GObj* gobj);
/* 2590C4 */ void fn_802590C4(HSD_GObj* gobj);
/* 2591BC */ void mnGallery_802591BC(HSD_GObj* gobj);
/* 25963C */ void mnGallery_8025963C(void);
/* 259868 */ void mnGallery_80259868(void);
#ifdef TARGET_PC
/* Passive diagnostic query: private gallery state (1 ready, 3 playing),
 * selection 0 Special Movie / 1 How to Play; -1 outside Archives. */
int mnGallery_DiagnosticState(int* selection);
#endif

#endif
