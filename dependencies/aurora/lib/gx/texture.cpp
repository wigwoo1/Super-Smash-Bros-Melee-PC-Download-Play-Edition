#include "texture.hpp"
#include "../binding_profile.hpp"
#include "../graphics_quality_scope.hpp"

#include "../gfx/recording.hpp"
#include "../gfx/original_texture_types.hpp"
#include "../gfx/tex_palette_conv.hpp"
#include "../gfx/texture_convert.hpp"
#include "../gfx/texture_replacement.hpp"
#include "shader_info.hpp"
#include "aurora/aurora.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <tracy/Tracy.hpp>
#include <xxhash.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <optional>
#include <utility>

namespace aurora::gx {
namespace {
constexpr Module Log{"aurora::gx::texture"};

struct DynamicPaletteKey {
  const void* sourceIdentity = nullptr;
  u32 width = 0;
  u32 height = 0;
  u32 format = 0;

  bool operator==(const DynamicPaletteKey& rhs) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const DynamicPaletteKey& key) {
    return H::combine(std::move(h), key.sourceIdentity, key.width, key.height, key.format);
  }
};

struct DynamicPaletteEntry {
  gfx::TextureHandle handle;
  u32 sourceRevision = 0;
  u32 tlutDataVersion = 0;
  uint64_t lastUsedFrame = 0;
};

struct CachedTextureEntry {
  gfx::TextureHandle handle;
  u32 texDataVersion = 0;
  u32 mipCount = 0;
  u32 tlutObjId = 0;
  u32 tlutDataVersion = 0;
  uint64_t replacementId = 0;
  uint64_t lastUsedFrame = 0;
};

struct CachedTlutTextureEntry {
  gfx::TextureHandle handle;
  u32 tlutDataVersion = 0;
};

struct TlutObjectCache {
  CachedTlutTextureEntry tlutTexture;
  absl::flat_hash_map<DynamicPaletteKey, DynamicPaletteEntry> dynamicPaletteTextures;
  absl::flat_hash_set<uint64_t> staticTextureUsers;
  uint64_t lastUsedFrame = 0;
};

uint64_t object_quality_key(u32 id) noexcept {
  return graphics_quality::texture_variant_key(id,aurora_get_effective_texture_quality());
}

struct TextureContentKey {
  u32 quality = 100;
  XXH128_hash_t textureHash{};
  u32 width = 0;
  u32 height = 0;
  u32 format = 0;
  u32 mipCount = 0;
  XXH128_hash_t tlutHash{};
  u32 tlutFormat = 0;
  u16 tlutEntries = 0;

  bool operator==(const TextureContentKey& rhs) const noexcept {
    return quality == rhs.quality && textureHash.low64 == rhs.textureHash.low64 && textureHash.high64 == rhs.textureHash.high64 &&
           width == rhs.width && height == rhs.height && format == rhs.format && mipCount == rhs.mipCount &&
           tlutHash.low64 == rhs.tlutHash.low64 && tlutHash.high64 == rhs.tlutHash.high64 &&
           tlutFormat == rhs.tlutFormat && tlutEntries == rhs.tlutEntries;
  }

  template <typename H>
  friend H AbslHashValue(H h, const TextureContentKey& key) {
    return H::combine(std::move(h), key.quality, key.textureHash.low64, key.textureHash.high64, key.width, key.height, key.format,
                      key.mipCount, key.tlutHash.low64, key.tlutHash.high64, key.tlutFormat, key.tlutEntries);
  }
};

struct ContentCacheEntry {
  gfx::TextureHandle handle;
  uint64_t bytes = 0;
  std::list<TextureContentKey>::iterator lruIt;
};

constexpr size_t SourceKeyCacheMaxEntries = 16384;

struct SourceKeyCacheEntry {
  aurora::texture::TextureSourceKey sourceKey;
  uint32_t minTlutIndex = UINT32_MAX;
  uint32_t maxTlutIndex = 0;
};

struct SourceKeyCacheKey {
  XXH128_hash_t hash;

  bool operator==(const SourceKeyCacheKey& rhs) const noexcept {
    return hash.low64 == rhs.hash.low64 && hash.high64 == rhs.hash.high64;
  }

