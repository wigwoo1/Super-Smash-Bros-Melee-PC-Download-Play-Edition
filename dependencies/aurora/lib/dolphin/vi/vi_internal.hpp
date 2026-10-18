#pragma once

#include <dolphin/gx/GXStruct.h>

#include <cstdint>
#include "presentation.hpp"
#include "../../gfx/texture.hpp"

#include <aurora/math.hpp>

namespace aurora::vi {
bool pump(bool allowPre) noexcept;
bool present_source(uint64_t frame, gfx::TextureHandle& texture, std::shared_ptr<Presentation>& ticket);
bool shutdown() noexcept;
void configure(const GXRenderModeObj* rm) noexcept;
Vec2<uint32_t> configured_fb_size() noexcept;
} // namespace aurora::vi
