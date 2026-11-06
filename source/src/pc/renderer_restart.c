#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "renderer_restart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define RENDERER_RESTART_TIMEOUT_MS 90000ULL
#define RENDERER_RESTART_COMMAND_LIMIT 32767

static HANDLE parent_process;
static HANDLE parent_ready;
static HANDLE parent_ack;
static ULONGLONG parent_started_at;
static HANDLE child_ready;
static HANDLE child_ack;
static unsigned (*capture_session)(void*, unsigned);
static int (*restore_session)(const void*, unsigned);
#define SESSION_ENV "MELEE_RENDERER_SESSION"
#define SESSION_ENV_W L"MELEE_RENDERER_SESSION="

void MeleePc_RendererRestartSetSessionTransfer(
    unsigned (*capture)(void*, unsigned), int (*restore)(const void*, unsigned))
{
    capture_session=capture;
    restore_session=restore;
}

static wchar_t* child_environment(void)
{
    static const char digits[]="0123456789abcdef";
    unsigned char bytes[MELEE_PC_RENDERER_SESSION_CAPACITY];
    unsigned size=capture_session(bytes,sizeof(bytes));
    LPWCH original;
    const wchar_t* entry;
    wchar_t* result;
    wchar_t setting[MELEE_PC_RENDERER_SESSION_CAPACITY*2+64];
    size_t total=0, used=0, key=wcslen(SESSION_ENV_W);
    size_t setting_length;
    int inserted=0;
    unsigned i;
    if(!size || size>sizeof(bytes)) return NULL;
    memcpy(setting,SESSION_ENV_W,key*sizeof(wchar_t));
    setting_length=key;
    for(i=0;i<size;i++) {
        setting[setting_length++]=digits[bytes[i]>>4];
        setting[setting_length++]=digits[bytes[i]&15];
    }
    setting[setting_length++]=0;
    original=GetEnvironmentStringsW();
    if(!original) return NULL;
    for(entry=original;*entry;entry+=wcslen(entry)+1) total+=wcslen(entry)+1;
    total+=key+2*(size_t)size+2;
    if(total>32767) { FreeEnvironmentStringsW(original);return NULL; }
    result=malloc(total*sizeof(wchar_t));
    if(!result) { FreeEnvironmentStringsW(original);return NULL; }
    for(entry=original;*entry;entry+=wcslen(entry)+1) {
        size_t length=wcslen(entry)+1;
        if(!_wcsnicmp(entry,SESSION_ENV_W,key)) continue;
        if(!inserted && _wcsicmp(entry,setting)>0) {
            memcpy(result+used,setting,setting_length*sizeof(wchar_t));
            used+=setting_length;inserted=1;
        }
        memcpy(result+used,entry,length*sizeof(wchar_t));used+=length;
    }
    FreeEnvironmentStringsW(original);
    if(!inserted) {
        memcpy(result+used,setting,setting_length*sizeof(wchar_t));
        used+=setting_length;
    }
    result[used]=0;
    return result;
}

static int hex_digit(char value)
{
    if(value>='0' && value<='9') return value-'0';
    if(value>='a' && value<='f') return value-'a'+10;
    return -1;
}

int MeleePc_RendererRestartRestoreSession(void)
{
    unsigned char bytes[MELEE_PC_RENDERER_SESSION_CAPACITY];
    char* text=NULL;
    size_t length=0,i;
    int result;
    if(!child_ready && !child_ack) return 1;
    if(_dupenv_s(&text,&length,SESSION_ENV)!=0) return 0;
    if(!text) return 1; /* Compatibility with callers without a provider. */
    if(_putenv_s(SESSION_ENV,"")!=0) { free(text);return 0; }
    length=strlen(text);
    if(!restore_session || !length || length%2 || length/2>sizeof(bytes)) {
        free(text);return 0;
    }
    for(i=0;i<length/2;i++) {
        int hi=hex_digit(text[2*i]),lo=hex_digit(text[2*i+1]);
        if(hi<0 || lo<0) { free(text);return 0; }
        bytes[i]=(unsigned char)((hi<<4)|lo);
    }
    result=restore_session(bytes,(unsigned)(length/2));
    free(text);
    return result;
}

