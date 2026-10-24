#ifndef MELEE_PC_COSTUME_KIRBY_COPY_PURIN_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_PURIN_PLAN_H
/* Source ftKirby costume table: six HSD joint/material-animation root pairs. */
typedef struct {uint32_t length,body,relocs,joint,matanim;const char* joint_name;const char* matanim_name;} MeleePc_KirbyPurinCostume;
static const MeleePc_KirbyPurinCostume melee_pc_kirby_purin_costumes[]={
 {185398,180528,1192,77332,112208,"PlyKirbyPr_Share_joint","PlyKirbyPr_Share_matanim_joint"},
 {185082,180208,1192,77332,112208,"PlyKirbyPrYe_Share_joint","PlyKirbyPrYe_Share_matanim_joint"},
 {185178,180304,1192,77332,112208,"PlyKirbyPrBu_Share_joint","PlyKirbyPrBu_Share_matanim_joint"},
 {185146,180272,1192,77332,112208,"PlyKirbyPrRe_Share_joint","PlyKirbyPrRe_Share_matanim_joint"},
 {185114,180240,1192,77332,112208,"PlyKirbyPrGr_Share_joint","PlyKirbyPrGr_Share_matanim_joint"},
 {185338,180464,1192,77332,112208,"PlyKirbyPrWh_Share_joint","PlyKirbyPrWh_Share_matanim_joint"},
};
static int MeleePc_CostumeKirbyCopyPurinMatch(size_t length,uint32_t body,uint32_t relocs,uint32_t joint,uint32_t matanim,const char* joint_name,const char* matanim_name){
 for(unsigned i=0;i<6;i++){
  const MeleePc_KirbyPurinCostume* x=&melee_pc_kirby_purin_costumes[i];
  if(length==x->length&&body==x->body&&relocs==x->relocs&&joint==x->joint&&matanim==x->matanim&&!strcmp(joint_name,x->joint_name)&&!strcmp(matanim_name,x->matanim_name))return 1;
 }
 return 0;
}
static void MeleePc_CostumeKirbyCopyPurinPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 for(unsigned i=0;i<6;i++){
  const MeleePc_KirbyPurinCostume* x=&melee_pc_kirby_purin_costumes[i];
  if(p->size==x->body&&root==x->joint){title_enqueue(p,root,TITLE_HSD_Joint);title_enqueue(p,x->matanim,TITLE_HSD_MatAnimJoint);return;}
 }
 p->error=-2;
}
#endif
