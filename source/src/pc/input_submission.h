#ifndef MELEE_PC_INPUT_SUBMISSION_H
#define MELEE_PC_INPUT_SUBMISSION_H
#include <pc/input_packet.h>
#include <pc/input_gather.h>
/* Client subset of one tick. Peer identity is supplied by the connection,
 * never by this payload. Inactive ports use canonical disconnected values. */
#define MELEE_PC_INPUT_SUBMISSION_SIZE 88
static int MeleePc_EncodeInputSubmission(unsigned char* out,unsigned size,
    uint64_t session,uint32_t tick,unsigned mask,const PADStatus ports[4])
{
    unsigned char packet[MELEE_PC_INPUT_SUBMISSION_SIZE]={0};
    PADStatus selected[4]={{0}};
    if(!out||!ports||size!=sizeof(packet)||!mask||(mask&~15u))return 0;
    for(unsigned i=0;i<4;i++){
        selected[i].err=PAD_ERR_NO_CONTROLLER;
        if(mask&(1u<<i))selected[i]=ports[i];
    }
    if(!MeleePc_EncodeInputPacket(packet,84,session,tick,selected))return 0;
    memcpy(packet,"MIPS",4);MeleePc_InputPut(packet+6,sizeof(packet),2);
    packet[84]=(unsigned char)mask;memcpy(out,packet,sizeof(packet));return 1;
}
/* Validate and apply the whole subset atomically. A conflict on the last port
 * cannot leave an earlier port committed. 2 means all were duplicates. */
static int MeleePc_GatherInputSubmission(MeleePc_InputGather* gather,unsigned peer,
    const unsigned char* data,unsigned size)
{
    unsigned char packet[84];PADStatus ports[4],empty={0};uint32_t tick;
    MeleePc_InputGather candidate;unsigned mask;int changed=0;
    if(!gather||!data||size!=MELEE_PC_INPUT_SUBMISSION_SIZE||peer>=4||
       memcmp(data,"MIPS",4)||MeleePc_InputGet(data+6,2)!=size||
       !data[84]||(data[84]&~15u)||data[85]||data[86]||data[87])return 0;
    mask=data[84];memcpy(packet,data,84);memcpy(packet,"MINP",4);
    MeleePc_InputPut(packet+6,84,2);
    if(!MeleePc_DecodeInputPacket(packet,84,gather->session,&tick,ports))return 0;
    empty.err=PAD_ERR_NO_CONTROLLER;candidate=*gather;
    for(unsigned i=0;i<4;i++){
        if(mask&(1u<<i)){
            int result=MeleePc_InputGatherSubmit(&candidate,gather->session,tick,peer,i,&ports[i]);
            if(!result)return 0;
            if(result==1)changed=1;
        }else if(!MeleePc_InputEqual(&ports[i],&empty))return 0;
    }
    *gather=candidate;return changed?1:2;
}
#endif
