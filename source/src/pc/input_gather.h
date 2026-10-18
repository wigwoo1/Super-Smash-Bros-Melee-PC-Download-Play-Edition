#ifndef MELEE_PC_INPUT_GATHER_H
#define MELEE_PC_INPUT_GATHER_H
#include <pc/frame_input.h>
#define MELEE_PC_NO_PORT_OWNER 255
/* Owner-thread host assembly. Peer identity must come from an authenticated
 * connection, not an untrusted peer-number field supplied in a packet. */
typedef struct MeleePc_InputGather {
    uint64_t session,next_tick;
    unsigned required,received;
    unsigned char owner[4];
    PADStatus ports[4];
} MeleePc_InputGather;
static void MeleePc_InputGatherClearTick(MeleePc_InputGather* g)
{
    g->received=0;memset(g->ports,0,sizeof(g->ports));
    for(unsigned i=0;i<4;i++)g->ports[i].err=PAD_ERR_NO_CONTROLLER;
}
static void MeleePc_InputGatherEnd(MeleePc_InputGather* g)
{if(g)memset(g,0,sizeof(*g));}
static int MeleePc_InputGatherBegin(MeleePc_InputGather* g,uint64_t session,uint32_t first,
                                    const unsigned char owner[4])
{
    unsigned required=0;
    if(!g||g->session||!session||!owner)return 0;
    for(unsigned i=0;i<4;i++){
        if(owner[i]!=MELEE_PC_NO_PORT_OWNER){if(owner[i]>=4)return 0;required|=1u<<i;}
    }
    if(!required)return 0;
    MeleePc_InputGatherEnd(g);g->session=session;g->next_tick=first;
    memcpy(g->owner,owner,4);g->required=required;MeleePc_InputGatherClearTick(g);return 1;
}
static int MeleePc_InputEqual(const PADStatus* a,const PADStatus* b)
{
    return a->button==b->button&&a->stickX==b->stickX&&a->stickY==b->stickY&&
        a->substickX==b->substickX&&a->substickY==b->substickY&&
        a->triggerLeft==b->triggerLeft&&a->triggerRight==b->triggerRight&&
        a->analogA==b->analogA&&a->analogB==b->analogB&&a->err==b->err&&a->extButton==b->extButton;
}
/* 0 reject, 1 new input, 2 identical duplicate. Never overwrite conflicts. */
static int MeleePc_InputGatherSubmit(MeleePc_InputGather* g,uint64_t session,uint32_t tick,
                                    unsigned peer,unsigned port,const PADStatus* status)
{
    if(!g||!g->session||g->next_tick>UINT32_MAX||session!=g->session||tick!=g->next_tick||
       peer>=4||port>=4||g->owner[port]!=peer||!status||
       (status->err!=PAD_ERR_NONE&&status->err!=PAD_ERR_NO_CONTROLLER))return 0;
    if(g->received&(1u<<port))return MeleePc_InputEqual(&g->ports[port],status)?2:0;
    g->ports[port]=*status;g->received|=1u<<port;return 1;
}
static int MeleePc_InputGatherReady(const MeleePc_InputGather* g)
{return g&&g->session&&g->next_tick<=UINT32_MAX&&(g->received&g->required)==g->required;}
static int MeleePc_InputGatherPeek(const MeleePc_InputGather* g,uint32_t* tick,PADStatus ports[4])
{
    if(!tick||!ports||!MeleePc_InputGatherReady(g))return 0;
    memcpy(ports,g->ports,sizeof(g->ports));*tick=(uint32_t)g->next_tick;return 1;
}
/* Advance only after the assembled tick has entered the outgoing reliable
 * queue or local controller. Future per-port packets must buffer separately. */
static int MeleePc_InputGatherAdvance(MeleePc_InputGather* g)
{
    if(!MeleePc_InputGatherReady(g))return 0;
    ++g->next_tick;MeleePc_InputGatherClearTick(g);return 1;
}
#endif
