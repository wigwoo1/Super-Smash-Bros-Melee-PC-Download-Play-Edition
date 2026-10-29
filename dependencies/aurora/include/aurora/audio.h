#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* AX owner-thread service, configured 32/48 kHz with stereo or six-channel
 * output. This compatibility entry point requires stereo. Up to240 frames.
 * Preflight/render refusal leaves output and voice state unchanged. Effects
 * require exactly160 frames at32kHz and an enabled owner interrupt gate.
 * A callback/device failure after effect processing faults the AX lifetime;
 * effect contexts cannot be rolled back or replayed. Unsupported DSP
 * features are refused. No game callbacks run from the device thread.
 * Explicit host service: the game owner loop calls aurora_ax_service. */
int aurora_ax_render_frames(int16_t* stereo,uint32_t frames);
/* Render the configured output layout into a caller-sized buffer. Six-channel
 * order is FL FR FC LFE BL BR. Original L/R/S maps to fronts and a -3dB rear
 * pair; FC/LFE remain silent. This does not invent six independent sources. */
int aurora_ax_render_output_frames(int16_t* samples,uint32_t frames,uint32_t capacity_samples);
enum AuroraAudioOutputStatus {
  AURORA_AUDIO_OUTPUT_STEREO = 0,
  AURORA_AUDIO_OUTPUT_SURROUND = 1,
  AURORA_AUDIO_OUTPUT_UNAVAILABLE = 2,
  AURORA_AUDIO_OUTPUT_PENDING = 3,
  AURORA_AUDIO_OUTPUT_ERROR = 4
};
/* Owner only, session-only. Accepts 2 or 6; acceptance records a REQUEST, not
 * successful surround playback. Service drains the old format before changing
 * source layout. A stereo physical endpoint retains stereo with UNAVAILABLE.
 * Only an effective six-channel device is accepted for discrete 5.1 here;
 * SDL's ability to downmix a six-channel input is not capability evidence. */
int aurora_ax_request_output_channels(uint32_t channels);
uint32_t aurora_ax_requested_output_channels(void);
uint32_t aurora_ax_output_channels(void);
int aurora_ax_output_status(void);
int aurora_ax_surround_available(void);
/* Optional stereo-to-five-speaker matrix, disabled by default. Ignored for
 * stereo output/fallback, which remains bit-identical. Source L/R is expanded
 * to FL/FR/FC/BL/BR; LFE stays silent. This is not authored 3D positional audio.
 * Owner only, outside callbacks; preference is retained during a format drain. */
int aurora_ax_set_stereo_upmix(int enabled);
int aurora_ax_stereo_upmix_enabled(void);
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
