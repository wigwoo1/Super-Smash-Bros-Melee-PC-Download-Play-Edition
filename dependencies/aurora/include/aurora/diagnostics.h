#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Runtime-initialization owner check; false before initialize/after shutdown. */
int aurora_diagnostic_is_owner(void);
/* Owner-only actual active packet ID; zero outside recording or off-owner.
 * Recording identity is not proof of GPU submission or completion. */
uint64_t aurora_diagnostic_recording_id(void);
/* Owner recording target query; invalid calls preserve outputs. */
int aurora_diagnostic_render_target(uint32_t* width, uint32_t* height, int* offscreen);
/* Owner-only lookup of latest decoded copy destination. Drains FIFO; outputs
 * untouched on null/missing/off-owner. Metadata is not GPU completion proof. */
int aurora_diagnostic_snapshot_info(const void* destination, uint32_t* width,
                                    uint32_t* height, uint32_t* revision);
/* Select once for matching active frame, from decoded RGBA/BGRA8 color destination.
 * Owner-only diagnostic; no completion, HSD ownership or slot reuse proof. */
int aurora_diagnostic_present_snapshot(const void* destination, uint64_t frame_id);
/* Record an opaque-black 640x480 snapshot upload once for an unused identity.
 * Requires matching active owner frame. Submission/completion still required. */
/* One-shot synthetic GPU readback atlas; exercises the real copy shader. */
int aurora_diagnostic_display_filter_pattern(uint64_t frame_id);
int aurora_diagnostic_filtered_snapshot(const void* destination, uint64_t frame_id);
int aurora_diagnostic_blank_snapshot(const void* destination, uint64_t frame_id);
/* Thread-safe process-lifetime diagnostic observations; ready means all
 * requested draw pipelines available for that pass, not visual correctness. */
void aurora_diagnostic_pipeline_counts(uint32_t* queued, uint32_t* created);
/* Owner-only nonblocking readiness of pipelines discovered so far:
 * 0=empty/pending,1=all cached handles valid and queues drained,-1=failed/stopped,
 * -2=wrong owner/runtime absent. New draw variants may make ready become pending.
 * Does not discover every pipeline required by future game scenes. */
int aurora_diagnostic_pipeline_state(void);
/* Owner-only locked snapshot of queued foreground/background, in-flight creation
 * and failed cached entries. All outputs required; rejection preserves outputs. */
int aurora_diagnostic_pipeline_work(uint32_t* foreground, uint32_t* background,
                                    uint32_t* compiling, uint32_t* failed);
void aurora_diagnostic_draw_pass_counts(uint64_t* ready, uint64_t* unavailable);
/* Owner-thread FIFO-draining probe of decoded display-copy BP24-bit payloads.
 * Does not execute a display copy. Null/off-owner leaves all outputs untouched. */
/* Owner-only FIFO-decoded copy-filter metadata; six outputs, no pixel proof. */
int aurora_diagnostic_copy_filter_regs(uint32_t* registers);
/* Shadow display/texture controls plus decoded trigger; no pixel semantics proof. */
int aurora_diagnostic_copy_controls(uint32_t* display, uint32_t* texture, uint32_t* decoded);
int aurora_diagnostic_display_copy_regs(uint32_t* source, uint32_t* size,
                                       uint32_t* stride, uint32_t* scale);
/* Opt-in AURORA_DIAGNOSTIC_GPU_COMPLETION counters, current diagnostic generation.
 * Presentation means successful API acceptance, not physical scanout.
 * No game callback is dispatched; asynchronous failures are counted separately.
 * Intended for isolated diagnostics, not production framebuffer ownership. */
void aurora_diagnostic_gpu_counts(uint64_t* submitted, uint64_t* completed,
                                  uint64_t* failed, uint64_t* presented);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded256-record generation-tagged mailbox. kind0=submitted,kind1=GPUcompletion,
 * kind2=presentation result; status0=success,1=failure (including unavailable surface). Distinct
 * recorded Aurora frame IDs, not HSD framebuffer indices. Overflow counted, never blocks callback.
 * Null/empty/off-owner leaves outputs untouched. Owner is runtime initialization
 * thread; ownership is revoked at shutdown. Drain on that host thread. */
int aurora_diagnostic_gpu_ack(uint64_t* generation,uint64_t* id,uint32_t* kind,uint32_t* status);
uint64_t aurora_diagnostic_gpu_ack_drops(void);
/* Owner-only: schedule Dawn callback service on worker without a new frame.
 * Off-owner or after-shutdown calls enqueue nothing. */
void aurora_diagnostic_gpu_poll(void);
/* Isolated owner-thread diagnostic only, between frames with submissions stopped.
 * Bounded timeout0..2000ms; services Dawn on live render worker, no game callbacks.
 * Success requires CPU quiescence plus all tracked GPU completions, no errors/drops.
 * Does not drain acknowledgement mailbox or prove presentation success/device-loss
 * cancellation. Caller must consume events and check tickets before resource reuse.
 * Disabled diagnostics, missing device, timeout or invalid bound return0. */
int aurora_diagnostic_gpu_drain(uint32_t timeout_ms);
/* Owner-thread only, between frames; requires quiescent worker, completed
 * submissions, no failed/dropped events and a fully drained mailbox. */
int aurora_diagnostic_gpu_reset(uint64_t generation);
/* Owner-only, once before the first successful frame: prepare ImGui resources and close
 * the UI frame without drawing or presenting. This is CPU setup, not a GPU
 * completion/presentation guarantee. Call before starting input/game clocks. */
int aurora_diagnostic_prepare_ui(void);
/* Owner-only: queue a real SDL Quit event for the normal host event flow. */
int aurora_diagnostic_queue_quit(void);
int aurora_diagnostic_queue_window_close(void);
/* Refresh the startup-cached command-processor flags that the native test
 * harness can deliberately change between scenes/frames. */
void aurora_diagnostic_refresh_command_flags(void);
#ifdef __cplusplus
}
#endif








