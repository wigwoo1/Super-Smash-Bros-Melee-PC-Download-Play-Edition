#pragma once

#include "gx.hpp"

namespace aurora::gx {
ShaderInfo build_shader_info(const ShaderConfig& config) noexcept;
// Explicit mode keeps a cache key and the generated metadata in one snapshot.
ShaderInfo build_shader_info(const ShaderConfig& config,bool diagnosticLayout) noexcept;
gfx::Range build_uniform(const ShaderInfo& info) noexcept;
void refresh_uniform_diagnostic_flags() noexcept;
u8 color_channel(GXChannelID id) noexcept;
}; // namespace aurora::gx
