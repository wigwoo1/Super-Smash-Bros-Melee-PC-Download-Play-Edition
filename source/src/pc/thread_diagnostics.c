#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <limits.h>
#include <dolphin/os.h>
#include "thread_diagnostics.h"
int MeleePc_CurrentStackBounds(uintptr_t* low,uintptr_t* high)
{
    ULONG_PTR bottom,top;
    uintptr_t anchor=(uintptr_t)&bottom;
    MEMORY_BASIC_INFORMATION page;
    if(!low || !high || low==high)return 0;
    GetCurrentThreadStackLimits(&bottom,&top);
    if(!bottom || bottom>=top || anchor<bottom || anchor>=top)return 0;
    if(!VirtualQuery((void*)anchor,&page,sizeof(page)) || page.State!=MEM_COMMIT ||
       (page.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return 0;
    *low=(uintptr_t)bottom;*high=(uintptr_t)top;return 1;
}
s32 OSCheckActiveThreads(void)
{
    uintptr_t low,high;
    if(!MeleePc_CurrentStackBounds(&low,&high))OSPanic(__FILE__,__LINE__,"Native current stack bounds invalid");
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snapshot==INVALID_HANDLE_VALUE)OSPanic(__FILE__,__LINE__,"Native thread snapshot failed: %lu",(unsigned long)GetLastError());
    THREADENTRY32 entry={0};entry.dwSize=sizeof(entry);
    unsigned count=0;int current_found=0;
    BOOL more=Thread32First(snapshot,&entry);
    if(!more){DWORD error=GetLastError();CloseHandle(snapshot);OSPanic(__FILE__,__LINE__,"Native thread snapshot empty: %lu",(unsigned long)error);}
    do{
        if(entry.th32OwnerProcessID==GetCurrentProcessId()){
            ++count;if(entry.th32ThreadID==GetCurrentThreadId())current_found=1;
        }
        entry.dwSize=sizeof(entry);more=Thread32Next(snapshot,&entry);
    }while(more);
    DWORD error=GetLastError();CloseHandle(snapshot);
    if(error!=ERROR_NO_MORE_FILES || !current_found || count>INT_MAX)
        OSPanic(__FILE__,__LINE__,"Native thread snapshot inconsistent");
    /* Host thread presence/current stack validation, not emulated OS run,
     * priority, stack-watermark or mutex-ownership queue checks. */
    return (s32)count;
}
void MeleePc_PrintThreadInfo(void)
{
    uintptr_t low,high,anchor=(uintptr_t)&low;
    if(!MeleePc_CurrentStackBounds(&low,&high))OSPanic(__FILE__,__LINE__,"Native stack query failed");
    OSReport("Native thread %lu: stack_low=%p stack_high=%p reserved_bytes=%lu current_depth_bytes=%lu host_threads=%d peak_watermark=unavailable\n",
        (unsigned long)GetCurrentThreadId(),(void*)low,(void*)high,
        (unsigned long)(high-low),(unsigned long)(high-anchor),OSCheckActiveThreads());
}
