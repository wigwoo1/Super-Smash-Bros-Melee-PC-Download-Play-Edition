#pragma once
#include "frame_packet.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
namespace aurora::gfx::draw_readiness_diagnostic {
struct Counts {unsigned ready=0,missing=0;uint64_t skippedVertices=0;};
inline uint64_t currentFrame=0,totalMissing=0;
inline std::map<uint64_t,Counts> frames;
inline std::set<uint64_t> missed,retired;
inline bool enabled(){return std::getenv("AURORA_DIAGNOSTIC_DRAW_READINESS");}
inline void begin(uint64_t frame){
 if(!enabled())return;currentFrame=frame;frames.try_emplace(frame);
 while(frames.size()>64)frames.erase(frames.begin());
}
inline void record(const gx::DrawData& draw,bool ready){
 if(!enabled())return;auto& c=frames[currentFrame];
 if(ready){++c.ready;
  if(missed.contains(draw.pipeline) && retired.size()<128 && retired.insert(draw.pipeline).second)
   std::fprintf(stderr,"DRAW_READINESS_FIRST_READY frame=%llu pipeline=%llu\n",static_cast<unsigned long long>(currentFrame),static_cast<unsigned long long>(draw.pipeline));
 }else{++c.missing;++totalMissing;c.skippedVertices+=draw.vtxCount;
  if(missed.size()<128 && missed.insert(draw.pipeline).second)
   std::fprintf(stderr,"DRAW_READINESS_FIRST_MISSING frame=%llu pipeline=%llu vertices=%u indices=%u\n",static_cast<unsigned long long>(currentFrame),static_cast<unsigned long long>(draw.pipeline),draw.vtxCount,draw.indexCount);
 }
}
inline void capture(uint64_t frame){
 if(!enabled())return;auto it=frames.find(frame);Counts c=it==frames.end()?Counts{}:it->second;
 std::fprintf(stderr,"DRAW_READINESS_CAPTURE frame=%llu available=%u ready=%u missing=%u skipped_vertices=%llu cumulative_missing=%llu\n",static_cast<unsigned long long>(frame),unsigned(it!=frames.end()),c.ready,c.missing,static_cast<unsigned long long>(c.skippedVertices),static_cast<unsigned long long>(totalMissing));
}
}
