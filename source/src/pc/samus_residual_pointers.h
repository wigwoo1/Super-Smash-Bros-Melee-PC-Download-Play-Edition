#ifndef MELEE_PC_SAMUS_RESIDUAL_POINTERS_H
#define MELEE_PC_SAMUS_RESIDUAL_POINTERS_H
#include "fighter_sound_tables.h"
/* Original relocation provenance only; no enclosing scalar/descriptor type or runtime ownership claim. */
static void MeleePc_SamusResidualPointersPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={39380,39384,39388,39392,39396,39400,39408,39412,39420,39424,39432,39444,88204,88212,88216,97248,97256,97260,103728,103736,103744,103748,103752,104716,104724,104732,104736,104740,105708,105716,105724,105728,105732,105740,105744,105748,105752,105756};
 const uint32_t expected[]={105160,104168,97672,71496,105420,105684,104428,104692,99796,100060,88164,97208,88164,71496,88204,97208,88776,97248,99796,100060,97672,103728,103736,104428,104692,104168,104716,104724,105420,105684,105160,105708,105716,88212,97256,103744,104732,105724};
 if(p->error)return;
 if(p->size!=303808){p->error=-2;return;}
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++){uint32_t target;
  if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){p->error=-1;return;}
 }
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)title_pointer(p,slots[i]);
}
#endif
