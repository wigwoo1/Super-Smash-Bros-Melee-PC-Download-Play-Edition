#pragma once

#include <filesystem>
#include <cstdint>
#include <optional>

#include "texture_convert.hpp"

namespace aurora::gfx::png {
std::optional<ConvertedTexture> parse_png_bytes(ArrayRef<uint8_t> bytes,
                                               uint64_t maxDecodedBytes = UINT64_MAX) noexcept;
std::optional<ConvertedTexture> load_png_file(const std::filesystem::path& path,
                                             uint64_t maxDecodedBytes = UINT64_MAX) noexcept;
} // namespace aurora::gfx::png
