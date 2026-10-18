#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace aurora {
// Single-owner cache for plain computed state. Fixed storage; hashes select a
// bucket but never establish identity. Keys are compared fully, even on hash
// collisions. Values returned by find remain valid only until the next insert.
template<class Key,class Value,size_t Buckets=16,size_t Ways=4>
class BoundedStateCache {
  static_assert(Buckets && Ways);
  struct Entry { Key key{}; Value value{}; uint64_t hash=0; bool valid=false; };
  std::array<std::array<Entry,Ways>,Buckets> entries{};
  std::array<size_t,Buckets> next{};
public:
  static constexpr size_t capacity=Buckets*Ways;
  const Value* find(uint64_t hash,const Key& key) const {
    for(const auto& entry:entries[hash%Buckets])
      if(entry.valid && entry.hash==hash && entry.key==key)return &entry.value;
    return nullptr;
  }
  void insert(uint64_t hash,const Key& key,const Value& value) {
    const size_t bucket=hash%Buckets;
    for(auto& entry:entries[bucket])if(entry.valid && entry.hash==hash && entry.key==key){entry.value=value;return;}
    for(auto& entry:entries[bucket])if(!entry.valid){entry={key,value,hash,true};return;}
    entries[bucket][next[bucket]]={key,value,hash,true};
    next[bucket]=(next[bucket]+1)%Ways;
  }
  void clear() {
    for(auto& bucket:entries)for(auto& entry:bucket)entry.valid=false;
    next.fill(0);
  }
};
} // namespace aurora
