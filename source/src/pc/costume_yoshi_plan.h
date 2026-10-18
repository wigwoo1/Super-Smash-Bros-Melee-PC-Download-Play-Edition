#ifndef MELEE_PC_COSTUME_YOSHI_PLAN_H
#define MELEE_PC_COSTUME_YOSHI_PLAN_H
/* Exact audited costume roots; caller validates all relocations before commit. */
static void MeleePc_CostumeYoshiPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 switch(p->size){
 case 287776:
  if(root!=139944){p->error=-2;return;}
  title_enqueue(p,139944,TITLE_HSD_Joint);
  title_enqueue(p,270536,TITLE_HSD_MatAnimJoint);
  break;
 case 288192:
  if(root!=139944){p->error=-2;return;}
  title_enqueue(p,139944,TITLE_HSD_Joint);
  title_enqueue(p,270952,TITLE_HSD_MatAnimJoint);
  break;
 case 287872:
  if(root!=139976){p->error=-2;return;}
  title_enqueue(p,139976,TITLE_HSD_Joint);
  title_enqueue(p,270632,TITLE_HSD_MatAnimJoint);
  break;
 case 288096:
  if(root!=139976){p->error=-2;return;}
  title_enqueue(p,139976,TITLE_HSD_Joint);
  title_enqueue(p,270856,TITLE_HSD_MatAnimJoint);
  break;
 case 288128:
  if(root!=139976){p->error=-2;return;}
  title_enqueue(p,139976,TITLE_HSD_Joint);
  title_enqueue(p,270888,TITLE_HSD_MatAnimJoint);
  break;
 default:p->error=-2;break;
 }
}
#endif
