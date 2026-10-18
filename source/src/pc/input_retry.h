#ifndef MELEE_PC_INPUT_RETRY_H
#define MELEE_PC_INPUT_RETRY_H
#include <pc/input_packet.h>
/* One outstanding authoritative input per channel. Callers supply monotonic
 * milliseconds. Confirmation requires the same validated packet contents;
 * transport authentication and multi-peer acknowledgement remain separate. */
typedef struct MeleePc_InputRetry {
    unsigned char packet[MELEE_PC_INPUT_PACKET_SIZE];
    uint64_t started,last_send;
    unsigned interval,timeout,max_attempts,attempts;
    int pending;
} MeleePc_InputRetry;
static void MeleePc_InputRetryEnd(MeleePc_InputRetry* r)
{if(r)memset(r,0,sizeof(*r));}
static int MeleePc_InputRetryBegin(MeleePc_InputRetry* r,const unsigned char* packet,unsigned size,
    uint64_t session,uint64_t now,unsigned interval,unsigned timeout,unsigned max_attempts)
{
    uint32_t tick;PADStatus ports[4];
    if(!r||r->pending||!interval||timeout<interval||!max_attempts||
       !MeleePc_DecodeInputPacket(packet,size,session,&tick,ports))return 0;
    MeleePc_InputRetryEnd(r);memcpy(r->packet,packet,size);
    r->started=now;r->last_send=now;r->interval=interval;r->timeout=timeout;
    r->max_attempts=max_attempts;r->pending=1;return 1;
}
/* -1 terminal timeout/attempt exhaustion/clock regression, 0 wait, 1 send.
 * No packet or attempt is consumed merely by checking readiness. */
static int MeleePc_InputRetryDue(const MeleePc_InputRetry* r,uint64_t now)
{
    if(!r||!r->pending)return 0;
    if(now<r->started||now<r->last_send||now-r->started>=r->timeout)return -1;
    if(r->attempts&&now-r->last_send<r->interval)return 0;
    return r->attempts>=r->max_attempts?-1:1;
}
static int MeleePc_InputRetrySent(MeleePc_InputRetry* r,uint64_t now)
{
    if(MeleePc_InputRetryDue(r,now)!=1)return 0;
    r->last_send=now;++r->attempts;return 1;
}
static int MeleePc_InputRetryConfirm(MeleePc_InputRetry* r,const unsigned char* packet,unsigned size)
{
    if(!r||!r->pending||!r->attempts||!packet||size!=MELEE_PC_INPUT_PACKET_SIZE||
       memcmp(r->packet,packet,size))return 0;
    MeleePc_InputRetryEnd(r);return 1;
}
#endif
