#ifndef MELEE_PC_ITEM_ARTICLE_PLAN_H
#define MELEE_PC_ITEM_ARTICLE_PLAN_H
#include "item_attribute_dispatch.h"
#include "item_article_attributes.h"
#include "item_hurtbones.h"
#include "item_model_descriptors.h"
#include "item_state_animations.h"
#include "item_command_plan.h"
/* Caller derives state_count from the original state table, never next-reference
 * distances. Unknown special payloads remain untouched and unclaimed. */
static void MeleePc_ItemArticlePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind,uint32_t state_count){
 if(p->error||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t targets[6];
 for(unsigned i=0;i<6 && !p->error;i++)targets[i]=item_dynamics_pointer(p,root+i*4);
 if(p->error)return;
 if(targets[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,targets[0]);
 /* Bow code consumes common ItemAttr and its state/model descriptors, but
  * has no special-attribute reader. Preserve that opaque allocation. */
 /* Klaptrap callbacks/enemy helpers do not read special attributes; generic
  * fighter getters are restricted to Rabbit Hood/Metal Box callers. */
 /* Air TJolt callbacks/core never consume special attributes; source getter
  * callers are guarded for Rabbit Hood, Metal Box and Star. Preserve opaque
  * bytes under the source-pinned air TJolt audit. */
 /* Shared Fox/Falco blaster callbacks consume no special attributes; preserve its opaque
  * allocation, as with Bow. Generic getters are scoped to wearable/star items. */
 /* Source-audited shared Mario cape/Dr. Mario sheet callbacks do not consume special attributes. */
 /* Held needle callbacks consume common attrs/model and owner state; its
  * special allocation has no reader and remains opaque. */
 if(targets[1]!=UINT32_MAX && kind!=80 && kind!=83 && kind!=84 && kind!=30 && kind!=33 && kind!=74 && kind!=75 && kind!=76 && kind!=90 && kind!=207 && kind!=218)MeleePc_ItemAttributePlan(p,targets[1],kind);
 if(targets[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,targets[2]);
 if(targets[3]==UINT32_MAX && state_count){p->error=-1;return;}
 if(targets[3]!=UINT32_MAX){
  MeleePc_ItemStateAnimationsPlan(p,targets[3],state_count);
  if(p->error)return;
  for(uint32_t i=0;i<state_count && !p->error;i++){
   uint32_t command=title_pointer(p,targets[3]+i*16+12);
   MeleePc_ItemCommandPlan(p,&command,1);
  }
 }
 if(targets[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,targets[4]);
 if(targets[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,targets[5]);
}
#endif
