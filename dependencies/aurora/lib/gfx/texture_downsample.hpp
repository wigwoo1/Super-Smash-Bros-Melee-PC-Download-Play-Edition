#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace aurora::gfx {
struct ReducedRgbaTexture {
  uint32_t width, height;
  std::vector<uint8_t> pixels;
};

// Area filtering of decoded RGBA bytes, including alpha. Every source pixel
// contributes for odd dimensions; input storage is never modified.
inline std::optional<ReducedRgbaTexture> reduce_rgba_texture(
    uint32_t width, uint32_t height, unsigned percent, std::span<const uint8_t> source) {
  if (!width || !height || width > 16384 || height > 16384 ||
      (percent != 100 && percent != 50 && percent != 25)) return std::nullopt;
  const uint64_t bytes = uint64_t(width) * height * 4;
  if (source.size() != bytes) return std::nullopt;
  ReducedRgbaTexture out{std::max(1u, width * percent / 100),
                         std::max(1u, height * percent / 100), {}};
  out.pixels.resize(size_t(out.width) * out.height * 4);
  if (percent == 100) {
    std::copy(source.begin(), source.end(), out.pixels.begin());
    return out;
  }
  const uint64_t area = uint64_t(width) * height;
  for (uint32_t y = 0; y < out.height; ++y) {
    const uint64_t top = uint64_t(y) * height, bottom = uint64_t(y + 1) * height;
    for (uint32_t x = 0; x < out.width; ++x) {
      const uint64_t left = uint64_t(x) * width, right = uint64_t(x + 1) * width;
      uint64_t sums[4]{};
      for (uint32_t sy = uint32_t(top / out.height); sy < (bottom + out.height - 1) / out.height; ++sy) {
        const uint64_t wy = std::min(bottom, uint64_t(sy + 1) * out.height) -
                            std::max(top, uint64_t(sy) * out.height);
        for (uint32_t sx = uint32_t(left / out.width); sx < (right + out.width - 1) / out.width; ++sx) {
          const uint64_t wx = std::min(right, uint64_t(sx + 1) * out.width) -
                              std::max(left, uint64_t(sx) * out.width);
          for (unsigned c = 0; c < 4; ++c)
            sums[c] += uint64_t(source[(size_t(sy) * width + sx) * 4 + c]) * wx * wy;
        }
      }
      for (unsigned c = 0; c < 4; ++c)
        out.pixels[(size_t(y) * out.width + x) * 4 + c] = uint8_t((sums[c] + area / 2) / area);
    }
  }
  return out;
}
struct ReducedRgbaMipChain {
  uint32_t width, height, mips;
  std::vector<uint8_t> pixels;
};

inline std::optional<ReducedRgbaMipChain> reduce_rgba_mip_chain(
    uint32_t width, uint32_t height, uint32_t mips, unsigned percent,
    std::span<const uint8_t> source) {
  if (!width || !height || width > 16384 || height > 16384 || !mips ||
      (percent != 100 && percent != 50 && percent != 25)) return std::nullopt;
  uint32_t maxMips = 1;
  for (uint32_t size = std::max(width, height); size > 1; size >>= 1) ++maxMips;
  if (mips > maxMips) return std::nullopt;
  size_t required = 0;
  for (uint32_t mip = 0; mip < mips; ++mip)
    required += size_t(std::max(1u, width >> mip)) * std::max(1u, height >> mip) * 4;
  if (source.size() != required) return std::nullopt;
  ReducedRgbaMipChain out{std::max(1u, width * percent / 100),
                          std::max(1u, height * percent / 100), 1, {}};
  for (uint32_t size = std::max(out.width, out.height); size > 1 && out.mips < mips; size >>= 1)
    ++out.mips;
  size_t offset = 0;
  for (uint32_t mip = 0; mip < out.mips; ++mip) {
    const uint32_t mw = std::max(1u, width >> mip), mh = std::max(1u, height >> mip);
    const size_t bytes = size_t(mw) * mh * 4;
    auto level = reduce_rgba_texture(mw, mh, percent, source.subspan(offset, bytes));
    if (!level || level->width != std::max(1u, out.width >> mip) ||
        level->height != std::max(1u, out.height >> mip)) return std::nullopt;
    out.pixels.insert(out.pixels.end(), level->pixels.begin(), level->pixels.end());
    offset += bytes;
  }
  return out;
}
} // namespace aurora::gfx
