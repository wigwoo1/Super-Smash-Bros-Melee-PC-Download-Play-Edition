#include "../lib/graphics_quality_scope.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <set>
#include <thread>
using namespace aurora::graphics_quality;
int main() {
  assert(!ProcessingContext::active());
  { const ProcessingContext outer;assert(ProcessingContext::active());
    { const ProcessingContext nested;assert(ProcessingContext::active()); }
    bool independent=false;std::thread other([&]{independent=!ProcessingContext::active();});other.join();
    assert(independent && ProcessingContext::active()); }
  assert(!ProcessingContext::active());
  SceneScope scene;DrawScope draw;
  unsigned combinations=0,refusals=0;
  for(unsigned quality:{100u,50u,25u})for(bool light:{false,true})for(int combat:{0,1}) {
    assert(scene.set(true,true,combat));const auto frame=scene.snapshot({quality,light});
    draw.begin(frame);assert(draw.current()==frame);++combinations;
    // Unowned, idle and display-list calls must be rejected without mutation.
    for(const auto flags:std::array<std::array<bool,3>,3>{{{false,true,false},{true,false,false},{true,true,true}}}) {
      assert(!draw.push(flags[0],flags[1],flags[2]));++refusals;
      assert(draw.depth()==0 && draw.current()==frame);
    }
    assert(!draw.pop(true,true,false));++refusals;
    for(unsigned depth=1;depth<=8;++depth) {
      assert(draw.push(true,true,false));
      assert(draw.depth()==depth && draw.current().texturePercent==100 && draw.current().lightingEnabled);
      assert(draw.current().requested==frame.requested && draw.current().gameplay==frame.gameplay);
    }
    assert(!draw.push(true,true,false));++refusals;
    for(const auto flags:std::array<std::array<bool,3>,3>{{{false,true,false},{true,false,false},{true,true,true}}}) {
      assert(!draw.pop(flags[0],flags[1],flags[2]));++refusals;
      assert(draw.depth()==8 && draw.current().texturePercent==100);
    }
    for(unsigned depth=7;depth>0;--depth) {
      assert(draw.pop(true,true,false));assert(draw.depth()==depth && draw.current().texturePercent==100);
    }
    const auto retainedHud=draw.current();assert(draw.pop(true,true,false));
    assert(draw.depth()==0 && draw.current()==frame);
    assert(retainedHud.texturePercent==100 && retainedHud.lightingEnabled);
    assert(scene.snapshot({quality,light})==frame);
  }
  // Retained draw snapshots survive later frames, preferences and cancellation.
  draw.begin(scene.snapshot({25,false}));assert(draw.push(true,true,false));
  const auto retained=draw.current();draw.begin({});
  assert(draw.depth()==0 && retained.texturePercent==100 && retained.requested.texturePercent==25);
  // Shared GX image identity must have distinct variants even for the high-bit IDs.
  std::set<uint64_t> identities;
  for(unsigned id:{1u,2u,0x7fffffffu,0x80000000u,0xffffffffu})
    for(unsigned quality:{100u,50u,25u})assert(identities.insert(texture_variant_key(id,quality)).second);
  // Two queued requests for the same asset retain independent desired qualities.
  const std::array<TextureLoadKey,3> jobs{{{0xffffffffffffffffull,100},{0xffffffffffffffffull,25},{1,25}}};
  assert(jobs[0]!=jobs[1] && jobs[1]!=jobs[2]);
  draw.begin(scene.snapshot({50,true}));
  assert(jobs[0].quality==100 && jobs[1].quality==25);
  assert(combinations==12 && refusals==96 && identities.size()==15);
  std::puts("HUD_QUALITY_SCOPE_PASS combinations=12 refusals=96 retained_snapshots=1 cache_identities=15");
}
