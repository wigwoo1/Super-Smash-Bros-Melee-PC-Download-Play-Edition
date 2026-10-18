#ifndef MELEE_PC_UDP_INPUT_H
#define MELEE_PC_UDP_INPUT_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pc/input_buffer.h>
#pragma comment(lib,"ws2_32.lib")
/* Zero-initialize before first open. Owner-thread IPv4 transport. Endpoint
 * filtering is not cryptographic authentication; lobby negotiation is required. */
typedef struct MeleePc_UdpInput {
    SOCKET socket;
    int started,peer_set;
    unsigned short port;
    struct sockaddr_in peer;
    unsigned accepted,rejected,duplicates;
    MeleePc_InputBuffer input;
} MeleePc_UdpInput;
static void MeleePc_UdpClose(MeleePc_UdpInput* t)
{
    if(!t)return;
    if(t->started){closesocket(t->socket);WSACleanup();}
    memset(t,0,sizeof(*t));
}
static int MeleePc_UdpOpen(MeleePc_UdpInput* t,const char* bind_ip,unsigned short port,
                          uint64_t session,uint32_t first_tick)
{
    WSADATA wsa;struct sockaddr_in local={0};int length=sizeof(local);u_long nonblocking=1;
    if(!t||t->started||!bind_ip||!session)return 0;
    local.sin_family=AF_INET;local.sin_port=htons(port);
    if(InetPtonA(AF_INET,bind_ip,&local.sin_addr)!=1||WSAStartup(MAKEWORD(2,2),&wsa))return 0;
    t->socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);t->started=1;
    if(t->socket==INVALID_SOCKET||ioctlsocket(t->socket,FIONBIO,&nonblocking)||
       bind(t->socket,(struct sockaddr*)&local,sizeof(local))||
       getsockname(t->socket,(struct sockaddr*)&local,&length)){
        MeleePc_UdpClose(t);return 0;
    }
    t->port=ntohs(local.sin_port);
    MeleePc_InputBufferBegin(&t->input,session,first_tick);return 1;
}
static int MeleePc_UdpSetPeer(MeleePc_UdpInput* t,const char* ip,unsigned short port)
{
    struct sockaddr_in peer={0};peer.sin_family=AF_INET;peer.sin_port=htons(port);
    if(!t||!t->started||!ip||!port||InetPtonA(AF_INET,ip,&peer.sin_addr)!=1)return 0;
    t->peer=peer;t->peer_set=1;return 1;
}
static int MeleePc_UdpSend(MeleePc_UdpInput* t,const unsigned char* bytes,unsigned size)
{
    uint32_t tick;PADStatus ports[4];
    if(!t||!t->started||!t->peer_set||
       !MeleePc_DecodeInputPacket(bytes,size,t->input.session,&tick,ports))return 0;
    return sendto(t->socket,(const char*)bytes,size,0,(const struct sockaddr*)&t->peer,sizeof(t->peer))==(int)size;
}
/* Bounded nonblocking drain. -1 socket failure, otherwise datagrams inspected.
 * WSAEMSGSIZE discards the truncated datagram; no partial input is published. */
static int MeleePc_UdpPump(MeleePc_UdpInput* t)
{
    if(!t||!t->started||!t->peer_set)return -1;
    for(unsigned count=0;count<32;count++){
        unsigned char bytes[MELEE_PC_INPUT_PACKET_SIZE+1];struct sockaddr_in from;
        int length=sizeof(from);
        int size=recvfrom(t->socket,(char*)bytes,sizeof(bytes),0,(struct sockaddr*)&from,&length);
        if(size==SOCKET_ERROR){
            int error=WSAGetLastError();
            if(error==WSAEWOULDBLOCK)return count;
            if(error==WSAEMSGSIZE){++t->rejected;continue;}
            return -1;
        }
        if(length!=sizeof(from)||from.sin_family!=AF_INET||
           from.sin_addr.s_addr!=t->peer.sin_addr.s_addr||from.sin_port!=t->peer.sin_port){++t->rejected;continue;}
        int result=MeleePc_InputBufferReceive(&t->input,bytes,size);
        if(result==1)++t->accepted;else if(result==2)++t->duplicates;else ++t->rejected;
    }
    return 32;
}
#endif
