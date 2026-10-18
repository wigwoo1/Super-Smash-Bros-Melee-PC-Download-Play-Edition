#ifndef MELEE_PC_ITEM_KYASARIN_ATTRIBUTES_H
#define MELEE_PC_ITEM_KYASARIN_ATTRIBUTES_H
#include "item_dynamics.h"
/* itCommonItems.h: Birdo has a pointer, 15 floats and three s32 words;
 * Egg has a pointer, two floats and two s32 words. Pointee extent/type
 * is deliberately not inferred from neighboring allocation addresses.
 * Original kind callbacks never read x0; generic getters are guarded for
 * Rabbit Hood, Metal Box and Star. Keep target bytes opaque, marking only
 * its validated address byte, as with source-unused Whispy attribute data.
 * This schema remains staged until archive-wide independent validation. */
static void MeleePc_ItemKyasarinAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 if(p->error)return;
 uint32_t size=kind==233?76:kind==236?20:0;
 if(!size){p->error=-2;return;}
 if((root&3)||!title_range(p,root,size)){p->error=-1;return;}
 if(!p->relocs[root/4]){p->error=-1;return;}
 uint32_t target=item_dynamics_pointer(p,root);
 if(p->error||target==UINT32_MAX||(target&3)||!title_range(p,target,1)){p->error=-1;return;}
 title_claim(p,target,1,1);
 item_dynamics_scalars(p,root+4,kind==233?60:8,0);
 item_dynamics_scalars(p,root+(kind==233?64:12),kind==233?12:8,kind==233?12:8);
}
#endif
