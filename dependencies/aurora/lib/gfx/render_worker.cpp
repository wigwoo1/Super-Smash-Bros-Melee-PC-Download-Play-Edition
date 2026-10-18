#include "render_worker.hpp"

#include "../thread.hpp"
#include "../serviced_wait.hpp"
#include <aurora/aurora.h>
#include <aurora/diagnostics.h>

#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <tracy/Tracy.hpp>

namespace aurora::gfx::render_worker {
namespace {
constexpr size_t QueueCapacity = 256;
constexpr auto IdlePumpInterval = std::chrono::milliseconds{1};
bool service_wait(){
 if(!aurora_diagnostic_is_owner() || aurora_service_owner())return true;
 std::fputs("Native owner service failed during render-worker wait\n",stderr);
 std::abort();
}

BoundedQueue g_queue{QueueCapacity};
thread::Thread g_thread;
std::atomic_bool g_running = false;
std::atomic_size_t g_pendingItems = 0;
// Each caller reads its own identity flag; a shared std::thread::id would
// race with worker startup/exit when producers query it concurrently.
thread_local bool g_isWorkerThread = false;

void complete_sync(const std::shared_ptr<SyncState>& sync) {
  if (!sync) {
    return;
  }
  ZoneScoped;
  {
    std::lock_guard lock{sync->mutex};
    sync->complete = true;
  }
  sync->cv.notify_all();
}

void worker_main(std::stop_token token) {
  g_isWorkerThread = true;
  const bool traceWork = std::getenv("AURORA_DIAGNOSTIC_WORKER_ITEMS") != nullptr;
  unsigned traceRecords = 0;

  while (true) {
    bool closed = false;
    auto item = g_queue.pop_for(IdlePumpInterval, closed);
    if (!item) {
      if (closed || token.stop_requested()) {
        break;
      }
      continue;
    }

    const uint64_t workStart = traceWork ? SDL_GetTicksNS() : 0;
    if (item->work) {
      ZoneScopedN("QueueItem work");
      item->work();
    }
    if(traceWork && traceRecords<131072){
      const uint64_t workEnd = SDL_GetTicksNS();
      std::fprintf(stderr,"NATIVE_WORKER_ITEM record=%u type=%u frame=%llu pass=%u start_ns=%llu end_ns=%llu\n",
        ++traceRecords,unsigned(item->type),(unsigned long long)item->frameId,item->passIndex,
        (unsigned long long)workStart,(unsigned long long)workEnd);
    }
    complete_sync(item->sync);
    g_pendingItems.fetch_sub(1, std::memory_order_acq_rel);
    if (item->type == ItemType::Shutdown) {
      break;
    }
  }

  g_isWorkerThread = false;
}

void enqueue(QueueItem item) {
  ZoneScoped;
  if (is_worker_thread()) {
    if (item.work) {
      item.work();
    }
    complete_sync(item.sync);
    return;
  }

  if (!g_running.load(std::memory_order_acquire)) {
    if (item.work) {
      item.work();
    }
    complete_sync(item.sync);
    return;
  }

  g_pendingItems.fetch_add(1, std::memory_order_acq_rel);
  if (!g_queue.push(std::move(item))) {
    g_pendingItems.fetch_sub(1, std::memory_order_acq_rel);
  }
}
} // namespace

BoundedQueue::BoundedQueue(size_t capacity) : m_capacity(capacity) {}

bool BoundedQueue::push(QueueItem item) {
  ZoneScoped;
  std::unique_lock lock{m_mutex};
  aurora::serviced_wait(m_notFull,lock,[&] { return m_closed || m_items.size() < m_capacity; },service_wait);
  if (m_closed) {
    return false;
  }
  m_items.emplace_back(std::move(item));
  lock.unlock();
  m_notEmpty.notify_one();
  return true;
}

std::optional<QueueItem> BoundedQueue::pop_for(std::chrono::milliseconds timeout, bool& closed) {
  std::unique_lock lock{m_mutex};
  m_notEmpty.wait_for(lock, timeout, [&] { return m_closed || !m_items.empty(); });
  closed = m_closed && m_items.empty();
  if (m_items.empty()) {
    return std::nullopt;
  }
  auto item = std::move(m_items.front());
  m_items.pop_front();
  lock.unlock();
  m_notFull.notify_one();
  return item;
}

bool BoundedQueue::try_push(QueueItem item) {
  std::unique_lock lock{m_mutex, std::try_to_lock};
  if (!lock.owns_lock() || m_closed || m_items.size() >= m_capacity) return false;
  m_items.emplace_back(std::move(item));
  lock.unlock();
  m_notEmpty.notify_one();
  return true;
}

void BoundedQueue::close() {
  {
    std::lock_guard lock{m_mutex};
    m_closed = true;
  }
  m_notEmpty.notify_all();
  m_notFull.notify_all();
}

void BoundedQueue::reset() {
  {
    std::lock_guard lock{m_mutex};
    m_items.clear();
    m_closed = false;
  }
  m_notFull.notify_all();
}

size_t BoundedQueue::size() const {
  std::lock_guard lock{m_mutex};
  return m_items.size();
}

FrameSlotPool::FrameSlotPool(size_t slotCount) : m_freeSlots(slotCount, true) {}

size_t FrameSlotPool::acquire() {
  std::unique_lock lock{m_mutex};
  aurora::serviced_wait(m_cv,lock,[&] {
    for (const bool free : m_freeSlots) {
      if (free) {
        return true;
      }
    }
    return false;
  },service_wait);
  for (size_t i = 0; i < m_freeSlots.size(); ++i) {
    if (m_freeSlots[i]) {
      m_freeSlots[i] = false;
      return i;
    }
  }
  return 0;
}

std::optional<size_t> FrameSlotPool::try_acquire() {
  std::lock_guard lock{m_mutex};
  for (size_t i = 0; i < m_freeSlots.size(); ++i) {
    if (m_freeSlots[i]) {
      m_freeSlots[i] = false;
      return i;
    }
  }
  return std::nullopt;
}

void FrameSlotPool::release(size_t slot) {
  {
    std::lock_guard lock{m_mutex};
    if (slot < m_freeSlots.size()) {
      m_freeSlots[slot] = true;
    }
  }
  m_cv.notify_one();
}

void FrameSlotPool::reset() {
  {
    std::lock_guard lock{m_mutex};
    std::fill(m_freeSlots.begin(), m_freeSlots.end(), true);
  }
  m_cv.notify_all();
}

size_t FrameSlotPool::free_count() const {
  std::lock_guard lock{m_mutex};
  return static_cast<size_t>(std::ranges::count(m_freeSlots, true));
}

void initialize() {
  if (g_running.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  g_queue.reset();
  g_pendingItems.store(0, std::memory_order_release);
  g_thread = thread::Thread{{
                                .name = "Aurora render worker",
                                .affinity = thread::Affinity::SharedCache,
                            },
                            worker_main};
}

void shutdown() {
  if (!g_running.load(std::memory_order_acquire)) {
    return;
  }
  enqueue({
      .type = ItemType::Shutdown,
  });
  if (g_thread.joinable()) {
    g_thread.join();
  }
  g_running.store(false, std::memory_order_release);
  g_queue.close();
  g_queue.reset();
  g_pendingItems.store(0, std::memory_order_release);
}

void enqueue_begin_frame(uint64_t frameId, WorkCallback work) {
  enqueue({
      .type = ItemType::BeginFrame,
      .frameId = frameId,
      .work = std::move(work),
  });
}

void enqueue_encode_pass(uint64_t frameId, uint32_t passIndex, WorkCallback work) {
  enqueue({
      .type = ItemType::EncodePass,
      .frameId = frameId,
      .passIndex = passIndex,
      .work = std::move(work),
  });
}

void enqueue_end_frame(uint64_t frameId, WorkCallback work) {
  enqueue({
      .type = ItemType::EndFrame,
      .frameId = frameId,
      .work = std::move(work),
  });
}

void enqueue_work(WorkCallback work) {
  enqueue({
      .type = ItemType::Sync,
      .work = std::move(work),
  });
}

bool try_enqueue_work(WorkCallback work) {
  if (is_worker_thread()) { if (work) work(); return true; }
  // Device-event polling must remain on the live render worker.
  if (!g_running.load(std::memory_order_acquire)) return false;
  g_pendingItems.fetch_add(1, std::memory_order_acq_rel);
  if (g_queue.try_push({.type=ItemType::Sync, .work=std::move(work)})) return true;
  g_pendingItems.fetch_sub(1, std::memory_order_acq_rel);
  return false;
}

void synchronize() {
  if (is_worker_thread()) {
    return;
  }

  ZoneScoped;
  auto sync = std::make_shared<SyncState>();
  enqueue({
      .type = ItemType::Sync,
      .sync = sync,
  });
  std::unique_lock lock{sync->mutex};
  aurora::serviced_wait(sync->cv,lock,[&] { return sync->complete; },service_wait);
}

bool is_worker_thread() noexcept { return g_isWorkerThread; }

bool is_idle() noexcept { return g_pendingItems.load(std::memory_order_acquire) == 0; }

} // namespace aurora::gfx::render_worker
