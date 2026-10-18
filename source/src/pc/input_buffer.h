#ifndef MELEE_PC_INPUT_BUFFER_H
#define MELEE_PC_INPUT_BUFFER_H
#include <pc/input_packet.h>
#define MELEE_PC_INPUT_WINDOW 16
typedef struct MeleePc_InputBuffer {
    uint64_t session;
    uint64_t next_tick;
    unsigned char valid[MELEE_PC_INPUT_WINDOW];
    unsigned char packet[MELEE_PC_INPUT_WINDOW][MELEE_PC_INPUT_PACKET_SIZE];
} MeleePc_InputBuffer;
static void MeleePc_InputBufferEnd(MeleePc_InputBuffer* b)
{ if(b)memset(b,0,sizeof(*b)); }
static int MeleePc_InputBufferBegin(MeleePc_InputBuffer* b,uint64_t session,uint32_t first)
{
    if(!b||!session)return 0;
    MeleePc_InputBufferEnd(b);b->session=session;b->next_tick=first;return 1;
}
/* 0 rejected, 1 inserted, 2 identical pending duplicate. Owner thread only.
 * Conflicting duplicate data is never allowed to replace an accepted tick. */
static int MeleePc_InputBufferReceive(MeleePc_InputBuffer* b,const unsigned char* bytes,unsigned size)
{
    uint32_t tick;PADStatus ports[4];unsigned slot;
    if(!b||!b->session||b->next_tick>UINT32_MAX||
       !MeleePc_DecodeInputPacket(bytes,size,b->session,&tick,ports))return 0;
    if(tick<b->next_tick||(uint64_t)tick-b->next_tick>=MELEE_PC_INPUT_WINDOW)return 0;
    slot=tick%MELEE_PC_INPUT_WINDOW;
    if(b->valid[slot])return !memcmp(b->packet[slot],bytes,size)?2:0;
    memcpy(b->packet[slot],bytes,size);b->valid[slot]=1;return 1;
}
static const unsigned char* MeleePc_InputBufferPeek(const MeleePc_InputBuffer* b)
{
    if(!b||!b->session||b->next_tick>UINT32_MAX||!b->valid[b->next_tick%MELEE_PC_INPUT_WINDOW])return NULL;
    return b->packet[b->next_tick%MELEE_PC_INPUT_WINDOW];
}
/* Only drop after the controller successfully stages this exact front tick. */
static int MeleePc_InputBufferDrop(MeleePc_InputBuffer* b)
{
    if(!MeleePc_InputBufferPeek(b))return 0;
    unsigned slot=(unsigned)(b->next_tick%MELEE_PC_INPUT_WINDOW);
    memset(b->packet[slot],0,MELEE_PC_INPUT_PACKET_SIZE);b->valid[slot]=0;
    ++b->next_tick;return 1;
}
#endif
