#pragma once
#include "frame_packet.hpp"
#include "raster_reference.hpp"
#include "texture_sample_reference.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
namespace aurora::gfx::occlusion_pair_diagnostic {
struct Pair {
  unsigned first=0,second=0;
  std::vector<unsigned> offsets;
  std::vector<std::array<double,4>> nearestColour,farColour;
  std::vector<double> nearestDepth,farDepth;
  bool wholeDrawVisible=false;
  bool wholeOnly=false;
};
inline std::set<uint64_t> found;
inline std::set<uint64_t> wholeSubmitted;
inline std::map<uint64_t,unsigned> attempts;
inline std::set<uint64_t> sweptUvStates;
inline bool finished(uint64_t shader) {
  if(shader==0x3f73e0adull && std::getenv("AURORA_DIAGNOSTIC_UV_STATE_SWEEP"))
    return sweptUvStates.size()>=8;
  return found.contains(shader);
}
struct WholeDrawRefusals {unsigned incomplete=0,ambiguous=0,obscured=0,bounds=0,invalidClip=0,degenerate=0;unsigned maxVertices=0,maxIndices=0;};
inline std::map<uint64_t,WholeDrawRefusals> wholeDrawRefusals;
// Metadata-only feasibility survey; never an occlusion pixel acceptance test.
template<typename Consumer>
inline void survey(const detail::FramePacket& frame,const detail::DiagnosticReplayDraw& snapshot,unsigned vertices,
                   const std::array<double,1024>& positions,const std::array<float,768>& coords,
                   bool normal,const std::array<int,768>& first,int factor,Consumer consume) {
  const auto& draw=snapshot.draw;
  if(!std::getenv("AURORA_DIAGNOSTIC_OCCLUSION_SURVEY") || finished(draw.diagnosticShader) || attempts[draw.diagnosticShader]>=128 || !draw.indexCount)return;
  const bool sweep=draw.diagnosticShader==0x3f73e0adull && std::getenv("AURORA_DIAGNOSTIC_UV_STATE_SWEEP");
  if(sweep && (snapshot.vertexStreamHash!=9251992207170105361ull ||
               snapshot.vertexStreamBytes!=640 || !snapshot.uniformHash ||
               sweptUvStates.contains(snapshot.uniformHash)))return;
  const auto attempt=++attempts[draw.diagnosticShader];
  // Report exhaustion even when the final candidate is refused by an input
  // guard. Missing metadata must not masquerade as a completed survey.
  struct ExhaustionReport {
    uint64_t shader;
    unsigned attempt;
    ~ExhaustionReport() {
      if(attempt==128 && !found.contains(shader))
        std::fprintf(stderr,"OCCLUSION_PAIR_UNAVAILABLE shader=%llu attempts=128 scope=bounded_original_geometry_only\n",static_cast<unsigned long long>(shader));
      if(attempt==128 && std::getenv("AURORA_DIAGNOSTIC_WHOLE_DRAW")) {
        const auto& r=wholeDrawRefusals[shader];
        std::fprintf(stderr,"WHOLE_DRAW_REFUSALS shader=%llu incomplete=%u ambiguous=%u obscured=%u max_vertices=%u max_indices=%u scope=cpu_eligibility_only\n",static_cast<unsigned long long>(shader),r.incomplete,r.ambiguous,r.obscured,r.maxVertices,r.maxIndices);
        std::fprintf(stderr,"WHOLE_DRAW_GEOMETRY_REFUSALS shader=%llu bounds=%u invalid_clip=%u degenerate=%u scope=cpu_eligibility_only\n",static_cast<unsigned long long>(shader),r.bounds,r.invalidClip,r.degenerate);
      }
    }
  } exhaustion{draw.diagnosticShader,attempt};
  if(!snapshot.pixels)return;
  const auto& sampler=snapshot.sampler;
  using texture_sample_reference::Wrap;
  auto wrap=[](wgpu::AddressMode mode,Wrap& out) {
    if(mode==wgpu::AddressMode::ClampToEdge)out=Wrap::Clamp;
    else if(mode==wgpu::AddressMode::Repeat)out=Wrap::Repeat;
    else if(mode==wgpu::AddressMode::MirrorRepeat)out=Wrap::Mirror;
    else return false;return true;
  };
  Wrap s{},t{};
  if(!wrap(sampler.addressModeU,s) || !wrap(sampler.addressModeV,t) || sampler.magFilter!=sampler.minFilter || sampler.maxAnisotropy!=1)return;
  const size_t offset=(normal?1568:1520)+960,base=draw.uniformRange.offset;
  if(draw.uniformRange.size<offset+144 || base+offset+144>frame.uniforms.size())return;
  float scales[2],dimensions[2];std::memcpy(scales,frame.uniforms.data()+base+offset,8);std::memcpy(dimensions,frame.uniforms.data()+base+offset+128,8);
  if(dimensions[0]<=0 || dimensions[1]<=0)return;
  auto colour=[&](const std::array<uint16_t,3>& indices,const std::array<double,3>& weights,std::array<double,3>& result) {
    double coord[3]{},vertexColour[3]{};
    for(unsigned n=0;n<3;++n) {
      for(unsigned c=0;c<(normal?3u:2u);++c)coord[c]+=weights[n]*coords[indices[n]*(normal?3:2)+c];
      for(unsigned c=0;c<3;++c)vertexColour[c]+=weights[n]*first[indices[n]*3+c]/255.;
    }
    const double q=normal && coord[2]!=0?coord[2]:1;
    std::array<double,4> sample{};
    if(!texture_sample_reference::sample(snapshot.pixels->data(),snapshot.pixels->size(),snapshot.width,snapshot.height,coord[0]/q*scales[0]/dimensions[0],coord[1]/q*scales[1]/dimensions[1],s,t,sampler.magFilter==wgpu::FilterMode::Linear?texture_sample_reference::Filter::Linear:texture_sample_reference::Filter::Nearest,sample))return false;
    auto byte=[](double v){return std::clamp(int(std::floor(v*255+.5)),0,255);};
    for(unsigned c=0;c<3;++c) {
      const int tex=byte(sample[c]),a=normal?byte(vertexColour[c]):0,b=normal?tex:byte(vertexColour[c]);
      const int f=normal?factor:tex,expanded=f+f/128;
      result[c]=double((a*(256-expanded)+b*expanded+128)/256)/255.;
    }
    return true;
  };
  struct Triangle {std::array<std::array<double,4>,3> clip{};double left=256,right=0,top=256,bottom=0;unsigned number=0;std::array<uint16_t,3> indices{};};
  std::vector<Triangle> triangles;
  const bool wholeDraw=std::getenv("AURORA_DIAGNOSTIC_WHOLE_DRAW")!=nullptr;
  if(wholeDraw) {
    auto& r=wholeDrawRefusals[draw.diagnosticShader];
    r.maxVertices=std::max(r.maxVertices,draw.vtxCount);r.maxIndices=std::max(r.maxIndices,draw.indexCount);
  }
  bool completeGeometry=draw.vtxCount<=vertices && draw.indexCount%3==0 && draw.indexCount/3<=4096;
  if(wholeDraw && !completeGeometry)++wholeDrawRefusals[draw.diagnosticShader].bounds;
  for(unsigned t=0;t<std::min(draw.indexCount/3,wholeDraw?4096u:64u);++t) {
    const size_t at=size_t(draw.idxRange.offset)+t*6;
    if(t*6+6>draw.idxRange.size || at+6>frame.indices.size()){completeGeometry=false;if(wholeDraw)++wholeDrawRefusals[draw.diagnosticShader].bounds;break;}
    uint16_t indices[3];std::memcpy(indices,frame.indices.data()+at,6);
    if(indices[0]>=vertices || indices[1]>=vertices || indices[2]>=vertices){completeGeometry=false;if(wholeDraw)++wholeDrawRefusals[draw.diagnosticShader].bounds;continue;}
    Triangle triangle;triangle.number=t;std::copy(std::begin(indices),std::end(indices),triangle.indices.begin());
    for(unsigned n=0;n<3;++n)for(unsigned c=0;c<4;++c)triangle.clip[n][c]=positions[indices[n]*4+c];
    // The original GPU index buffer is unchanged; zero-area triangles cannot
    // contribute fragments and need not make CPU visibility incomplete.
    if(wholeDraw && raster_reference::zeroArea(triangle.clip))continue;
    bool clockwise=false;if(!raster_reference::clockwise(triangle.clip,clockwise)){
      completeGeometry=false;
      if(wholeDraw) {
        bool valid=true;
        for(const auto& clip:triangle.clip) {
          for(double v:clip)valid=valid && std::isfinite(v);
          valid=valid && clip[3]>0 && clip[2]>=0 && clip[2]<=clip[3];
          if(valid)valid=std::isfinite(clip[0]/clip[3]) && std::isfinite(clip[1]/clip[3]);
        }
        auto& r=wholeDrawRefusals[draw.diagnosticShader];
        if(valid)++r.degenerate;else ++r.invalidClip;
      }
      continue;
    }if(!clockwise)continue;
    for(const auto& clip:triangle.clip) {
      const double x=(clip[0]/clip[3]+1)*128,y=(1-clip[1]/clip[3])*128;
      triangle.left=std::min(triangle.left,x);triangle.right=std::max(triangle.right,x);
      triangle.top=std::min(triangle.top,y);triangle.bottom=std::max(triangle.bottom,y);
    }
    triangles.push_back(triangle);
  }
  if(wholeDraw && !completeGeometry) {++wholeDrawRefusals[draw.diagnosticShader].incomplete;return;}
  if(wholeDraw && normal && !wholeSubmitted.contains(draw.diagnosticShader)) {
    Pair reference;reference.wholeDrawVisible=true;reference.wholeOnly=true;
    // Sparse pixel centres; every front-facing original triangle participates
    // in winner selection. Edge ambiguity and near-tied depth are refused.
    for(unsigned y=0;y<16 && reference.offsets.size()<16;++y)for(unsigned x=0;x<16 && reference.offsets.size()<16;++x) {
      const double px=x*16+8.5,py=y*16+8.5;
      bool valid=true,hit=false;double nearest=-1;std::array<double,3> winner{};
      for(const auto& triangle:triangles) {
        if(px<triangle.left || px>triangle.right || py<triangle.top || py>triangle.bottom || raster_reference::definitelyOutside(triangle.clip,px,py))continue;
        std::array<double,3> weights{},sample{};double depth=0;
        if(!raster_reference::weights(triangle.clip,256,256,px,py,weights,depth) || !colour(triangle.indices,weights,sample)){valid=false;break;}
        if(hit && std::abs(depth-nearest)<=2e-7){valid=false;break;}
        if(depth>nearest){nearest=depth;winner=sample;hit=true;}
      }
      if(!valid || !hit)continue;
      reference.offsets.push_back((unsigned(py)*256+unsigned(px))*4);
      reference.nearestColour.push_back({winner[0],winner[1],winner[2],.25});
      reference.nearestDepth.push_back(nearest);
      auto altered=[](double value){return value+(value>.5?-.125:.125);};
      reference.farColour.push_back({altered(winner[0]),altered(winner[1]),altered(winner[2]),.25});
      reference.farDepth.push_back(nearest+.01);
    }
    if(reference.offsets.size()>=4 && consume(reference))wholeSubmitted.insert(draw.diagnosticShader);
  }
  for(unsigned a=0;a<std::min<size_t>(triangles.size(),64);++a)for(unsigned b=a+1;b<std::min<size_t>(triangles.size(),64);++b) {
    const auto& first=triangles[a];const auto& second=triangles[b];
    const double left=std::max({0.,first.left,second.left}),right=std::min({256.,first.right,second.right});
    const double top=std::max({0.,first.top,second.top}),bottom=std::min({256.,first.bottom,second.bottom});
    if(right-left<1 || bottom-top<1)continue;
    Pair pair;pair.first=first.number;pair.second=second.number;
    std::set<unsigned> samples;unsigned firstNear=0,secondNear=0;double gapMin=1,gapMax=0,colourMin=1,colourMax=0;
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
      const unsigned px=unsigned(std::floor(left+(x+.5)*(right-left)/8));
      const unsigned py=unsigned(std::floor(top+(y+.5)*(bottom-top)/8));
      if(px>=256 || py>=256 || samples.contains(py*256+px))continue;
      std::array<double,3> w1{},w2{};double z1=0,z2=0;
      if(!raster_reference::weights(first.clip,256,256,px+.5,py+.5,w1,z1) || !raster_reference::weights(second.clip,256,256,px+.5,py+.5,w2,z2))continue;
      const double gap=std::abs(z1-z2);if(gap<=1e-6)continue;
      std::array<double,3> colour1{},colour2{};
      if(!colour(first.indices,w1,colour1) || !colour(second.indices,w2,colour2))continue;
      double contrast=0;for(unsigned c=0;c<3;++c)contrast=std::max(contrast,std::abs(colour1[c]-colour2[c]));
      if(contrast<=4./255)continue;
      colourMin=std::min(colourMin,contrast);colourMax=std::max(colourMax,contrast);
      const auto& nearColour=z1>z2?colour1:colour2;const auto& farColour=z1>z2?colour2:colour1;
      pair.offsets.push_back((py*256+px)*4);
      pair.nearestColour.push_back({nearColour[0],nearColour[1],nearColour[2],.25});
      pair.farColour.push_back({farColour[0],farColour[1],farColour[2],.25});
      pair.nearestDepth.push_back(std::max(z1,z2));pair.farDepth.push_back(std::min(z1,z2));
      samples.insert(py*256+px);gapMin=std::min(gapMin,gap);gapMax=std::max(gapMax,gap);
      if(z1>z2)++firstNear;else ++secondNear;
    }
    if(samples.size()<4)continue;
    if(wholeDraw) {
      bool visible=completeGeometry;
      for(unsigned p=0;p<pair.offsets.size() && visible;++p) {
        const double px=(pair.offsets[p]/4)%256+.5,py=(pair.offsets[p]/4)/256+.5;
        for(const auto& triangle:triangles) {
          if(triangle.number==pair.first || triangle.number==pair.second)continue;
          if(px<triangle.left || px>triangle.right || py<triangle.top || py>triangle.bottom)continue;
          if(raster_reference::definitelyOutside(triangle.clip,px,py))continue;
          std::array<double,3> weights{};double depth=0;
          // Bounding-box hits whose coverage is ambiguous are refused. Never
          // treat an edge/clipping refusal as proof the triangle is absent.
          if(!raster_reference::weights(triangle.clip,256,256,px,py,weights,depth)){++wholeDrawRefusals[draw.diagnosticShader].ambiguous;visible=false;break;}
          if(depth>=pair.nearestDepth[p]-2e-7){++wholeDrawRefusals[draw.diagnosticShader].obscured;visible=false;break;}
        }
      }
      if(!visible)continue;
      pair.wholeDrawVisible=true;
    }
    if(!consume(pair))return;
    if(sweep) {
      sweptUvStates.insert(snapshot.uniformHash);
      std::fprintf(stderr,"UV_STATE_SWEEP_SUBMITTED states=%zu limit=8 uniform_hash=%llu texture_hash=%llu scope=diagnostic_original_draw_states\n",sweptUvStates.size(),static_cast<unsigned long long>(snapshot.uniformHash),static_cast<unsigned long long>(snapshot.textureHash));
    }
    found.insert(draw.diagnosticShader);
    std::fprintf(stderr,"OCCLUSION_PAIR_COLOUR shader=%llu pixels=%zu min_contrast=%.9g max_contrast=%.9g scope=cpu_original_material_only\n",static_cast<unsigned long long>(draw.diagnosticShader),samples.size(),colourMin,colourMax);
    std::fprintf(stderr,"OCCLUSION_PAIR_FOUND shader=%llu first=%u second=%u pixels=%zu first_near=%u second_near=%u depth_gap_min=%.9g depth_gap_max=%.9g vertices=%u scope=cpu_original_geometry_only\n",static_cast<unsigned long long>(draw.diagnosticShader),first.number,second.number,samples.size(),firstNear,secondNear,gapMin,gapMax,vertices);return;
  }
}
}
