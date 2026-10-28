#ifndef AURORA_AURORA_H
#define AURORA_AURORA_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

extern "C" {
#else
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#endif

typedef enum {
  SAMPLER_BILINEAR,
  SAMPLER_AREA,
} AuroraSampler;

typedef enum {
  BACKEND_AUTO,
  BACKEND_D3D11,
  BACKEND_D3D12,
  BACKEND_METAL,
  BACKEND_VULKAN,
  BACKEND_OPENGL,
  BACKEND_OPENGLES,
  BACKEND_WEBGPU,
  BACKEND_NULL,
} AuroraBackend;

typedef enum {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_FATAL,
} AuroraLogLevel;

typedef struct {
  int32_t x;
  int32_t y;
} AuroraWindowPos;

typedef struct {
  uint32_t width;
  uint32_t height;

  /**
   * Width of the main GX framebuffer.
   */
  uint32_t fb_width;

  /**
   * Height of the main GX framebuffer.
   */
  uint32_t fb_height;

  /**
   * The size of the framebuffer used to present to the operating system.
   * May differ from fb_width if Aurora is instructed to force an aspect ratio or resolution configuration.
   */
  uint32_t native_fb_width;

  /**
   * The size of the framebuffer used to present to the operating system.
   * May differ from fb_height if Aurora is instructed to force an aspect ratio or resolution configuration.
   */
  uint32_t native_fb_height;
  float scale;
} AuroraWindowSize;

typedef struct SDL_Window SDL_Window;
typedef struct AuroraEvent AuroraEvent;

typedef void (*AuroraLogCallback)(AuroraLogLevel level, const char* module, const char* message, unsigned int len);
typedef void (*AuroraImGuiInitCallback)(const AuroraWindowSize* size);

#define MEM1_DEFAULT_SIZE (24 * 1024 * 1024)
#define ARAM_DEFAULT_SIZE (16 * 1024 * 1024)

typedef struct {
  const char* appName;
  const char* userPath;
  const char* cachePath;
  const char* resourcesPath;
  AuroraBackend desiredBackend;
  uint32_t msaa;
  uint16_t maxTextureAnisotropy;
  bool vsync;
  bool startFullscreen;
  bool allowJoystickBackgroundEvents;
  bool pauseOnFocusLost;
  bool allowTextureDumps;
  bool allowCpuAdapter;
  int32_t windowPosX;
  int32_t windowPosY;
  uint32_t windowWidth;
  uint32_t windowHeight;
  void* iconRGBA8;
  uint32_t iconWidth;
  uint32_t iconHeight;
  AuroraLogCallback logCallback;
  AuroraLogLevel logLevel;
  AuroraImGuiInitCallback imGuiInitCallback;

  /*
   * The size of the GameCube's main memory, or MEM1 on the Wii.
   * Note that it will not be allocated at the exact 0x80000000 address, as that cannot be guaranteed.
   * This can be set to 0 to disable allocating this region.
   */
  uint32_t mem1Size;

  /*
   * The size of the GameCube's ARAM, or MEM2 on the Wii.
   * This can be set to 0 to disable allocating this region.
   */
  uint32_t mem2Size;
  /* Keep user preferences in memory; ignore userPath and skip preference files.
   * Shader caches remain separate and may use cachePath. */
  bool sessionOnlyPreferences;
} AuroraConfig;

typedef struct {
  AuroraBackend backend;
  const char* userPath;
  const char* cachePath;
  SDL_Window* window;
  AuroraWindowSize windowSize;
} AuroraInfo;

AuroraInfo aurora_initialize(int argc, char* argv[], const AuroraConfig* config);
void aurora_shutdown();
const AuroraEvent* aurora_update();
/* Owner-only window/exit service outside a recording; no simulation advance. */
int aurora_service_input_wait(void);
/* Owner event polling only: no simulation advance, sleep or device callbacks. */
int aurora_service_wait_events(void);
bool aurora_begin_frame();
void aurora_end_frame();
/* Owner-only optional GPU frame. Returns an opaque GXCopyDisp destination,
 * or NULL before recording if capacity/window is unavailable. Copy using the
 * normal HSD display-copy setup, then aurora_end_frame; no CPU XFB/VI ticket.
 * Does not advance gameplay. Still experimental until high-refresh acceptance. */
void* aurora_begin_render_snapshot_frame(void);
/* Optional native GPU scene ownership. Enable only after HSD CPU copies drain.
 * Returns false while legacy VI changes/completions remain. Clock callbacks
 * continue at 60Hz without claiming a legacy XFB was newly displayed. Disable
 * before returning to normal scene rendering; snapshot GPU retirement is separate. */
bool aurora_set_render_snapshot_scene(bool enabled);
/* Idle owner: update native device/input state and handle a queued host exit.
 * Keeps SDL event definitions out of the game library's public C boundary. */
int aurora_service_frame_events(void);
/* Diagnostic scene-frame acquisition: first submit a selected completed XFB
 * without game drawing, then leave a new recording open for the caller. */
bool aurora_begin_scene_frame(void);
/* Owner-only ready-XFB refresh, called before publication, never a retrace. */
int aurora_set_ready_xfb_refresh(void (*callback)(void));
/* Owner-only menu/lobby lifecycle; disabling closes editor and releases its input capture. */
int aurora_set_controls_available(bool available);
/* Session-only, noninteractive presented-FPS counter. Owner setter; 0/1. */
int aurora_set_fps_counter_enabled(int enabled);
int aurora_get_fps_counter_enabled(void);
/* Owner-only session render height: 0 = window resolution, otherwise 240..4320.
 * Refuses dimensions beyond device limits without changing the current scale. */
int aurora_set_render_height(uint32_t height);
uint32_t aurora_get_render_height(void);
/* Session display preset: 0=4:3,1=16:9,2=16:10,3=21:9. Owner only.
 * Resizes the window and enables render-only native camera widening. */
int aurora_set_display_preset(uint32_t preset);
uint32_t aurora_get_display_preset(void);
int aurora_get_widescreen_enabled(void);
/* Display-mode changes are staged until a VI-safe black scene boundary. */
int aurora_has_pending_display_preset(void);
int aurora_apply_display_preset_at_boundary(void);
/* Session-only fighter shadows; setter requires initialization owner. */
int aurora_set_fighter_shadows(int enabled);
int aurora_get_fighter_shadows(void);
/* Session fighter shadow map size: 64,128,256 (original default). */
int aurora_set_fighter_shadow_resolution(uint32_t size);
uint32_t aurora_get_fighter_shadow_resolution(void);
int aurora_set_texture_quality(uint32_t percent);
uint32_t aurora_get_texture_quality(void);
/* Owner-only idle scene handoff: 0 protects authored menu/FMV/Results artwork,
 * 1 applies requested gameplay texture/lighting preferences. No preference reset.
 * Effective getters use the active frame's frozen settings on the recording owner. */
int aurora_set_gameplay_graphics(int gameplay);
/* Balanced owner-only HUD draw scope within an active recording, outside display
 * lists. Protects textures/lighting without changing requested session settings. */
int aurora_push_hud_graphics(void);
int aurora_pop_hud_graphics(void);
uint32_t aurora_get_effective_texture_quality(void);
int aurora_get_effective_lighting_enabled(void);
/* Session texture pack: 0=Original, 1=installed HD. Owner requests are applied
 * before the next idle frame acquisition; get reports the applied choice. */
int aurora_set_texture_pack(uint32_t pack);
uint32_t aurora_get_texture_pack(void);
int aurora_is_hd_texture_pack_available(void);
int aurora_is_texture_pack_pending(void);
/* Session render-only material/emboss lighting. Owner setter; 0/1 only. */
int aurora_set_lighting_enabled(int enabled);
int aurora_get_lighting_enabled(void);
/* Prefer existing low-detail fighter render groups; never changes collision. */
int aurora_set_low_poly_fighters(int enabled);
int aurora_get_low_poly_fighters(void);
/* Owner-thread current framebuffer aspect; zero when unavailable. */
float aurora_get_render_aspect(void);
/* Owner-only, once before any GXSetDrawDone request. Provider is called only
 * on the owner; return nonzero when game completion callbacks may run.
 * Masked callbacks remain queued. SDK blocking waits refuse masked pending work.
 * GX draw-done requests require a provider; null/late/duplicate installs fail. */
int aurora_set_gx_callback_gate(int (*provider)(void));
/* Install once after initialize. Synchronous bounded host callback is serviced
 * on the initialization owner during VI/GPU waits and update. No worker/audio
 * thread dispatch; nested calls do not redispatch. Provider must honor masking
 * and must not advance gameplay or record graphics. Cleared at shutdown. */
int aurora_set_owner_service(int (*provider)(void));
int aurora_service_owner(void);
/* Owner-only terminal host callback for exit from original blocking VI loops.
 * Install once; callback must shut down runtime and terminate, never return. */
int aurora_set_exit_handler(void (*handler)(void));
void aurora_handle_exit(void);

/* Owner-only native CPU XFB storage enrollment. Aligned live memory, at most three
 * nonoverlapping buffers, width multiple16 <=640, height<=480. No disk storage.
 * Registration never writes bytes. Unregister refuses an outstanding copy.
 * GXCopyDisp requires enrollment; completion includes GPU readback and owner copy.
 * Query refuses pending/unknown buffers and preserves output on refusal. */
int aurora_register_xfb(void* destination, size_t bytes, uint32_t width, uint32_t height);
int aurora_unregister_xfb(void* destination);
int aurora_xfb_revision(void* destination, uint32_t* revision);

void aurora_set_log_level(AuroraLogLevel level);
void aurora_set_pause_on_focus_lost(bool value);
void aurora_set_background_input(bool value);
void aurora_set_resampler(AuroraSampler sampler);

/* Selected backend after successful initialization, including fallback. */
AuroraBackend aurora_get_backend();
/* Compiled candidate order only; this is not an adapter/device capability probe.
 * Initialization still must validate a compatible surface and usable device. */
const AuroraBackend* aurora_get_available_backends(size_t* count);

#ifdef __cplusplus
}
#endif

#endif

