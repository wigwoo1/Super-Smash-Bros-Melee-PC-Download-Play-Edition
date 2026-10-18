#pragma once
#include <string>
#include <cstdint>
namespace aurora::gfx::original_mesh_replay {
// Observation-only return adapter. Keep every production vertex equation and
// local expression intact. Never regenerate a parallel normal/emboss equation.
struct DirectionSource {std::string source;uint32_t mask=0;};
inline DirectionSource observe_directions(std::string source,uint32_t colors=0,uint32_t uv=0,uint32_t texgen=0){
  const auto vertex=source.find("fn vs_main("),fragment=source.find("@fragment");
  if(vertex==std::string::npos||fragment==std::string::npos||vertex>=fragment)return {};
  const std::string signature=") -> VertexOutput {",returned="return out;";
  const auto type=source.find(signature,vertex),ret=source.find(returned,vertex);
  if(type==std::string::npos||ret==std::string::npos||type>=ret||ret>=fragment||
     source.find("let nrm_tmp =",vertex)>=ret||source.find("let mv_nrm =",vertex)>=ret)return {};
  if(colors>3||uv>255||texgen>255||(texgen&&!uv))return {};
  for(unsigned i=0;i<8;i++)if((texgen&(1u<<i))&&source.find("out.tex"+std::to_string(i)+"_uv =",vertex)>=ret)return {};
  for(unsigned i=0;i<8;i++)if((uv&(1u<<i))&&source.find("let in_tex"+std::to_string(i)+"_uv =",vertex)>=ret)return {};
  for(unsigned i=0;i<2;i++)if((colors&(1u<<i))&&source.find("out.cc"+std::to_string(i),vertex)>=ret)return {};
  std::string bin="vec3f(0.0)",tan=bin;uint32_t mask=3;
  const std::string prefix="let bump_bin";auto bump=source.find(prefix,vertex);
  if(bump<ret){
    auto end=bump+prefix.size();while(end<ret&&source[end]>='0'&&source[end]<='9')end++;
    if(end==bump+prefix.size()||source.substr(end,3)!=" = ")return {};
    const auto suffix=source.substr(bump+prefix.size(),end-bump-prefix.size());
    bin="bump_bin"+suffix;tan="bump_tan"+suffix;
    if(source.find("let "+tan+" =",vertex)>=ret)return {};
    mask=15;
  }
  const auto color0=(colors&1)?"out.cc0":"vec4f(0.0)",color1=(colors&2)?"out.cc1":"vec4f(0.0)";
  std::string uvValues,uvFields;
  if(uv)for(unsigned i=0;i<8;i++){
    uvValues+=","+((uv&(1u<<i))?"vec4f(in_tex"+std::to_string(i)+"_uv,0.0,0.0)":std::string("vec4f(0.0)"));
    uvFields+="uv"+std::to_string(i)+":vec4f,";
  }
  if(texgen)for(unsigned i=0;i<8;i++){
    uvValues+=","+((texgen&(1u<<i))?"vec4f(out.tex"+std::to_string(i)+"_uv,0.0,0.0)":std::string("vec4f(0.0)"));
    uvFields+="texgen"+std::to_string(i)+":vec4f,";
  }
  source.replace(ret,returned.size(),"return MeshDirectionOutput(out.pos,vec4f(nrm_tmp,0.0),vec4f(mv_nrm,0.0),vec4f("+bin+",0.0),vec4f("+tan+",0.0),"+color0+","+color1+uvValues+");");
  source.replace(type,signature.size(),") -> MeshDirectionOutput {");
  source.insert(vertex,"struct MeshDirectionOutput {pos:vec4f,normalRaw:vec4f,normalUnit:vec4f,binormal:vec4f,tangent:vec4f,color0:vec4f,color1:vec4f,"+uvFields+"};\n");
  return {std::move(source),mask};
}
}
