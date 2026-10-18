#ifndef MELEE_PC_FROZEN_OVERLAY_H
#define MELEE_PC_FROZEN_OVERLAY_H
#include <windows.h>
#include <aurora/dvd.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
/* Zero initialize. Keep provider alive until overlay registry is detached and
 * all readers are closed. Guard uses read-only sharing, never modifies files. */
typedef struct MeleePc_FrozenOverlay {
    SRWLOCK lock;HANDLE guard;wchar_t* path;u32 size;volatile LONG readers;
    BY_HANDLE_FILE_INFORMATION identity;
} MeleePc_FrozenOverlay;
typedef struct MeleePc_FrozenReader {MeleePc_FrozenOverlay* provider;HANDLE file;} MeleePc_FrozenReader;
static int MeleePc_FrozenOverlayBegin(MeleePc_FrozenOverlay* p,const wchar_t* path)
{
    HANDLE file;BY_HANDLE_FILE_INFORMATION info;wchar_t* copy;DWORD length,written;
    if(!p||!path)return 0;
    AcquireSRWLockExclusive(&p->lock);
    if(p->path){ReleaseSRWLockExclusive(&p->lock);return 0;}
    /* Resolve once before opening; later readers must not depend on the
     * process working directory. Publish only the path actually guarded. */
    length=GetFullPathNameW(path,0,NULL,NULL);
    if(!length){ReleaseSRWLockExclusive(&p->lock);return 0;}
    copy=(wchar_t*)malloc((size_t)length*sizeof(wchar_t));
    if(!copy){ReleaseSRWLockExclusive(&p->lock);return 0;}
    written=GetFullPathNameW(path,length,copy,NULL);
    if(!written||written>=length){free(copy);ReleaseSRWLockExclusive(&p->lock);return 0;}
    file=CreateFileW(copy,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE){free(copy);ReleaseSRWLockExclusive(&p->lock);return 0;}
    if(!GetFileInformationByHandle(file,&info)||info.nFileSizeHigh||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
        CloseHandle(file);free(copy);ReleaseSRWLockExclusive(&p->lock);return 0;
    }
    p->guard=file;p->path=copy;p->size=info.nFileSizeLow;p->identity=info;
    ReleaseSRWLockExclusive(&p->lock);return 1;
}
static int MeleePc_FrozenOverlayEnd(MeleePc_FrozenOverlay* p)
{
    if(!p)return 0;AcquireSRWLockExclusive(&p->lock);
    if(!p->path||p->readers){ReleaseSRWLockExclusive(&p->lock);return 0;}
    CloseHandle(p->guard);free(p->path);p->path=NULL;p->guard=NULL;p->size=0;
    memset(&p->identity,0,sizeof(p->identity));ReleaseSRWLockExclusive(&p->lock);return 1;
}
static void* MeleePc_FrozenOverlayOpen(void* userdata)
{
    MeleePc_FrozenOverlay* p=(MeleePc_FrozenOverlay*)userdata;MeleePc_FrozenReader* reader=NULL;
    HANDLE file=INVALID_HANDLE_VALUE;BY_HANDLE_FILE_INFORMATION info;
    if(!p)return NULL;AcquireSRWLockShared(&p->lock);
    if(!p->path)goto done;
    file=CreateFileW(p->path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE||!GetFileInformationByHandle(file,&info)||
       info.dwVolumeSerialNumber!=p->identity.dwVolumeSerialNumber||
       info.nFileIndexHigh!=p->identity.nFileIndexHigh||info.nFileIndexLow!=p->identity.nFileIndexLow)goto done;
    reader=(MeleePc_FrozenReader*)malloc(sizeof(*reader));
    if(reader){reader->file=file;reader->provider=p;InterlockedIncrement(&p->readers);file=INVALID_HANDLE_VALUE;}
done:if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);ReleaseSRWLockShared(&p->lock);return reader;
}
static void MeleePc_FrozenOverlayClose(void* handle)
{
    MeleePc_FrozenReader* reader=(MeleePc_FrozenReader*)handle;
    if(reader){CloseHandle(reader->file);InterlockedDecrement(&reader->provider->readers);free(reader);}
}
static int64_t MeleePc_FrozenOverlayRead(void* handle,uint8_t* bytes,size_t size)
{
    MeleePc_FrozenReader* reader=(MeleePc_FrozenReader*)handle;DWORD count;
    if(!reader||(!bytes&&size)||size>MAXDWORD)return -1;
    return ReadFile(reader->file,bytes,(DWORD)size,&count,NULL)?count:-1;
}
static int64_t MeleePc_FrozenOverlaySeek(void* handle,int64_t offset,int32_t whence)
{
    MeleePc_FrozenReader* reader=(MeleePc_FrozenReader*)handle;LARGE_INTEGER move,result;
    if(!reader||whence<0||whence>2)return -1;move.QuadPart=offset;
    return SetFilePointerEx(reader->file,move,&result,(DWORD)whence)?result.QuadPart:-1;
}
static const AuroraOverlayCallbacks MeleePc_FrozenOverlayCallbacks={MeleePc_FrozenOverlayOpen,
    MeleePc_FrozenOverlayClose,MeleePc_FrozenOverlayRead,MeleePc_FrozenOverlaySeek};
#endif
