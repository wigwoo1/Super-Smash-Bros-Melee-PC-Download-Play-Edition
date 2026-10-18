#ifndef MELEE_PC_INPUT_PACKET_H
#define MELEE_PC_INPUT_PACKET_H
#include <pc/frame_input.h>
/* Version 1 host-assembled four-port tick, explicit little-endian fields.
 * Session ID separates matches; it is not authentication. Transport must
 * authenticate the sender and enforce negotiated port ownership separately. */
#define MELEE_PC_INPUT_PACKET_SIZE 84
static void MeleePc_InputPut(unsigned char* p,uint64_t v,unsigned count)
{ for(unsigned i=0;i<count;i++)p[i]=(unsigned char)(v>>(8*i)); }
static uint64_t MeleePc_InputGet(const unsigned char* p,unsigned count)
{ uint64_t v=0;for(unsigned i=0;i<count;i++)v|=(uint64_t)p[i]<<(8*i);return v; }
static signed char MeleePc_InputSigned(unsigned char v)
{ return (signed char)(v<=127?(int)v:(int)v-256); }
static int MeleePc_EncodeInputPacket(unsigned char* out,unsigned size,
    uint64_t session,uint32_t tick,const PADStatus ports[4])
{
    unsigned char candidate[MELEE_PC_INPUT_PACKET_SIZE]={0};
    if(!out||!ports||size!=sizeof(candidate)||!session)return 0;
    candidate[0]='M';candidate[1]='I';candidate[2]='N';candidate[3]='P';
    MeleePc_InputPut(candidate+4,1,2);MeleePc_InputPut(candidate+6,sizeof(candidate),2);
    MeleePc_InputPut(candidate+8,session,8);MeleePc_InputPut(candidate+16,tick,4);
    for(unsigned i=0;i<4;i++){
        const PADStatus* s=&ports[i];unsigned char* p=candidate+20+i*16;
        if(s->err!=PAD_ERR_NONE&&s->err!=PAD_ERR_NO_CONTROLLER)return 0;
        MeleePc_InputPut(p,s->button,2);
        p[2]=(unsigned char)s->stickX;p[3]=(unsigned char)s->stickY;
        p[4]=(unsigned char)s->substickX;p[5]=(unsigned char)s->substickY;
        p[6]=s->triggerLeft;p[7]=s->triggerRight;p[8]=s->analogA;p[9]=s->analogB;
        p[10]=(unsigned char)s->err;
        MeleePc_InputPut(p+12,s->extButton,4);
    }
    memcpy(out,candidate,sizeof(candidate));return 1;
}
static int MeleePc_DecodeInputPacket(const unsigned char* data,unsigned size,
    uint64_t expected_session,uint32_t* tick,PADStatus ports[4])
{
    PADStatus candidate[4]={{0}};
    if(!data||!tick||!ports||size!=MELEE_PC_INPUT_PACKET_SIZE||!expected_session)return 0;
    if(memcmp(data,"MINP",4)||MeleePc_InputGet(data+4,2)!=1||
       MeleePc_InputGet(data+6,2)!=size||MeleePc_InputGet(data+8,8)!=expected_session)return 0;
    for(unsigned i=0;i<4;i++){
        const unsigned char* p=data+20+i*16;PADStatus* s=&candidate[i];
        if(p[11]||(p[10]!=0&&p[10]!=255))return 0;
        s->button=(unsigned short)MeleePc_InputGet(p,2);
        s->stickX=MeleePc_InputSigned(p[2]);s->stickY=MeleePc_InputSigned(p[3]);
        s->substickX=MeleePc_InputSigned(p[4]);s->substickY=MeleePc_InputSigned(p[5]);
        s->triggerLeft=p[6];s->triggerRight=p[7];s->analogA=p[8];s->analogB=p[9];
        s->err=MeleePc_InputSigned(p[10]);
        s->extButton=(uint32_t)MeleePc_InputGet(p+12,4);
    }
    memcpy(ports,candidate,sizeof(candidate));*tick=(uint32_t)MeleePc_InputGet(data+16,4);return 1;
}
#endif
