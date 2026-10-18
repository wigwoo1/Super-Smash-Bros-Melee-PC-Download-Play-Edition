#pragma once
#include <string_view>
namespace aurora::gx {
inline constexpr std::string_view TevCompareWGSL=R"(
fn gx_tev_compare_test(a:vec3i,b:vec3i,mode:u32,eq:bool) -> bool {
  var aa=a.x; var bb=b.x;
  if(mode==1u) {aa=a.x+a.y*256;bb=b.x+b.y*256;}
  if(mode==2u) {aa=a.x+a.y*256+a.z*65536;bb=b.x+b.y*256+b.z*65536;}
  return select(aa>bb,aa==bb,eq);
}
fn gx_tev_compare3(a:vec3f,b:vec3f,c:vec3f,d:vec3f,mode:u32,eq:bool) -> vec3f {
  let aa=vec3i(round(a*255.0)) & vec3i(255);
  let bb=vec3i(round(b*255.0)) & vec3i(255);
  let cc=vec3i(round(c*255.0)) & vec3i(255);
  let dd=vec3i(round(d*255.0));
  if(mode==3u) {return vec3f(dd+select(vec3i(0),cc,select(aa>bb,aa==bb,eq)))/255.0;}
  return vec3f(dd+select(vec3i(0),cc,gx_tev_compare_test(aa,bb,mode,eq)))/255.0;
}
fn gx_tev_compare1(a:vec3f,b:vec3f,aa:f32,ba:f32,c:f32,d:f32,mode:u32,eq:bool) -> f32 {
  let ai=vec3i(round(a*255.0)) & vec3i(255);
  let bi=vec3i(round(b*255.0)) & vec3i(255);
  var test=gx_tev_compare_test(ai,bi,mode,eq);
  if(mode==3u) {
    let av=i32(round(aa*255.0)) & 255;let bv=i32(round(ba*255.0)) & 255;
    test=select(av>bv,av==bv,eq);
  }
  return f32(i32(round(d*255.0))+select(0,i32(round(c*255.0)) & 255,test))/255.0;
}
)";
}
