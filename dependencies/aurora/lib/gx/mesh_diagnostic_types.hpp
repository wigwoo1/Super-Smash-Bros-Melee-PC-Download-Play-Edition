#pragma once
#include <aurora/mesh_diagnostics.h>
#include <aurora/mesh_diagnostics_validation.h>
#include "pipeline.hpp"
#include <atomic>
#include <array>
#include <memory>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>
namespace aurora::gx::mesh_diagnostic {
inline bool enabled(){
  static const bool value=std::getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&std::getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH_GPU");
  return value;
}
struct Context {
  AuroraOriginalMesh source{};
  std::vector<uint8_t> original;
  uint64_t id=0,geometryHash=0;
  std::atomic<uint32_t> cursor{0};
};
// pending is initialization/recording-owner only. active is read by the FIFO
// decoder; caller drains both before publication and before clearing it.
inline std::shared_ptr<Context> pending;
inline std::atomic<std::shared_ptr<Context>> active;
inline uint64_t nextId=0;
inline uint64_t fingerprint(const void* data,size_t bytes,uint64_t hash=14695981039346656037ull){
  const auto* p=static_cast<const uint8_t*>(data);for(size_t n=0;n<bytes;n++)hash=(hash^p[n])*1099511628211ull;return hash;
}
inline bool valid(const AuroraOriginalMesh& s){
  return AuroraOriginalMeshValid(&s)!=0;
}
struct Snapshot {
  DrawData draw{};
  ShaderConfig config{};
  std::shared_ptr<const Context> context;
  uint32_t primitive=0,firstVertex=0,points=0,selectorMask=0;
  uint64_t matrixHash=14695981039346656037ull,expectedHash=0;
  std::array<double,AURORA_MESH_POINTS*4> expected{};
  std::array<double,AURORA_MESH_POINTS*16> expectedDirections{};
  uint32_t colorMask=0,colorLanes=0;
  std::array<double,AURORA_MESH_POINTS*8> expectedColors{};
  uint32_t texgenMask=0;
  std::array<double,AURORA_MESH_POINTS*16> expectedTexgens{};
};
}
