#include "gx.hpp"
#include "__gx.h"

#include "../../gx/fifo.hpp"
#include "../../gx/mesh_diagnostic_types.hpp"
#include <aurora/diagnostics.h>
#include <cstdio>

#include <cstring>

static __GXData_struct sSavedGXData;

extern "C" {
int aurora_diagnostic_original_mesh(const AuroraOriginalMesh* mesh){
  using namespace aurora::gx::mesh_diagnostic;
  if(!enabled()||!aurora_diagnostic_is_owner()||!mesh||pending||!valid(*mesh))return 0;
  auto context=std::make_shared<Context>();context->source=*mesh;
  context->original.assign(static_cast<const uint8_t*>(mesh->original),static_cast<const uint8_t*>(mesh->original)+mesh->bytes);
  context->geometryHash=fingerprint(context->original.data(),context->original.size());
  for(uint32_t p=0;p<mesh->primitives;p++)for(uint32_t n=0;n<mesh->primitive[p].points;n++){
    const auto& point=mesh->primitive[p].point[n];
    context->geometryHash=fingerprint(point.position,sizeof(point.position),context->geometryHash);
    context->geometryHash=fingerprint(&point.selector,sizeof(point.selector),context->geometryHash);
  }
  context->source.original=nullptr;context->id=++nextId;pending=std::move(context);return 1;
}
void GXBeginDisplayList(void* list, u32 size) {
  CHECK(!aurora::gx::fifo::in_display_list(), "Display list began twice!");

  // Flush any pending dirty state before recording
  if (__gx->dirtyState != 0) {
    __GXSetDirtyState();
  }

  // Save current shadow register state if requested
  if (__gx->dlSaveContext != 0) {
    std::memcpy(&sSavedGXData, __gx, sizeof(sSavedGXData));
  }

  __gx->inDispList = 1;

  // Redirect FIFO writes to the user-provided buffer
  aurora::gx::fifo::begin_display_list(static_cast<u8*>(list), size);
}

u32 GXEndDisplayList() {
  // Flush any pending dirty state into the display list
  if (__gx->dirtyState != 0) {
    __GXSetDirtyState();
  }

  // End FIFO redirection and get the byte count (ROUNDUP32)
  u32 bytesWritten = aurora::gx::fifo::end_display_list();

  // Restore saved shadow register state
  if (__gx->dlSaveContext != 0) {
    std::memcpy(__gx, &sSavedGXData, sizeof(*__gx));
  }

  __gx->inDispList = 0;

  return bytesWritten;
}

void GXCallDisplayList(const void* data, u32 nbytes) {
  // Flush any pending dirty state before calling
  if (__gx->dirtyState != 0) {
    __GXSetDirtyState();
  }

  // Flush pending primitives
  if (*reinterpret_cast<u32*>(&__gx->vNum) != 0) {
    __GXSendFlushPrim();
  }

  auto diagnostic=std::move(aurora::gx::mesh_diagnostic::pending);
  if(diagnostic){
    AURORA_ASSERT(diagnostic->source.display==data&&diagnostic->source.bytes==nbytes,"Original mesh diagnostic display identity");
    // Flush prior dirty/flush-primitive traffic before the scoped original DL.
    aurora::gx::fifo::drain();
    aurora::gx::mesh_diagnostic::active.store(diagnostic,std::memory_order_release);
  }
  // Write display list contents to the FIFO
  aurora::gx::fifo::write_data(data, nbytes);
  aurora::gx::fifo::publish();
  if(diagnostic){
    aurora::gx::fifo::drain();
    aurora::gx::mesh_diagnostic::active.store({},std::memory_order_release);
    std::fprintf(stderr,"MESH_GPU_SCOPE id=%llu actor=%u frame=%u expected=%u decoded=%u\n",
      (unsigned long long)diagnostic->id,diagnostic->source.actor,diagnostic->source.frame,
      diagnostic->source.primitives,diagnostic->cursor.load());
  }
}

}
