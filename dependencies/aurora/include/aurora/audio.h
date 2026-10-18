#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* AX owner-thread service, configured 32/48 kHz stereo. Up to 240 frames.
 * Preflight/render refusal leaves output and voice state unchanged. Effects
 * require exactly160 frames at32kHz and an enabled owner interrupt gate.
 * A callback/device failure after effect processing faults the AX lifetime;
 * effect contexts cannot be rolled back or replayed. Unsupported DSP
 * features are refused. No game callbacks run from the device thread.
 * Explicit host service: call aurora_ax_service from the owner loop. Full
 * game startup/loop integration is still pending. */
int aurora_ax_render_frames(int16_t* stereo,uint32_t frames);
/* Render and queue a bounded block to the real playback stream; queue is
 * bounded to100ms. Queue-pressure refusal does not advance voices. Device
 * failure after effect callbacks is terminal until AXQuit/reinitialization. */
int aurora_ax_submit_frames(uint32_t frames);
/* Owner only; -1 on unavailable stream or SDL error. Not physical playback proof. */
int aurora_ax_queued_bytes(void);
/* End a finite submitted segment, allowing SDL resampler tail to drain.
 * Continuous playback should keep submitting instead of flushing each block. */
int aurora_ax_flush_pending(void);
/* Install once per AX lifetime on the owner before registering a game callback.
 * Return nonzero when callbacks may run (native OS interrupt mask provider). */
int aurora_ax_set_callback_gate(int (*provider)(void));
/* Owner loop service: one 5ms block (160/240 frames) per elapsed AX tick. Masking and
 * queue pressure defer work without invoking callbacks. Over 100ms backlog or
 * a render/device failure is reported, not silently dropped. Reentry refused.
 * Call from the game owner loop; no callback runs on a device worker. */
int aurora_ax_service(void);
/* Actual stream source rate, or 0 when unavailable/off-owner. */
uint32_t aurora_ax_source_rate(void);
/* Separate AI streaming bus gain for already decoded native stereo PCM.
 * Does not scale AX voices or start a disc stream. Up to 240 frames; aliasing
 * supported. Owner-only. Decoder/device routing of this bus is still pending. */
int aurora_ai_apply_stream_volume(const int16_t* input,int16_t* output,uint32_t frames);
#ifdef __cplusplus
}
#endif
