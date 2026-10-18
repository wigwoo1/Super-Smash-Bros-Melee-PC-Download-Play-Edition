#pragma once
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
namespace aurora::gfx::uniform_diagnostic {
inline constexpr Module Log{"aurora::gfx::uniform_diagnostic"};
struct Job { wgpu::Buffer buffer; std::vector<uint8_t> expected; uint32_t offset; std::atomic<int> status{0}; };
inline std::vector<std::shared_ptr<Job>> pending;
inline bool enabled() { return std::getenv("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD") != nullptr; }
inline void drain() {
  for (auto it=pending.begin();it!=pending.end();) {
    auto job=*it; const int status=job->status.load(std::memory_order_acquire);
    if (!status) {++it;continue;}
    bool equal=false;
    if (status==1) {
      const auto* data=job->buffer.GetConstMappedRange(0,job->expected.size());
      equal=data && std::memcmp(data,job->expected.data(),job->expected.size())==0;
      job->buffer.Unmap();
    }
    std::fprintf(stderr,"UNIFORM_UPLOAD_%s offset=%u bytes=%zu\n",equal?"PASS":"FAIL",job->offset,job->expected.size());
    it=pending.erase(it);
  }
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,
                   const wgpu::Buffer& source,uint32_t start,uint32_t end,
                   const std::vector<uint8_t>& snapshot) {
  static unsigned count=0;
  if (!enabled() || end<=start || count>=128 || snapshot.empty()) return;
  AURORA_ASSERT(end<=snapshot.size(),"Uniform diagnostic snapshot extent");
  ++count;
  auto job=std::make_shared<Job>();job->offset=start;
  job->expected.assign(snapshot.data()+start,snapshot.data()+end);
  const wgpu::BufferDescriptor desc{.label="Uniform diagnostic readback",
    .usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=end-start};
  job->buffer=webgpu::g_device.CreateBuffer(&desc);
  cmd.CopyBufferToBuffer(source,start,job->buffer,0,end-start);
  pending.push_back(job);
  std::fprintf(stderr,"UNIFORM_UPLOAD_QUEUED offset=%u bytes=%zu\n",start,job->expected.size());
  frame.afterSubmitCallbacks.emplace_back([job] {
    job->buffer.MapAsync(wgpu::MapMode::Read,0,job->expected.size(),wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView) {
        job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);
      });
  });
}
}
