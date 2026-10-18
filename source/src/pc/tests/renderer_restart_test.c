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

static int run_parent(int expect_child_failure)
{
    ULONGLONG started = GetTickCount64();
    MeleePc_RendererRestartPoll state;
    if (MeleePc_RendererRestartStart("d3d11")) {
        do {
            state = MeleePc_RendererRestartPollChild();
            if (state == MELEE_PC_RENDERER_RESTART_READY) {
                if (expect_child_failure) {
                    puts("FAIL: child failure was reported ready");
                    return 0;
                }
                puts("PASS: ready child acknowledged before parent handoff");
                return 1;
            }
            if (state == MELEE_PC_RENDERER_RESTART_FAILED) {
                if (!expect_child_failure) {
                    puts("FAIL: ready child was rejected");
                    return 0;
                }
                puts("PASS: failed child kept parent alive and was rejected");
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

int main(int argc, char** argv)
{
    const char* requested = NULL;
    if (has_arg(argc, argv, "--test-repeated-arguments"))
        return test_repeated_arguments() ? EXIT_SUCCESS : EXIT_FAILURE;
    const int attached = MeleePc_RendererRestartAttach(argc, argv, &requested);
    if (attached < 0) {
        puts("FAIL: malformed handoff arguments");
        return EXIT_FAILURE;
    }
    if (attached == 2) {
        if (has_arg(argc, argv, "--test-child-fail")) return 42;
        if (!requested || strcmp(requested, "d3d11") != 0) return 43;
        return MeleePc_RendererRestartChildReady() ? EXIT_SUCCESS : 44;
    }
    if (has_arg(argc, argv, "--test-invalid-backend")) {
        if (MeleePc_RendererRestartStart("not-a-renderer")) return EXIT_FAILURE;
        puts("PASS: invalid renderer rejected without spawning a child");
        return EXIT_SUCCESS;
    }
    return run_parent(has_arg(argc, argv, "--test-child-fail"))
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
