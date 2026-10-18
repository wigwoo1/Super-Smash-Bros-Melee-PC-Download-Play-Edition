#include "../lib/graphics_quality_scope.hpp"
#include <array>
#include <cassert>
#include <cstdio>
using namespace aurora::graphics_quality;
int main() {
  SceneScope scope;
  unsigned captures=0,refusals=0;
  for(uint32_t percent:std::array<uint32_t,3>{100,50,25})
    for(bool lighting:{false,true}) {
      const Preferences choice{percent,lighting};
      for(int gameplay:{0,1}) {
        assert(scope.set(true,true,gameplay));
        const auto frame=scope.snapshot(choice);++captures;
        assert(frame.requested==choice && frame.gameplay==bool(gameplay));
        assert(frame.texturePercent==(gameplay?percent:100));
        assert(frame.lightingEnabled==(gameplay?lighting:true));
        for(int rejected:{-1,2,255}) {
          assert(!scope.set(true,true,rejected));++refusals;
          assert(scope.snapshot(choice)==frame);
        }
        assert(!scope.set(false,true,!gameplay));++refusals;
        assert(!scope.set(true,false,!gameplay));++refusals;
        assert(scope.snapshot(choice)==frame);
      }
    }
  // Multiple in-flight frames retain distinct scene/preference snapshots.
  Preferences choice{25,false};
  assert(scope.set(true,true,1));const auto combat=scope.snapshot(choice);
  assert(scope.set(true,true,0));const auto menu=scope.snapshot(choice);
  choice={50,true};assert(scope.set(true,true,1));const auto second=scope.snapshot(choice);
  assert(combat.texturePercent==25 && !combat.lightingEnabled && combat.gameplay);
  assert(menu.texturePercent==100 && menu.lightingEnabled && !menu.gameplay);
  assert(menu.requested.texturePercent==25 && !menu.requested.lightingEnabled);
  assert(second.texturePercent==50 && second.lightingEnabled && second.gameplay);
  assert(combat.requested!=second.requested);
  scope.reset();assert(scope.snapshot({25,false}).gameplay);
  assert(captures==12 && refusals==60);
  std::puts("GRAPHICS_QUALITY_SCOPE_PASS captures=12 refusals=60 frame_snapshots=3 preferences_preserved=1 standalone_compatible=1");
}
