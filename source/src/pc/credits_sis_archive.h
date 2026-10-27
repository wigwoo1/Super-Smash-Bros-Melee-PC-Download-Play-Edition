#ifndef MELEE_PC_CREDITS_SIS_ARCHIVE_H
#define MELEE_PC_CREDITS_SIS_ARCHIVE_H
/* Exact original SIS_StRollData: atlas/byte metrics and3054 encoded streams.
 * hsd_3A76 consumes BE glyph words explicitly; only table pointers become
 * native scalars. The original streams use end/newline/font-prefix commands. */
static void MeleePc_CreditsSisPlan(MeleePc_TitlePlan* p,uint32_t root)
{
    const uint32_t entries=3056,metrics=76521,atlas=77056,glyphs=263;
    if(root || p->size!=211712 || title_u32(p,0)!=atlas ||
       title_u32(p,4)!=metrics || title_u32(p,8)!=entries*4){p->error=-2;return;}
    for(uint32_t i=0;i<entries&&!p->error;++i){
        if(p->relocs[i]!=1 || (p->external_slots&&p->external_slots[i]) ||
           title_pointer(p,i*4)==UINT32_MAX){p->error=-2;return;}
    }
    for(uint32_t i=2;i<entries&&!p->error;++i){
        uint32_t at=title_u32(p,i*4),end=i+1<entries?title_u32(p,(i+1)*4):metrics;
        int ended=0;
        if(at<entries*4 || at>=end || end>metrics){p->error=-2;return;}
        while(at<end&&!p->error){
            unsigned op=p->bytes[32+at],size=1;
            if(op>=32){
                if(end-at<2){p->error=-1;return;}
                unsigned glyph=title_u16(p,at);size=2;
                if(!((glyph>=0x2000&&glyph<0x2120) ||
                     (glyph>=0x4000&&glyph<0x4000+glyphs))){p->error=-2;return;}
            }else if(op!=0 && op!=3 && op!=26){p->error=-2;return;}
            stadium_sis_bytes(p,at,size);at+=size;
            if(!op){ended=1;break;}
        }
        if(!ended || at!=end){p->error=-1;return;}
    }
    for(uint32_t at=metrics+glyphs*2;at<atlas&&!p->error;++at)
        if(p->bytes[32+at])p->error=-2;
    if(!p->error)stadium_sis_bytes(p,metrics,p->size-metrics);
}
#endif
