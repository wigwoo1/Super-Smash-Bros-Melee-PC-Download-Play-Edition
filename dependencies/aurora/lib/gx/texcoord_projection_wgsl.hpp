#pragma once
#include <string_view>
#include <string>
#include <fmt/format.h>
#include <dolphin/gx/GXEnum.h>
namespace aurora::gx {
inline std::string indexed_texcoord_transform_wgsl(unsigned index) {
  return fmt::format(
      "\n    var tc{0}_tmp = tc{0}.xyz;"
      "\n    if (in_texmtxidx{0} != {1}u) {{"
      "\n        tc{0}_tmp = gx_transform_texcoord(tc{0}, ubuf.postex_mtx[in_texmtxidx{0} / 3u]);"
      "\n    }}", index, static_cast<unsigned>(GX_IDENTITY));
}
// GX zero-Q behavior, independently expressed; see rendering progress references.
inline constexpr std::string_view TexcoordProjectionWGSL=R"(
fn gx_transform_texcoord(coord: vec4f, matrix: mat3x4f) -> vec3f {
  return coord * matrix;
}
fn gx_texgen_finish(coord: vec3f) -> vec3f {
  if (coord.z == 0.0) {
    return vec3f(clamp(coord.xy * 0.5, vec2f(-1.0), vec2f(1.0)), coord.z);
  }
  return coord;
}
fn gx_project_texcoord(coord: vec3f) -> vec2f {
  // Change the denominator before division, so neither branch evaluates /0.
  return coord.xy / select(coord.z, 1.0, coord.z == 0.0);
}
fn gx_scale_texcoord(coord: vec2f, scale: vec2f, original_size: vec2f) -> vec2f {
  return (coord * scale) / original_size;
}
)";
}
