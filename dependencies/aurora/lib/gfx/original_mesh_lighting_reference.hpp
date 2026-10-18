#pragma once
#include <aurora/mesh_diagnostics.h>
#include <cmath>
namespace aurora::gx::mesh_diagnostic {
inline double light_round(double v){const double lo=std::floor(v),fraction=v-lo;return fraction<.5?lo:fraction>.5?lo+1:std::fmod(lo,2)==0?lo:lo+1;}
inline bool lighting_reference(const AuroraOriginalMesh& source,const AuroraMeshPoint& point,unsigned channel,
    const double position[3],const double normal[3],double output[4],uint32_t components=15){
  for(unsigned component=0;component<4;component++){
    if(!(components&(1u<<component))){output[component]=0;continue;}
    const auto& ch=source.channels[channel+(component==3?2:0)];
    const auto* material=ch.materialSource?point.color[channel]:ch.material;
    if(!ch.enabled){output[component]=material[component]/255.;continue;}
    if(ch.lightMask&~source.lightMask)return false;
    double accumulated=(ch.ambientSource?point.color[channel]:ch.ambient)[component];
    for(unsigned i=0;i<8;i++)if(ch.lightMask&(1u<<i)){
      const auto& light=source.lights[i];double dir[3],distance2=0,attenuation=1;
      for(unsigned n=0;n<3;n++){dir[n]=light.position[n]-position[n];distance2+=dir[n]*dir[n];}
      const double distance=std::sqrt(distance2);
      for(unsigned n=0;n<3;n++)dir[n]=distance?dir[n]/distance:ch.attenuation==2?normal[n]:0;
      double dot=0;for(unsigned n=0;n<3;n++)dot+=normal[n]*dir[n];
      if(ch.attenuation!=2){
        double term=0,denominator=0,numerator=0;
        if(ch.attenuation==0){if(dot>=0)for(unsigned n=0;n<3;n++)term+=normal[n]*light.direction[n];}
        else for(unsigned n=0;n<3;n++)term+=dir[n]*light.direction[n];
        term=std::fmax(0,term);const double polynomial[3]={1,term,term*term},distances[3]={1,distance,distance2};
        for(unsigned n=0;n<3;n++){numerator+=light.cosine[n]*polynomial[n];denominator+=light.distance[n]*(ch.attenuation==0?polynomial[n]:distances[n]);}
        if(!denominator)return false;attenuation=std::fmax(0,numerator)/denominator;
      }
      const double diffuse=ch.diffuse==0?1:ch.diffuse==1?dot:std::fmax(0,dot);
      const double contribution=attenuation*diffuse*light_round(light.color[component]*255.);
      if(!std::isfinite(contribution))return false;accumulated+=light_round(contribution);
    }
    const int intensity=static_cast<int>(std::fmin(255,std::fmax(0,accumulated)));
    output[component]=((material[component]*(intensity+intensity/128))/256)/255.;
  }
  return true;
}
}
