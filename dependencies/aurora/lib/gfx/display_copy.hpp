#pragma once

#include <array>
#include <cstdint>

namespace aurora::gfx {
// Recorded by value with the pass, never read from mutable GX state on the worker.
// Progressive RGB8 EFB -> native RGBA display surface. This does not write CPU YUYV XFB memory.
struct DisplayCopyOptions {
  bool enabled = false;
  bool clampTop = false;
  bool clampBottom = false;
  float gammaReciprocal = 1.f;
  float logicalRowStep = 1.f;
  std::array<uint32_t, 3> rowWeights{0, 64, 0};
};
inline std::array<uint32_t, 3> display_copy_weights(uint32_t first, uint32_t second) {
  return {(first & 63) + ((first >> 6) & 63),
          ((first >> 12) & 63) + ((first >> 18) & 63) + (second & 63),
          ((second >> 6) & 63) + ((second >> 12) & 63)};
}
struct DisplayCopyUniform {
  std::array<float, 4> uv;
  std::array<float, 4> rows;
  std::array<uint32_t, 4> weights;
};
static_assert(sizeof(DisplayCopyUniform) == 48);
inline DisplayCopyUniform display_copy_uniform(const DisplayCopyOptions& options,
    float x, float y, float width, float height, float sourceWidth, float sourceHeight) {
  return {{x / sourceWidth, y / sourceHeight, width / sourceWidth, height / sourceHeight},
          {options.logicalRowStep / sourceHeight,
           options.clampTop ? (y + 0.5f) / sourceHeight : 0.5f / sourceHeight,
           options.clampBottom ? (y + height - 0.5f) / sourceHeight : 1.f - 0.5f / sourceHeight,
           options.gammaReciprocal},
          {options.rowWeights[0], options.rowWeights[1], options.rowWeights[2], 0}};
}
} // namespace aurora::gfx
