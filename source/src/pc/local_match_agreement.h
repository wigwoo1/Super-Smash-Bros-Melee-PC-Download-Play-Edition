#ifndef MELEE_PC_LOCAL_MATCH_AGREEMENT_H
#define MELEE_PC_LOCAL_MATCH_AGREEMENT_H
#include <pc/match_agreement.h>
#include <pc/match_rules_digest.h>
#include <pc/runtime_content_digest.h>
/* Owner-thread preparation only. Caller must freeze disc/overlay bytes before
 * agreement and retain that freeze through gameplay. This does not acknowledge
 * remote peers, authenticate connections, or cover external gameplay globals.
 * Build identity currently covers this executable, not a dependency manifest. */
static int MeleePc_LocalMatchAgreement(unsigned char output[MELEE_PC_MATCH_AGREEMENT_SIZE],
    const StartMeleeData* match,uint64_t session,uint32_t first,uint32_t seed,
    const unsigned char owners[4])
{
    unsigned char build[32],content[32],rules[32];int success=0;
    wchar_t* path=(wchar_t*)calloc(32768,sizeof(wchar_t));DWORD length;
    if(!path||!output||!match||aurora_dvd_overlays_locked()!=1)goto done;
    length=GetModuleFileNameW(NULL,path,32768);
    if(!length||length>=32768||!MeleePc_FileDigest(path,build)||
       !MeleePc_VersusRulesDigest(match,rules)||
       !MeleePc_RuntimeContentDigest(content))goto done;
    success=MeleePc_MatchAgreementEncode(output,MELEE_PC_MATCH_AGREEMENT_SIZE,
        session,first,seed,owners,build,content,rules);
done:free(path);return success;
}
#endif
