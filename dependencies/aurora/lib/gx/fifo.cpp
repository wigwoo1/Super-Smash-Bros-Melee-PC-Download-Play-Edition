#include "fifo.hpp"

#include "../thread.hpp"
#include "../thread_cpu_clock.hpp"
#include "../serviced_wait.hpp"
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <cstdio>
extern "C" int aurora_diagnostic_is_owner(void);
extern "C" int aurora_service_owner(void);
#include "command_processor.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <mutex>

#include <tracy/Tracy.hpp>

namespace aurora::gx::fifo {
namespace detail {
uint8_t* sBufferData = nullptr;
uint32_t sBufferSize = 0;
uint32_t sBufferCapacity = 0;
bool sInDisplayList = false;
uint8_t* sDlBuffer = nullptr;
uint32_t sDlSize = 0;
uint32_t sDlWritePos = 0;
} // namespace detail

namespace {
constexpr Module Log{"aurora::gx::fifo"};
constexpr auto kProcessingMode = ProcessingMode::Thread;
constexpr uint32_t kDrawBatchSize = 1;

bool sFrameActive = false;
uint32_t sPendingDraws = 0;
std::atomic<uint64_t> sPublished{0};
std::atomic<uint64_t> sProcessed{0};
std::atomic<uint64_t> sWorkerProcessTotalNs{0};
std::atomic<uint64_t> sWorkerBufferLockWaitTotalNs{0};
std::atomic<uint64_t> sWorkerProcessBodyTotalNs{0};
uint64_t sStreamBase = 0;
std::timed_mutex sBufferMutex;
std::mutex sProcessedWaitMutex;
std::condition_variable sProcessedCv;
std::atomic<uint32_t> sWorkerWake{0};
thread::Thread sWorkerThread;
bool sPacingProfileEnabled = false;
bool sCpuProfileEnabled = false;
struct WorkerCpuProfile { uint64_t cpuNs=0,wallNs=0,samples=0,failures=0; };
std::mutex sWorkerCpuProfileMutex;
WorkerCpuProfile sWorkerCpuProfile,sDrainCpuProfile;
WorkerCpuProfile worker_cpu_profile() {
  // No owner buffer lock is held by callers. Worker records while holding its
  // buffer lock, so this short diagnostic mutex introduces no reverse order.
  std::lock_guard lock{sWorkerCpuProfileMutex};return sWorkerCpuProfile;
}
uint64_t sDrainWaitNs = 0, sDrainWaitMaxNs = 0;
uint64_t sWorkerProcessNs = 0, sWorkerProcessMaxNs = 0;
uint64_t sWorkerBufferLockWaitNs = 0, sWorkerProcessBodyNs = 0;
uint64_t sDrainPendingBytes = 0;
uint32_t sDrainProfileCalls = 0, sDrainWorkerSamples = 0;

uint64_t steady_now_ns() noexcept {
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}

void report_drain_profile(uint64_t ownerWaitNs, uint64_t workerProcessNs,
                          uint64_t workerBufferLockWaitNs, uint64_t workerProcessBodyNs,
                          uint64_t pendingBytes,WorkerCpuProfile cpu) {
  if (!sPacingProfileEnabled) return;
  ++sDrainProfileCalls;
  sDrainWaitNs += ownerWaitNs;
  sDrainWaitMaxNs = std::max(sDrainWaitMaxNs, ownerWaitNs);
  sWorkerProcessNs += workerProcessNs;
  sWorkerProcessMaxNs = std::max(sWorkerProcessMaxNs, workerProcessNs);
  sWorkerBufferLockWaitNs += workerBufferLockWaitNs;
  sWorkerProcessBodyNs += workerProcessBodyNs;
  sDrainPendingBytes += pendingBytes;
  sDrainCpuProfile.cpuNs+=cpu.cpuNs;sDrainCpuProfile.wallNs+=cpu.wallNs;
  sDrainCpuProfile.samples+=cpu.samples;sDrainCpuProfile.failures+=cpu.failures;
  if (workerProcessNs) ++sDrainWorkerSamples;
  if (sDrainProfileCalls != 60) return;
  std::fprintf(stderr,
      "NATIVE_GX_FIFO_DRAIN_PROFILE calls=%u worker_samples=%u owner_wait_avg_us=%llu "
      "owner_wait_max_us=%llu worker_process_wall_avg_us=%llu worker_process_wall_max_us=%llu "
      "worker_buffer_lock_wait_avg_us=%llu worker_process_body_avg_us=%llu pending_bytes_avg=%llu\n",
      sDrainProfileCalls, sDrainWorkerSamples,
      static_cast<unsigned long long>(sDrainWaitNs / 1000 / sDrainProfileCalls),
      static_cast<unsigned long long>(sDrainWaitMaxNs / 1000),
      static_cast<unsigned long long>(sWorkerProcessNs / 1000 / sDrainProfileCalls),
      static_cast<unsigned long long>(sWorkerProcessMaxNs / 1000),
      static_cast<unsigned long long>(sWorkerBufferLockWaitNs / 1000 / sDrainProfileCalls),
      static_cast<unsigned long long>(sWorkerProcessBodyNs / 1000 / sDrainProfileCalls),
      static_cast<unsigned long long>(sDrainPendingBytes / sDrainProfileCalls));
  if(sCpuProfileEnabled)
    std::fprintf(stderr,
      "NATIVE_GX_FIFO_CPU_PROFILE calls=%u measured_batches=%llu failed_samples=%llu worker_cpu_total_us=%llu worker_wall_total_us=%llu measurement=current_thread_times includes_buffer_lock=1\n",
      sDrainProfileCalls,(unsigned long long)sDrainCpuProfile.samples,
      (unsigned long long)sDrainCpuProfile.failures,
      (unsigned long long)(sDrainCpuProfile.cpuNs/1000),
      (unsigned long long)(sDrainCpuProfile.wallNs/1000));
  sDrainCpuProfile={};
  sDrainWaitNs = sDrainWaitMaxNs = sWorkerProcessNs = sWorkerProcessMaxNs = sDrainPendingBytes = 0;
  sWorkerBufferLockWaitNs = sWorkerProcessBodyNs = 0;
  sDrainProfileCalls = sDrainWorkerSamples = 0;
}

std::unique_lock<std::timed_mutex> lock_buffer() {
  std::unique_lock lock{sBufferMutex,std::defer_lock};
  if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES") && aurora_diagnostic_is_owner()) {
    unsigned calls=0;
    while(!lock.try_lock_for(std::chrono::milliseconds{1})) {
      ++calls;
      AURORA_ASSERT(aurora_service_owner(),"FIFO buffer owner service failed");
    }
    static unsigned samples=0;
    if(calls && samples++<128) std::fprintf(stderr,"NATIVE_FIFO_BUFFER_SERVICE calls=%u owner=1\n",calls);
  } else lock.lock();
  return lock;
}

void wake_worker() noexcept {
  sWorkerWake.fetch_add(1, std::memory_order_release);
  sWorkerWake.notify_all();
}

void process_inline(uint64_t target) noexcept {
  const uint64_t processed = sProcessed.load(std::memory_order_relaxed);
  if (processed >= target) {
    return;
  }

  const auto start = static_cast<uint32_t>(processed - sStreamBase);
  const auto size = static_cast<uint32_t>(target - processed);
  process(detail::sBufferData + start, size);
  sProcessed.store(target, std::memory_order_relaxed);
}

void worker_main(std::stop_token token) noexcept {
  std::stop_callback wakeOnStop{token, wake_worker};
  uint64_t processed = sProcessed.load(std::memory_order_relaxed);
  while (true) {
    const uint32_t event = sWorkerWake.load(std::memory_order_acquire);
    const uint64_t published = sPublished.load(std::memory_order_acquire);
    if (published != processed) {
      const uint64_t processStartNs = sPacingProfileEnabled ? steady_now_ns() : 0;
      const auto cpuStart=sCpuProfileEnabled ? thread_cpu_sample() : ThreadCpuSample{};
      const uint64_t lockWaitStartNs = processStartNs;
      {
        std::lock_guard lock{sBufferMutex};
        const uint64_t processBodyStartNs = sPacingProfileEnabled ? steady_now_ns() : 0;
        if (sPacingProfileEnabled)
          sWorkerBufferLockWaitTotalNs.fetch_add(processBodyStartNs - lockWaitStartNs,
                                                 std::memory_order_relaxed);
        const auto start = static_cast<uint32_t>(processed - sStreamBase);
        const auto size = static_cast<uint32_t>(published - processed);
        process(detail::sBufferData + start, size);
        const uint64_t processBodyEndNs = sPacingProfileEnabled ? steady_now_ns() : 0;
        if (sPacingProfileEnabled)
          sWorkerProcessBodyTotalNs.fetch_add(processBodyEndNs - processBodyStartNs,
                                              std::memory_order_relaxed);
        processed = published;
        if(sCpuProfileEnabled){
          const auto cpuEnd=thread_cpu_sample();
          const uint64_t wallEnd=steady_now_ns();
          std::lock_guard cpuLock{sWorkerCpuProfileMutex};
          if(cpuStart.available && cpuEnd.available && cpuEnd.nanoseconds>=cpuStart.nanoseconds){
            sWorkerCpuProfile.cpuNs+=cpuEnd.nanoseconds-cpuStart.nanoseconds;
            sWorkerCpuProfile.wallNs+=wallEnd-processStartNs;++sWorkerCpuProfile.samples;
          }else ++sWorkerCpuProfile.failures;
        }
        if (sPacingProfileEnabled)
          sWorkerProcessTotalNs.fetch_add(steady_now_ns() - processStartNs, std::memory_order_relaxed);
        sProcessed.store(processed, std::memory_order_release);
      }
      sProcessed.notify_all();
      sProcessedCv.notify_all();
      continue;
    }

    if (token.stop_requested()) {
      break;
    }
    sWorkerWake.wait(event, std::memory_order_acquire);
  }
}

void start_worker() {
  if (kProcessingMode != ProcessingMode::Thread || sWorkerThread.joinable()) {
    return;
  }
  sWorkerThread = thread::Thread{{
                                     .name = "Aurora FIFO processor",
                                     .affinity = thread::Affinity::SharedCache,
                                 },
                                 worker_main};
}

void stop_worker() {
  if (!sWorkerThread.joinable()) {
    return;
  }
  sWorkerThread.request_stop();
  sWorkerThread.join();
}
} // namespace

