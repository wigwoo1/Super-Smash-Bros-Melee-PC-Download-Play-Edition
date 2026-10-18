#ifndef MELEE_PC_EFFECTS_KIRBY_DONKEY_COPY_PLAN_H
#define MELEE_PC_EFFECTS_KIRBY_DONKEY_COPY_PLAN_H
/* Source-required Donkey-copy model descriptors 39000..39001; total table extent not inferred. */
static void MeleePc_EffectsKirbyDonkeyCopyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=31868 || root!=0 || !title_range(p,0,48)){p->error=-2;return;}
 for(unsigned at=0;at<8;at++)if(p->bytes[32+at] || p->relocs[at/4] || (p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 title_claim(p,0,8,4);
 static const unsigned types[]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned at=8;at<48;at+=20){
  if(p->relocs[at/4] || (p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  uint32_t bits=title_u32(p,at);float lifetime;memcpy(&lifetime,&bits,4);
  if(!isfinite(lifetime)){p->error=-1;return;}
  title_claim(p,at,4,4);
  for(unsigned i=0;i<4&&!p->error;i++){
   uint32_t target=title_pointer(p,at+4+i*4);
   if(target!=UINT32_MAX)title_enqueue(p,target,types[i]);
  }
 }
}
#endif
