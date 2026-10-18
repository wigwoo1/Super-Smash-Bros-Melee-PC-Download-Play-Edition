#pragma once
#include <algorithm>
#include <cstdint>

namespace aurora::gfx::clear {
struct CopyClearRegion {
  uint32_t x, y, width, height;
  bool coversTarget;
};

// Copy clears affect the copied EFB rectangle, independent of the draw scissor.
inline CopyClearRegion copy_clear_region(int32_t x, int32_t y, int32_t width, int32_t height,
                                        uint32_t targetWidth, uint32_t targetHeight) noexcept {
  if (width <= 0 || height <= 0 || targetWidth == 0 || targetHeight == 0)
    return {};
  const auto left = std::clamp<int64_t>(x, 0, targetWidth);
  const auto top = std::clamp<int64_t>(y, 0, targetHeight);
  const auto right = std::clamp<int64_t>(int64_t(x) + width, 0, targetWidth);
  const auto bottom = std::clamp<int64_t>(int64_t(y) + height, 0, targetHeight);
  return {uint32_t(left), uint32_t(top), uint32_t(right - left), uint32_t(bottom - top),
          left == 0 && top == 0 && right == targetWidth && bottom == targetHeight};
}
} // namespace aurora::gfx::clear
