#ifndef MELEE_PC_ITEM_HEIHO_ATTRIBUTES_H
#define MELEE_PC_ITEM_HEIHO_ATTRIBUTES_H
#include "item_dynamics.h"
/* Source-consumed Heiho prefix: health pointer, speed[3], opaque word,
 * scatter speed. Offset16 has no proven consumer and stays unclaimed. */
static void MeleePc_ItemHeihoAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,24)){p->error=-1;return;}
 if(!p->relocs[root/4] || (p->external_slots && p->external_slots[root/4])){p->error=-1;return;}
 uint32_t health=title_pointer(p,root);
 if(p->error || health==UINT32_MAX || (health&3) || !title_range(p,health,4)){p->error=-1;return;}
 if(health<root+24 && root<health+4){p->error=-1;return;}
 item_dynamics_scalars(p,health,4,4);
 item_dynamics_scalars(p,root+4,12,0);
 item_dynamics_scalars(p,root+20,4,0);
 /* An untyped opaque word cannot hide a pointer slot. */
 if(p->relocs[(root+16)/4] || (p->external_slots && p->external_slots[(root+16)/4]))p->error=-2;
}
#endif
