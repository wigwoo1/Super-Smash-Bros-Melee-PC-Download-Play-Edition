#ifndef MELEE_PC_COSTUME_KIRBY_COPY_MEWTWO_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_MEWTWO_PLAN_H
/* Source ftKirby costume table: six HSD joint/material-animation root pairs. */
typedef struct {uint32_t length,body,relocs,joint,matanim;const char* joint_name;const char* matanim_name;} MeleePc_KirbyMewtwoCostume;
static const MeleePc_KirbyMewtwoCostume melee_pc_kirby_mewtwo_costumes[]={
 {171230,166032,1274,66484,97696,"PlyKirbyMt_Share_joint","PlyKirbyMt_Share_matanim_joint"},
 {170382,165296,1245,66068,97280,"PlyKirbyMtYe_Share_joint","PlyKirbyMtYe_Share_matanim_joint"},
 {170478,165392,1245,66068,97280,"PlyKirbyMtBu_Share_joint","PlyKirbyMtBu_Share_matanim_joint"},
 {170446,165360,1245,66068,97280,"PlyKirbyMtRe_Share_joint","PlyKirbyMtRe_Share_matanim_joint"},
 {170414,165328,1245,66068,97280,"PlyKirbyMtGr_Share_joint","PlyKirbyMtGr_Share_matanim_joint"},
 {170638,165552,1245,66068,97280,"PlyKirbyMtWh_Share_joint","PlyKirbyMtWh_Share_matanim_joint"},
};
static int MeleePc_CostumeKirbyCopyMewtwoMatch(size_t length,uint32_t body,uint32_t relocs,uint32_t joint,uint32_t matanim,const char* joint_name,const char* matanim_name){
 for(unsigned i=0;i<6;i++){
  const MeleePc_KirbyMewtwoCostume* x=&melee_pc_kirby_mewtwo_costumes[i];
  if(length==x->length&&body==x->body&&relocs==x->relocs&&joint==x->joint&&matanim==x->matanim&&!strcmp(joint_name,x->joint_name)&&!strcmp(matanim_name,x->matanim_name))return 1;
 }
 return 0;
}
static void MeleePc_CostumeKirbyCopyMewtwoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 for(unsigned i=0;i<6;i++){
  const MeleePc_KirbyMewtwoCostume* x=&melee_pc_kirby_mewtwo_costumes[i];
  if(p->size==x->body&&root==x->joint){title_enqueue(p,root,TITLE_HSD_Joint);title_enqueue(p,x->matanim,TITLE_HSD_MatAnimJoint);return;}
 }
 p->error=-2;
}
#endif
