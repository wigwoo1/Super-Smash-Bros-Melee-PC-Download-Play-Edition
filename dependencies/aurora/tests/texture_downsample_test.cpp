#include "gfx/texture_downsample.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

std::vector<uint8_t> solid(uint32_t width, uint32_t height,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  std::vector<uint8_t> pixels(size_t(width) * height * 4);
  for (size_t i = 0; i < pixels.size(); i += 4) {
    pixels[i] = r;
    pixels[i + 1] = g;
    pixels[i + 2] = b;
    pixels[i + 3] = a;
  }
  return pixels;
}

void verify_dimensions_and_area_filter() {
  // Distinct source blocks make the test sensitive to both dimensions and
  // the filter's per-pixel weights, rather than just checking output length.
  std::vector<uint8_t> source(4 * 2 * 4);
  for (uint32_t y = 0; y < 2; ++y) {
    for (uint32_t x = 0; x < 4; ++x) {
      const auto value = uint8_t(x * 30 + y * 10);
      const size_t p = (size_t(y) * 4 + x) * 4;
      source[p] = value;
      source[p + 1] = uint8_t(value + 1);
      source[p + 2] = uint8_t(value + 2);
      source[p + 3] = uint8_t(value + 3);
    }
  }
  const auto half = aurora::gfx::reduce_rgba_texture(4, 2, 50, source);
  require(half.has_value(), "50% reduction rejected valid RGBA data");
  require(half->width == 2 && half->height == 1,
          "50% reduction dimensions are not half width and height");
  require(half->pixels == std::vector<uint8_t>({20, 21, 22, 23, 80, 81, 82, 83}),
          "50% area filter produced unexpected channel values");

  // Quarter dimensions of a 2x2 texture clamp to 1x1; alpha is filtered too.
  const std::vector<uint8_t> alphaPattern{
      10, 20, 30, 0, 10, 20, 30, 0,
      10, 20, 30, 0, 10, 20, 30, 255};
  const auto quarter = aurora::gfx::reduce_rgba_texture(2, 2, 25, alphaPattern);
  require(quarter && quarter->width == 1 && quarter->height == 1,
          "25% dimensions must clamp to at least 1x1");
  require(quarter->pixels == std::vector<uint8_t>({10, 20, 30, 64}),
          "25% reduction did not area-filter alpha");

  // Odd source sizes must preserve a constant texture and never drop edge
  // texels when the destination dimensions use integer truncation.
  const auto oddSource = solid(5, 3, 73, 121, 199, 251);
  const auto odd = aurora::gfx::reduce_rgba_texture(5, 3, 50, oddSource);
  require(odd && odd->width == 2 && odd->height == 1,
          "odd 5x3 half-size dimensions are incorrect");
  require(odd->pixels == solid(2, 1, 73, 121, 199, 251),
          "odd-size reduction changed a constant texture");
}

void verify_mip_chain_and_validation() {
  auto source = solid(4, 2, 20, 30, 40, 50);
  const auto level1 = solid(2, 1, 10, 11, 12, 13);
  const auto level2 = solid(1, 1, 5, 6, 7, 8);
  source.insert(source.end(), level1.begin(), level1.end());
  source.insert(source.end(), level2.begin(), level2.end());

  const auto half = aurora::gfx::reduce_rgba_mip_chain(4, 2, 3, 50, source);
  require(half && half->width == 2 && half->height == 1 && half->mips == 2,
          "half-size mip chain metadata is incorrect");
  require(half->pixels.size() == 12, "half-size mip chain storage is incorrect");
  require(std::vector<uint8_t>(half->pixels.begin(), half->pixels.begin() + 8) ==
              solid(2, 1, 20, 30, 40, 50),
          "base mip was not reduced at requested quality");
  require(std::vector<uint8_t>(half->pixels.begin() + 8, half->pixels.end()) ==
              solid(1, 1, 10, 11, 12, 13),
          "second mip was not reduced at requested quality");

  const auto original = aurora::gfx::reduce_rgba_mip_chain(4, 2, 3, 100, source);
  require(original && original->mips == 3 && original->pixels == source,
          "100% mip chain must preserve source bytes exactly");
  require(!aurora::gfx::reduce_rgba_texture(4, 2, 75, source),
          "unsupported quality percentage was accepted");
  require(!aurora::gfx::reduce_rgba_texture(4, 2, 50,
              std::span<const uint8_t>(source.data(), source.size() - 1)),
          "truncated texture storage was accepted");
  require(!aurora::gfx::reduce_rgba_mip_chain(4, 2, 4, 50, source),
          "mip count beyond the geometric chain was accepted");
  require(!aurora::gfx::reduce_rgba_mip_chain(4, 2, 3, 50,
              std::span<const uint8_t>(source.data(), source.size() - 1)),
          "truncated mip-chain storage was accepted");
}
}  // namespace

int main() {
  try {
    verify_dimensions_and_area_filter();
    verify_mip_chain_and_validation();
    std::cout << "texture_downsample_tests: PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "texture_downsample_tests: FAIL: " << error.what() << '\n';
    return 1;
  }
}
