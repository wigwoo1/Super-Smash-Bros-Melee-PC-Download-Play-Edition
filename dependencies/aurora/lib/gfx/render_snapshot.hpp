#pragma once
#include "snapshot_pool.hpp"
#include "types.hpp"
#include <cstdlib>

namespace aurora::gfx::render_snapshot {
using Pool = SnapshotPool<TextureHandle>;
using Ticket = Pool::Ticket;
inline Pool pool;
inline Pool::Handle active;
inline uint64_t generation = 0;
inline bool initialize() {
  return !active && pool.initialize(++generation);
}
inline bool destination(const void* address, uint64_t frame) {
  return active && active->frame == frame && frame && pool.token(active) == address;
}
inline bool attach(const void* address, uint64_t frame, TextureHandle texture) {
  if (!destination(address, frame) || active->resource || !texture) return false;
  active->resource = std::move(texture); return true;
}
inline Pool::Handle publish(uint64_t frame) {
  if (!active) return {};
  if (active->frame != frame || !pool.submit(active)) std::abort();
  return std::exchange(active, {});
}
} // namespace aurora::gfx::render_snapshot
