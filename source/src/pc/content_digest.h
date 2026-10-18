#ifndef MELEE_PC_CONTENT_DIGEST_H
#define MELEE_PC_CONTENT_DIGEST_H
#include <pc/file_digest.h>
typedef struct MeleePc_ContentEntry {
    const char* id; /* Stable logical identifier, independent of install path. */
    const wchar_t* path;
} MeleePc_ContentEntry;
static unsigned MeleePc_ContentIdLength(const char* id)
{
    unsigned length=0,segment=0;
    if(!id)return 0;
    for(;id[length];length++){
        unsigned char c=(unsigned char)id[length];
        if(length>=255)return 0;
        if(c=='/'){if(!segment)return 0;segment=0;}
        else {if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;segment++;}
        if(c=='.'&&(length==0||id[length-1]=='/'))return 0;
    }
    return segment?length:0;
}
static int MeleePc_ContentHashU32(BCRYPT_HASH_HANDLE hash,unsigned value)
{
    unsigned char bytes[4];for(unsigned i=0;i<4;i++)bytes[i]=(unsigned char)(value>>(8*i));
    return BCryptHashData(hash,bytes,4,0)>=0;
}
/* Entries strictly sorted by ASCII logical ID; do not hash machine-specific
 * absolute paths. Caller must enumerate every active asset/mod dependency and
 * hold content immutable through agreement and gameplay. */
static int MeleePc_ContentDigest(const MeleePc_ContentEntry* entries,unsigned count,unsigned char output[32])
{
    static const unsigned char domain[]="MELEE-CONTENT-v1";
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;
    unsigned char file_digest[32],digest[32];int success=0;
    if(!entries||!count||count>4096||!output)return 0;
    for(unsigned i=0;i<count;i++)if(!entries[i].path||!MeleePc_ContentIdLength(entries[i].id)||
        (i&&strcmp(entries[i-1].id,entries[i].id)>=0))return 0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
    if(BCryptHashData(hash,(PUCHAR)domain,sizeof(domain)-1,0)<0||!MeleePc_ContentHashU32(hash,count))goto done;
    for(unsigned i=0;i<count;i++){
        unsigned length=MeleePc_ContentIdLength(entries[i].id);
        if(!MeleePc_FileDigest(entries[i].path,file_digest)||!MeleePc_ContentHashU32(hash,length)||
           BCryptHashData(hash,(PUCHAR)entries[i].id,length,0)<0||
           BCryptHashData(hash,file_digest,32,0)<0)goto done;
    }
    if(BCryptFinishHash(hash,digest,32,0)<0)goto done;
    memcpy(output,digest,32);success=1;
done:
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    return success;
}
#endif
