#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Independent diagnostic reference for an uncompressed RGBA8 base mip.
// Explicit LOD zero only: callers must reject samplers whose LOD clamp selects
// another mip. This does not emulate derivatives, anisotropy or rasterization.
namespace aurora::gfx::texture_sample_reference {
enum class Wrap { Clamp, Repeat, Mirror };
enum class Filter { Nearest, Linear };
inline bool valid(Wrap wrap) noexcept {
  return wrap == Wrap::Clamp || wrap == Wrap::Repeat || wrap == Wrap::Mirror;
}
inline int address(int index, int extent, Wrap wrap) noexcept {
  if (wrap == Wrap::Clamp) return index < 0 ? 0 : index >= extent ? extent - 1 : index;
  const int period = wrap == Wrap::Mirror ? extent * 2 : extent;
  int value = index % period;
  if (value < 0) value += period;
  return value >= extent ? period - 1 - value : value;
}
inline bool sample(const uint8_t* bytes, size_t size, uint32_t width, uint32_t height,
                   double u, double v, Wrap s, Wrap t, Filter filter,
                   std::array<double, 4>& output) noexcept {
  // Bounds keep integer addressing and byte-size arithmetic safe on x86.
  if (!bytes || !width || !height || width > 16384 || height > 16384 ||
      size / 4 / width < height || !std::isfinite(u) || !std::isfinite(v) ||
      std::abs(u) > 1024 || std::abs(v) > 1024 || !valid(s) || !valid(t) ||
      (filter != Filter::Nearest && filter != Filter::Linear)) return false;
  const double x = u * width - (filter == Filter::Linear ? 0.5 : 0.0);
  const double y = v * height - (filter == Filter::Linear ? 0.5 : 0.0);
  const int ix = static_cast<int>(std::floor(x));
  const int iy = static_cast<int>(std::floor(y));
  const double fx = x - std::floor(x), fy = y - std::floor(y);
  const auto fetch = [&](int px, int py, unsigned c) {
    const auto offset = (size_t(address(py, int(height), t)) * width +
                         size_t(address(px, int(width), s))) * 4 + c;
    return double(bytes[offset]) / 255.0;
  };
  std::array<double, 4> result{};
  for (unsigned c = 0; c < 4; ++c) {
    if (filter == Filter::Nearest) result[c] = fetch(ix, iy, c);
    else {
      const double top = fetch(ix, iy, c) * (1 - fx) + fetch(ix + 1, iy, c) * fx;
      const double bottom = fetch(ix, iy + 1, c) * (1 - fx) + fetch(ix + 1, iy + 1, c) * fx;
      result[c] = top * (1 - fy) + bottom * fy;
    }
  }
  output = result;
  return true;
}
} // namespace aurora::gfx::texture_sample_reference
