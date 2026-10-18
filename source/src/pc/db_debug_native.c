/* Native replacement for dberror.c's PowerPC exception setup. */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <fenv.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <dolphin/os.h>

static INIT_ONCE crash_once=INIT_ONCE_STATIC_INIT;
static void native_abort_signal(int code)
{
    void* frames[48];
    unsigned count=CaptureStackBackTrace(0,48,frames,NULL);
    HANDLE output=GetStdHandle(STD_ERROR_HANDLE);
    for(unsigned i=0;i<count;i++){
        MEMORY_BASIC_INFORMATION region={0};
        void* base=NULL;
        if(VirtualQuery(frames[i],&region,sizeof(region))==sizeof(region))
            base=region.AllocationBase;
        char text[192];DWORD written;
        int size=snprintf(text,sizeof(text),
            "NATIVE_ABORT_STACK signal=%d frame=%u address=%p base=%p rva=%08lx thread=%lu\n",
            code,i,frames[i],base,(unsigned long)((uintptr_t)frames[i]-(uintptr_t)base),
            (unsigned long)GetCurrentThreadId());
        if(size>0 && output && output!=INVALID_HANDLE_VALUE)
            WriteFile(output,text,(DWORD)(size<(int)sizeof(text)?size:sizeof(text)-1),&written,NULL);
    }
    TerminateProcess(GetCurrentProcess(),72);
}
static LONG WINAPI native_unhandled_exception(EXCEPTION_POINTERS* exception)
{
    char text[256];
    const EXCEPTION_RECORD* record=exception ? exception->ExceptionRecord : NULL;
    MEMORY_BASIC_INFORMATION region={0};
    void* base=NULL;
    uintptr_t rva=0;
    if(record && VirtualQuery(record->ExceptionAddress,&region,sizeof(region))==sizeof(region)){
        base=region.AllocationBase;
        rva=(uintptr_t)record->ExceptionAddress-(uintptr_t)base;
    }
    int size=snprintf(text,sizeof(text),
        "Native unhandled exception: code=0x%08lx address=%p thread=%lu flags=0x%08lx base=%p rva=0x%08lx access=%lu target=%p\n",
        record ? (unsigned long)record->ExceptionCode : 0,
        record ? record->ExceptionAddress : NULL,
        (unsigned long)GetCurrentThreadId(),
        record ? (unsigned long)record->ExceptionFlags : 0,
        base,(unsigned long)rva,
        record && record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION && record->NumberParameters>=2 ? (unsigned long)record->ExceptionInformation[0] : 0,
        record && record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION && record->NumberParameters>=2 ? (void*)record->ExceptionInformation[1] : NULL);
    if(size>0){
        DWORD written;
        DWORD bytes=(DWORD)(size<(int)sizeof(text) ? size : sizeof(text)-1);
        HANDLE output=GetStdHandle(STD_ERROR_HANDLE);
        if(output && output!=INVALID_HANDLE_VALUE)WriteFile(output,text,bytes,&written,NULL);
    }
    if(record){
        HANDLE output=GetStdHandle(STD_ERROR_HANDLE);DWORD written;
        for(unsigned i=0;i<record->NumberParameters && i<EXCEPTION_MAXIMUM_PARAMETERS;i++){
            size=snprintf(text,sizeof(text),"NATIVE_FAULT_PARAMETER index=%u value=%08lx\n",
                i,(unsigned long)record->ExceptionInformation[i]);
            if(size>0 && output && output!=INVALID_HANDLE_VALUE)
                WriteFile(output,text,(DWORD)size,&written,NULL);
        }
    }
#if defined(_M_IX86) || defined(__i386__)
    if(exception && exception->ContextRecord){
        const CONTEXT* context=exception->ContextRecord;
        HANDLE output=GetStdHandle(STD_ERROR_HANDLE);DWORD written;
        size=snprintf(text,sizeof(text),"NATIVE_FAULT_CONTEXT eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx\n",
            context->Eax,context->Ebx,context->Ecx,context->Edx,
            context->Esi,context->Edi,context->Ebp,context->Esp,context->Eip);
        if(size>0 && output && output!=INVALID_HANDLE_VALUE)
            WriteFile(output,text,(DWORD)(size<(int)sizeof(text)?size:sizeof(text)-1),&written,NULL);
        /* ReadProcessMemory fails safely on inaccessible stack pages. No
         * speculative dereferences or game callbacks during crash handling. */
        uint32_t words[256];SIZE_T read=0;
        if(ReadProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)context->Esp,words,sizeof(words),&read)){
            for(unsigned i=0;i<read/sizeof(words[0]);i++){
                if(i>=32){
                    MEMORY_BASIC_INFORMATION candidate={0};
                    if(VirtualQuery((void*)(uintptr_t)words[i],&candidate,sizeof(candidate))!=sizeof(candidate) ||
                       !(candidate.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))continue;
                }
                size=snprintf(text,sizeof(text),"NATIVE_FAULT_STACK slot=%08lx value=%08lx\n",
                    context->Esp+i*4,(unsigned long)words[i]);
                if(size>0 && output && output!=INVALID_HANDLE_VALUE)
                    WriteFile(output,text,(DWORD)size,&written,NULL);
                MEMORY_BASIC_INFORMATION candidate={0};char module[MAX_PATH]={0};
                if(VirtualQuery((void*)(uintptr_t)words[i],&candidate,sizeof(candidate))==sizeof(candidate) &&
                   GetModuleFileNameA((HMODULE)candidate.AllocationBase,module,sizeof(module))){
                    size=snprintf(text,sizeof(text),"NATIVE_FAULT_STACK_MODULE slot=%08lx rva=%08lx module=%.160s\n",
                        context->Esp+i*4,(unsigned long)((uintptr_t)words[i]-(uintptr_t)candidate.AllocationBase),module);
                    if(size>0 && output && output!=INVALID_HANDLE_VALUE)
                        WriteFile(output,text,(DWORD)(size<(int)sizeof(text)?size:sizeof(text)-1),&written,NULL);
                }
            }
        }
    }
#endif
    /* Never resume failed game state or run DLL/game teardown while an
     * arbitrary faulting thread might own locks. OS reclaims this process. */
    TerminateProcess(GetCurrentProcess(),71);
    return EXCEPTION_EXECUTE_HANDLER;
}
static BOOL CALLBACK install_crash_handler(PINIT_ONCE once,PVOID parameter,PVOID* context)
{
    (void)once;(void)parameter;(void)context;
    SetErrorMode(GetErrorMode()|SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(native_unhandled_exception);
    /* Assertions must produce a terminal diagnostic instead of waiting for
     * an invisible CRT dialog. Never continue or tear down failed game state. */
    _set_error_mode(_OUT_TO_STDERR);
    signal(SIGABRT,native_abort_signal);
    return TRUE;
}
void db_SetupCrashHandler(void)
{
    if(!InitOnceExecuteOnce(&crash_once,install_crash_handler,NULL,NULL))
        OSPanic(__FILE__,__LINE__,"Native crash handler installation failed: %lu",(unsigned long)GetLastError());
}
void db_ClearFPUExceptions(void)
{
    /* Preserve rounding and exception masks; clear accrued flags on the
     * calling host thread instead of editing a console OSContext/FPSCR. */
    if(feclearexcept(FE_ALL_EXCEPT)!=0)
        OSPanic(__FILE__,__LINE__,"Native floating-point exception cleanup failed");
}
