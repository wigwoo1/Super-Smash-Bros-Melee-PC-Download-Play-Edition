#include "../lib/gfx/snapshot_pool.hpp"
#include <cassert>
#include <cstdio>
#include <thread>

using Pool = aurora::gfx::SnapshotPool<std::shared_ptr<int>, 2>;
int main() {
  Pool pool;
  assert(!pool.acquire());
  assert(pool.initialize(1));
  auto a = pool.acquire(), b = pool.acquire();
  assert(a && b && pool.token(a) != pool.token(b) && !pool.acquire());
  assert(!a->acknowledge(1, a->id, true, true));
  assert(pool.cancel_unencoded(b)); b.reset();
  assert(!pool.initialize(2));
  a->frame = 10; a->resource = std::make_shared<int>(7);
  std::weak_ptr<int> resource = a->resource;
  assert(!pool.cancel_unencoded(a) && pool.submit(a) && !pool.submit(a));
  assert(!a->acknowledge(2, a->id, true, true));
  assert(!a->acknowledge(1, a->id + 1, true, true));
  assert(a->acknowledge(1, a->id, false, true));
  assert(!a->acknowledge(1, a->id, false, true));
  pool.collect(); assert(!resource.expired() && pool.retired == 0);
  std::thread completion([ticket = a] {
    assert(ticket->acknowledge(ticket->generation, ticket->id, true, true));
  });
  completion.join(); pool.collect();
  assert(pool.retired == 0); // Caller still retains a; neither callback nor owner can destroy it yet.
  a.reset(); pool.collect(); assert(resource.expired() && pool.retired == 1);
  // Independent slots may complete out of order; neither advances VI state.
  a = pool.acquire(); b = pool.acquire();
  for (auto& t : {a, b}) { t->frame = 11 + t->id; t->resource = std::make_shared<int>(8); assert(pool.submit(t)); }
  const auto before = pool.retired;
  assert(b->acknowledge(1, b->id, true, true));
  assert(b->acknowledge(1, b->id, false, true));
  b.reset(); pool.collect(); assert(pool.retired == before + 1 && !pool.drained());
  assert(a->acknowledge(1, a->id, true, true));
  auto held = a;
  assert(a->acknowledge(1, a->id, false, true));
  a.reset(); pool.collect(); assert(pool.retired == before + 1);
  held.reset(); assert(pool.drained());
  assert(pool.initialize(2) && !pool.initialize(2));
  a = pool.acquire(); a->frame = 20; a->resource = std::make_shared<int>(9);
  assert(pool.submit(a) && !a->acknowledge(1, a->id, true, true));
  assert(a->acknowledge(2, a->id, false, false));
  pool.collect(); assert(pool.has_failed() && !pool.acquire() && !pool.drained());
  assert(a->acknowledge(2, a->id, true, true));
  a.reset(); assert(pool.drained() && !pool.acquire());
  assert(pool.initialize(3)); // Restart only after all prior references/results retire.
  a = pool.acquire(); a->frame = 30; a->resource = std::make_shared<int>(10);
  resource = a->resource; assert(pool.submit(a));
  const void* heldToken = pool.token(a);
  std::atomic<bool> acknowledged{false}, releaseCallback{false};
  std::thread heldCallback([ticket = a, &acknowledged, &releaseCallback] {
    assert(ticket->acknowledge(3, ticket->id, true, true));
    assert(ticket->acknowledge(3, ticket->id, false, true));
    acknowledged.store(true, std::memory_order_release);
    while (!releaseCallback.load(std::memory_order_acquire)) std::this_thread::yield();
  });
  while (!acknowledged.load(std::memory_order_acquire)) std::this_thread::yield();
  a.reset(); pool.collect(); assert(!resource.expired());
  b = pool.acquire(); assert(b && pool.token(b) != heldToken && !pool.acquire());
  assert(pool.cancel_unencoded(b)); b.reset();
  releaseCallback.store(true, std::memory_order_release); heldCallback.join();
  assert(pool.drained() && resource.expired());
  for (unsigned i = 0; i < 32; ++i) {
    a = pool.acquire(); a->frame = 100 + i; a->resource = std::make_shared<int>(11);
    assert(pool.submit(a));
    std::thread gpu([ticket = a] { assert(ticket->acknowledge(3, ticket->id, true, true)); });
    std::thread surface([ticket = a] { assert(ticket->acknowledge(3, ticket->id, false, true)); });
    gpu.join(); surface.join(); a.reset(); assert(pool.drained());
  }
  std::puts("PASS: bounded capacity, GPU/surface order, resource retention, stale/duplicate results, failure drain and generations");
}
