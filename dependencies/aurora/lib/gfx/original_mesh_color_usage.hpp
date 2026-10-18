#pragma once
#include "../gx/gx.hpp"
namespace aurora::gx::mesh_diagnostic {
// Track actual raster lanes read by TEV,including raster swizzles. HSD need
// not configure alpha1 when a specular stage reads only channel1 RGB.
inline uint32_t color_usage(const ShaderConfig& config,const ShaderInfo* info=nullptr){
  uint32_t lanes=0;
  for(unsigned i=0;i<config.tevStageCount;i++){
    const auto& stage=config.tevStages[i];unsigned channel;
    switch(stage.channelId){case GX_COLOR0:case GX_ALPHA0:case GX_COLOR0A0:channel=0;break;
      case GX_COLOR1:case GX_ALPHA1:case GX_COLOR1A1:channel=1;break;default:continue;}
    const auto& swap=config.tevSwapTable[stage.tevSwapRas];
    auto lane=[&](unsigned n){if(n<4)lanes|=1u<<(channel*4+n);};
    for(auto a:{stage.colorPass.a,stage.colorPass.b,stage.colorPass.c,stage.colorPass.d}){
      if(a==GX_CC_RASC){lane(swap.red);lane(swap.green);lane(swap.blue);}else if(a==GX_CC_RASA)lane(swap.alpha);
    }
    for(auto a:{stage.alphaPass.a,stage.alphaPass.b,stage.alphaPass.c,stage.alphaPass.d})if(a==GX_CA_RASA)lane(swap.alpha);
  }
  // SRTG reads the lit R/G lanes without a TEV raster operand or swap.
  // Use the submitted shader's complete coordinate dependency set.
  if(info)for(unsigned i=0;i<MaxTexCoord;i++)if(info->sampledTexCoords.test(i)){
    const auto& tcg=config.tcgs[i];
    if(tcg.type==GX_TG_SRTG&&(tcg.src==GX_TG_COLOR0||tcg.src==GX_TG_COLOR1))
      lanes|=3u<<(tcg.src==GX_TG_COLOR1?4:0);
  }
  return lanes;
}
}
