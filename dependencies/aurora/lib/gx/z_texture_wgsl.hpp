#pragma once
#include <string_view>
namespace aurora::gx {
// Independently implemented from the documented 24-bit GX depth arithmetic.
inline constexpr std::string_view ZTextureWGSL=R"(
fn gx_z_texture(sampled: vec4f, format: u32, operation: u32,
                bias: u32, raster_depth: f32) -> f32 {
  let bytes=vec4u(round(clamp(sampled,vec4f(0.0),vec4f(1.0))*255.0));
  var tex_depth=bytes.a;
  if (format==1u) { tex_depth=(bytes.a<<8u)|bytes.r; }
  if (format==2u) { tex_depth=(bytes.r<<16u)|(bytes.g<<8u)|bytes.b; }
  var depth=tex_depth+bias;
  if (operation==1u) {
    depth+=u32(clamp(raster_depth*16777216.0,0.0,16777215.0));
  }
  if (operation==0u) { return raster_depth; }
  return f32(depth&0xFFFFFFu)/16777216.0;
}
)";
}