ProcessingMode processing_mode() noexcept { return kProcessingMode; }

void init() {
  stop_worker();
  sPacingProfileEnabled = std::getenv("MELEE_DIAGNOSTIC_PACING_PROFILE") != nullptr;
  sCpuProfileEnabled=sPacingProfileEnabled && std::getenv("AURORA_DIAGNOSTIC_FIFO_CPU_PROFILE");
  {std::lock_guard lock{sWorkerCpuProfileMutex};sWorkerCpuProfile={};}
  sDrainCpuProfile={};
  sWorkerProcessTotalNs.store(0, std::memory_order_relaxed);
  sDrainWaitNs = sDrainWaitMaxNs = sWorkerProcessNs = sWorkerProcessMaxNs = sDrainPendingBytes = 0;
  sDrainProfileCalls = sDrainWorkerSamples = 0;

  constexpr uint32_t initialCapacity = 64 * 1024;
  free(detail::sBufferData);
  detail::sBufferData = static_cast<uint8_t*>(malloc(initialCapacity));
  AURORA_ASSERT(detail::sBufferData != nullptr, "fifo::init: failed to allocate {} bytes", initialCapacity);
  detail::sBufferSize = 0;
  detail::sBufferCapacity = initialCapacity;
  detail::sInDisplayList = false;
  detail::sDlBuffer = nullptr;
  detail::sDlSize = 0;
  detail::sDlWritePos = 0;

  sFrameActive = false;
  sPendingDraws = 0;
  sStreamBase = 0;
  sPublished.store(0, std::memory_order_relaxed);
  sProcessed.store(0, std::memory_order_relaxed);
  sWorkerWake.store(0, std::memory_order_relaxed);

  start_worker();
}

