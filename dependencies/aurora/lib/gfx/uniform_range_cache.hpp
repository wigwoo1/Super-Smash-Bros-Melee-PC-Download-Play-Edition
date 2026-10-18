#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace aurora::gfx::detail {

inline uint64_t uniform_range_hash(const uint8_t* data, size_t length) noexcept {
  uint64_t value = 14695981039346656037ull;
  for (size_t i = 0; i < length; ++i) value = (value ^ data[i]) * 1099511628211ull;
  return (value ^ length) * 1099511628211ull;
}

template <typename Range>
bool uniform_range_matches(Range range, const uint8_t* data, size_t length,
                           const uint8_t* storage, size_t storageSize) noexcept {
  if (!data || !storage || length == 0 || range.size != length ||
      range.offset > storageSize || length > storageSize - range.offset) {
    return false;
  }
  return std::memcmp(storage + range.offset, data, length) == 0;
}

} // namespace aurora::gfx::detail
