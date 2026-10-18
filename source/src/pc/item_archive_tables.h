#ifndef MELEE_PC_ITEM_ARCHIVE_TABLES_H
#define MELEE_PC_ITEM_ARCHIVE_TABLES_H
#include "item_common_parameters.h"
#include "item_dynamics.h"
#include "item_color_commands.h"
/* itPublicData root and original enum-sized Article pointer tables.
 * Original state command indices and item.c's direct request consume the
 * first seven color descriptors. This is a required prefix, not a claim
 * about the full allocation size; see the required-indices audit. */
static void MeleePc_ItemArchiveTablesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t targets[6];
 for(unsigned i=0;i<6 && !p->error;i++){
  targets[i]=item_dynamics_pointer(p,root+i*4);
  if(targets[i]==UINT32_MAX || (targets[i]&3) || !title_range(p,targets[i],4)){p->error=-1;return;}
 }
 if(p->error)return;
 MeleePc_ItemCommonParametersPlan(p,targets[0]);
 MeleePc_ItemSecondaryParametersPlan(p,targets[4]);
 MeleePc_ItemColorCommandsPlan(p,targets[5],7);
 const uint32_t counts[]={43,118,47};
 for(unsigned i=0;i<3 && !p->error;i++){
  uint32_t table=targets[i+1];
  if((table&3)||!title_range(p,table,counts[i]*4)){p->error=-1;return;}
  for(uint32_t j=0;j<counts[i] && !p->error;j++){
   uint32_t article=item_dynamics_pointer(p,table+j*4);
   if(article!=UINT32_MAX && ((article&3)||!title_range(p,article,24))){p->error=-1;return;}
  }
 }
}
#endif