void shutdown() { stop_worker(); }

void begin_frame() noexcept { sFrameActive = true; }

void end_frame() noexcept {
  sFrameActive = false;
  clear_draw_cache(); // command_processor
}

void write_data_grow(const void* data, uint32_t length) {
  const uint64_t needed64 = static_cast<uint64_t>(detail::sBufferSize) + length;
  AURORA_ASSERT(needed64 <= std::numeric_limits<uint32_t>::max(), "fifo::write_data: buffer size overflow");
  const auto needed = static_cast<uint32_t>(needed64);
  const auto doubledCapacity = static_cast<uint64_t>(detail::sBufferCapacity) * 2;
  const auto newCapacity = static_cast<uint32_t>(
      std::min<uint64_t>(std::max(doubledCapacity, needed64), std::numeric_limits<uint32_t>::max()));
  const auto grow = [newCapacity] {
    auto* resized = static_cast<uint8_t*>(realloc(detail::sBufferData, newCapacity));
    AURORA_ASSERT(resized != nullptr, "fifo::write_data: failed to allocate {} bytes", newCapacity);
    detail::sBufferData = resized;
  };
  if (sWorkerThread.joinable()) {
    auto lock=lock_buffer();
    grow();
  } else {
    grow();
  }
  std::memcpy(detail::sBufferData + detail::sBufferSize, data, length);
  detail::sBufferSize = needed;
  detail::sBufferCapacity = newCapacity;
}

