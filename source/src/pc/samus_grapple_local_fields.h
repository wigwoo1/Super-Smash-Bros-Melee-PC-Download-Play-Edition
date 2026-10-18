#ifndef MELEE_PC_SAMUS_GRAPPLE_LOCAL_FIELDS_H
#define MELEE_PC_SAMUS_GRAPPLE_LOCAL_FIELDS_H
#include "fighter_sound_tables.h"
/* Exact audited local grapple references and list terminators. External fields excluded; caller handles validated import chains. */
static void MeleePc_SamusGrappleLocalFieldsPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={17012,17016,17020,17024,17028,17032,17040,17044,17052,17056,17064,17076,88164,88168,97208,97212,99796,99800,100060,100064,104428,104432,104692,104696,105420,105424,105684,105688};
 const uint32_t expected[]={105160,104168,97672,71496,105420,105684,104428,104692,99796,100060,88164,97208,88184,UINT32_MAX,97228,UINT32_MAX,99816,UINT32_MAX,100072,UINT32_MAX,104448,UINT32_MAX,104704,UINT32_MAX,105440,UINT32_MAX,105696,UINT32_MAX};
 if(p->error)return;
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++){uint32_t target;if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){p->error=-1;return;}}
 for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)title_pointer(p,slots[i]);
}
#endif
