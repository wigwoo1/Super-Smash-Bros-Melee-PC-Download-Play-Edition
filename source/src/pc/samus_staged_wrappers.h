#ifndef MELEE_PC_SAMUS_STAGED_WRAPPERS_H
#define MELEE_PC_SAMUS_STAGED_WRAPPERS_H
#include "fighter_sound_tables.h"
/* Audited candidate fields only. No production registration or ownership claim. */
static void MeleePc_SamusStagedWrappersPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={65640,65644,65648,65652,65656,65660,65664,65668,65672,65676,65680,65684,70908,70912,70916,70920,70924,70928,70932,70936,70940,70944,70948,70952,232488,232492,232496,232500,232504,232508,232512,232516,232520,232524,232528,232532};
 const uint32_t expected[]={65164,UINT32_MAX,65560,UINT32_MAX,65616,UINT32_MAX,56040,65640,65648,65656,65664,UINT32_MAX,70636,UINT32_MAX,70780,UINT32_MAX,70860,UINT32_MAX,68648,70908,70916,70924,70932,UINT32_MAX,232064,UINT32_MAX,232272,UINT32_MAX,232392,UINT32_MAX,220680,232488,232496,232504,232512,UINT32_MAX};
 if(p->error)return;
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++){uint32_t target;if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){p->error=-1;return;}}
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)title_pointer(p,slots[i]);
}
#endif
