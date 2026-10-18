#ifndef MELEE_PC_RENDERER_RESTART_H
#define MELEE_PC_RENDERER_RESTART_H

/* Windows-only, owner-thread process handoff for a renderer change.
 * The current process remains alive until its child reports that host startup
 * completed with the requested renderer. The caller then exits the old host.
 */
typedef enum {
    MELEE_PC_RENDERER_RESTART_FAILED = -1,
    MELEE_PC_RENDERER_RESTART_IDLE = 0,
    MELEE_PC_RENDERER_RESTART_PENDING = 1,
    MELEE_PC_RENDERER_RESTART_READY = 2,
} MeleePc_RendererRestartPoll;

/* Returns nonzero after the replacement process starts. Names are auto,
 * d3d11, d3d12, or vulkan. Call Poll from the graphics menu while pending. */
int MeleePc_RendererRestartStart(const char* backend);
MeleePc_RendererRestartPoll MeleePc_RendererRestartPollChild(void);

/* Called by startup. Returns 0 for ordinary launch, 1 for a renderer request,
 * and -1 for malformed handoff arguments. The returned backend string points
 * into argv and is valid for the lifetime of main(). */
int MeleePc_RendererRestartAttach(int argc, char** argv, const char** backend);
/* Replacement process calls this only after runtime/backend/content startup
 * is ready. It signals the old process and waits for its acknowledgement. */
int MeleePc_RendererRestartChildReady(void);
void MeleePc_RendererRestartCleanup(void);

#endif