static int backend_is_valid(const char* backend)
{
    return backend && (!strcmp(backend, "auto") || !strcmp(backend, "d3d11") ||
                       !strcmp(backend, "d3d12") || !strcmp(backend, "vulkan"));
}

static void close_parent_handles(void)
{
    if (parent_process) CloseHandle(parent_process);
    if (parent_ready) CloseHandle(parent_ready);
    if (parent_ack) CloseHandle(parent_ack);
    parent_process = parent_ready = parent_ack = NULL;
    parent_started_at = 0;
}

int MeleePc_RendererRestartStart(const char* backend)
{
    wchar_t executable[32768];
    const wchar_t* current_command;
    wchar_t* command = NULL;
    wchar_t* wide_suffix = NULL;
    wchar_t* environment = NULL;
    char suffix[512];
    char ready_name[160];
    char ack_name[160];
    int suffix_bytes;
    int suffix_chars;
    size_t command_chars;
    size_t total_chars;
    DWORD executable_chars;
    STARTUPINFOW startup = { sizeof(startup) };
    PROCESS_INFORMATION process = { 0 };
    const ULONGLONG nonce = GetTickCount64();

    if (!backend_is_valid(backend) || parent_process || child_ready || child_ack)
        return 0;
    executable_chars = GetModuleFileNameW(NULL, executable,
                                          (DWORD)(sizeof(executable) / sizeof(executable[0])));
    if (!executable_chars || executable_chars >= sizeof(executable) / sizeof(executable[0]))
        return 0;

    if (snprintf(ready_name, sizeof(ready_name), "Local\\MeleeRendererReady_%lu_%llu",
                 GetCurrentProcessId(), (unsigned long long)nonce) < 0 ||
        snprintf(ack_name, sizeof(ack_name), "Local\\MeleeRendererAck_%lu_%llu",
                 GetCurrentProcessId(), (unsigned long long)nonce) < 0)
        return 0;
    parent_ready = CreateEventA(NULL, TRUE, FALSE, ready_name);
    parent_ack = CreateEventA(NULL, TRUE, FALSE, ack_name);
    if (!parent_ready || !parent_ack) goto fail;

    suffix_bytes = snprintf(suffix, sizeof(suffix),
                            " --melee-startup-renderer=%s --melee-renderer-ready=%s --melee-renderer-ack=%s",
                            backend, ready_name, ack_name);
    if (suffix_bytes < 0 || (size_t)suffix_bytes >= sizeof(suffix)) goto fail;
    suffix_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, suffix, -1, NULL, 0);
    if (suffix_chars <= 0) goto fail;
    wide_suffix = (wchar_t*)malloc((size_t)suffix_chars * sizeof(wchar_t));
    if (!wide_suffix || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                              suffix, -1, wide_suffix, suffix_chars))
        goto fail;

    current_command = GetCommandLineW();
    if (!current_command) goto fail;
    command_chars = wcslen(current_command);
    total_chars = command_chars + (size_t)suffix_chars + 1;
    if (total_chars > RENDERER_RESTART_COMMAND_LIMIT) goto fail;
    command = (wchar_t*)malloc(total_chars * sizeof(wchar_t));
    if (!command) goto fail;
    memcpy(command, current_command, command_chars * sizeof(wchar_t));
    memcpy(command + command_chars, wide_suffix,
           ((size_t)suffix_chars) * sizeof(wchar_t));

    if(capture_session && !(environment=child_environment())) goto fail;
    if (!CreateProcessW(executable, command, NULL, NULL, FALSE,
                        environment ? CREATE_UNICODE_ENVIRONMENT : 0,
                        environment, NULL, &startup, &process))
        goto fail;
    CloseHandle(process.hThread);
    parent_process = process.hProcess;
    parent_started_at = GetTickCount64();
    free(command);
    free(wide_suffix);
    free(environment);
    return 1;

