#pragma once
#include <atomic>
#include <cstdint>

namespace aurora::graphics_quality {
struct Preferences {
  uint32_t texturePercent=100;
  bool lightingEnabled=true;
  bool operator==(const Preferences&) const = default;
};
struct FrameQuality {
  Preferences requested{};
  uint32_t texturePercent=100;
  bool lightingEnabled=true;
  bool gameplay=true;
  bool operator==(const FrameQuality&) const = default;
};
// A frame-local HUD draw override never changes the captured scene/preferences.
// Commands retain their resolved texture handles and concrete pipeline settings.
class ProcessingContext {
  inline static thread_local uint32_t depth_=0;
public:
  ProcessingContext() noexcept { ++depth_; }
  ~ProcessingContext() { --depth_; }
  ProcessingContext(const ProcessingContext&)=delete;
  ProcessingContext& operator=(const ProcessingContext&)=delete;
  static bool active() noexcept { return depth_!=0; }
};
class DrawScope {
  FrameQuality base_{}, current_{};
  uint32_t depth_=0;
public:
  void begin(FrameQuality frame) noexcept { base_=current_=frame; depth_=0; }
  bool push(bool owner,bool active,bool displayList) noexcept {
    if(!owner || !active || displayList || depth_==8)return false;
    ++depth_;current_=base_;current_.texturePercent=100;current_.lightingEnabled=true;
    return true;
  }
  bool pop(bool owner,bool active,bool displayList) noexcept {
    if(!owner || !active || displayList || depth_==0)return false;
    if(--depth_==0)current_=base_;
    return true;
  }
  const FrameQuality& current() const noexcept { return current_; }
  uint32_t depth() const noexcept { return depth_; }
};
// Quality is part of the object identity; all 32 bits of GX object IDs survive.
constexpr uint64_t texture_variant_key(uint32_t id,uint32_t quality) noexcept {
  return (uint64_t(id)<<8)|quality;
}
struct TextureLoadKey {
  uint64_t id=0;
  uint32_t quality=100;
  bool operator==(const TextureLoadKey&) const = default;
};
class SceneScope {
  // Keep standalone Aurora clients compatible until they choose a scene scope.
  std::atomic<bool> gameplay_{true};
public:
  void reset() noexcept { gameplay_.store(true); }
  bool set(bool owner,bool idle,int gameplay) noexcept {
    if(!owner || !idle || (gameplay!=0 && gameplay!=1))return false;
    gameplay_.store(gameplay!=0);return true;
  }
  FrameQuality snapshot(Preferences requested) const noexcept {
    const bool gameplay=gameplay_.load();
    return {requested,gameplay?requested.texturePercent:100,
            gameplay?requested.lightingEnabled:true,gameplay};
  }
};
// Read requested session preferences and scope on the recording owner. The
// returned value is copied into each FramePacket; GPU consumers use captured
// pipeline configurations and retained texture handles, never later globals.
FrameQuality scene_snapshot() noexcept;
} // namespace aurora::graphics_quality
