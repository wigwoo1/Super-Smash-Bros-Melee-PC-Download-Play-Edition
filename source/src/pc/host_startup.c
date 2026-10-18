#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <aurora/aurora.h>
#include <aurora/diagnostics.h>
#include <aurora/dvd.h>
#include <aurora/audio.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <dolphin/ar.h>
#include <dolphin/pad.h>
#include <dolphin/dvd.h>
#include "callback_gate.h"
#include "alarm_clock.h"
#include "os_alarm.h"
#include "pad_sampling.h"
#include "pad_poll_policy.h"
#include "host_lifecycle.h"
#include "overlay_manifest.h"
#include "local_disc_path.h"
#include "renderer_restart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SDL's public type is forward-declared by Aurora. The Melee static library
 * deliberately does not depend on SDL's private include directory. */
extern bool SDL_HideWindow(SDL_Window* window);
extern bool SDL_ShowWindow(SDL_Window* window);

int MeleePc_GameMain(void);
void db_SetupCrashHandler(void);
static void host_shutdown(int code);
static int renderer_handoff_child;
static int renderer_handoff_ready;
static SDL_Window* renderer_handoff_window;
static int host_service(void) {
    /* Timer/device completion only: simulation remains in the original loop. */
    if (!MeleePc_CallbacksEnabled()) return 1;
    int alarm_result=MeleePc_DispatchAlarms(NULL,64);
    if(alarm_result<0) {
        fputs("NATIVE_HOST_SERVICE_FAILED component=alarms\n",stderr);
        return 0;
    }
    if(!aurora_arq_service()) {
        fputs("NATIVE_HOST_SERVICE_FAILED component=arq\n",stderr);
        return 0;
    }
    if(!aurora_dvd_service()) {
        fputs("NATIVE_HOST_SERVICE_FAILED component=dvd\n",stderr);
        return 0;
    }
    if(aurora_ax_source_rate() && !aurora_ax_service()) {
        fputs("NATIVE_HOST_SERVICE_FAILED component=audio\n",stderr);
        return 0;
    }
    if(renderer_handoff_child && !renderer_handoff_ready) {
        if(!MeleePc_RendererRestartChildReady())
            host_shutdown(4);
        renderer_handoff_ready=1;
        if(renderer_handoff_window) SDL_ShowWindow(renderer_handoff_window);
        fputs("NATIVE_RENDERER_HANDOFF child_ready=1 acknowledged=1\n",stderr);
        fflush(stderr);
        if(getenv("MELEE_DIAGNOSTIC_RENDERER_HANDOFF_QUIT_AFTER_READY")) {
            if(!aurora_diagnostic_queue_quit()) host_shutdown(98);
            fputs("NATIVE_RENDERER_HANDOFF diagnostic_quit_queued=1\n",stderr);
            fflush(stderr);
        }
    }
    return 1;
}
static DWORD owner;
/* Held before DVD mount until DVD workers/readers have shut down. Prevent
 * ordinary filesystem writers/deletes from changing the selected disc. */
