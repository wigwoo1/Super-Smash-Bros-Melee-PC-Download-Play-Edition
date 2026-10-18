#ifndef MELEE_PC_EFFECTS_NESS_PLAN_H
#define MELEE_PC_EFFECTS_NESS_PLAN_H
/* Exact original archive profile: three populated source-dispatched records.
 * The zero tail is preserved as opaque bytes; no fourth effect is invented. */
static void MeleePc_EffectsNessPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=98548||root!=0||!title_range(p,0,96)){p->error=-2;return;}
 if(title_pointer(p,0)!=UINT32_MAX||title_pointer(p,4)!=UINT32_MAX||p->error){p->error=-2;return;}
 static const unsigned types[]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned at=8;at<68;at+=20){
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
  uint32_t bits=title_u32(p,at);float lifetime;memcpy(&lifetime,&bits,4);
  if(!isfinite(lifetime)){p->error=-1;return;}
  title_claim(p,at,4,4);
  for(unsigned i=0;i<4&&!p->error;i++){
   uint32_t target=title_pointer(p,at+4+i*4);
   if(target==UINT32_MAX){p->error=-2;return;}
   title_enqueue(p,target,types[i]);
  }
 }
 for(unsigned at=68;at<96;at++)if(p->bytes[32+at]||p->relocs[at/4]||
  (p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
}
#endif
