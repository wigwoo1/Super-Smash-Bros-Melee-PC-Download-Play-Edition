#ifndef MELEE_PC_ITEM_ATTRIBUTE_DISPATCH_H
#define MELEE_PC_ITEM_ATTRIBUTE_DISPATCH_H
#include "item_scalar_attributes.h"
#include "item_mixed_attributes.h"
#include "item_mball_attributes.h"
#include "item_egg_bomb_attributes.h"
#include "item_food_attributes.h"
#include "item_mushroom_attributes.h"
#include "item_wstar_attributes.h"
#include "item_unown_attributes.h"
#include "item_monster_attributes.h"
#include "item_tier_attributes.h"
#include "item_heiho_attributes.h"
#include "item_whispy_attributes.h"
#include "stage_corneria_laser_attributes.h"
#include "item_link_attributes.h"
#include "item_pikachu_attributes.h"
#include "item_ice_climbers_attributes.h"
#include "item_sheik_chain_attributes.h"
#include "item_kinoko_route_attributes.h"
/* This dispatch covers source-required fields only, not full allocation extents.
 * Unknown payloads remain unsupported pending archive-wide preservation policy. */
static void MeleePc_ItemAttributePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 switch(kind){
 case 211:case 212:case 219:case 220:MeleePc_ItemKinokoRouteAttributesPlan(p,root,kind);break;
 case 34:MeleePc_ItemMBallAttributesPlan(p,root);break;
 case 97:MeleePc_ItemSheikChainAttributesPlan(p,root);break;
 case It_Kind_IceClimber_Ice:case It_Kind_IceClimber_Blizzard:case It_Kind_IceClimber_GumStrings:
  MeleePc_ItemIceClimbersAttributesPlan(p,root,kind);break;
 case 225:case 226:MeleePc_ItemWhispyAttributesPlan(p,root);break;
 case 81:case 89:MeleePc_ItemPikachuAttributesPlan(p,root,kind);break;
 case 60:case 62:case 64:MeleePc_ItemLinkAttributesPlan(p,root,kind);break;
 case 234:case 235:MeleePc_StageCorneriaLaserAttributesPlan(p,root,kind);break;
 case 86:case 88:case 110:case 112:case 93:case 94:case 95:case 48:case 49:case 50:case 52:case 54:case 55:case 56:case 57:case 58:case 100:case 105:case 132:MeleePc_ItemScalarAttributesPlan(p,root,kind);break;
 case 0:case 1:case 2:case 10:case 11:case 16:case 17:case 20:case 21:case 23:case 24:case 25:case 35:case 39:MeleePc_ItemScalarAttributesPlan(p,root,kind);break;
 case 3:case 6:MeleePc_ItemEggBombAttributesPlan(p,root,kind);break;
 case 18:MeleePc_ItemFoodAttributesPlan(p,root);break;
 case 26:case 27:MeleePc_ItemMushroomAttributesPlan(p,root);break;
 case 29:MeleePc_ItemWstarAttributesPlan(p,root);break;
 case 172:case 199:MeleePc_ItemUnownAttributesPlan(p,root,kind);break;
 case 43:case 44:case 45:case 46:MeleePc_ItemMonsterAttributesPlan(p,root,kind);break;
 case 210:MeleePc_ItemHeihoAttributesPlan(p,root);break;
 case 158:MeleePc_ItemTierAttributesPlan(p,root);break;
 default:MeleePc_ItemMixedAttributesPlan(p,root,kind);break;
 }
}
#endif