static HANDLE disc_guard=INVALID_HANDLE_VALUE;
static MeleePc_FrozenContentSet session_overlays;
static char host_cache_path[MAX_PATH];
static const char* resolve_host_cache_path(void)
{
    const char* diagnostic_path=getenv("MELEE_DIAGNOSTIC_CACHE_PATH");
    DWORD temp_length;
    int written;
    DWORD attributes;
    if(diagnostic_path && diagnostic_path[0]) return diagnostic_path;
    temp_length=GetTempPathA((DWORD)sizeof(host_cache_path),host_cache_path);
    if(!temp_length || temp_length>=sizeof(host_cache_path)) {
        fprintf(stderr,"NATIVE_CACHE_RESOLVE temp_path_failed error=%lu\n",GetLastError());
        return NULL;
    }
    if(host_cache_path[temp_length-1]!='\\' && host_cache_path[temp_length-1]!='/') {
        if(temp_length+1>=sizeof(host_cache_path)) return NULL;
        host_cache_path[temp_length++]='\\';
        host_cache_path[temp_length]='\0';
    }
    written=snprintf(host_cache_path+temp_length,
                     sizeof(host_cache_path)-temp_length,
                     "MeleePCPortCache");
    if(written<0 || (size_t)written>=sizeof(host_cache_path)-temp_length) {
        fputs("NATIVE_CACHE_RESOLVE temp_path_overflow\n",stderr);
        return NULL;
    }
    attributes=GetFileAttributesA(host_cache_path);
    if(attributes==INVALID_FILE_ATTRIBUTES) {
        if(!CreateDirectoryA(host_cache_path,NULL) &&
           GetLastError()!=ERROR_ALREADY_EXISTS) {
            fprintf(stderr,"NATIVE_CACHE_RESOLVE create_failed error=%lu path=%s\n",
                    GetLastError(),host_cache_path);
            return NULL;
        }
        attributes=GetFileAttributesA(host_cache_path);
    }
    if(attributes==INVALID_FILE_ATTRIBUTES ||
       !(attributes&FILE_ATTRIBUTE_DIRECTORY)) {
        fprintf(stderr,"NATIVE_CACHE_RESOLVE not_directory error=%lu path=%s\n",
                GetLastError(),host_cache_path);
        return NULL;
    }
    return host_cache_path;
}
static int prepare_overlays(void)
{
    const char* path=getenv("MELEE_MOD_OVERLAY_MANIFEST");
    wchar_t* wide;int length;
    if(!path)return 1;
    length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);
    if(length<=0)return 0;
    wide=(wchar_t*)malloc((size_t)length*sizeof(wchar_t));if(!wide)return 0;
    int ready=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,length)&&
        MeleePc_OverlayManifestBegin(&session_overlays,wide);
    free(wide);if(!ready)return 0;
    s32* entry_numbers=(s32*)malloc(session_overlays.count*sizeof(s32));
    if(!entry_numbers)return 0;
    aurora_dvd_overlay_callbacks(&MeleePc_FrozenOverlayCallbacks);
    aurora_dvd_overlay_files(session_overlays.files,session_overlays.count,entry_numbers);
    ready=1;
    for(unsigned i=0;i<session_overlays.count;i++)if(entry_numbers[i]<0)ready=0;
    free(entry_numbers);
    if(!ready||!aurora_dvd_lock_overlays())return 0;
    fprintf(stderr,"NATIVE_MOD_OVERLAYS files=%u registry_frozen=1\n",session_overlays.count);
    return 1;
}
static int guard_disc(const char* path)
{
    int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);
    wchar_t* wide;BY_HANDLE_FILE_INFORMATION info;
    if(length<=0)return 0;
    wide=(wchar_t*)malloc((size_t)length*sizeof(wchar_t));
    if(!wide)return 0;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,length)) {
        free(wide);return 0;
    }
    disc_guard=CreateFileW(wide,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,NULL);
    if(disc_guard==INVALID_HANDLE_VALUE){fprintf(stderr,"NATIVE_DISC_GUARD open_error=%lu\n",GetLastError());free(wide);return 0;}
    if(!GetFileInformationByHandle(disc_guard,&info)||
       (info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
        CloseHandle(disc_guard);disc_guard=INVALID_HANDLE_VALUE;free(wide);return 0;
    }
    if(getenv("MELEE_DIAGNOSTIC_LOCAL_AGREEMENT")) {
        HANDLE writer=CreateFileW(wide,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        DWORD error=GetLastError();
        if(writer!=INVALID_HANDLE_VALUE){CloseHandle(writer);free(wide);return 0;}
        /* Read-only attributes/ACLs may reject write access before sharing
         * checks. Report that separately; do not claim a sharing oracle. */
        if(error!=ERROR_SHARING_VIOLATION && error!=ERROR_ACCESS_DENIED){fprintf(stderr,"NATIVE_DISC_GUARD writer_error=%lu\n",error);free(wide);return 0;}
        fprintf(stderr,"NATIVE_DISC_GUARD writer_denied=1 held_before_mount=1 error=%lu sharing_verified=%d\n",error,error==ERROR_SHARING_VIOLATION);
    }
    free(wide);return 1;
}
static void release_disc_guard(void)
{
    if(disc_guard!=INVALID_HANDLE_VALUE) {
        CloseHandle(disc_guard);disc_guard=INVALID_HANDLE_VALUE;
        fputs("NATIVE_DISC_GUARD released_after_runtime=1\n",stderr);
    }
}
static LARGE_INTEGER counter_origin,counter_frequency;
static MeleePc_AlarmClock alarm_clock;
static MeleePc_PadPollPolicy pad_poll_policy;
static int on_owner(void) { return GetCurrentThreadId()==owner; }
static void (*session_cleanup)(void*);
static void* session_cleanup_data;
int MeleePc_RegisterSessionCleanup(void (*cleanup)(void*),void* data)
{
    if(!on_owner()||!cleanup||!data||session_cleanup)return 0;
    session_cleanup=cleanup;session_cleanup_data=data;return 1;
}
int MeleePc_UnregisterSessionCleanup(void (*cleanup)(void*),void* data)
{
    if(!on_owner()||!cleanup||session_cleanup!=cleanup||session_cleanup_data!=data)return 0;
    session_cleanup=NULL;session_cleanup_data=NULL;return 1;
}
static void run_session_cleanup(void)
{
    void (*cleanup)(void*)=session_cleanup;
    void* data=session_cleanup_data;
    session_cleanup=NULL;session_cleanup_data=NULL;
    if(cleanup)cleanup(data);
}
static uint64_t monotonic_ns(void)
{
    LARGE_INTEGER current;
    uint64_t delta,frequency;
    if(!QueryPerformanceCounter(&current) || current.QuadPart<counter_origin.QuadPart)
        OSPanic(__FILE__,__LINE__,"Host performance counter failed");
    delta=(uint64_t)(current.QuadPart-counter_origin.QuadPart);
    frequency=(uint64_t)counter_frequency.QuadPart;
    return (delta/frequency)*1000000000ULL+(delta%frequency)*1000000000ULL/frequency;
}
static int pad_owner(void* data)
{
    return data==&pad_poll_policy && on_owner();
}
static int pad_apply(void* data,unsigned rate)
{
    return pad_owner(data) && MeleePc_PadPollPolicyInit(data,rate,monotonic_ns());
}
static OSTime alarm_time(void* unused)
{
    int64_t ticks;
    (void)unused;
    if(!on_owner() || !MeleePc_AlarmClockRead(&alarm_clock,monotonic_ns(),&ticks))
        OSPanic(__FILE__,__LINE__,"Host alarm clock failed");
    return ticks;
}
static void host_log(AuroraLogLevel level,const char* module,const char* text,unsigned length)
{
    fprintf(stderr,"[%s] %.*s\n",module,(int)length,text);
    if(level==LOG_FATAL) { fflush(NULL); _Exit(90); }
}

static DVDFileInfo exit_reads[8];
static __declspec(align(32)) unsigned char exit_bytes[8][32];
static unsigned exit_callbacks;
static void exit_read_complete(s32 result,DVDCommandBlock* block)
{
    unsigned i;
    if(!on_owner()||(result!=32&&result!=DVD_RESULT_CANCELED))
        OSPanic(__FILE__,__LINE__,"Invalid terminal DVD callback");
    for(i=0;i<8;i++)if(&exit_reads[i].cb==block)break;
    if(i==8||!DVDClose(&exit_reads[i])) OSPanic(__FILE__,__LINE__,"Terminal DVD handle close failed");
    ++exit_callbacks;
}
static void host_shutdown(int code)
{
    fputs("NATIVE_EXIT requested=1 shutting_down=1\n",stderr);
    MeleePc_RendererRestartCleanup();
    run_session_cleanup();
    if(!aurora_dvd_service()) OSPanic(__FILE__,__LINE__,"DVD shutdown service failed");
    if(getenv("MELEE_DIAGNOSTIC_DVD_PENDING_EXIT")) {
        unsigned i;
        for(i=0;i<8;i++) {
            if(!DVDOpen("/audio/1padv.ssm",&exit_reads[i])||
               !DVDReadAbsAsyncPrio(&exit_reads[i].cb,exit_bytes[i],32,0,exit_read_complete,2))
                OSPanic(__FILE__,__LINE__,"Terminal DVD probe submission failed");
        }
        if(exit_callbacks) OSPanic(__FILE__,__LINE__,"DVD callback escaped owner delivery");
        fputs("NATIVE_DVD_PENDING_EXIT accepted=8 delivered_before_close=0\n",stderr);
    }
    aurora_dvd_close();
    if(getenv("MELEE_DIAGNOSTIC_DVD_PENDING_EXIT")) {
        if(exit_callbacks!=8) OSPanic(__FILE__,__LINE__,"Missing terminal DVD callbacks");
        fputs("NATIVE_DVD_PENDING_EXIT delivered=8 owner=1 handles_closed=1\n",stderr);
    }
    fputs("NATIVE_DVD_SHUTDOWN closed_before_runtime=1\n",stderr);
    if(session_overlays.count){
        if(!MeleePc_FrozenContentSetEnd(&session_overlays))
            OSPanic(__FILE__,__LINE__,"Overlay readers survived DVD shutdown");
        fputs("NATIVE_MOD_OVERLAYS released_after_dvd=1\n",stderr);
    }
    aurora_shutdown();
    if(getenv("MELEE_DIAGNOSTIC_LOCAL_AGREEMENT")) {
        if(aurora_dvd_overlays_locked()!=0) OSPanic(__FILE__,__LINE__,"Registry guard survived shutdown");
        fputs("NATIVE_REGISTRY_GUARD released_after_runtime=1\n",stderr);
    }
    release_disc_guard();
    fprintf(stderr,"NATIVE_EXIT shutdown_complete=1 code=%d\n",code);
    fflush(NULL);
    exit(code);
}
static void host_exit(void)
{
    host_shutdown(0);
}

int main(int argc,char** argv)
{
    AuroraConfig config={0};
    const char* renderer=getenv("MELEE_STARTUP_RENDERER");
    const char* command_renderer=NULL;
    const int handoff=MeleePc_RendererRestartAttach(argc,argv,&command_renderer);
    const char* disc=getenv("MELEE_DISC_PATH");
    char bundled_disc_path[32768];
    wchar_t executable_path[32768];
    char executable_path_utf8[32768];
    /* Keep a flushed first-entry marker in the bounded diagnostic host so
     * pre-engine loader/startup stalls can be distinguished from game stalls. */
    {
        const char* trace_path=getenv("MELEE_DIAGNOSTIC_TRACE_FILE");
        if(trace_path && trace_path[0]) {
            HANDLE trace=CreateFileA(trace_path,FILE_APPEND_DATA,
                FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,NULL);
            if(trace!=INVALID_HANDLE_VALUE) {
                static const char marker[]="NATIVE_MAIN_ENTER\r\n";
                DWORD written=0;
                WriteFile(trace,marker,(DWORD)(sizeof(marker)-1),&written,NULL);
                CloseHandle(trace);
            }
        }
    }
    if(handoff<0) return 4;
    if(command_renderer) {
        renderer=command_renderer;
        /* Game code reads this from the MSVC CRT environment with getenv. */
        if(_putenv_s("MELEE_STARTUP_RENDERER",renderer)!=0)
            return 4;
    }
    renderer_handoff_child=(handoff==2);
    /* A renderer replacement is a normal game launch, not a replay of the
     * parent's test-only scripted menu path. Keep unrelated settings (such as
     * muted audio) intact, but remove the injected navigation/auto-close. */
    if(renderer_handoff_child &&
       (_putenv_s("MELEE_DIAGNOSTIC_MENU_NO_SAVE","")!=0 ||
        _putenv_s("MELEE_DIAGNOSTIC_NO_SAVE_MENU_CLOSE","")!=0 ||
        _putenv_s("MELEE_DIAGNOSTIC_NO_SAVE_MENU_CLOSE_FRAME","")!=0 ||
        _putenv_s("MELEE_DIAGNOSTIC_GRAPHICS_MENU_TRACE","")!=0))
        return 4;
    fputs("NATIVE_MAIN_ENTER\n",stderr);
    fflush(stderr);
    /* Cover host/device initialization as well as original game startup.
     * The later original setup call is idempotent.  Startup is also the normal
     * player path: diagnostic environment variables only add bounded test
     * controls and telemetry; they are not required to enter the game. */
    db_SetupCrashHandler();
    fprintf(stderr,"NATIVE_HOST_INIT image_base=%p entering_aurora=1\n",GetModuleHandleA(NULL));
    /* Normal package layout is <install>/melee.exe plus
     * <install>/data/melee.nkit.iso. An explicit environment override remains
     * available for developer runs and authorized import workflows. Never
     * silently reach into a developer's Desktop or another external drive. */
    if(!disc || !disc[0]) {
        DWORD path_chars=GetModuleFileNameW(NULL,executable_path,
                                            (DWORD)(sizeof(executable_path)/sizeof(executable_path[0])));
        int path_bytes;
        if(!path_chars || path_chars>=sizeof(executable_path)/sizeof(executable_path[0]))
            OSPanic(__FILE__,__LINE__,"Cannot resolve installed executable path");
        path_bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,executable_path,-1,
                                       executable_path_utf8,sizeof(executable_path_utf8),NULL,NULL);
        if(path_bytes<=0 || !MeleePc_ResolveBundledDiscPath(executable_path_utf8,
                                                             bundled_disc_path,
                                                             sizeof(bundled_disc_path)))
            OSPanic(__FILE__,__LINE__,"Cannot resolve install-local data/melee.nkit.iso path");
        disc=bundled_disc_path;
        fprintf(stderr,"NATIVE_DISC_SOURCE mode=install_local path=%s\n",disc);
    } else {
        fputs("NATIVE_DISC_SOURCE mode=explicit_override\n",stderr);
    }
    config.appName="Melee PC Port";
    config.sessionOnlyPreferences=true;
    config.userPath=NULL;
    config.cachePath=resolve_host_cache_path();
    if(!config.cachePath)
        OSPanic(__FILE__,__LINE__,"Cannot initialize temporary renderer cache path");
    config.desiredBackend=BACKEND_AUTO;
    if(renderer && !strcmp(renderer,"vulkan")) config.desiredBackend=BACKEND_VULKAN;
    else if(renderer && !strcmp(renderer,"d3d12")) config.desiredBackend=BACKEND_D3D12;
    else if(renderer && !strcmp(renderer,"d3d11")) config.desiredBackend=BACKEND_D3D11;
    else if(renderer && strcmp(renderer,"auto")) return 2;
    /* Start in the requested widescreen shape. Users can still resize the
     * window, and the in-game display settings can select other aspect ratios. */
    config.windowWidth=1280; config.windowHeight=720;
    /* Exercise preservation of either initial VSync preference in native
     * scene-transition tests. Ordinary startup retains its existing default. */
    if(getenv("MELEE_DIAGNOSTIC_INITIAL_VSYNC")) {
        const char* value=getenv("MELEE_DIAGNOSTIC_INITIAL_VSYNC");
        if(strcmp(value,"0") && strcmp(value,"1")) return 2;
        config.vsync=!strcmp(value,"1");
        fprintf(stderr,"NATIVE_HOST_VSYNC initial=%u source=diagnostic\n",config.vsync);
    }
    config.mem1Size=MEM1_DEFAULT_SIZE; config.mem2Size=ARAM_DEFAULT_SIZE;
    config.logCallback=host_log;
    owner=GetCurrentThreadId();
    if(!QueryPerformanceFrequency(&counter_frequency) || counter_frequency.QuadPart<=0 ||
       !QueryPerformanceCounter(&counter_origin)) return 3;
    AuroraInfo runtime_info=aurora_initialize(argc,argv,&config);
    if(renderer_handoff_child) {
        renderer_handoff_window=runtime_info.window;
        if(renderer_handoff_window) SDL_HideWindow(renderer_handoff_window);
    }
    fputs("NATIVE_CACHE scope=temporary-renderer-cache user_preferences=disabled\n",stderr);
    /* Expose the session-only PC controls/graphics panel in native startup.
     * Without this owner-thread registration the existing F10 entry point is
     * hidden unless a diagnostic-only environment override is set. */
    if(!aurora_set_controls_available(true))
        OSPanic(__FILE__,__LINE__,"Cannot enable native session options");
    fputs("NATIVE_OPTIONS available=1 session_only=1 shortcut=F10\n",stderr);
    /* Protect bootstrap/title/menu artwork before the first game scene enters.
     * Standalone Aurora clients retain their existing unscoped behavior. */
    if(!aurora_set_gameplay_graphics(0))
        OSPanic(__FILE__,__LINE__,"Cannot initialize native menu graphics scope");
    const char* detail_probe=getenv("MELEE_DIAGNOSTIC_LOW_POLY");
    if(detail_probe){
        if(strlen(detail_probe)!=1||(detail_probe[0]!='0'&&detail_probe[0]!='1')||
           !aurora_set_low_poly_fighters(detail_probe[0]-'0'))
            OSPanic(__FILE__,__LINE__,"Invalid diagnostic model detail setting");
    }
    const char* lighting_probe=getenv("MELEE_DIAGNOSTIC_LIGHTING");
    if(lighting_probe){
        if(strlen(lighting_probe)!=1||(lighting_probe[0]!='0'&&lighting_probe[0]!='1')||
           !aurora_set_lighting_enabled(lighting_probe[0]-'0'))
            OSPanic(__FILE__,__LINE__,"Invalid diagnostic lighting setting");
        fprintf(stderr,"NATIVE_LIGHTING enabled=%d session_only=1\n",aurora_get_lighting_enabled());
    }
    const char* display_probe=getenv("MELEE_DIAGNOSTIC_DISPLAY_PRESET");
    if(display_probe){
        if(strlen(display_probe)!=1||display_probe[0]<'0'||display_probe[0]>'3'||
           !aurora_set_display_preset((unsigned)(display_probe[0]-'0')))
            OSPanic(__FILE__,__LINE__,"Invalid diagnostic display preset");
        fprintf(stderr,"NATIVE_DISPLAY_PRESET selected=%u session_only=1\n",aurora_get_display_preset());
    }
    if(runtime_info.userPath!=NULL)
        OSPanic(__FILE__,__LINE__,"Session-only startup exposed a preference path");
    fputs("NATIVE_PREFERENCES session_only=1 user_path=none\n",stderr);
    if(config.desiredBackend!=BACKEND_AUTO && aurora_get_backend()!=config.desiredBackend)
        OSPanic(__FILE__,__LINE__,"Requested renderer unavailable");
    if(!aurora_set_exit_handler(host_exit) ||
       !MeleePc_InstallCallbackOwner(on_owner) ||
       !aurora_set_gx_callback_gate(MeleePc_CallbacksEnabled) ||
       !aurora_set_owner_service(host_service) ||
       !aurora_arq_set_callback_gate(MeleePc_CallbacksEnabled) ||
       !aurora_dvd_set_callback_gate(MeleePc_CallbacksEnabled) ||
       !MeleePc_AlarmClockInit(&alarm_clock,monotonic_ns(),OSGetTime(),40500000) ||
       !MeleePc_InstallAlarmClock(alarm_time,NULL) ||
       !MeleePc_InstallPadSampling(pad_owner,pad_apply,&pad_poll_policy))
        OSPanic(__FILE__,__LINE__,"Host owner/clock installation failed");
    if(!guard_disc(disc)) OSPanic(__FILE__,__LINE__,"Disc read-only guard failed");
    if(!aurora_dvd_open(disc)) OSPanic(__FILE__,__LINE__,"Disc mount failed");
    if(!prepare_overlays()) OSPanic(__FILE__,__LINE__,"Invalid mod overlay manifest or unavailable file");
    if(getenv("MELEE_DIAGNOSTIC_LOCAL_AGREEMENT")) {
        if((aurora_dvd_overlays_locked()!=1&&!aurora_dvd_lock_overlays())||aurora_dvd_overlays_locked()!=1)
            OSPanic(__FILE__,__LINE__,"Cannot freeze diagnostic registry");
        fputs("NATIVE_REGISTRY_GUARD held_before_game=1\n",stderr);
    }
    VIInit();
    /* A normal native launch must accept input without diagnostic flags or
     * a saved profile. Users can edit/disable this session-only preset. */
    if (!PADInstallKeyboardPreset(0)) OSPanic(__FILE__,__LINE__,"Keyboard preset failed");
    fputs("NATIVE_KEYBOARD port=0 session_only=1 WASD=stick J=A K=B U=X I=Y Enter=Start Q=L E=R O=Z HLYN=Cstick arrows=Dpad\n",stderr);
    printf("NATIVE_GAME_HOST renderer=%d mem1=%u aram=%u owner=1 clock=1 disc_mounted=1 entering_original_main=1 image_base=%p\n",
           aurora_get_backend(),config.mem1Size,config.mem2Size,GetModuleHandleA(NULL));
    fflush(NULL);
    /* Subsequent original blocking loops still need host frame/event service. */
    int game_result=MeleePc_GameMain();
    fprintf(stderr,"NATIVE_GAME_MAIN_RETURN result=%d\n",game_result);
    host_shutdown(game_result);
    return game_result;
}