void publish() noexcept {
  if (!sFrameActive || kProcessingMode == ProcessingMode::Drain || detail::sInDisplayList) {
    return;
  }

  const uint64_t target = sStreamBase + detail::sBufferSize;
  if (target > sPublished.load(std::memory_order_relaxed)) {
    sPendingDraws = 0;
    sPublished.store(target, std::memory_order_release);
    if (kProcessingMode == ProcessingMode::Thread) {
      wake_worker();
    } else {
      process_inline(target);
    }
  }
}

void finish_draw() noexcept {
  if (!sFrameActive || kProcessingMode == ProcessingMode::Drain || detail::sInDisplayList) {
    return;
  }
  if (++sPendingDraws >= kDrawBatchSize) {
    publish();
  }
}

void patch_u32(uint32_t offset, uint32_t val) {
  AURORA_ASSERT(!detail::sInDisplayList && offset <= detail::sBufferSize &&
                    sizeof(uint32_t) <= detail::sBufferSize - offset,
                "fifo::patch_u32: invalid patch offset {} (buffer size {})", offset, detail::sBufferSize);
  AURORA_ASSERT(sStreamBase + offset >= sPublished.load(std::memory_order_relaxed),
                "fifo::patch_u32: offset {} is below the published watermark", offset);
  const auto out = bswap(val);
  std::memcpy(detail::sBufferData + offset, &out, sizeof(out));
}

void begin_display_list(uint8_t* buf, uint32_t size) {
  detail::sInDisplayList = true;
  detail::sDlBuffer = buf;
  detail::sDlSize = size;
  detail::sDlWritePos = 0;
}

uint32_t end_display_list() {
  detail::sInDisplayList = false;
  const uint32_t bytesWritten = detail::sDlWritePos;
  const uint32_t padded = (bytesWritten + 31) & ~31u;
  while (detail::sDlWritePos < padded && detail::sDlWritePos < detail::sDlSize) {
    detail::sDlBuffer[detail::sDlWritePos++] = 0;
  }
  detail::sDlBuffer = nullptr;
  detail::sDlSize = 0;
  detail::sDlWritePos = 0;
  return padded;
}

bool in_display_list() { return detail::sInDisplayList; }

