#define AURORA_XFB_TESTING
#include "gx/xfb.hpp"
#include "gfx/uniform_range_cache.hpp"

#include <array>
#include <cstdio>
#include <cstring>

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::fprintf(stderr, "FAIL line=%d: %s\n", __LINE__, #condition);       \
      return 1;                                                                 \
    }                                                                           \
  } while (false)

int main() {
  {
    struct TestRange { uint32_t offset; uint32_t size; };
    const std::array<uint8_t, 4> payload{0x12, 0x34, 0x56, 0x78};
    const std::array<uint8_t, 8> staging{0xEE, 0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC};
    const TestRange range{1, 4};
    CHECK(aurora::gfx::detail::uniform_range_matches(range, payload.data(), payload.size(),
                                                      staging.data(), staging.size()));
    const std::array<uint8_t, 4> changed{0x12, 0x34, 0x56, 0x79};
    // Even if a caller supplies a colliding hash bucket, only identical bytes reuse a range.
    CHECK(!aurora::gfx::detail::uniform_range_matches(range, changed.data(), changed.size(),
                                                       staging.data(), staging.size()));
    CHECK(!aurora::gfx::detail::uniform_range_matches(range, payload.data(), payload.size(),
                                                       staging.data(), 4));
    CHECK(!aurora::gfx::detail::uniform_range_matches(range, payload.data(), payload.size() - 1,
                                                       staging.data(), staging.size()));
    CHECK(aurora::gfx::detail::uniform_range_hash(payload.data(), payload.size()) ==
          aurora::gfx::detail::uniform_range_hash(payload.data(), payload.size()));
    CHECK(aurora::gfx::detail::uniform_range_hash(payload.data(), payload.size()) !=
          aurora::gfx::detail::uniform_range_hash(changed.data(), changed.size()));
  }

  alignas(32) std::array<uint8_t, 128> destination{};
  using namespace aurora::gx::xfb;

  CHECK(initialize());
  diagnosticJobPoolStatsEnabled=true;
  CHECK(register_buffer(destination.data(), destination.size(), 16, 4));
  CHECK(rgbaStorage[0].size() == 16u * 4u * 4u);

  auto first = begin(destination.data(), 1, 16, 4);
  CHECK(first && first->rgba.size() == 16u * 4u * 4u);
  CHECK(first->width==16 && first->height==4);
  const auto firstFrameJobs=frame_jobs(1);
  CHECK(firstFrameJobs[0]==first && !firstFrameJobs[1] && !firstFrameJobs[2]);
  auto* const stableData = first->rgba.data();
  const auto stableCapacity = rgbaStorage[0].capacity();
  CHECK(stableData == rgbaStorage[0].data());

  // A completed slot converts its mapped pixels before allowing buffer reuse.
  std::memset(first->rgba.data(), 0, first->rgba.size());
  first->gpuDone.store(true, std::memory_order_release);
  first->mapped.store(true, std::memory_order_release);
  CHECK(pump());
  CHECK(frame_ready(1));
  CHECK(destination[0] == 16 && destination[1] == 128 &&
        destination[2] == 16 && destination[3] == 128);
  CHECK(pin(destination.data()));
  CHECK(!begin(destination.data(), 2, 16, 4));
  CHECK(unpin(destination.data()));

  // Keep the old shared job alive: payload storage stays slot-owned and is
  // reused only after both asynchronous completion flags were observed.
  auto second = begin(destination.data(), 2, 16, 4);
  CHECK(second && first->frame == 1 && second->frame == 2);
  CHECK(second.get()!=first.get());
  const auto* const reusableJob=second.get();
  CHECK(second->rgba.data() == stableData);
  CHECK(rgbaStorage[0].capacity() == stableCapacity);
  std::memset(second->rgba.data(), 0, second->rgba.size());
  second->gpuDone.store(true, std::memory_order_release);
  second->mapped.store(true, std::memory_order_release);
  CHECK(pump() && frame_ready(2));

  bool allocationFailure = false;
  failNextJobAllocationForTest = true;
  CHECK(!begin(destination.data(), 3, 16, 4, &allocationFailure));
  CHECK(allocationFailure && !failNextJobAllocationForTest);
  CHECK(!jobs[0] && !failed);
  CHECK(second->frame==2); // The retained prior handle is never mutated.
  second.reset(); // Leave the pool as the sole owner so the next job can reuse it.
  auto recovered = begin(destination.data(), 4, 16, 4, &allocationFailure);
  CHECK(recovered && recovered.get()==reusableJob && !allocationFailure && recovered->frame == 4);
  CHECK(recovered->width==16 && recovered->height==4);
  CHECK(diagnosticJobPoolCreated==2 && diagnosticJobPoolReused==1);
  recovered->gpuDone.store(true, std::memory_order_release);
  recovered->mapped.store(true, std::memory_order_release);
  CHECK(pump() && frame_ready(4));
  CHECK(unregister_buffer(destination.data()));
  CHECK(rgbaStorage[0].empty());
  CHECK(shutdown());
  std::puts("xfb_buffer_reuse_tests: PASS");
  return 0;
}