  template <typename H>
  friend H AbslHashValue(H h, const SourceKeyCacheKey& key) {
    return H::combine(std::move(h), key.hash.low64, key.hash.high64);
  }
};

absl::flat_hash_map<uint64_t, CachedTextureEntry> s_textureObjectCaches;
absl::flat_hash_map<u32, TlutObjectCache> s_tlutObjectCaches;
absl::flat_hash_map<TextureContentKey, ContentCacheEntry> s_contentCache;
absl::flat_hash_map<SourceKeyCacheKey, SourceKeyCacheEntry> s_sourceKeyCache;
absl::flat_hash_map<uint64_t, absl::flat_hash_set<uint64_t>> s_replacementUsers;
std::list<TextureContentKey> s_contentLru;
uint64_t s_contentCacheBytes = 0;
uint64_t s_contentCacheBudgetBytes = texture::ContentCacheBudgetBytes;
uint64_t s_frameCount = 0;
uint64_t s_bindGeneration = 1;
std::atomic<uint64_t> s_pendingInvalidations = 0;
std::atomic<uint64_t> s_pendingCacheClears = 0;
TextureStats s_stats;

#if DEBUG
constexpr bool BuildSourceKeyForDebug = true;
#else
constexpr bool BuildSourceKeyForDebug = false;
#endif

constexpr uint32_t div_ceil(uint32_t value, uint32_t divisor) noexcept { return (value + divisor - 1) / divisor; }

void do_clear_static_texture_cache() noexcept {
  s_textureObjectCaches.clear();
  s_replacementUsers.clear();
  for (auto& [_, cache] : s_tlutObjectCaches) {
    cache.staticTextureUsers.clear();
  }
}

void apply_pending_invalidations() noexcept {
  static uint32_t appliedQuality = 100;
  const uint32_t quality = aurora_get_effective_texture_quality();
  if (quality != appliedQuality) {
    // Rebind quality-specific variants; do not flush either cache at each HUD pass.
    ++s_bindGeneration;
    appliedQuality = quality;
  }
  const uint64_t pendingCacheClears = s_pendingCacheClears.exchange(0, std::memory_order_acq_rel);
  const uint64_t pendingInvalidations = s_pendingInvalidations.exchange(0, std::memory_order_acq_rel);
  if (pendingCacheClears != 0) {
    do_clear_static_texture_cache();
  }
  s_bindGeneration += pendingCacheClears + pendingInvalidations;
  if (s_bindGeneration == 0) {
    s_bindGeneration = 1;
  }
}

DynamicPaletteKey make_dynamic_palette_key(const GXTexObj_& obj, const GXState::CopyTextureRef& source) {
  return {
      .sourceIdentity = source.handle.get(),
      .width = obj.width(),
      .height = obj.height(),
      .format = obj.format(),
  };
}

void clear_texture_dependency(uint64_t texObjId, u32 tlutObjId, uint64_t replacementId) {
  if (texObjId == 0) {
    return;
  }
  if (tlutObjId != 0) {
    if (auto it = s_tlutObjectCaches.find(tlutObjId); it != s_tlutObjectCaches.end()) {
      it->second.staticTextureUsers.erase(texObjId);
      if (!it->second.tlutTexture.handle && it->second.dynamicPaletteTextures.empty() &&
          it->second.staticTextureUsers.empty()) {
        s_tlutObjectCaches.erase(it);
      }
    }
  }
  if (replacementId != 0) {
    if (auto it = s_replacementUsers.find(replacementId); it != s_replacementUsers.end()) {
      it->second.erase(texObjId);
      if (it->second.empty()) {
        s_replacementUsers.erase(it);
      }
    }
  }
}

void clear_texture_dependency(uint64_t texObjId, const CachedTextureEntry& entry) {
  clear_texture_dependency(texObjId, entry.tlutObjId, entry.replacementId);
}

void store_cached_texture(const GXTexObj_& obj, gfx::TextureHandle handle, u32 tlutObjId = 0, u32 tlutDataVersion = 0,
                          uint64_t replacementId = 0) {
  if (obj.texObjId == 0) {
    return;
  }

  auto& entry = s_textureObjectCaches[object_quality_key(obj.texObjId)];
  if (entry.tlutObjId != tlutObjId || entry.replacementId != replacementId) {
    clear_texture_dependency(object_quality_key(obj.texObjId), entry);
  }

  entry.handle = std::move(handle);
  entry.texDataVersion = obj.texDataVersion;
  entry.mipCount = obj.mip_count();
  entry.tlutObjId = tlutObjId;
  entry.tlutDataVersion = tlutDataVersion;
  entry.replacementId = replacementId;
  entry.lastUsedFrame = s_frameCount;

  if (tlutObjId != 0) {
    auto& cache = s_tlutObjectCaches[tlutObjId];
    cache.staticTextureUsers.insert(object_quality_key(obj.texObjId));
    cache.lastUsedFrame = s_frameCount;
  }
  if (replacementId != 0) {
    s_replacementUsers[replacementId].insert(object_quality_key(obj.texObjId));
  }
}

void touch_content_cache(ContentCacheEntry& entry) {
  s_contentLru.splice(s_contentLru.begin(), s_contentLru, entry.lruIt);
  entry.lruIt = s_contentLru.begin();
}

gfx::TextureHandle find_content_texture(const TextureContentKey& key) {
  const auto it = s_contentCache.find(key);
  if (it == s_contentCache.end()) {
    return {};
  }
  touch_content_cache(it->second);
  ++s_stats.contentHits;
  return it->second.handle;
}

uint64_t texture_handle_size(const gfx::TextureHandle& handle) noexcept {
  if (!handle) {
    return 0;
  }
  return gfx::calc_texture_size(handle->format, handle->size.width, handle->size.height, handle->mipCount);
}

void cache_content_texture(TextureContentKey key, const gfx::TextureHandle& handle) {
  const uint64_t bytes = texture_handle_size(handle);
  if (!handle || bytes == 0 || bytes > s_contentCacheBudgetBytes) {
    return;
  }

  s_contentLru.push_front(key);
  const auto [it, inserted] = s_contentCache.emplace(
      std::move(key), ContentCacheEntry{.handle = handle, .bytes = bytes, .lruIt = s_contentLru.begin()});
  if (!inserted) {
    s_contentLru.pop_front();
    touch_content_cache(it->second);
    return;
  }
  s_contentCacheBytes += bytes;

  while (s_contentCacheBytes > s_contentCacheBudgetBytes && !s_contentLru.empty()) {
    const auto cacheIt = s_contentCache.find(s_contentLru.back());
    if (cacheIt != s_contentCache.end()) {
      s_contentCacheBytes -= cacheIt->second.bytes;
      s_contentCache.erase(cacheIt);
      ++s_stats.evictions;
    }
    s_contentLru.pop_back();
  }
  s_stats.contentCacheBytes = s_contentCacheBytes;
  s_stats.contentCacheEntries = s_contentCache.size();
}

struct TextureKeys {
  TextureContentKey contentKey;
  std::optional<aurora::texture::TextureSourceKey> sourceKey;
};

TextureKeys hash_texture_source(const GXTexObj_& obj, const GXTlutObj_* tlut, bool buildSourceKey) {
  ZoneScoped;
  const size_t textureBytes = texture::texture_source_size(obj.format(), obj.width(), obj.height(), obj.mip_count());
  CHECK(obj.has_data() && textureBytes != 0,
        "invalid texture source for content hash: data={} format={} width={} height={} mips={} bytes={}",
        obj.data, unsigned(obj.format()), obj.width(), obj.height(), obj.mip_count(), textureBytes);

  TextureKeys keys;
  keys.contentKey = {
      .quality = aurora_get_effective_texture_quality(),
      .textureHash = XXH3_128bits(obj.data, textureBytes),
      .width = obj.width(),
      .height = obj.height(),
      .format = obj.format(),
      .mipCount = obj.mip_count(),
  };
  s_stats.hashedBytes += textureBytes;

  uint32_t minTlutIndex = UINT32_MAX;
  uint32_t maxTlutIndex = 0;
  if (buildSourceKey) {
    if (const auto it = s_sourceKeyCache.find(SourceKeyCacheKey{keys.contentKey.textureHash});
        it != s_sourceKeyCache.end()) {
      keys.sourceKey = it->second.sourceKey;
      minTlutIndex = it->second.minTlutIndex;
      maxTlutIndex = it->second.maxTlutIndex;
      buildSourceKey = false;
    }
  }
  if (buildSourceKey) {
    const size_t baseBytes = texture::texture_source_size(obj.format(), obj.width(), obj.height(), 1);
    keys.sourceKey = aurora::texture::TextureSourceKey{
        .textureHash = XXH64(obj.data, baseBytes, 0),
        .width = obj.width(),
        .height = obj.height(),
        .format = obj.format(),
        .hasTlut = is_palette_format(obj.format()),
    };

    const auto* textureData = static_cast<const uint8_t*>(obj.data);
    switch (obj.format()) {
    case GX_TF_C4:
      for (size_t i = 0; i < baseBytes; ++i) {
        minTlutIndex = std::min(
            {minTlutIndex, static_cast<uint32_t>(textureData[i] & 0xf), static_cast<uint32_t>(textureData[i] >> 4)});
        maxTlutIndex = std::max(
            {maxTlutIndex, static_cast<uint32_t>(textureData[i] & 0xf), static_cast<uint32_t>(textureData[i] >> 4)});
      }
      break;
    case GX_TF_C8:
      for (size_t i = 0; i < baseBytes; ++i) {
        minTlutIndex = std::min(minTlutIndex, static_cast<uint32_t>(textureData[i]));
        maxTlutIndex = std::max(maxTlutIndex, static_cast<uint32_t>(textureData[i]));
      }
      break;
    case GX_TF_C14X2:
      for (size_t i = 0; i + sizeof(uint16_t) <= baseBytes; i += sizeof(uint16_t)) {
        uint16_t value = 0;
        std::memcpy(&value, textureData + i, sizeof(value));
        const uint32_t index = bswap(value) & 0x3fff;
        minTlutIndex = std::min(minTlutIndex, index);
        maxTlutIndex = std::max(maxTlutIndex, index);
      }
      break;
    default:
      break;
    }

    if (s_sourceKeyCache.size() >= SourceKeyCacheMaxEntries) {
      s_sourceKeyCache.clear();
    }
    s_sourceKeyCache.emplace(SourceKeyCacheKey{keys.contentKey.textureHash},
                             SourceKeyCacheEntry{*keys.sourceKey, minTlutIndex, maxTlutIndex});
  }

  if (tlut != nullptr) {
    const size_t tlutBytes = texture::tlut_source_size(tlut->numEntries);
    CHECK(tlut->data != nullptr && tlutBytes != 0, "invalid TLUT source for content hash");
    keys.contentKey.tlutHash = XXH3_128bits(tlut->data, tlutBytes);
    keys.contentKey.tlutFormat = static_cast<u32>(tlut->format);
    keys.contentKey.tlutEntries = tlut->numEntries;
    s_stats.hashedBytes += tlutBytes;

    if (keys.sourceKey.has_value() && minTlutIndex != UINT32_MAX) {
      const size_t tlutOffset = static_cast<size_t>(minTlutIndex) * sizeof(uint16_t);
      const size_t referencedBytes = static_cast<size_t>(maxTlutIndex + 1 - minTlutIndex) * sizeof(uint16_t);
      if (tlutOffset + referencedBytes <= tlutBytes) {
        const auto* tlutData = static_cast<const uint8_t*>(tlut->data);
        keys.sourceKey->tlutHash = XXH64(tlutData + tlutOffset, referencedBytes, 0);
      }
    }
  }
  return keys;
}

gfx::TextureHandle get_tlut_texture(const GXTlutObj_& tlut) {
  if (tlut.tlutObjId != 0) {
    auto& cache = s_tlutObjectCaches[tlut.tlutObjId];
    cache.lastUsedFrame = s_frameCount;
    if (cache.tlutTexture.handle && cache.tlutTexture.tlutDataVersion == tlut.tlutDataVersion) {
      return cache.tlutTexture.handle;
    }
    cache.dynamicPaletteTextures.clear();
    for (const uint64_t texObjId : cache.staticTextureUsers) {
      if (const auto textureIt = s_textureObjectCaches.find(texObjId); textureIt != s_textureObjectCaches.end()) {
        const uint64_t replacementId = textureIt->second.replacementId;
        s_textureObjectCaches.erase(textureIt);
        clear_texture_dependency(texObjId, 0, replacementId);
      }
    }
    cache.staticTextureUsers.clear();
  }

  const size_t sourceBytes = texture::tlut_source_size(tlut.numEntries);
  const auto handle = gfx::new_static_texture_2d(tlut.numEntries, 1, 1, gfx::tlut_texture_format(tlut.format),
                                                 {static_cast<const u8*>(tlut.data), sourceBytes}, true, "Loaded TLUT");
  if (tlut.tlutObjId != 0) {
    auto& cache = s_tlutObjectCaches[tlut.tlutObjId];
    cache.tlutTexture.handle = handle;
    cache.tlutTexture.tlutDataVersion = tlut.tlutDataVersion;
    cache.lastUsedFrame = s_frameCount;
  }
  return handle;
}

gfx::TextureHandle resolve_dynamic_palette_texture(const GXTexObj_& obj, const GXState::CopyTextureRef& source,
                                                   const GXTlutObj_& tlut) {
  ZoneScoped;

  const auto tlutHandle = get_tlut_texture(tlut);
  auto& tlutCache = s_tlutObjectCaches[tlut.tlutObjId];
  tlutCache.lastUsedFrame = s_frameCount;
  auto& entry = tlutCache.dynamicPaletteTextures[make_dynamic_palette_key(obj, source)];
  entry.lastUsedFrame = s_frameCount;
  if (!entry.handle) {
    // Use source size instead of target (logical) size.
    entry.handle = gfx::new_conv_texture(source.handle->size.width, source.handle->size.height, GX_TF_RGBA8,
                                         "GX Dynamic Palette Texture");
  }
  if (entry.sourceRevision != source.revision || entry.tlutDataVersion != tlut.tlutDataVersion) {
    gfx::queue_palette_conv({
        .variant = obj.format() == GX_TF_C4 ? gfx::tex_palette_conv::Variant::FromFloat4
                                            : gfx::tex_palette_conv::Variant::FromFloat8,
        .src = source.handle,
        .dst = entry.handle,
        .tlut = tlutHandle,
    });
    entry.sourceRevision = source.revision;
    entry.tlutDataVersion = tlut.tlutDataVersion;
  }
  return entry.handle;
}

u32 resolved_format_for_handle(const gfx::TextureHandle& handle) {
  if (!handle) {
    return GX_TF_RGBA8;
  }
  if (handle->gxFormat != gfx::InvalidTextureFormat) {
    return handle->gxFormat;
  }
  return GX_TF_RGBA8_PC;
}

void touch_bound_texture(const GXTexObj_& obj) {
  if (auto it = s_textureObjectCaches.find(object_quality_key(obj.texObjId)); it != s_textureObjectCaches.end()) {
    it->second.lastUsedFrame = s_frameCount;
    if (it->second.tlutObjId != 0) {
      if (auto tlutIt = s_tlutObjectCaches.find(it->second.tlutObjId); tlutIt != s_tlutObjectCaches.end()) {
        tlutIt->second.lastUsedFrame = s_frameCount;
      }
    }
  }

  if (!is_palette_format(obj.format()) || obj.tlut >= g_gxState.loadedTluts.size()) {
    return;
  }
  const auto copyIt = g_gxState.copyTextures.find(obj.data);
  if (copyIt == g_gxState.copyTextures.end()) {
    return;
  }
  const auto& tlut = g_gxState.loadedTluts[obj.tlut];
  if (auto tlutIt = s_tlutObjectCaches.find(tlut.tlutObjId); tlutIt != s_tlutObjectCaches.end()) {
    tlutIt->second.lastUsedFrame = s_frameCount;
    if (auto dynamicIt = tlutIt->second.dynamicPaletteTextures.find(make_dynamic_palette_key(obj, copyIt->second));
        dynamicIt != tlutIt->second.dynamicPaletteTextures.end()) {
      dynamicIt->second.lastUsedFrame = s_frameCount;
    }
  }
}

void sweep_object_caches() {
  const auto expired = [](uint64_t lastUsedFrame) {
    return s_frameCount > lastUsedFrame && s_frameCount - lastUsedFrame > texture::ObjectCacheIdleFrames;
  };

  for (auto it = s_textureObjectCaches.begin(); it != s_textureObjectCaches.end();) {
    if (expired(it->second.lastUsedFrame)) {
      const uint64_t texObjId = it->first;
      const CachedTextureEntry entry = it->second;
      s_textureObjectCaches.erase(it++);
      clear_texture_dependency(texObjId, entry);
    } else {
      ++it;
    }
  }

  for (auto cacheIt = s_tlutObjectCaches.begin(); cacheIt != s_tlutObjectCaches.end();) {
    auto& cache = cacheIt->second;
    for (auto it = cache.dynamicPaletteTextures.begin(); it != cache.dynamicPaletteTextures.end();) {
      if (expired(it->second.lastUsedFrame)) {
        cache.dynamicPaletteTextures.erase(it++);
      } else {
        ++it;
      }
    }
    if (expired(cache.lastUsedFrame) && cache.staticTextureUsers.empty() && cache.dynamicPaletteTextures.empty()) {
      s_tlutObjectCaches.erase(cacheIt++);
    } else {
      ++cacheIt;
    }
  }
}

bool use_replacement(std::optional<gfx::texture_replacement::ReplacementResult> replacement, gfx::TextureHandle& handle,
                     uint64_t& replacementId) noexcept {
  if (!replacement.has_value()) {
    return false;
  }
  handle = std::move(replacement->handle);
  replacementId = replacement->id;
  if (handle) {
    ++s_stats.replacementHits;
  }
  return true;
}
} // namespace