void drain() {
  if (detail::sBufferSize == 0) {
    return;
  }

  ZoneScoped;
  const uint64_t target = sStreamBase + detail::sBufferSize;
  const uint64_t processStart = sProcessed.load(std::memory_order_acquire);
  uint64_t ownerWaitNs = 0;
  uint64_t workerProcessNs = 0;
  uint64_t workerBufferLockWaitNs = 0;
  uint64_t workerProcessBodyNs = 0;
  WorkerCpuProfile cpu;

  switch (kProcessingMode) {
  case ProcessingMode::Drain:
    process(detail::sBufferData, detail::sBufferSize);
    sPublished.store(target, std::memory_order_relaxed);
    sProcessed.store(target, std::memory_order_relaxed);
    break;
  case ProcessingMode::Inline:
    sPublished.store(target, std::memory_order_relaxed);
    process_inline(target);
    break;
  case ProcessingMode::Thread: {
    const uint64_t workerTimeAtStart =
        sPacingProfileEnabled ? sWorkerProcessTotalNs.load(std::memory_order_relaxed) : 0;
    const uint64_t workerLockWaitAtStart = sPacingProfileEnabled
        ? sWorkerBufferLockWaitTotalNs.load(std::memory_order_relaxed) : 0;
    const uint64_t workerBodyAtStart = sPacingProfileEnabled
        ? sWorkerProcessBodyTotalNs.load(std::memory_order_relaxed) : 0;
    const auto cpuBefore=sCpuProfileEnabled ? worker_cpu_profile() : WorkerCpuProfile{};
    sPublished.store(target, std::memory_order_release);
    wake_worker();
    const uint64_t waitStartNs = sPacingProfileEnabled ? steady_now_ns() : 0;

    if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES") && aurora_diagnostic_is_owner()) {
      unsigned serviced=0;
      std::unique_lock lock{sProcessedWaitMutex};
      AURORA_ASSERT(aurora::serviced_wait(sProcessedCv,lock,
        [=]{return sProcessed.load(std::memory_order_acquire)==target;},
        [&]{++serviced;return aurora_service_owner()!=0;}),"FIFO owner service failed during preparation");
      static unsigned samples=0;
      if(serviced && samples++<128) std::fprintf(stderr,"NATIVE_FIFO_PREPARATION_SERVICE calls=%u owner=1 simulation_advanced=0\n",serviced);
    } else {
      uint64_t processed=sProcessed.load(std::memory_order_acquire);
      while(processed!=target) {
        sProcessed.wait(processed,std::memory_order_acquire);
        processed=sProcessed.load(std::memory_order_acquire);
      }
    }
    if (sPacingProfileEnabled) {
      ownerWaitNs = steady_now_ns() - waitStartNs;
      workerProcessNs = sWorkerProcessTotalNs.load(std::memory_order_relaxed) - workerTimeAtStart;
      workerBufferLockWaitNs = sWorkerBufferLockWaitTotalNs.load(std::memory_order_relaxed) - workerLockWaitAtStart;
      workerProcessBodyNs = sWorkerProcessBodyTotalNs.load(std::memory_order_relaxed) - workerBodyAtStart;
      if(sCpuProfileEnabled){
        const auto after=worker_cpu_profile();
        cpu={after.cpuNs-cpuBefore.cpuNs,after.wallNs-cpuBefore.wallNs,
             after.samples-cpuBefore.samples,after.failures-cpuBefore.failures};
      }
    }
    break;
  }
  }

  {
    auto lock=lock_buffer();
    sStreamBase = target;
    detail::sBufferSize = 0;
  }
  report_drain_profile(ownerWaitNs, workerProcessNs, workerBufferLockWaitNs,
                       workerProcessBodyNs, target - processStart,cpu);
  sPendingDraws = 0;
}

const uint8_t* get_buffer_data() { return detail::sBufferData; }
uint32_t get_buffer_size() { return detail::sBufferSize; }

void clear_buffer() {
  const uint64_t processed = sProcessed.load(std::memory_order_acquire);
  AURORA_ASSERT(sPublished.load(std::memory_order_acquire) == processed,
                "fifo::clear_buffer: published commands are still pending");
  auto lock=lock_buffer();
  sStreamBase = processed;
  detail::sBufferSize = 0;
  sPendingDraws = 0;
}

} // namespace aurora::gx::fifo
