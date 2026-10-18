#ifndef MELEE_PC_ITEM_ARTICLE_ATTRIBUTES_H
#define MELEE_PC_ITEM_ARTICLE_ATTRIBUTES_H
/* ItemAttr in it/types.h: four flag/byte/padding bytes, then scalar
 * fields including float ECB/Vec2 values. This is not per-kind data. */
static void MeleePc_ItemArticleAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,0x84)){p->error=-1;return;}
 for(uint32_t at=0;at<0x84;at+=4){
  if(p->relocs[(root+at)/4] || (p->external_slots && p->external_slots[(root+at)/4])){p->error=-1;return;}
  if(at>=4 && at<=0x60 && at!=8 &&
     (title_u32(p,root+at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,4,1);title_claim(p,root+4,0x80,4);
}
#endif
