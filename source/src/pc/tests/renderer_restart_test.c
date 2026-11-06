#include "../renderer_restart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static int has_arg(int argc, char** argv, const char* value)
{
    int i;
    for (i = 1; i < argc; ++i)
        if (!strcmp(argv[i], value)) return 1;
    return 0;
}

static int reject_session;
static int refuse_capture;
static unsigned session_capture(void* bytes,unsigned capacity)
{
    unsigned i;
    if(refuse_capture || capacity<37) return 0;
    for(i=0;i<37;i++) ((unsigned char*)bytes)[i]=(unsigned char)(i*13);
    return 37;
}
static int session_restore(const void* bytes,unsigned size)
{
    unsigned i;
    if(reject_session || size!=37) return 0;
    for(i=0;i<37;i++) if(((const unsigned char*)bytes)[i]!=(unsigned char)(i*13)) return 0;
    return 1;
}

static int run_parent(const char* backend, int expect_child_failure)
{
    ULONGLONG started = GetTickCount64();
    MeleePc_RendererRestartPoll state;
    if (MeleePc_RendererRestartStart(backend)) {
        do {
            state = MeleePc_RendererRestartPollChild();
            if (state == MELEE_PC_RENDERER_RESTART_READY) {
                if (expect_child_failure) {
                    puts("FAIL: child failure was reported ready");
                    return 0;
                }
                printf("PASS: %s child acknowledged before parent handoff\n",
                       backend);
                return 1;
            }
            if (state == MELEE_PC_RENDERER_RESTART_FAILED) {
                if (!expect_child_failure) {
                    puts("FAIL: ready child was rejected");
                    return 0;
                }
                printf("PASS: failed %s child kept parent alive and was rejected\n",
                       backend);
                return 1;
            }
            Sleep(2);
        } while (GetTickCount64() - started < 10000);
    }
    MeleePc_RendererRestartCleanup();
    puts("FAIL: handoff did not complete within ten seconds");
    return 0;
}

static int test_repeated_arguments(void)
{
    char ready_name[128], ack_name[128], ready_arg[160], ack_arg[160];
    char* args[] = { "test", "--melee-startup-renderer=d3d12",
        "--melee-renderer-ready=Local\\MeleeExpiredReady",
        "--melee-renderer-ack=Local\\MeleeExpiredAck",
        "--melee-startup-renderer=vulkan", ready_arg, ack_arg };
    const char* requested = NULL;
    HANDLE ready, ack;
    int attached, passed;
    snprintf(ready_name, sizeof(ready_name), "Local\\MeleeRepeatReady_%lu", GetCurrentProcessId());
    snprintf(ack_name, sizeof(ack_name), "Local\\MeleeRepeatAck_%lu", GetCurrentProcessId());
    snprintf(ready_arg, sizeof(ready_arg), "--melee-renderer-ready=%s", ready_name);
    snprintf(ack_arg, sizeof(ack_arg), "--melee-renderer-ack=%s", ack_name);
    ready = CreateEventA(NULL, TRUE, FALSE, ready_name);
    ack = CreateEventA(NULL, TRUE, TRUE, ack_name);
    if (!ready || !ack) {
        if (ready) CloseHandle(ready);
        if (ack) CloseHandle(ack);
        return 0;
    }
    attached = MeleePc_RendererRestartAttach(7, args, &requested);
    passed = attached == 2 && requested && !strcmp(requested, "vulkan");
    if (passed) passed = MeleePc_RendererRestartChildReady() &&
                        WaitForSingleObject(ready, 0) == WAIT_OBJECT_0;
    MeleePc_RendererRestartCleanup();
    CloseHandle(ready); CloseHandle(ack);
    /* An invalid latest value must not silently reuse an old valid renderer. */
    args[4] = "--melee-startup-renderer=invalid";
    passed = passed && MeleePc_RendererRestartAttach(5, args, &requested) == -1;
    MeleePc_RendererRestartCleanup();
    puts(passed ? "PASS: latest renderer and handshake replace stale arguments" :
                  "FAIL: repeated renderer arguments selected stale handoff");
    return passed;
}

static int test_renderer_handoffs(void)
{
    static const char* backends[] = { "auto", "d3d11", "d3d12", "vulkan" };
    size_t i;
    for (i = 0; i < sizeof(backends) / sizeof(backends[0]); ++i)
        if (!run_parent(backends[i], 0)) return 0;
    return 1;
}

int main(int argc, char** argv)
{
    const char* requested = NULL;
    const int session=has_arg(argc,argv,"--test-session") || has_arg(argc,argv,"--test-session-reject");
    if(session) MeleePc_RendererRestartSetSessionTransfer(session_capture,session_restore);
    reject_session=has_arg(argc,argv,"--test-session-reject");
    if (has_arg(argc, argv, "--test-repeated-arguments"))
        return test_repeated_arguments() ? EXIT_SUCCESS : EXIT_FAILURE;
    const int attached = MeleePc_RendererRestartAttach(argc, argv, &requested);
    if (attached < 0) {
        puts("FAIL: malformed handoff arguments");
        return EXIT_FAILURE;
    }
    if (attached == 2) {
        if (has_arg(argc, argv, "--test-child-fail")) return 42;
        if (!requested ||
            (strcmp(requested, "auto") != 0 &&
             strcmp(requested, "d3d11") != 0 &&
             strcmp(requested, "d3d12") != 0 &&
             strcmp(requested, "vulkan") != 0)) return 43;
        if(!MeleePc_RendererRestartRestoreSession()) return 45;
        return MeleePc_RendererRestartChildReady() ? EXIT_SUCCESS : 44;
    }
    if(session) {
        int passed;
        char* inherited=NULL;
        size_t length=0;
        if(_putenv_s("MELEE_RENDERER_SESSION","parent-value-kept")) return 46;
        passed=run_parent("vulkan",reject_session);
        if(_dupenv_s(&inherited,&length,"MELEE_RENDERER_SESSION")) return 47;
        if(!inherited || strcmp(inherited,"parent-value-kept")) { free(inherited);return 47; }
        free(inherited);
        refuse_capture=1;
        if(MeleePc_RendererRestartStart("vulkan")) return 48;
        _putenv_s("MELEE_RENDERER_SESSION","");
        if(passed) puts("PASS: session payload roundtrip/rejection, refused capture, parent environment unchanged");
        return passed ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (has_arg(argc, argv, "--test-invalid-backend")) {
        if (MeleePc_RendererRestartStart("not-a-renderer")) return EXIT_FAILURE;
        puts("PASS: invalid renderer rejected without spawning a child");
        return EXIT_SUCCESS;
    }
    if (has_arg(argc, argv, "--test-child-fail"))
        return run_parent("d3d11", 1) ? EXIT_SUCCESS : EXIT_FAILURE;
    return test_renderer_handoffs() ? EXIT_SUCCESS : EXIT_FAILURE;
}
