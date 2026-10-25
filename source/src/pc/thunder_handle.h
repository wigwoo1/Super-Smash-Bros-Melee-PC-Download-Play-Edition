#ifndef MELEE_PC_THUNDER_HANDLE_H
#define MELEE_PC_THUNDER_HANDLE_H
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
/* A retained GObj address alone does not establish native item lifetime.
 * Compare against live nodes before inspecting the retained address. */
static int MeleePc_ThunderHandleLive(HSD_GObj* expected,HSD_GObj* owner)
{
    if(!expected || !owner)return 0;
    for(HSD_GObj* node=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];node;node=node->next){
        if(node!=expected)continue;
        if(node->classifier!=HSD_GOBJ_CLASS_ITEM || !node->user_data)return 0;
        Item* item=node->user_data;
        return (item->kind==It_Kind_Pikachu_Thunder || item->kind==It_Kind_Pichu_Thunder) &&
               item->xDD4_itemVar.pikachuthunder.x0==0 &&
               item->xDD4_itemVar.pikachuthunder.x38==owner;
    }
    return 0;
}
#endif
