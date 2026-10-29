#pragma once
#include "ax_voice_mixer.hpp"
#include "aux_ring.hpp"

namespace aurora::audio {
enum class AxBlockResult { ok, invalid, voice_rejected, surround_return_rejected };

inline bool ax_output_channels_valid(unsigned channels) { return channels == 2 || channels == 6; }
inline unsigned ax_frame_bytes(unsigned channels) { return ax_output_channels_valid(channels) ? channels * sizeof(int16_t) : 0; }

// SDL's six-channel order: FL FR FC LFE BL BR. Original AX has L/R/S,
// not six authored stems. Split mono S across the rear pair at -3dB each;
// do not synthesize center/bass or fold S into the front channels.
inline void ax_pack_buses(int64_t left, int64_t right, int64_t surround,
                          unsigned channels, int16_t* output, bool stereoUpmix=false) {
  output[0] = clamp_sample(left); output[1] = clamp_sample(right);
  if (channels == 6) {
    if (stereoUpmix) {
      // Optional stereo matrix, not authored 3D placement. In exact arithmetic
      // it preserves total signal power: fronts=L/R/sqrt(2), center=(L+R)/2,
      // rears=+/-(L-R)/(2*sqrt(2)). Quantize only after the stereo bus clips as
      // it does in original output. No unfiltered artificial LFE is generated.
      const int64_t l=output[0],r=output[1];
      output[0]=clamp_sample(floor_q15(l*23170));
      output[1]=clamp_sample(floor_q15(r*23170));
      output[2]=clamp_sample(floor_q15((l+r)*16384));
      output[3]=0;
      const int64_t authored=floor_q15(surround*23170);
      output[4]=clamp_sample(floor_q15((l-r)*11585)+authored);
      output[5]=clamp_sample(floor_q15((r-l)*11585)+authored);
      return;
    }
    const auto rear = clamp_sample(floor_q15(surround * 23170));
    output[2] = output[3] = 0;
    output[4] = output[5] = rear;
  }
}

// Both callers and fixtures use this production block implementation. All
// voice/sample work completes in snapshots before effects execute. Only a
// successful block commits output/voices/rings. A rejected nonzero S return
// in stereo occurs AFTER callbacks and therefore faults the host lifetime;
// callback context changes cannot be rolled back or replayed.
template<size_t N>
AxBlockResult ax_mix_block(std::array<AXVPB,N>& voices,
    std::array<AxVoiceMixer,N>& mixers, const std::array<bool,N>& used,
    const std::array<bool,N>& reserved, std::array<AuxRing,2>& aux,
    SampleRead read, void* context, int16_t* output, unsigned frames,
    unsigned channels, bool& renderingEffects, bool stereoUpmix=false) {
  if (!output || !read || !frames || frames > 240 ||
      !ax_output_channels_valid(channels) || renderingEffects) return AxBlockResult::invalid;
  const bool effects = aux[0].callback || aux[1].callback;
  if (effects && frames != 160) return AxBlockResult::invalid;
  for (unsigned i=0;i<N;++i) if (used[i] && !reserved[i] && voices[i].pb.state) {
    const auto& m = voices[i].pb.mix;
    if (channels == 2 && (m.vS || m.vDeltaS || m.vAuxAS || m.vAuxBS ||
                         m.vDeltaAuxAS || m.vDeltaAuxBS)) return AxBlockResult::invalid;
    if (!aux[0].callback && (m.vAuxAL || m.vAuxAR || m.vAuxAS ||
         m.vDeltaAuxAL || m.vDeltaAuxAR || m.vDeltaAuxAS)) return AxBlockResult::invalid;
    if (!aux[1].callback && (m.vAuxBL || m.vAuxBR || m.vAuxBS ||
         m.vDeltaAuxBL || m.vDeltaAuxBR || m.vDeltaAuxBS)) return AxBlockResult::invalid;
  }
  auto nextVoices=voices; auto nextMixers=mixers; auto rings=aux;
  std::array<int16_t,240*6> samples{};
  if (effects) for (auto& ring:rings) ring.blocks[ring.write]={};
  for (unsigned frame=0;frame<frames;++frame) {
    int64_t left=0,right=0,surround=0;
    AxAuxSample sends;
    for (unsigned i=0;i<N;++i) if (used[i] && !reserved[i] &&
        !ax_mix_voice(nextVoices[i],nextMixers[i],read,context,left,right,
                      effects?&sends:nullptr,channels==6?&surround:nullptr))
      return AxBlockResult::voice_rejected;
    if (effects) for (unsigned bus=0;bus<2;++bus) if (rings[bus].callback) {
      auto& ring=rings[bus]; const auto& values=bus?sends.b:sends.a;
      for (unsigned channel=0;channel<3;++channel)
        ring.blocks[ring.write][channel][frame]=static_cast<long>(values[channel]);
      left+=ring.blocks[ring.read][0][frame]; right+=ring.blocks[ring.read][1][frame];
      surround+=ring.blocks[ring.read][2][frame];
    }
    ax_pack_buses(left,right,surround,channels,samples.data()+frame*channels,stereoUpmix);
  }
  if (effects) {
    renderingEffects=true;
    for (auto& ring:rings) if (ring.callback) {
      const auto slot=ring.cpu; ring.process();
      if (channels==2) for (auto sample:ring.blocks[slot][2]) if (sample) {
        renderingEffects=false; return AxBlockResult::surround_return_rejected;
      }
    }
    renderingEffects=false;
  }
  voices=nextVoices; mixers=nextMixers; aux=rings;
  std::copy_n(samples.data(),frames*channels,output);
  return AxBlockResult::ok;
}
}