fail:
    free(command);
    free(wide_suffix);
    free(environment);
    if (process.hThread) CloseHandle(process.hThread);
    if (process.hProcess) CloseHandle(process.hProcess);
    close_parent_handles();
    return 0;
}

MeleePc_RendererRestartPoll MeleePc_RendererRestartPollChild(void)
{
    DWORD ready_status;
    DWORD process_status;
    if (!parent_process) return MELEE_PC_RENDERER_RESTART_IDLE;
    ready_status = WaitForSingleObject(parent_ready, 0);
    if (ready_status == WAIT_OBJECT_0) {
        if (!SetEvent(parent_ack)) {
            TerminateProcess(parent_process, 96);
            close_parent_handles();
            return MELEE_PC_RENDERER_RESTART_FAILED;
        }
        close_parent_handles();
        return MELEE_PC_RENDERER_RESTART_READY;
    }
    if (ready_status == WAIT_FAILED) {
        TerminateProcess(parent_process, 96);
        close_parent_handles();
        return MELEE_PC_RENDERER_RESTART_FAILED;
    }
    process_status = WaitForSingleObject(parent_process, 0);
    if (process_status == WAIT_OBJECT_0) {
        close_parent_handles();
        return MELEE_PC_RENDERER_RESTART_FAILED;
    }
    if (process_status == WAIT_FAILED || GetTickCount64() - parent_started_at > RENDERER_RESTART_TIMEOUT_MS) {
        TerminateProcess(parent_process, 97);
        WaitForSingleObject(parent_process, 5000);
        close_parent_handles();
        return MELEE_PC_RENDERER_RESTART_FAILED;
    }
    return MELEE_PC_RENDERER_RESTART_PENDING;
}

static const char* argument_value(int argc, char** argv, const char* prefix)
{
    size_t length = strlen(prefix);
    int i;
    /* RestartStart appends a fresh renderer and handshake to this command.
     * A later restart must not reopen the previous parent's expired events. */
    for (i = argc - 1; i >= 1; --i)
        if (strncmp(argv[i], prefix, length) == 0)
            return argv[i] + length;
    return NULL;
}

int MeleePc_RendererRestartAttach(int argc, char** argv, const char** backend)
{
    const char* requested = argument_value(argc, argv, "--melee-startup-renderer=");
    const char* ready_name = argument_value(argc, argv, "--melee-renderer-ready=");
    const char* ack_name = argument_value(argc, argv, "--melee-renderer-ack=");
    if (backend) *backend = requested;
    if (!ready_name && !ack_name) return requested ? (backend_is_valid(requested) ? 1 : -1) : 0;
    if (!requested || !backend_is_valid(requested) || !ready_name || !ack_name ||
        !ready_name[0] || !ack_name[0] || !strcmp(ready_name, ack_name))
        return -1;
    child_ready = OpenEventA(EVENT_MODIFY_STATE, FALSE, ready_name);
    child_ack = OpenEventA(SYNCHRONIZE, FALSE, ack_name);
    if (!child_ready || !child_ack) {
        if (child_ready) CloseHandle(child_ready);
        if (child_ack) CloseHandle(child_ack);
        child_ready = child_ack = NULL;
        return -1;
    }
    return 2;
}

int MeleePc_RendererRestartChildReady(void)
{
    DWORD result;
    if (!child_ready && !child_ack) return 1;
    if (!child_ready || !child_ack || !SetEvent(child_ready)) goto fail;
    result = WaitForSingleObject(child_ack, (DWORD)RENDERER_RESTART_TIMEOUT_MS);
    if (result != WAIT_OBJECT_0) goto fail;
    CloseHandle(child_ready);
    CloseHandle(child_ack);
    child_ready = child_ack = NULL;
    return 1;
fail:
    MeleePc_RendererRestartCleanup();
    return 0;
}

void MeleePc_RendererRestartCleanup(void)
{
    close_parent_handles();
    if (child_ready) CloseHandle(child_ready);
    if (child_ack) CloseHandle(child_ack);
    child_ready = child_ack = NULL;
}