const TextureStats& texture_stats() noexcept { return s_stats; }

namespace texture {
size_t texture_source_size(u32 format, u32 width, u32 height, u32 mipCount) noexcept {
  if (width == 0 || height == 0 || mipCount == 0) {
    return 0;
  }

  uint64_t total = 0;
  for (u32 mip = 0; mip < mipCount; ++mip) {
    const uint64_t mipWidth = std::max(width >> mip, 1u);
    const uint64_t mipHeight = std::max(height >> mip, 1u);
    switch (format) {
    case GX_TF_R8_PC:
      total += mipWidth * mipHeight;
      break;
    case GX_TF_RG8_PC:
      total += mipWidth * mipHeight * 2;
      break;
    case GX_TF_RGBA8_PC:
      total += mipWidth * mipHeight * 4;
      break;
    case GX_TF_BC1_PC:
      total += div_ceil(static_cast<u32>(mipWidth), 4) * div_ceil(static_cast<u32>(mipHeight), 4) * 8;
      break;
    case GX_TF_I4:
    case GX_TF_C4:
    case GX_TF_CMPR:
      total += div_ceil(static_cast<u32>(mipWidth), 8) * div_ceil(static_cast<u32>(mipHeight), 8) * 32;
      break;
    case GX_TF_I8:
    case GX_TF_Z8:
    case GX_TF_IA4:
    case GX_TF_C8:
      total += div_ceil(static_cast<u32>(mipWidth), 8) * div_ceil(static_cast<u32>(mipHeight), 4) * 32;
      break;
    case GX_TF_IA8:
    case GX_TF_Z16:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
    case GX_TF_C14X2:
      total += div_ceil(static_cast<u32>(mipWidth), 4) * div_ceil(static_cast<u32>(mipHeight), 4) * 32;
      break;
    case GX_TF_RGBA8:
    case GX_TF_Z24X8:
      total += div_ceil(static_cast<u32>(mipWidth), 4) * div_ceil(static_cast<u32>(mipHeight), 4) * 64;
      break;
    default:
      return 0;
    }
  }
  CHECK(total <= SIZE_MAX, "texture source size overflow");
  return static_cast<size_t>(total);
}

size_t tlut_source_size(u16 numEntries) noexcept { return static_cast<size_t>(numEntries) * sizeof(u16); }

void invalidate_bindings() noexcept { s_pendingInvalidations.fetch_add(1, std::memory_order_release); }

uint64_t current_bind_generation() noexcept {
  apply_pending_invalidations();
  return s_bindGeneration;
}

void invalidate_replacement(uint64_t replacementId) noexcept {
  const auto users = s_replacementUsers.find(replacementId);
  if (users == s_replacementUsers.end()) {
    return;
  }

  auto texObjIds = std::move(users->second);
  s_replacementUsers.erase(users);
  for (const uint64_t texObjId : texObjIds) {
    const auto it = s_textureObjectCaches.find(texObjId);
    if (it == s_textureObjectCaches.end()) {
      continue;
    }
    const CachedTextureEntry entry = it->second;
    s_textureObjectCaches.erase(it);
    clear_texture_dependency(texObjId, entry.tlutObjId, 0);
  }
}

void trace_quality_upload(const GXTexObj_& obj, const gfx::TextureHandle& handle, bool palette) {
  if (!std::getenv("MELEE_DIAGNOSTIC_TEXTURE_QUALITY") || !handle) return;
  static uint32_t previous = 0, previousPack = 0, count = 0;
  const uint32_t quality = aurora_get_effective_texture_quality();
  const uint32_t pack = aurora_get_texture_pack();
  if (quality != previous || pack != previousPack) { previous = quality; previousPack = pack; count = 0; }
  if (++count <= 12)
    std::fprintf(stderr, "NATIVE_TEXTURE_UPLOAD quality=%u source_width=%u source_height=%u gpu_width=%u gpu_height=%u palette=%u\n",
                 quality, obj.width(), obj.height(), handle->size.width, handle->size.height, unsigned(palette));
}

gfx::TextureHandle resolve_static_texture(const GXTexObj_& obj) {
  ZoneScoped;

  if (obj.texObjId != 0) {
    if (const auto it = s_textureObjectCaches.find(object_quality_key(obj.texObjId)); it != s_textureObjectCaches.end()) {
      auto& entry = it->second;
      if (entry.handle && entry.texDataVersion == obj.texDataVersion && entry.mipCount == obj.mip_count() &&
          entry.tlutObjId == 0) {
        entry.lastUsedFrame = s_frameCount;
        ++s_stats.objectHits;
        return entry.handle;
      }
    }
  }

  gfx::TextureHandle handle;
  uint64_t replacementId = 0;
  std::optional<TextureKeys> keys;
  if (!use_replacement(gfx::texture_replacement::find_pointer_replacement(obj), handle, replacementId)) {
    const bool buildSourceKey = gfx::texture_replacement::should_build_source_key() || BuildSourceKeyForDebug;
    keys = hash_texture_source(obj, nullptr, buildSourceKey);
    if (keys->sourceKey.has_value()) {
      use_replacement(gfx::texture_replacement::find_source_replacement(obj, *keys->sourceKey), handle, replacementId);
    }
  }
  if (!handle) {
    if (!keys.has_value()) {
      keys = hash_texture_source(obj, nullptr, BuildSourceKeyForDebug);
    }
    handle = find_content_texture(keys->contentKey);
    if (!handle) {
#if DEBUG
      const auto name = gfx::texture_replacement::build_texture_replacement_name(*keys->sourceKey);
      const auto nameStr = name.c_str();
#else
      const auto nameStr = "GX Static Texture";
#endif
      const size_t sourceBytes = texture_source_size(obj.format(), obj.width(), obj.height(), obj.mip_count());
      handle = gfx::new_static_texture_2d(obj.width(), obj.height(), obj.mip_count(), obj.format(),
                                          {static_cast<const uint8_t*>(obj.data), sourceBytes}, false, nameStr,
                                          aurora_get_effective_texture_quality());
      trace_quality_upload(obj, handle, false);
      ++s_stats.misses;
      s_stats.uploadBytes += texture_handle_size(handle);
      cache_content_texture(std::move(keys->contentKey), handle);
    }
  }
  if (!obj.no_cache()) {
    store_cached_texture(obj, handle, 0, 0, replacementId);
  }
  return handle;
}

gfx::TextureHandle resolve_static_palette_texture(const GXTexObj_& obj, const GXTlutObj_& tlut) {
  ZoneScoped;

  if (obj.texObjId != 0) {
    if (const auto it = s_textureObjectCaches.find(object_quality_key(obj.texObjId)); it != s_textureObjectCaches.end()) {
      auto& entry = it->second;
      if (entry.handle && entry.texDataVersion == obj.texDataVersion && entry.tlutObjId == tlut.tlutObjId &&
          entry.tlutDataVersion == tlut.tlutDataVersion && entry.mipCount == obj.mip_count()) {
        entry.lastUsedFrame = s_frameCount;
        if (auto tlutIt = s_tlutObjectCaches.find(tlut.tlutObjId); tlutIt != s_tlutObjectCaches.end()) {
          tlutIt->second.lastUsedFrame = s_frameCount;
        }
        ++s_stats.objectHits;
        return entry.handle;
      }
    }
  }

  gfx::TextureHandle handle;
  uint64_t replacementId = 0;
  std::optional<TextureKeys> keys;
  if (!use_replacement(gfx::texture_replacement::find_pointer_replacement(obj), handle, replacementId)) {
    keys = hash_texture_source(obj, &tlut, gfx::texture_replacement::should_build_source_key());
    if (keys->sourceKey.has_value()) {
      use_replacement(gfx::texture_replacement::find_source_replacement(obj, *keys->sourceKey), handle, replacementId);
    }
  }
  if (!handle) {
    if (!keys.has_value()) {
      keys = hash_texture_source(obj, &tlut, false);
    }
    handle = find_content_texture(keys->contentKey);
    if (!handle) {
      const size_t textureBytes = texture_source_size(obj.format(), obj.width(), obj.height(), obj.mip_count());
      const size_t tlutBytes = tlut_source_size(tlut.numEntries);
      auto converted = gfx::convert_texture_palette(obj.format(), obj.width(), obj.height(), obj.mip_count(),
                                                    {static_cast<const u8*>(obj.data), textureBytes}, tlut.format,
                                                    tlut.numEntries, {static_cast<const u8*>(tlut.data), tlutBytes});
      if (converted.data.empty()) {
        return {};
      }
      handle = gfx::new_static_texture_2d(obj.width(), obj.height(), obj.mip_count(), GX_TF_RGBA8_PC,
                                          {converted.data.data(), converted.data.size()}, false,
                                          "GX Static Palette Texture", aurora_get_effective_texture_quality());
      trace_quality_upload(obj, handle, true);
      handle->hasArbitraryMips = converted.hasArbitraryMips;
      ++s_stats.misses;
      s_stats.uploadBytes += texture_handle_size(handle);
      cache_content_texture(std::move(keys->contentKey), handle);
    }
  }
  if (!obj.no_cache() && !tlut.no_cache()) {
    store_cached_texture(obj, handle, tlut.tlutObjId, tlut.tlutDataVersion, replacementId);
  }
  return handle;
}

void end_frame() noexcept {
  const auto streamingStats = gfx::texture_replacement::process_streaming();
  s_stats.pendingLoads = streamingStats.pendingLoads;
  s_stats.publishes = streamingStats.publishes;
  s_stats.publishBytes = streamingStats.publishBytes;
  TracyPlot("aurora: textureUploadBytes", static_cast<int64_t>(s_stats.uploadBytes));
  TracyPlot("aurora: textureHashedBytes", static_cast<int64_t>(s_stats.hashedBytes));
  TracyPlot("aurora: textureObjectHits", static_cast<int64_t>(s_stats.objectHits));
  TracyPlot("aurora: textureContentHits", static_cast<int64_t>(s_stats.contentHits));
  TracyPlot("aurora: textureCacheBytes", static_cast<int64_t>(s_contentCacheBytes));
  TracyPlot("aurora: textureCacheEntries", static_cast<int64_t>(s_contentCache.size()));
  TracyPlot("aurora: texturePendingReplacementLoads", static_cast<int64_t>(s_stats.pendingLoads));
  TracyPlot("aurora: textureReplacementPublishes", static_cast<int64_t>(s_stats.publishes));
  TracyPlot("aurora: textureReplacementPublishBytes", static_cast<int64_t>(s_stats.publishBytes));

  s_stats = {};
  s_stats.contentCacheBytes = s_contentCacheBytes;
  s_stats.contentCacheEntries = s_contentCache.size();
  s_stats.pendingLoads = streamingStats.pendingLoads;
  s_stats.publishes = streamingStats.publishes;
  s_stats.publishBytes = streamingStats.publishBytes;
  ++s_frameCount;
  apply_pending_invalidations();
  sweep_object_caches();
}

void shutdown() noexcept {
  s_textureObjectCaches.clear();
  s_tlutObjectCaches.clear();
  s_replacementUsers.clear();
  s_contentCache.clear();
  s_contentLru.clear();
  s_sourceKeyCache.clear();
  s_contentCacheBytes = 0;
  s_contentCacheBudgetBytes = ContentCacheBudgetBytes;
  s_frameCount = 0;
  s_bindGeneration = 1;
  s_pendingInvalidations.store(0, std::memory_order_release);
  s_pendingCacheClears.store(0, std::memory_order_release);
  s_stats = {};
}

void set_content_cache_budget_for_testing(uint64_t bytes) noexcept {
  s_contentCacheBudgetBytes = bytes;
  while (s_contentCacheBytes > s_contentCacheBudgetBytes && !s_contentLru.empty()) {
    const auto cacheIt = s_contentCache.find(s_contentLru.back());
    if (cacheIt != s_contentCache.end()) {
      s_contentCacheBytes -= cacheIt->second.bytes;
      s_contentCache.erase(cacheIt);
    }
    s_contentLru.pop_back();
  }
  s_stats.contentCacheBytes = s_contentCacheBytes;
  s_stats.contentCacheEntries = s_contentCache.size();
}
} // namespace texture

