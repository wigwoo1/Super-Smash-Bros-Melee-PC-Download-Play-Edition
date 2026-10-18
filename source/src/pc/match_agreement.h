#ifndef MELEE_PC_MATCH_AGREEMENT_H
#define MELEE_PC_MATCH_AGREEMENT_H
#include <pc/input_packet.h>
#include <pc/submission_window.h>
#define MELEE_PC_MATCH_AGREEMENT_SIZE 124
/* Digests must be computed from actual canonical build/content/rules data.
 * They are compatibility checks, not signatures or sender authentication. */
typedef struct MeleePc_MatchAgreement {
    unsigned char packet[MELEE_PC_MATCH_AGREEMENT_SIZE];
    unsigned required,acknowledged;
} MeleePc_MatchAgreement;
static int MeleePc_MatchDigestPresent(const unsigned char* digest)
{unsigned bits=0;if(!digest)return 0;for(unsigned i=0;i<32;i++)bits|=digest[i];return bits!=0;}
static int MeleePc_MatchAgreementEncode(unsigned char* out,unsigned size,uint64_t session,
    uint32_t first,uint32_t seed,const unsigned char owners[4],
    const unsigned char build[32],const unsigned char content[32],const unsigned char rules[32])
{
    unsigned char packet[MELEE_PC_MATCH_AGREEMENT_SIZE]={0};unsigned required=0;
    if(!out||size!=sizeof(packet)||!session||!owners||!MeleePc_MatchDigestPresent(build)||
       !MeleePc_MatchDigestPresent(content)||!MeleePc_MatchDigestPresent(rules))return 0;
    for(unsigned i=0;i<4;i++)if(owners[i]!=255){if(owners[i]>=4)return 0;required|=1u<<owners[i];}
    if(!required)return 0;
    memcpy(packet,"MAGR",4);MeleePc_InputPut(packet+4,1,2);MeleePc_InputPut(packet+6,sizeof(packet),2);
    MeleePc_InputPut(packet+8,session,8);MeleePc_InputPut(packet+16,first,4);
    MeleePc_InputPut(packet+20,seed,4);memcpy(packet+24,owners,4);
    memcpy(packet+28,build,32);memcpy(packet+60,content,32);memcpy(packet+92,rules,32);
    memcpy(out,packet,sizeof(packet));return 1;
}
static void MeleePc_MatchAgreementEnd(MeleePc_MatchAgreement* a)
{if(a)memset(a,0,sizeof(*a));}
static int MeleePc_MatchAgreementBegin(MeleePc_MatchAgreement* a,const unsigned char* packet,unsigned size)
{
    unsigned char canonical[MELEE_PC_MATCH_AGREEMENT_SIZE];unsigned required=0;
    if(!a||a->required||!packet||size!=sizeof(canonical)||memcmp(packet,"MAGR",4)||
       MeleePc_InputGet(packet+4,2)!=1||MeleePc_InputGet(packet+6,2)!=size)return 0;
    if(!MeleePc_MatchAgreementEncode(canonical,size,MeleePc_InputGet(packet+8,8),
       (uint32_t)MeleePc_InputGet(packet+16,4),(uint32_t)MeleePc_InputGet(packet+20,4),
       packet+24,packet+28,packet+60,packet+92)||memcmp(canonical,packet,size))return 0;
    for(unsigned i=0;i<4;i++)if(packet[24+i]!=255)required|=1u<<packet[24+i];
    MeleePc_MatchAgreementEnd(a);memcpy(a->packet,packet,size);a->required=required;return 1;
}
/* Peer ID must come from authenticated connection context. Each peer sends
 * its independently constructed agreement; accepting an echoed offer alone
 * would not prove local content or rules compatibility. */
static int MeleePc_MatchAgreementAccept(MeleePc_MatchAgreement* a,unsigned peer,
    const unsigned char* local_packet,unsigned size)
{
    if(!a||!a->required||peer>=4||!(a->required&(1u<<peer))||!local_packet||
       size!=sizeof(a->packet)||memcmp(a->packet,local_packet,size))return 0;
    if(a->acknowledged&(1u<<peer))return 2;
    a->acknowledged|=1u<<peer;return 1;
}
static int MeleePc_MatchAgreementReady(const MeleePc_MatchAgreement* a)
{return a&&a->required&&(a->acknowledged&a->required)==a->required;}
static int MeleePc_MatchAgreementBeginWindow(const MeleePc_MatchAgreement* a,MeleePc_SubmissionWindow* window)
{
    if(!MeleePc_MatchAgreementReady(a))return 0;
    return MeleePc_SubmissionWindowBegin(window,MeleePc_InputGet(a->packet+8,8),
        (uint32_t)MeleePc_InputGet(a->packet+16,4),a->packet+24);
}
#endif
