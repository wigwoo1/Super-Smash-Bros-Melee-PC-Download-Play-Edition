#pragma once

#include "types.hpp"
#include "display_copy.hpp"

#include <dolphin/gx/GXEnum.h>
#include <string_view>

namespace aurora::gfx::tex_copy_conv {

enum class SampleFilter : uint8_t {
  Nearest,
  Linear,
};

struct ConvRequest {
  GXTexFmt fmt;
  wgpu::TextureView srcView; // View of resolved EFB / offscreen color/depth
  Range uniformRange;        // UV transform uniform (offset + scale)
  TextureHandle dst;         // Destination texture
  SampleFilter sampleFilter = SampleFilter::Nearest;
  bool displayCopy = false;
};

bool needs_conversion(GXTexFmt fmt);
// Diagnostic compilation uses the exact helper embedded in the copy shaders.
std::string_view diagnostic_quantize4_wgsl() noexcept;

void initialize();
void shutdown();
void run(const wgpu::CommandEncoder& cmd, const ConvRequest& req);
void diagnostic_run_r4(const wgpu::CommandEncoder& cmd, const ConvRequest& req, const wgpu::Buffer& uniform);
void blit(const wgpu::CommandEncoder& cmd, const ConvRequest& req);
void display_copy(const wgpu::CommandEncoder& cmd, const ConvRequest& req);

bool snapshot_depth_supported() noexcept;
void snapshot_depth(const wgpu::CommandEncoder& cmd, const wgpu::TextureView& srcDepth, uint32_t msaaSamples,
                    const wgpu::TextureView& dst);

} // namespace aurora::gfx::tex_copy_conv
