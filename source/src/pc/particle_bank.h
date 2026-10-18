#ifndef MELEE_PC_PARTICLE_BANK_H
#define MELEE_PC_PARTICLE_BANK_H
/* Uses the typed archive claim planner. Embedded offsets remain relative until
 * the original psInitDataBankLocate runs; packed commands/textures stay BE. */
static uint32_t particle_relative(MeleePc_TitlePlan* p,uint32_t base,uint32_t slot){
    if(!title_claim(p,slot,4,4))return UINT32_MAX;
    if(p->relocs[slot/4]){p->error=-1;return UINT32_MAX;}
    uint32_t relative=title_u32(p,slot);
    if(!relative)return UINT32_MAX;
    if(relative>p->size-base){
#ifdef MELEE_PC_TITLE_TRACE
        fprintf(stderr,"PARTICLE_RELATIVE_RANGE base=%u slot=%u value=0x%08x size=%u\n",base,slot,relative,p->size);
#endif
        p->error=-1;return UINT32_MAX;}
    return base+relative;
}
/* One exact retail effect profile has a palette pointer into shared stock
 * effect data rather than a bank-relative offset. Accept only that exact
 * unrelocated word, keeping it NULL until the checked runtime alias binds it. */
static uint32_t particle_relative_external_palette(MeleePc_TitlePlan* p,uint32_t base,uint32_t slot,uint32_t external_slot){
    if(slot==external_slot){
        if(!title_claim(p,slot,4,4))return UINT32_MAX;
        if(p->relocs[slot/4] || base!=1120 || title_u32(p,slot)!=0x80a8812aU){p->error=-1;return UINT32_MAX;}
        return UINT32_MAX;
    }
    return particle_relative(p,base,slot);
}
static int particle_texture_size(uint32_t w,uint32_t h,uint32_t fmt,uint32_t* size){
    unsigned tw=4,th=4,tb=32;
    switch(fmt){case 0:case 8:case 14:tw=8;th=8;break;case 1:case 2:case 9:tw=8;break;
    case 3:case 4:case 5:case 10:break;case 6:tb=64;break;default:return -2;}
    if(!w || !h || w>65535 || h>65535)return -1;
    uint64_t bytes=(uint64_t)((w+tw-1)/tw)*((h+th-1)/th)*tb;
    if(bytes>UINT32_MAX)return -1;*size=(uint32_t)bytes;return 0;
}
static void MeleePc_ParticleBanksPlanExternal(MeleePc_TitlePlan* p,uint32_t cmd,uint32_t tex,uint32_t external_palette_slot){
    if(p->error)return;
    if(cmd==UINT32_MAX && tex==UINT32_MAX)return;
    if(cmd==UINT32_MAX || tex==UINT32_MAX || (cmd&3) || (tex&3) || cmd>=tex){p->error=-2;return;}
    if(!title_range(p,cmd,8) || !title_range(p,tex,4))return;
    unsigned version=title_u16(p,cmd);uint32_t local,table;
    if(!title_claim(p,cmd,2,2) || !title_claim(p,cmd+2,2,1))return;
    if(version==0){local=title_u32(p,cmd+4);table=cmd+8;title_claim(p,cmd+4,4,4);}
    else if(version>=0x40 && version<=0x43){
        if(!title_range(p,cmd,12))return;
        local=title_u32(p,cmd+8);table=cmd+12;title_claim(p,cmd+4,8,4);
        /* Original psInitDataBankLoad biases its pointer table by this
         * first command index; it is not an inherited-bank reference. */
        uint32_t first=title_u32(p,cmd+4);
        if(first>INT32_MAX || local>INT32_MAX-first){p->error=-1;return;}
    }else{p->error=-2;return;}
    if(table>tex || local>(tex-table)/4){p->error=-1;return;}
    uint32_t payload_start=table+local*4;
    for(uint32_t i=0;i<local && !p->error;++i){
        uint32_t target=particle_relative(p,cmd,table+i*4);
        if(target==UINT32_MAX)continue;
        if((target&3) || target<payload_start || target>tex || tex-target<60){p->error=-1;break;}
        title_claim(p,target,8,2);title_claim(p,target+8,52,4);
        for(uint32_t at=target+12;at<target+60 && !p->error;at+=4){
            uint32_t bits=title_u32(p,at);float value;memcpy(&value,&bits,4);
            if(!isfinite(value))p->error=-1;
        }
        uint32_t end=tex;
        for(uint32_t j=0;j<local && !p->error;++j){
            uint32_t relative=title_u32(p,table+j*4);
            if(relative>p->size-cmd){p->error=-1;break;}
            uint32_t next=cmd+relative;
            if(relative && next>target && next<end)end=next;
        }
        if(end<=target+60){p->error=-1;break;}
        title_claim(p,target+60,end-target-60,1);
    }
    if(p->error)return;
    uint32_t groups=title_u32(p,tex);
    if(groups>(p->size-tex-4)/4){p->error=-1;return;}
    title_claim(p,tex,4,4);
    for(uint32_t i=0;i<groups && !p->error;++i){
        uint32_t group=particle_relative(p,tex,tex+4+i*4);
        if(group==UINT32_MAX)continue;
        if((group&3) || group<tex+4+groups*4 || !title_range(p,group,24)){p->error=-1;break;}
        uint32_t number=title_u32(p,group),fmt=title_u32(p,group+4);
        uint32_t tlutfmt=title_u32(p,group+8),size=0;
        unsigned palnum=title_u16(p,group+20),palflag=title_u16(p,group+22);
        int result=particle_texture_size(title_u32(p,group+12),title_u32(p,group+16),fmt,&size);
        if(result){p->error=result;break;}
        uint32_t palettes=(fmt==8 || fmt==9 || fmt==10) ? ((palflag&1)?1:(palnum?palnum:number)) : 0;
        /* psdisp.c narrows the stored word to u8 before GXTlutFmt use. */
        if(palettes && (tlutfmt&0xff)>2){p->error=-2;break;}
        uint64_t count=(uint64_t)number+palettes;
        if(count>(p->size-group-24)/4){p->error=-1;break;}
        title_claim(p,group,20,4);title_claim(p,group+20,4,2);
        for(uint32_t j=0;j<count && !p->error;++j){
            uint32_t target=particle_relative_external_palette(p,tex,group+24+j*4,external_palette_slot);
            if(target==UINT32_MAX)continue;
            uint32_t bytes=j<number ? size : fmt==8?32:fmt==9?512:32768;
            if(target<tex+4+groups*4){p->error=-1;break;}
            title_payload(p,target,bytes,1);
        }
    }
}
static void MeleePc_ParticleBanksPlan(MeleePc_TitlePlan* p,uint32_t cmd,uint32_t tex){
    MeleePc_ParticleBanksPlanExternal(p,cmd,tex,UINT32_MAX);
}
#endif
