#ifndef MELEE_PC_ITEM_DYNAMICS_H
#define MELEE_PC_ITEM_DYNAMICS_H
static void item_dynamics_scalars(MeleePc_TitlePlan* p,uint32_t at,uint32_t size,unsigned integer_prefix){
 if(!title_range(p,at,size) || (at&3)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){
  if(p->relocs[(at+off)/4] || (p->external_slots && p->external_slots[(at+off)/4])){p->error=-1;return;}
  if(off>=integer_prefix){uint32_t bits=title_u32(p,at+off);float value;memcpy(&value,&bits,4);if(!isfinite(value)){p->error=-1;return;}}
 }
 title_claim(p,at,size,4);
}
static uint32_t item_dynamics_pointer(MeleePc_TitlePlan* p,uint32_t at){
 if(p->external_slots && p->external_slots[at/4]){p->error=-2;return UINT32_MAX;}
 return title_pointer(p,at);
}
static void MeleePc_ItemDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,16)){p->error=-1;return;}
 item_dynamics_scalars(p,root,4,4);item_dynamics_scalars(p,root+8,4,4);
 uint32_t count=title_u32(p,root),collision_count=title_u32(p,root+8);
 uint32_t bones=item_dynamics_pointer(p,root+4),coll=item_dynamics_pointer(p,root+12);
 if(count>p->size/24 || collision_count>2 || (count && bones==UINT32_MAX) ||
    (collision_count && coll==UINT32_MAX)){p->error=-1;return;}
 if(count && ((bones&3) || !title_range(p,bones,count*24))){p->error=-1;return;}
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=bones+i*24;
  item_dynamics_scalars(p,at,4,4);item_dynamics_scalars(p,at+8,16,4);
  uint32_t params=item_dynamics_pointer(p,at+4),n=title_u32(p,at+8);
  if(n>p->size/60 || (n && params==UINT32_MAX)){p->error=-1;return;}
  if(n)item_dynamics_scalars(p,params,n*60,0);
 }
 if(collision_count && ((coll&3) || !title_range(p,coll,collision_count*20))){p->error=-1;return;}
 for(uint32_t i=0;i<collision_count && !p->error;i++)item_dynamics_scalars(p,coll+i*20,20,4);
}
#endif
