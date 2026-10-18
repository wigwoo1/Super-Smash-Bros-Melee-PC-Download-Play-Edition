#pragma once
#include <aurora/original_texture_diagnostics.h>
#include "../gx/pipeline.hpp"
#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdio>
namespace aurora::gfx::original_texture {
inline bool enabled(){static const bool v=std::getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&std::getenv("MELEE_DIAGNOSTIC_ORIGINAL_TEXTURE_GPU");return v;}
inline uint64_t hash(const void* data,size_t bytes){const auto* p=(const uint8_t*)data;uint64_t h=14695981039346656037ULL;while(bytes--)h=(h^*p++)*1099511628211ULL;return h;}
struct Source {AuroraOriginalTexture metadata{};std::vector<uint8_t> bytes,palette;uint64_t id=0,hash=0,paletteHash=0;};
inline std::mutex registryMutex;
inline std::vector<std::shared_ptr<const Source>> registry;
inline std::map<std::pair<unsigned,unsigned>,std::shared_ptr<const Source>> objectSources;
inline std::map<const void*,std::shared_ptr<const Source>> currentSources;
inline uint64_t nextSource=0;
inline bool valid(const AuroraOriginalTexture& s){
  if(!s.data||!s.original||!s.owner||!s.bodyHash||!s.width||!s.height||s.width>512||s.height>512)return false;
  if(s.format==14)return s.bytes==size_t((s.width+7)/8)*((s.height+7)/8)*32&&!s.paletteData&&!s.paletteOriginal&&!s.paletteBytes&&!s.paletteEntries;
  return s.format==9&&s.bytes==size_t((s.width+7)/8)*((s.height+3)/4)*32&&s.paletteData&&s.paletteOriginal&&s.paletteEntries&&s.paletteEntries<=256&&s.paletteBytes==s.paletteEntries*2&&s.paletteFormat<=2;
}
inline bool publish(const AuroraOriginalTexture& s){
  if(!enabled()||!valid(s))return false;const auto fingerprint=hash(s.original,s.bytes),paletteFingerprint=s.paletteBytes?hash(s.paletteOriginal,s.paletteBytes):0;const std::lock_guard lock(registryMutex);
  for(const auto& old:registry)if(old->metadata.owner==s.owner&&old->metadata.data==s.data&&old->metadata.bodyHash==s.bodyHash&&old->metadata.textureOffset==s.textureOffset&&old->metadata.imageOffset==s.imageOffset&&old->metadata.paletteData==s.paletteData){
    const bool same=old->hash==fingerprint&&old->paletteHash==paletteFingerprint&&old->metadata.width==s.width&&old->metadata.height==s.height&&old->metadata.format==s.format&&old->metadata.paletteFormat==s.paletteFormat&&old->metadata.paletteEntries==s.paletteEntries&&old->metadata.paletteOffset==s.paletteOffset;if(same)currentSources[s.owner]=old;return same;
  }
  if(registry.size()>=128)return false;auto source=std::make_shared<Source>();source->metadata=s;
  source->metadata.original=source->metadata.paletteOriginal=nullptr;source->bytes.assign((const uint8_t*)s.original,(const uint8_t*)s.original+s.bytes);if(s.paletteBytes)source->palette.assign((const uint8_t*)s.paletteOriginal,(const uint8_t*)s.paletteOriginal+s.paletteBytes);source->hash=fingerprint;source->paletteHash=paletteFingerprint;source->id=++nextSource;registry.push_back(source);
  currentSources[s.owner]=source;
  if(s.paletteBytes)std::fprintf(stderr,"ORIGINAL_PALETTE_REGISTERED source=%llu texture=%u image=%u palette=%u format=%u entries=%u hash=%llu\n",(unsigned long long)source->id,s.textureOffset,s.imageOffset,s.paletteOffset,s.paletteFormat,s.paletteEntries,(unsigned long long)paletteFingerprint);
  std::fprintf(stderr,"ORIGINAL_TEXTURE_REGISTERED source=%llu texture=%u image=%u width=%u height=%u bytes=%u hash=%llu\n",(unsigned long long)source->id,s.textureOffset,s.imageOffset,s.width,s.height,s.bytes,(unsigned long long)fingerprint);return true;
}
inline std::shared_ptr<const Source> find(const void* data){const std::lock_guard lock(registryMutex);for(auto it=registry.rbegin();it!=registry.rend();++it)if((*it)->metadata.data==data)return *it;return {};}
inline void forget(const void* owner){const std::lock_guard lock(registryMutex);currentSources.erase(owner);for(auto it=registry.begin();it!=registry.end();)if((*it)->metadata.owner==owner)it=registry.erase(it);else ++it;for(auto it=objectSources.begin();it!=objectSources.end();)if(it->second->metadata.owner==owner)it=objectSources.erase(it);else ++it;}
inline bool tag(unsigned object,unsigned version,const void* data,const void* owner){
  if(!object||!version)return false;const std::lock_guard lock(registryMutex);
  const auto found=currentSources.find(owner);if(found==currentSources.end()||found->second->metadata.data!=data)return false;const auto& source=found->second;
  const auto key=std::make_pair(object,version);if(auto it=objectSources.find(key);it!=objectSources.end())return it->second==source;
  if(objectSources.size()>=32768)return false;objectSources.emplace(key,std::move(source));return true;
}
inline std::shared_ptr<const Source> find_object(unsigned object,unsigned version){const std::lock_guard lock(registryMutex);auto it=objectSources.find({object,version});return it==objectSources.end()?nullptr:it->second;}
struct Snapshot {gx::DrawData draw{};std::shared_ptr<const Source> source;uint32_t slot=0;uint64_t sequence=0;gx::ShaderConfig config{};uint32_t firstVertex=0;};
}
