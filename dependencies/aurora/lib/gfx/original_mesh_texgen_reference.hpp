#pragma once
#include "../gx/gx.hpp"
#include <aurora/mesh_diagnostics.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace aurora::gx::mesh_diagnostic {
struct TexgenUniformResult {bool valid=true;const char* reason="ok";unsigned mask=0,components=0;double maximum=0;};
inline TexgenUniformResult texgen_uniforms(const void* bytes,size_t size,const ShaderConfig& config,const ShaderInfo& info,const AuroraOriginalMesh& source){
  TexgenUniformResult result;size_t offset=80+30*48+info.loadsTevReg.count()*16;
  if(info.lightingEnabled)offset+=8*80+16;
  for(unsigned i=0;i<2;i++)if(info.sampledColorChannels.test(i))for(unsigned alpha=0;alpha<2;alpha++){
    const auto& channel=config.colorChannels[i+alpha*2];
    if(channel.lightingEnabled&&channel.ambSrc==GX_SRC_REG)offset+=16;
    if(channel.matSrc==GX_SRC_REG)offset+=16;
  }
  offset+=info.sampledKColors.count()*16;
  auto fail=[&](const char* why){result.valid=false;result.reason=why;};
  for(unsigned coord=0;coord<8;coord++)if((source.texgenMask&(1u<<coord))&&info.sampledTexCoords.test(coord)){
    const auto& ref=source.texgen[coord];const auto& tcg=config.tcgs[coord];
    if(ref.source>=8||!(source.uvMask&(1u<<ref.source))||ref.postMatrix<64||ref.postMatrix>121||(ref.postMatrix-64)%3){fail("texgen-reference");return result;}
    const unsigned slot=(ref.postMatrix-64)/3;
    if(tcg.type!=GX_TG_MTX2x4||tcg.src!=GX_TG_TEX0+ref.source||tcg.mtx!=GX_IDENTITY||tcg.normalize||tcg.postMtx!=ref.postMatrix||
       config.attrs[GX_VA_TEX0MTXIDX+coord].attrType!=GX_NONE||!info.usesPTTexMtx.test(slot)){fail("hsd-texgen-config");return result;}
    const size_t at=offset+slot*48;
    if(!bytes||at>size||48>size-at){fail("texgen-uniform-range");return result;}
    for(unsigned c=0;c<12;c++){
      float actual;std::memcpy(&actual,static_cast<const unsigned char*>(bytes)+at+c*4,4);
      const double expected=ref.matrix[c],error=std::abs(double(actual)-expected);result.components++;
      if(std::isfinite(error))result.maximum=std::max(result.maximum,error);
      if(!std::isfinite(actual)||!std::isfinite(expected)||error>1e-6*(1+std::abs(expected))){fail("hsd-texgen-upload");return result;}
    }
    result.mask|=1u<<coord;
  }
  return result;
}
inline bool texgen_reference(const AuroraMeshTexgen& ref,const AuroraMeshPoint& point,double out[2]){
  if(ref.source>=8)return false;
  double projected[3]{};
  for(unsigned row=0;row<3;row++){
    const auto* m=ref.matrix+row*4;
    projected[row]=m[0]*point.uv[ref.source][0]+m[1]*point.uv[ref.source][1]+m[2]+m[3];
    if(!std::isfinite(projected[row]))return false;
  }
  // Ordinary HSD UV uses identity main matrix,2x4 (Q=1),then its 3x4
  // post matrix. ST outputs are not divided by Q;zero-Q finish still applies.
  for(unsigned c=0;c<2;c++)out[c]=projected[2]==0?std::clamp(projected[c]*.5,-1.,1.):projected[c];
  return true;
}
}
