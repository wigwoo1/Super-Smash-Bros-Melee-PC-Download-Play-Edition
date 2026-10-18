#pragma once
#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace aurora::gfx::staged_texture_diagnostic {
inline constexpr Module Log{"aurora::gfx::staged_texture_diagnostic"};
struct Job {
  wgpu::Buffer buffer;
  std::vector<uint8_t> expected;
  uint32_t width, height, pitch, mip;
  uint64_t size;
  std::atomic<int> status{0};
};
// Accessed only by the render owner; completion callbacks publish status only.
inline std::vector<std::shared_ptr<Job>> pending;
inline void drain() {
  for (auto it = pending.begin(); it != pending.end();) {
    auto job = *it;
    const int status = job->status.load(std::memory_order_acquire);
    if (!status) { ++it; continue; }
    bool equal = status == 1;
    if (equal) {
      const auto* pixels = static_cast<const uint8_t*>(job->buffer.GetConstMappedRange(0, job->size));
      equal = pixels != nullptr;
      for (uint32_t y = 0; equal && y < job->height; ++y)
        equal = std::memcmp(pixels + uint64_t(y) * job->pitch,
                            job->expected.data() + uint64_t(y) * job->width * 4,
                            job->width * 4) == 0;
      job->buffer.Unmap();
    }
    std::fprintf(stderr, "STAGED_TEXTURE_%s mip=%u width=%u height=%u bytes=%zu\n",
                 equal ? "PASS" : "FAIL", job->mip, job->width, job->height, job->expected.size());
    it = pending.erase(it);
  }
}
inline void encode(const wgpu::CommandEncoder& cmd, detail::FramePacket& frame, const TextureUpload& upload) {
  if (upload.diagnosticExpected.empty()) return;
  auto job = std::make_shared<Job>();
  job->width = upload.size.width; job->height = upload.size.height;
  job->mip = upload.tex.mipLevel; job->pitch = (job->width * 4 + 255) & ~255u;
  job->size = uint64_t(job->pitch) * job->height;
  job->expected = upload.diagnosticExpected;
  AURORA_ASSERT(job->expected.size() == uint64_t(job->width) * job->height * 4,
                "Invalid staged texture diagnostic extent");
  const wgpu::BufferDescriptor descriptor{
      .label = "Staged texture readback",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, .size = job->size};
  job->buffer = webgpu::g_device.CreateBuffer(&descriptor);
  const wgpu::TexelCopyBufferInfo destination{
      .layout = {.offset = 0, .bytesPerRow = job->pitch, .rowsPerImage = job->height},
      .buffer = job->buffer};
  cmd.CopyTextureToBuffer(&upload.tex, &destination, &upload.size);
  pending.push_back(job);
  std::fprintf(stderr, "STAGED_TEXTURE_QUEUED mip=%u width=%u height=%u bytes=%zu\n",
               job->mip, job->width, job->height, job->expected.size());
  frame.afterSubmitCallbacks.emplace_back([job] {
    job->buffer.MapAsync(wgpu::MapMode::Read, 0, job->size, wgpu::CallbackMode::AllowSpontaneous,
        [job](wgpu::MapAsyncStatus status, wgpu::StringView) {
          job->status.store(status == wgpu::MapAsyncStatus::Success ? 1 : -1, std::memory_order_release);
        });
  });
}
} // namespace aurora::gfx::staged_texture_diagnostic
