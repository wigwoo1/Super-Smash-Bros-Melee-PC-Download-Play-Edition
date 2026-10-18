#pragma once
#include <string_view>
namespace aurora::gx {
// Independently expressed GX regular-stage integer arithmetic.
inline constexpr std::string_view TevRegularWGSL=R"(
fn gx_tev_regular_i32(a: i32,b: i32,c: i32,d: i32,sub: bool,bias: i32,scale: u32) -> i32 {
  let aa=a & 255; let bb=b & 255; let cc=c & 255;
  let factor=cc + (cc >> 7u);
  let shift=select(scale,0u,scale==3u);
  let rounding=select(select(128,127,sub),0,scale==3u);
  let interpolation=(((aa*256+(bb-aa)*factor)<<shift)+rounding)>>8u;
  var result=((d+bias)<<shift)+select(interpolation,-interpolation,sub);
  if(scale==3u) {result=result>>1u;}
  return result;
}
fn gx_tev_regular1(a:f32,b:f32,c:f32,d:f32,sub:bool,bias:i32,scale:u32) -> f32 {
  return f32(gx_tev_regular_i32(i32(round(a*255.0)),i32(round(b*255.0)),
                              i32(round(c*255.0)),i32(round(d*255.0)),sub,bias,scale))/255.0;
}
fn gx_tev_regular3(a:vec3f,b:vec3f,c:vec3f,d:vec3f,sub:bool,bias:i32,scale:u32) -> vec3f {
  return vec3f(gx_tev_regular1(a.x,b.x,c.x,d.x,sub,bias,scale),
               gx_tev_regular1(a.y,b.y,c.y,d.y,sub,bias,scale),
               gx_tev_regular1(a.z,b.z,c.z,d.z,sub,bias,scale));
}
)";
}
