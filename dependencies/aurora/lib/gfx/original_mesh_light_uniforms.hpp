#pragma once
#include "../gx/gx.hpp"
#include "original_mesh_color_usage.hpp"
#include <aurora/mesh_diagnostics.h>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace aurora::gx::mesh_diagnostic {
struct LightUniformResult {bool valid=true;const char* reason="ok";unsigned components=0;double maximum=0;size_t firstOffset=0;};
inline LightUniformResult light_uniforms(const void* bytes,size_t size,const ShaderConfig& config,const ShaderInfo& info,const AuroraOriginalMesh& source){
  LightUniformResult result;size_t offset=80+30*48+info.loadsTevReg.count()*16;
  auto invalid=[&](const char* why){if(result.valid){result.firstOffset=offset;result.reason=why;}result.valid=false;};
  auto number=[&](double expected){
    if(offset>size||4>size-offset){invalid("light-uniform-range");return;}
    float f;std::memcpy(&f,static_cast<const unsigned char*>(bytes)+offset,4);offset+=4;result.components++;
    double error=std::abs(double(f)-expected);if(std::isfinite(error))result.maximum=std::fmax(result.maximum,error);
    if(!std::isfinite(f)||error>1e-6*(1+std::abs(expected)))invalid("light-uniform-value");
  };
  auto packed=[&](uint32_t expected){if(offset>size||4>size-offset){invalid("light-mask-range");return;}uint32_t u;std::memcpy(&u,static_cast<const unsigned char*>(bytes)+offset,4);offset+=4;result.components++;if(u!=expected)invalid("light-mask-value");};
  unsigned used=0;const auto lanes=color_usage(config,&info);
  for(unsigned channel=0;channel<2;channel++)if(info.sampledColorChannels.test(channel))for(unsigned alpha=0;alpha<2;alpha++){
    unsigned index=channel+alpha*2;
    const bool needed=lanes&(alpha?(8u<<(channel*4)):(7u<<(channel*4)));
    if(!needed)continue;
    if(!(source.channelMask&(1u<<index))){invalid("hsd-channel-invalid");return result;}
    const auto& c=source.channels[index];const auto& actual=config.colorChannels[index];
    if(actual.matSrc!=c.materialSource||actual.lightingEnabled!=bool(c.enabled)||
       (c.enabled&&(actual.ambSrc!=c.ambientSource||actual.diffFn!=c.diffuse||actual.attnFn!=c.attenuation))){
      static unsigned reports=0;if(reports++<8)std::fprintf(stderr,"MESH_LIGHT_CHANNEL_DIFF index=%u disabled=%u hsd_enable=%u gpu_enable=%u hsd_mat=%u gpu_mat=%u hsd_amb=%u gpu_amb=%u hsd_diff=%u gpu_diff=%u hsd_attn=%u gpu_attn=%u\n",index,unsigned(config.lightingDisabled),c.enabled,unsigned(actual.lightingEnabled),c.materialSource,unsigned(actual.matSrc),c.ambientSource,unsigned(actual.ambSrc),c.diffuse,unsigned(actual.diffFn),c.attenuation,unsigned(actual.attnFn));
      invalid("hsd-channel-config");return result;}
    if(c.enabled)used|=c.lightMask;
  }
  if(used&~source.lightMask){invalid("hsd-light-unavailable");return result;}
  if(info.lightingEnabled){
    for(unsigned i=0;i<8;i++){
      const auto& l=source.lights[i];
      if(used&(1u<<i)){
        for(unsigned n=0;n<3;n++)number(l.position[n]);offset+=4;
        for(unsigned n=0;n<3;n++)number(l.direction[n]);offset+=4;
        for(unsigned n=0;n<4;n++)number(l.color[n]);
        for(unsigned n=0;n<3;n++)number(l.cosine[n]);offset+=4;
        for(unsigned n=0;n<3;n++)number(l.distance[n]);offset+=4;
      }else offset+=80;
    }
    for(unsigned i=0;i<4;i++){
      if(lanes&((i<2?7u:8u)<<((i&1)*4)))packed(source.channels[i].lightMask);else offset+=4;
    }
  }
  for(unsigned channel=0;channel<2;channel++)if(info.sampledColorChannels.test(channel))for(unsigned alpha=0;alpha<2;alpha++){
    const auto& ch=source.channels[channel+alpha*2];
    const auto& actual=config.colorChannels[channel+alpha*2];
    auto color=[&](const uint8_t values[4]){for(unsigned n=0;n<4;n++){if((lanes&(1u<<(channel*4+n)))&&((!alpha&&n<3)||(alpha&&n==3)))number(values[n]/255.);else offset+=4;}};
    if(actual.lightingEnabled&&actual.ambSrc==GX_SRC_REG)color(ch.ambient);
    if(actual.matSrc==GX_SRC_REG)color(ch.material);
  }
  if(offset>size)invalid("light-uniform-tail");return result;
}
}
