#ifndef MELEE_PC_UDP_SUBMISSION_H
#define MELEE_PC_UDP_SUBMISSION_H
#include <pc/udp_input.h>
#include <pc/input_submission.h>
#include <pc/submission_window.h>
typedef struct MeleePc_UdpPeers {
    struct sockaddr_in endpoint[4];
    unsigned assigned;
} MeleePc_UdpPeers;
/* Negotiation must authenticate this mapping before use. No claimed peer ID
 * from the datagram is trusted. One endpoint cannot represent two peers. */
static int MeleePc_UdpAssignPeer(MeleePc_UdpPeers* peers,unsigned id,const char* ip,unsigned short port)
{
    struct sockaddr_in endpoint={0};
    if(!peers||id>=4||!ip||!port||(peers->assigned&(1u<<id)))return 0;
    endpoint.sin_family=AF_INET;endpoint.sin_port=htons(port);
    if(InetPtonA(AF_INET,ip,&endpoint.sin_addr)!=1)return 0;
    for(unsigned i=0;i<4;i++)if((peers->assigned&(1u<<i))&&
       peers->endpoint[i].sin_addr.s_addr==endpoint.sin_addr.s_addr&&
       peers->endpoint[i].sin_port==endpoint.sin_port)return 0;
    peers->endpoint[id]=endpoint;peers->assigned|=1u<<id;return 1;
}
static int MeleePc_UdpSendSubmission(MeleePc_UdpInput* t,uint32_t tick,unsigned mask,const PADStatus ports[4])
{
    unsigned char bytes[MELEE_PC_INPUT_SUBMISSION_SIZE];
    if(!t||!t->started||!t->peer_set||
       !MeleePc_EncodeInputSubmission(bytes,sizeof(bytes),t->input.session,tick,mask,ports))return 0;
    return sendto(t->socket,(const char*)bytes,sizeof(bytes),0,
        (const struct sockaddr*)&t->peer,sizeof(t->peer))==sizeof(bytes);
}
static int MeleePc_UdpPumpSubmissionsInternal(MeleePc_UdpInput* t,const MeleePc_UdpPeers* peers,MeleePc_InputGather* gather,MeleePc_SubmissionWindow* window)
{
    uint64_t session=window?window->session:gather?gather->session:0;
    if(!t||!t->started||!peers||!session||t->input.session!=session)return -1;
    for(unsigned count=0;count<32;count++){
        unsigned char bytes[MELEE_PC_INPUT_SUBMISSION_SIZE+1];struct sockaddr_in from;
        int length=sizeof(from),peer=-1;
        int size=recvfrom(t->socket,(char*)bytes,sizeof(bytes),0,(struct sockaddr*)&from,&length);
        if(size==SOCKET_ERROR){
            int error=WSAGetLastError();if(error==WSAEWOULDBLOCK)return count;
            if(error==WSAEMSGSIZE){++t->rejected;continue;}return -1;
        }
        if(length==sizeof(from)&&from.sin_family==AF_INET)
            for(unsigned i=0;i<4;i++)if((peers->assigned&(1u<<i))&&
               from.sin_addr.s_addr==peers->endpoint[i].sin_addr.s_addr&&
               from.sin_port==peers->endpoint[i].sin_port){peer=(int)i;break;}
        int result=peer<0?0:window?MeleePc_SubmissionWindowReceive(window,(unsigned)peer,bytes,size):
            MeleePc_GatherInputSubmission(gather,(unsigned)peer,bytes,size);
        if(result==1)++t->accepted;else if(result==2)++t->duplicates;else ++t->rejected;
    }
    return 32;
}
static int MeleePc_UdpPumpSubmissions(MeleePc_UdpInput* t,const MeleePc_UdpPeers* peers,MeleePc_InputGather* gather)
{return MeleePc_UdpPumpSubmissionsInternal(t,peers,gather,NULL);}
static int MeleePc_UdpPumpSubmissionWindow(MeleePc_UdpInput* t,const MeleePc_UdpPeers* peers,MeleePc_SubmissionWindow* window)
{return MeleePc_UdpPumpSubmissionsInternal(t,peers,NULL,window);}
#endif
