#ifndef MELEE_PC_RUNTIME_CONTENT_DIGEST_H
#define MELEE_PC_RUNTIME_CONTENT_DIGEST_H
#include <pc/content_digest.h>
#include <aurora/dvd.h>
#include <stdlib.h>
typedef struct MeleePc_ResolvedFingerprint {char id[256];unsigned char digest[32];} MeleePc_ResolvedFingerprint;
typedef struct MeleePc_ResolvedCollector {
    MeleePc_ResolvedFingerprint* files;unsigned count;BCRYPT_ALG_HANDLE algorithm;
} MeleePc_ResolvedCollector;
static int MeleePc_CollectResolved(const char* path,u32 length,
    int64_t (*read)(void*,uint8_t*,size_t),void* reader,void* userdata)
{
    MeleePc_ResolvedCollector* c=(MeleePc_ResolvedCollector*)userdata;
    BCRYPT_HASH_HANDLE hash=NULL;unsigned char bytes[65536];unsigned remaining=length;
    unsigned n;int success=0;MeleePc_ResolvedFingerprint* file;
    if(c->count>=4096||!path||path[0]!='/')return 0;
    file=&c->files[c->count];memcpy(file->id,"base",4);
    for(n=0;path[n];n++){
        unsigned char ch=(unsigned char)path[n];if(n>=251)return 0;
        file->id[4+n]=(char)(ch>='A'&&ch<='Z'?ch+('a'-'A'):ch);
    }
    file->id[4+n]=0;if(!MeleePc_ContentIdLength(file->id))return 0;
    if(BCryptCreateHash(c->algorithm,&hash,NULL,0,NULL,0,0)<0)return 0;
    while(remaining){
        unsigned want=remaining<sizeof(bytes)?remaining:sizeof(bytes);
        int64_t count=read(reader,bytes,want);
        if(count<=0||(uint64_t)count>want||BCryptHashData(hash,bytes,(ULONG)count,0)<0)goto done;
        remaining-=(unsigned)count;
    }
    if(read(reader,bytes,1)!=0||BCryptFinishHash(hash,file->digest,32,0)<0)goto done;
    ++c->count;success=1;
done:BCryptDestroyHash(hash);return success;
}
static int MeleePc_ResolvedCompare(const void* a,const void* b)
{return strcmp(((const MeleePc_ResolvedFingerprint*)a)->id,((const MeleePc_ResolvedFingerprint*)b)->id);}
/* Hash all currently resolved DVD bytes. Native gameplay code/build and mod
 * settings belong in their separate agreement digests. Freeze content first;
 * this function does not itself lock external overlay bytes across a match. */
static int MeleePc_RuntimeContentDigest(unsigned char output[32])
{
    static const unsigned char domain[]="MELEE-CONTENT-v1";
    MeleePc_ResolvedCollector collector={0};BCRYPT_HASH_HANDLE hash=NULL;
    unsigned char digest[32];int visited,success=0;
    if(!output)return 0;
    collector.files=(MeleePc_ResolvedFingerprint*)calloc(4096,sizeof(*collector.files));
    if(!collector.files)return 0;
    if(BCryptOpenAlgorithmProvider(&collector.algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    visited=aurora_dvd_visit_content(MeleePc_CollectResolved,&collector);
    if(visited<=0||(unsigned)visited!=collector.count)goto done;
    qsort(collector.files,collector.count,sizeof(*collector.files),MeleePc_ResolvedCompare);
    for(unsigned i=1;i<collector.count;i++)if(!strcmp(collector.files[i-1].id,collector.files[i].id))goto done;
    if(BCryptCreateHash(collector.algorithm,&hash,NULL,0,NULL,0,0)<0||
       BCryptHashData(hash,(PUCHAR)domain,sizeof(domain)-1,0)<0||!MeleePc_ContentHashU32(hash,collector.count))goto done;
    for(unsigned i=0;i<collector.count;i++){
        MeleePc_ResolvedFingerprint* file=&collector.files[i];unsigned length=MeleePc_ContentIdLength(file->id);
        if(!MeleePc_ContentHashU32(hash,length)||BCryptHashData(hash,(PUCHAR)file->id,length,0)<0||
           BCryptHashData(hash,file->digest,32,0)<0)goto done;
    }
    if(BCryptFinishHash(hash,digest,32,0)<0)goto done;
    memcpy(output,digest,32);success=1;
done:
    if(hash)BCryptDestroyHash(hash);
    if(collector.algorithm)BCryptCloseAlgorithmProvider(collector.algorithm,0);
    free(collector.files);return success;
}
#endif
