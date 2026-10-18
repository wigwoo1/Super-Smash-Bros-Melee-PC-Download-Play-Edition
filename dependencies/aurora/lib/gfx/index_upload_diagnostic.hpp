#pragma once
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
namespace aurora::gfx::index_upload_diagnostic {
inline constexpr Module Log{"aurora::gfx::index_upload_diagnostic"};
struct Job {wgpu::Buffer buffer;std::vector<uint8_t> expected;uint32_t offset;std::atomic<int> status{0};};
inline std::vector<std::shared_ptr<Job>> pending;
inline bool enabled(){return std::getenv("AURORA_DIAGNOSTIC_INDEX_UPLOAD")!=nullptr;}
inline void drain(){
  for(auto it=pending.begin();it!=pending.end();){
    auto job=*it;int status=job->status.load(std::memory_order_acquire);
    if(!status){++it;continue;}
    unsigned mismatches=0;bool negative=false;
    if(status==1){
      const auto* data=static_cast<const uint8_t*>(job->buffer.GetConstMappedRange(0,job->expected.size()));
      if(data){
        for(size_t i=0;i<job->expected.size();++i) if(data[i]!=job->expected[i])++mismatches;
        negative=data[0]!=uint8_t(job->expected[0]^1u);
      }else mismatches=unsigned(job->expected.size());
      job->buffer.Unmap();
    }else mismatches=unsigned(job->expected.size());
    std::fprintf(stderr,"INDEX_UPLOAD_%s offset=%u bytes=%zu mismatches=%u negative_reference_rejected=%u\n",
                 !mismatches && negative?"PASS":"FAIL",job->offset,job->expected.size(),mismatches,unsigned(negative));
    it=pending.erase(it);
  }
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const wgpu::Buffer& source,
                   uint32_t start,uint32_t end,const std::vector<uint8_t>& snapshot){
  static unsigned count=0;
  if(!enabled() || !std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") || end<=start || count>=64 || snapshot.empty())return;
  AURORA_ASSERT(end<=snapshot.size(),"Index diagnostic snapshot extent");++count;
  auto job=std::make_shared<Job>();job->offset=start;
  job->expected.assign(snapshot.begin()+start,snapshot.begin()+end);
  const wgpu::BufferDescriptor desc{.label="Index upload diagnostic readback",
    .usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=end-start};
  job->buffer=webgpu::g_device.CreateBuffer(&desc);
  cmd.CopyBufferToBuffer(source,start,job->buffer,0,end-start);
  pending.push_back(job);
  std::fprintf(stderr,"INDEX_UPLOAD_QUEUED offset=%u bytes=%zu\n",start,job->expected.size());
  frame.afterSubmitCallbacks.emplace_back([job]{
    job->buffer.MapAsync(wgpu::MapMode::Read,0,job->expected.size(),wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});
  });
}
}