void evict_texture_object(u32 texObjId) noexcept {
  for (const u32 quality : {100u,50u,25u}) {
    const auto key=graphics_quality::texture_variant_key(texObjId,quality);
    if (const auto it=s_textureObjectCaches.find(key);it!=s_textureObjectCaches.end()) {
      const CachedTextureEntry entry=it->second;
      s_textureObjectCaches.erase(it);clear_texture_dependency(key,entry);
    }
  }
  // If there is a loaded slot with this ID, mark it as no_cache to avoid inserting it when it's resolved.
  // This also handles the case where the texture was created, loaded, and immediately destroyed before we resolved it.
  for (auto& obj : g_gxState.loadedTextures) {
    if (obj.texObjId == texObjId) {
      obj.set_no_cache(true);
    }
  }
}

void evict_tlut_object(u32 tlutObjId) noexcept {
  if (const auto it = s_tlutObjectCaches.find(tlutObjId); it != s_tlutObjectCaches.end()) {
    for (const uint64_t texObjId : it->second.staticTextureUsers) {
      if (const auto textureIt = s_textureObjectCaches.find(texObjId); textureIt != s_textureObjectCaches.end()) {
        const uint64_t replacementId = textureIt->second.replacementId;
        s_textureObjectCaches.erase(textureIt);
        clear_texture_dependency(texObjId, 0, replacementId);
      }
    }
    s_tlutObjectCaches.erase(it);
  }
  for (auto& obj : g_gxState.loadedTluts) {
    if (obj.tlutObjId == tlutObjId) {
      obj.set_no_cache(true);
    }
  }
  texture::invalidate_bindings();
}

