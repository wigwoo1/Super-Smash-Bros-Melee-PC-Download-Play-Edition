#ifndef MELEE_PC_ITEM_MODEL_DESCRIPTORS_H
#define MELEE_PC_ITEM_MODEL_DESCRIPTORS_H
/* ItemModelDesc: joint pointer, two scalar words, flag byte/padding. */
static void MeleePc_ItemModelDescriptorPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,16)){p->error=-1;return;}
 for(unsigned off=4;off<16;off+=4)
  if(p->relocs[(root+off)/4] || (p->external_slots && p->external_slots[(root+off)/4])){p->error=-1;return;}
 /* Validated imports (including Belay's shared TopN joint) are claimed by
  * title_pointer without enqueuing their external-chain link as a joint.
  * Runtime import binding remains the archive loader's responsibility. */
 title_claim(p,root+4,8,4);title_claim(p,root+12,4,1);
 title_enqueue(p,title_pointer(p,root),TITLE_HSD_Joint);
}
#endif
