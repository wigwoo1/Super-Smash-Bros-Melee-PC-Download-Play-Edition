#ifndef MELEE_PC_MATCH_RULES_DIGEST_H
#define MELEE_PC_MATCH_RULES_DIGEST_H
#include <windows.h>
#include <bcrypt.h>
#include <pc/match_rules.h>
#pragma comment(lib,"bcrypt.lib")
/* Ordinary versus defaults have no mode callbacks. Reject other modes until
 * their callbacks/payloads have registered semantic encodings. No caller can
 * invent callback IDs or accidentally hash process-local function addresses. */
static int MeleePc_VersusRulesDigest(const StartMeleeData* match,
                                    unsigned char output[32])
{
    const uint32_t ids[7]={0};
    unsigned char bytes[MELEE_PC_RULES_CAPACITY],digest[32];
    unsigned length;int success=0;
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;
    if(!output||!match)return 0;
    length=MeleePc_EncodeMatchRules(bytes,sizeof(bytes),match,ids);
    if(!length)return 0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
    if(BCryptHashData(hash,bytes,length,0)<0)goto done;
    if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)goto done;
    memcpy(output,digest,sizeof(digest));success=1;
done:
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    return success;
}
#endif