void clear_copy_texture_cache() noexcept {
  g_gxState.copyTextures.clear();
  g_gxState.copyTextureCache.clear();
  for (auto& [_, cache] : s_tlutObjectCaches) {
    cache.dynamicPaletteTextures.clear();
  }
  texture::invalidate_bindings();
}

void clear_static_texture_cache() noexcept { s_pendingCacheClears.fetch_add(1, std::memory_order_release); }

void evict_copy_texture(const void* dest) noexcept {
  unsigned removedCache=0,removedBinding=0;
  absl::flat_hash_set<const void*> sourceIdentities;
  if (const auto it = g_gxState.copyTextures.find(dest); it != g_gxState.copyTextures.end()) {
    if (it->second.handle) {
      sourceIdentities.insert(it->second.handle.get());
    }
    g_gxState.copyTextures.erase(it);
    ++removedBinding;
  }

  for (auto it = g_gxState.copyTextureCache.begin(); it != g_gxState.copyTextureCache.end();) {
    if (it->first.dest == dest) {
      if (it->second.handle) {
        sourceIdentities.insert(it->second.handle.get());
      }
      g_gxState.copyTextureCache.erase(it++);
      ++removedCache;
    } else {
      ++it;
    }
  }

  if (!sourceIdentities.empty()) {
    for (auto& [_, cache] : s_tlutObjectCaches) {
      for (auto it = cache.dynamicPaletteTextures.begin(); it != cache.dynamicPaletteTextures.end();) {
        if (sourceIdentities.contains(it->first.sourceIdentity)) {
          cache.dynamicPaletteTextures.erase(it++);
        } else {
          ++it;
        }
      }
    }
  }
  texture::invalidate_bindings();
  if(std::getenv("MELEE_DIAGNOSTIC_SHADOW_RESOLUTION")) {
    unsigned remaining=0;
    for(const auto& entry:g_gxState.copyTextureCache)remaining+=entry.first.dest==dest;
    remaining+=g_gxState.copyTextures.contains(dest);
    std::fprintf(stderr,"NATIVE_COPY_TEXTURE_RETIRED destination=%zx cache_removed=%u binding_removed=%u remaining=%u\n",reinterpret_cast<size_t>(dest),removedCache,removedBinding,remaining);
  }
}

