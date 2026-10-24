#ifndef MELEE_PC_COSTUME_KIRBY_COPY_ALTS_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_ALTS_PLAN_H
/* Source ftKirby costume table: ten alternate HSD joint/material-animation root pairs. */
typedef struct {uint32_t length,body,relocs,joint,matanim;const char* joint_name;const char* matanim_name;} MeleePc_KirbyAltsCostume;
static const MeleePc_KirbyAltsCostume melee_pc_kirby_alts_costumes[]={
 {181222,176656,1115,57876,108716,"PlyKirbyDkYe_Share_joint","PlyKirbyDkYe_Share_matanim_joint"},
 {181734,177168,1115,58292,109132,"PlyKirbyDkBu_Share_joint","PlyKirbyDkBu_Share_matanim_joint"},
 {181286,176720,1115,57876,108716,"PlyKirbyDkRe_Share_joint","PlyKirbyDkRe_Share_matanim_joint"},
 {181254,176688,1115,57876,108716,"PlyKirbyDkGr_Share_joint","PlyKirbyDkGr_Share_matanim_joint"},
 {181478,176912,1115,57876,108716,"PlyKirbyDkWh_Share_joint","PlyKirbyDkWh_Share_matanim_joint"},
 {186678,183792,695,50740,115508,"PlyKirbyFcYe_Share_joint","PlyKirbyFcYe_Share_matanim_joint"},
 {169746,166864,694,50740,115508,"PlyKirbyFcBu_Share_joint","PlyKirbyFcBu_Share_matanim_joint"},
 {186678,183792,695,50740,115508,"PlyKirbyFcRe_Share_joint","PlyKirbyFcRe_Share_matanim_joint"},
 {186678,183792,695,50740,115508,"PlyKirbyFcGr_Share_joint","PlyKirbyFcGr_Share_matanim_joint"},
 {186678,183792,695,50740,115508,"PlyKirbyFcWh_Share_joint","PlyKirbyFcWh_Share_matanim_joint"},
};
static int MeleePc_CostumeKirbyCopyAltsMatch(size_t length,uint32_t body,uint32_t relocs,uint32_t joint,uint32_t matanim,const char* joint_name,const char* matanim_name){
 for(unsigned i=0;i<10;i++){
  const MeleePc_KirbyAltsCostume* x=&melee_pc_kirby_alts_costumes[i];
  if(length==x->length&&body==x->body&&relocs==x->relocs&&joint==x->joint&&matanim==x->matanim&&!strcmp(joint_name,x->joint_name)&&!strcmp(matanim_name,x->matanim_name))return 1;
 }
 return 0;
}
static void MeleePc_CostumeKirbyCopyAltsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 for(unsigned i=0;i<10;i++){
  const MeleePc_KirbyAltsCostume* x=&melee_pc_kirby_alts_costumes[i];
  if(p->size==x->body&&root==x->joint){title_enqueue(p,root,TITLE_HSD_Joint);title_enqueue(p,x->matanim,TITLE_HSD_MatAnimJoint);return;}
 }
 p->error=-2;
}
#endif
