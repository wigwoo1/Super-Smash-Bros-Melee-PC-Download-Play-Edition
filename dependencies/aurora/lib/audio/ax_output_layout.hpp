#pragma once
#include <aurora/audio.h>
#include <cstdint>

namespace aurora::audio {
// Owner-thread format transaction. Stream implementations verify their format
// after set_format. A failed change must restore/read back the previous layout
// before service may resume. This class never runs a game/effect callback.
struct AxOutputLayout {
  uint32_t channels=2,requested=2;
  int status=AURORA_AUDIO_OUTPUT_STEREO;
  bool pending=false,flushed=false;
  uint64_t drainStarted=0;
  bool request(uint32_t value) {
    if(value!=2 && value!=6)return false;
    requested=value;pending=true;flushed=false;status=AURORA_AUDIO_OUTPUT_PENDING;
    return true;
  }
  void device_changed(int deviceChannels) {
    if(channels==6 && !pending && deviceChannels!=6) {
      pending=true;flushed=false;status=AURORA_AUDIO_OUTPUT_PENDING;
    }
  }
  int finish(int value) {pending=false;flushed=false;status=value;return 1;}
  template<class Stream> int service(Stream& stream,uint32_t rate) {
    if(!pending)return 1;
    if(rate!=32000 && rate!=48000)return finish(AURORA_AUDIO_OUTPUT_ERROR);
    const int deviceChannels=stream.device_channels();
    const uint32_t target=requested==6 && deviceChannels==6?6:2;
    const int nextStatus=deviceChannels<=0?AURORA_AUDIO_OUTPUT_ERROR:
        requested==6 && target!=6?AURORA_AUDIO_OUTPUT_UNAVAILABLE:
        target==6?AURORA_AUDIO_OUTPUT_SURROUND:AURORA_AUDIO_OUTPUT_STEREO;
    if(target==channels)return finish(nextStatus);
    if(!flushed) {
      if(!stream.flush())return finish(AURORA_AUDIO_OUTPUT_ERROR);
      flushed=true;drainStarted=stream.now_ms();
    }
    const int queued=stream.queued(),available=stream.available();
    if(queued<0 || available<0)return finish(AURORA_AUDIO_OUTPUT_ERROR);
    if(queued || available) {
      const auto now=stream.now_ms();
      // A stalled/paused endpoint must not trap the menu indefinitely. The
      // original source format/voice clock remain intact on this refusal.
      if(now<drainStarted || now-drainStarted>=500)return finish(AURORA_AUDIO_OUTPUT_ERROR);
      return 0;
    }
    if(!stream.set_format(target,rate)) {
      if(!stream.set_format(channels,rate)) {
        finish(AURORA_AUDIO_OUTPUT_ERROR);return -1; // Unknown source format: terminal.
      }
      return finish(AURORA_AUDIO_OUTPUT_ERROR);
    }
    channels=target;return finish(nextStatus);
  }
};
}
