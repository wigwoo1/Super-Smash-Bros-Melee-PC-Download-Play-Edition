#ifndef MELEE_PC_COSTUME_PURIN_PLAN_H
#define MELEE_PC_COSTUME_PURIN_PLAN_H
static void MeleePc_CostumePurinPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root!=67280){p->error=-2;return;}
 switch(p->size){
 case 241712:
  title_enqueue(p,67280,TITLE_HSD_Joint);
  title_enqueue(p,105696,TITLE_HSD_MatAnimJoint);
  break;
 case 251712:
  title_enqueue(p,67280,TITLE_HSD_Joint);
  title_enqueue(p,105504,TITLE_HSD_MatAnimJoint);
  title_enqueue(p,243432,TITLE_HSD_Joint);
  break;
 case 254720:
  title_enqueue(p,67280,TITLE_HSD_Joint);
  title_enqueue(p,105504,TITLE_HSD_MatAnimJoint);
  title_enqueue(p,253680,TITLE_HSD_Joint);
  break;
 case 252576:
  title_enqueue(p,67280,TITLE_HSD_Joint);
  title_enqueue(p,105504,TITLE_HSD_MatAnimJoint);
  title_enqueue(p,246928,TITLE_HSD_Joint);
  break;
 case 261120:
  title_enqueue(p,67280,TITLE_HSD_Joint);
  title_enqueue(p,105504,TITLE_HSD_MatAnimJoint);
  title_enqueue(p,242600,TITLE_HSD_Joint);
  break;
 default:p->error=-2;break;
 }
}
#endif
