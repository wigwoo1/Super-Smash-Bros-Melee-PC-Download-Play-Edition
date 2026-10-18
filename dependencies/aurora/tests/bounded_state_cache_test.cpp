#include "../lib/bounded_state_cache.hpp"
#include <cassert>
#include <cstdio>
struct Key { unsigned shader,alignment;bool diagnostic;bool operator==(const Key&) const=default; };
int main(){
  aurora::BoundedStateCache<Key,unsigned,2,2> c;
  static_assert(decltype(c)::capacity==4);
  // Deliberately force full hash collisions; the entire key must still match.
  c.insert(7,{1,256,false},11);c.insert(7,{1,128,false},12);
  assert(*c.find(7,{1,256,false})==11 && *c.find(7,{1,128,false})==12);
  assert(!c.find(7,{1,256,true}) && !c.find(7,{2,256,false}));
  c.insert(7,{1,256,false},13);assert(*c.find(7,{1,256,false})==13);
  c.insert(7,{1,256,true},14);assert(!c.find(7,{1,256,false}));
  assert(*c.find(7,{1,128,false})==12 && *c.find(7,{1,256,true})==14);
  c.insert(9,{2,256,false},15);assert(!c.find(7,{1,128,false}));
  c.insert(8,{3,256,false},16);assert(*c.find(8,{3,256,false})==16);
  assert(!c.find(6,{3,256,false})); // Same bucket, different hash is a miss.
  c.clear();assert(!c.find(8,{3,256,false}) && !c.find(9,{2,256,false}));
  // Repeated eviction remains bounded and cannot return a stale colliding key.
  for(unsigned i=0;i<1024;++i){
    c.insert(0,{i,256,false},i+1);
    assert(*c.find(0,{i,256,false})==i+1);
    if(i>1)assert(!c.find(0,{i-2,256,false}));
  }
  puts("PASS: full key equality, forced hash collisions, diagnostic/alignment variants, bounded eviction and clear");
}