void evict_copy_texture_range(uint64_t low, uint64_t high) noexcept {
  if (low == 0 || low >= high) return;
  absl::flat_hash_set<const void*> destinations;
  const auto inRange = [low, high](const void* dest) {
    const auto address = reinterpret_cast<uintptr_t>(dest);
    return address >= low && address < high;
  };
  for (const auto& entry : g_gxState.copyTextures)
    if (inRange(entry.first)) destinations.insert(entry.first);
  for (const auto& entry : g_gxState.copyTextureCache)
    if (inRange(entry.first.dest)) destinations.insert(entry.first.dest);
  for (const void* dest : destinations) evict_copy_texture(dest);
  if (std::getenv("MELEE_DIAGNOSTIC_SHADOW_RESOLUTION")) {
    unsigned remaining = 0;
    for (const auto& entry : g_gxState.copyTextures) remaining += inRange(entry.first);
    for (const auto& entry : g_gxState.copyTextureCache) remaining += inRange(entry.first.dest);
    std::fprintf(stderr, "NATIVE_COPY_TEXTURE_RANGE_RETIRED low=%llx high=%llx destinations=%u remaining=%u\n",
                 static_cast<unsigned long long>(low), static_cast<unsigned long long>(high),
                 static_cast<unsigned>(destinations.size()), remaining);
  }
}

