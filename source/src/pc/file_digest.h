#ifndef MELEE_PC_FILE_DIGEST_H
#define MELEE_PC_FILE_DIGEST_H
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#pragma comment(lib,"bcrypt.lib")
/* Read-only Windows SHA256. Deny concurrent writers during hashing. Caller
 * must still prevent content changes between agreement and match loading. */
static int MeleePc_FileDigest(const wchar_t* path,unsigned char output[32])
{
    HANDLE file=INVALID_HANDLE_VALUE;BCRYPT_ALG_HANDLE algorithm=NULL;
    BCRYPT_HASH_HANDLE hash=NULL;unsigned char chunk[65536],digest[32];DWORD count;
    int success=0;
    if(!path||!output)return 0;
    file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
    for(;;){
        if(!ReadFile(file,chunk,sizeof(chunk),&count,NULL))goto done;
        if(!count)break;
        if(BCryptHashData(hash,chunk,count,0)<0)goto done;
    }
    if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)goto done;
    memcpy(output,digest,sizeof(digest));success=1;
done:
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);return success;
}
#endif
