#ifndef MELEE_PC_KIRBY_COPY_SWORD_PLAN_H
#define MELEE_PC_KIRBY_COPY_SWORD_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"

/* Source-defined ftKbCopyHat_Sword (Marth and Roy).  The original archive
 * profile below is pinned to PlKbCpMs.dat; no other sword-copy archive is
 * inferred from this layout. */
static void MeleePc_KirbyCopySwordPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=65976||root!=316||!title_range(p,root,20)){p->error=-2;return;}
 if(!fighter_sound_scalar(p,root+4)||title_u32(p,root+4)!=1){p->error=-2;return;}
 title_claim(p,root+4,4,4);

 uint32_t hat=title_pointer(p,root);
 uint32_t vis=title_pointer(p,root+8);
 uint32_t sword=title_pointer(p,root+12);
 uint32_t dynamics=title_pointer(p,root+16);
 if(hat==UINT32_MAX||vis==UINT32_MAX||sword==UINT32_MAX||dynamics==UINT32_MAX){p->error=-1;return;}

 title_enqueue(p,hat,TITLE_HSD_Joint);
 title_enqueue(p,sword,TITLE_HSD_Joint);
 if(!fighter_sound_span(p,vis,16)){p->error=-1;return;}
 for(unsigned variant=0;variant<4&&!p->error;variant++){
  uint32_t slot=vis+variant*4,lookup;
  if(!fighter_sound_pointer(p,slot,&lookup)){p->error=-1;return;}
  title_pointer(p,slot);
  if(lookup!=UINT32_MAX)MeleePc_FighterVisibilityLookupPlan(p,lookup,1);
 }

 /* Kirby's source-visible sword dynamics use the same ftDynamics layout as
  * fighter data, with no per-animation mapping table for this copy profile. */
 MeleePc_FighterDynamicsPlan(p,dynamics,0);

 /* Two exact-profile detached references remain outside the runtime root
  * graph. Keep their target/relocation shape pinned and convert only the
  * pointer words; the intervening zero words are endian-invariant scalars. */
 static const uint32_t detached[][2]={{65952,57544},{65968,65952}};
 for(unsigned i=0;i<sizeof(detached)/sizeof(detached[0])&&!p->error;i++){
  uint32_t at=detached[i][0],target=detached[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||!p->relocs[at/4]||
     (p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t at=65956+i*4;
  if(!title_range(p,at,4)||title_u32(p,at)!=0||p->relocs[at/4]||
     (p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
  title_claim(p,at,4,4);
 }
 for(unsigned i=0;i<sizeof(detached)/sizeof(detached[0])&&!p->error;i++)title_pointer(p,detached[i][0]);
}
#endif