void resolve_sampled_textures(const ShaderInfo& info) noexcept {
  ZoneScoped;
  const auto profileStart=binding_profile::begin();
  apply_pending_invalidations();

  for (u32 i = 0; i < MaxTextures; ++i) {
    if (!info.sampledTextures.test(i)) {
      continue;
    }

    GXTexObj_ obj = g_gxState.loadedTextures[i];
    auto& textureBind = g_gxState.textures[i];
    if (textureBind.generation == s_bindGeneration && obj.texObjId != 0 &&
        obj.texObjId == textureBind.texObj.texObjId && obj.texDataVersion == textureBind.texObj.texDataVersion &&
        obj.tlut == textureBind.texObj.tlut && obj.mip_count() == textureBind.texObj.mip_count()) {
      touch_bound_texture(obj);
      // Image reuse must not freeze its mutable GX sampler registers. Preserve
      // the resolved image format (e.g. a CI texture converted to RGBA).
      textureBind.texObj.mode0 = obj.mode0;
      textureBind.texObj.mode1 = obj.mode1;
      continue;
    }

    gfx::TextureHandle handle;
    const auto copyIt = g_gxState.copyTextures.find(obj.data);
    const GXState::CopyTextureRef* copyRef = copyIt != g_gxState.copyTextures.end() ? &copyIt->second : nullptr;
    if (is_palette_format(obj.format())) {
      const auto tlutIdx = static_cast<size_t>(obj.tlut);
      if (tlutIdx < g_gxState.loadedTluts.size()) {
        const auto& tlut = g_gxState.loadedTluts[tlutIdx];
        if (tlut.data != nullptr) {
          if (copyRef != nullptr) {
            handle = resolve_dynamic_palette_texture(obj, *copyRef, tlut);
          } else if (obj.has_data()) {
            handle = texture::resolve_static_palette_texture(obj, tlut);
          }
        }
      }
    } else if (copyRef != nullptr) {
      handle = copyRef->handle;
    } else if (obj.has_data()) {
      handle = texture::resolve_static_texture(obj);
    }

    if (std::getenv("MELEE_DIAGNOSTIC_HUD_QUALITY") && handle) {
      static unsigned counts[2][3]={};const bool hud=gfx::detail::recording_hud_graphics();
      const auto quality=aurora_get_effective_texture_quality();
      if(counts[hud][quality==100?0:quality==50?1:2]++<64)
        std::fprintf(stderr,"NATIVE_HUD_TEXTURE_BIND frame=%llu hud=%u quality=%u source=%ux%u gpu=%ux%u replacement=%u\n",
          static_cast<unsigned long long>(gfx::detail::diagnostic_recording_id()),unsigned(hud),
          aurora_get_effective_texture_quality(),obj.width(),obj.height(),handle->size.width,handle->size.height,unsigned(handle->isReplacement));
    }
    obj.mFormat = resolved_format_for_handle(handle);
    textureBind = gfx::TextureBind{obj, std::move(handle), s_bindGeneration};
  }
  binding_profile::finish(binding_profile::Resolve,profileStart);
}
} // namespace aurora::gx

extern "C" int aurora_diagnostic_original_texture(const AuroraOriginalTexture* source){
  return source&&aurora::gfx::original_texture::publish(*source);
}
extern "C" void aurora_diagnostic_forget_original_texture(const void* owner){aurora::gfx::original_texture::forget(owner);}
extern "C" int aurora_diagnostic_original_texture_object(const void* object,const void* owner){
  if(!object)return 0;const auto& obj=*static_cast<const GXTexObj_*>(object);
  return aurora::gfx::original_texture::tag(obj.texObjId,obj.texDataVersion,obj.data,owner);
}
