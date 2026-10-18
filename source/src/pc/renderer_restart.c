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

    if (!CreateProcessW(executable, command, NULL, NULL, FALSE, 0,
                        NULL, NULL, &startup, &process))
        goto fail;
    CloseHandle(process.hThread);
    parent_process = process.hProcess;
    parent_started_at = GetTickCount64();
    free(command);
    free(wide_suffix);
    return 1;

fail:
    free(command);
    free(wide_suffix);
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
