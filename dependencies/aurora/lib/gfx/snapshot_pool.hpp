#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

namespace aurora::gfx {
// Pool operations are owner-thread only. Published tickets are immutable except
// for their two atomic results; callbacks never mutate the pool or call the game.
template <class Resource, size_t Capacity = 3> class SnapshotPool {
public:
  struct Ticket {
    const uint64_t generation, id;
    const size_t slot;
    Resource resource{};
    uint64_t frame = 0;
    bool submitted = false;
    // 0 pending, 1 success, 2 failure. Failure is terminal, not safe early reuse.
    std::atomic<unsigned> gpu{0}, surface{0};
    Ticket(uint64_t g, uint64_t i, size_t s) : generation(g), id(i), slot(s) {}
    bool acknowledge(uint64_t g, uint64_t i, bool gpuResult, bool success) {
      if (!submitted || g != generation || i != id) return false;
      unsigned expected = 0;
      return (gpuResult ? gpu : surface).compare_exchange_strong(
          expected, success ? 1u : 2u, std::memory_order_acq_rel);
    }
  };
  using Handle = std::shared_ptr<Ticket>;

private:
  std::array<Handle, Capacity> slots{};
  uint64_t generation = 0, sequence = 0;
  bool failed = false;

public:
  uint64_t acquired = 0, submitted = 0, retired = 0, deferred = 0, cancelled = 0;
  size_t high_water = 0;
  bool initialize(uint64_t nextGeneration) {
    for (const auto& ticket : slots) if (ticket) return false;
    if (!nextGeneration || nextGeneration <= generation) return false;
    generation = nextGeneration; sequence = 0; failed = false;
    acquired = submitted = retired = deferred = cancelled = 0;
    high_water = 0;
    return true;
  }
  void collect() {
    for (auto& ticket : slots) if (ticket && ticket->submitted) {
      const auto gpu = ticket->gpu.load(std::memory_order_acquire);
      const auto surface = ticket->surface.load(std::memory_order_acquire);
      if (gpu == 2 || surface == 2) failed = true;
      // References in the worker/GPU callbacks also retain the resource. Wait
      // for those callbacks to return so destruction stays on the owner.
      if (gpu && surface && ticket.use_count() == 1) {
        ticket.reset(); ++retired;
      }
    }
  }
  Handle acquire() {
    collect();
    if (!generation || failed || sequence == UINT64_MAX) { ++deferred; return {}; }
    for (size_t i = 0; i < Capacity; ++i) if (!slots[i]) {
      Handle ticket;
      try { ticket = std::make_shared<Ticket>(generation, sequence + 1, i); }
      catch (const std::bad_alloc&) { ++deferred; return {}; }
      ++sequence; ++acquired; slots[i] = ticket;
      size_t live = 0; for (const auto& entry : slots) if (entry) ++live;
      if (live > high_water) high_water = live;
      return ticket;
    }
    ++deferred; return {};
  }
  bool owns(const Handle& ticket) const {
    return ticket && ticket->slot < Capacity && slots[ticket->slot] == ticket &&
           ticket->generation == generation;
  }
  const void* token(const Handle& ticket) const {
    return owns(ticket) ? static_cast<const void*>(&slots[ticket->slot]) : nullptr;
  }
  bool cancel_unencoded(const Handle& ticket) {
    if (!owns(ticket) || ticket->submitted || ticket->resource) return false;
    slots[ticket->slot].reset(); ++cancelled; return true;
  }
  bool submit(const Handle& ticket) {
    if (!owns(ticket) || ticket->submitted || !ticket->resource || !ticket->frame) return false;
    ticket->submitted = true; ++submitted; return true;
  }
  bool drained() {
    collect();
    for (const auto& ticket : slots) if (ticket) return false;
    return true;
  }
  bool has_failed() const { return failed; }
};
} // namespace aurora::gfx
