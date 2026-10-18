#ifndef MELEE_PC_ITEM_MONSTER_ATTRIBUTES_H
#define MELEE_PC_ITEM_MONSTER_ATTRIBUTES_H
#include "item_dynamics.h"
static void item_monster_short(MeleePc_TitlePlan* p,uint32_t at){
 if((at&1)||!title_range(p,at,2)||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
 title_claim(p,at,2,2);
}
static void item_monster_bytes(MeleePc_TitlePlan* p,uint32_t at,uint32_t size){
 if(!title_range(p,at,size)){p->error=-1;return;}
 for(uint32_t i=at;i<at+size;i++)if(p->relocs[i/4]||(p->external_slots&&p->external_slots[i/4])){p->error=-1;return;}
 title_claim(p,at,size,1);
}
static void MeleePc_ItemMonsterAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=kind==43?16:kind==44?32:kind==45?32:kind==46?44:0;
 if(!size){p->error=-2;return;}
 if(p->error||(root&3)||!title_range(p,root,size)){p->error=-1;return;}
 uint32_t target=item_dynamics_pointer(p,root);
 if(target==UINT32_MAX){p->error=-1;return;}
 item_dynamics_scalars(p,target,kind==45?4:8,4);
 if(kind==43)item_dynamics_scalars(p,root+4,12,0);
 if(kind==44){item_dynamics_scalars(p,root+4,20,4);for(uint32_t i=24;i<30;i+=2)item_monster_short(p,root+i);item_monster_bytes(p,root+30,2);}
 if(kind==45){item_dynamics_scalars(p,root+4,24,0);item_monster_short(p,root+28);item_monster_bytes(p,root+30,2);}
 if(kind==46){item_dynamics_scalars(p,root+4,12,0);item_monster_bytes(p,root+16,4);item_dynamics_scalars(p,root+20,20,0);item_monster_bytes(p,root+40,4);}
}
#endif
