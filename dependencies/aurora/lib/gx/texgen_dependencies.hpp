#pragma once
#include "gx.hpp"
namespace aurora::gx {
// TEV may read only an emboss result whose source is itself unreferenced by
// TEV. Resolve the full coordinate dependency set before sizing uniforms.
inline void resolve_texgen_dependencies(const ShaderConfig& config,ShaderInfo& info){
  for(unsigned pass=0;pass<MaxTexCoord;pass++){
    const auto before=info.sampledTexCoords;
    for(unsigned i=0;i<MaxTexCoord;i++)if(info.sampledTexCoords.test(i)){
      const auto& tcg=config.tcgs[i];
      if(tcg.type>=GX_TG_BUMP0&&tcg.type<=GX_TG_BUMP7){
        CHECK(tcg.embossSrc<MaxTexCoord,"Invalid emboss source");
        info.sampledTexCoords.set(tcg.embossSrc);
        if(!config.lightingDisabled)info.lightingEnabled=true;
      }
    }
    if(before==info.sampledTexCoords)break;
  }
  for(unsigned i=0;i<MaxTexCoord;i++)if(info.sampledTexCoords.test(i)){
    const auto& tcg=config.tcgs[i];
    if(tcg.type==GX_TG_SRTG){
      CHECK(tcg.src==GX_TG_COLOR0||tcg.src==GX_TG_COLOR1,"Invalid SRTG color source");
      info.sampledColorChannels.set(tcg.src==GX_TG_COLOR1?1:0);
    }
  }
}
}
