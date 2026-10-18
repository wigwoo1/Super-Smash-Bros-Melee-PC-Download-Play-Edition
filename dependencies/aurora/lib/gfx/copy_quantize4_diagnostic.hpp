#pragma once
#include "tex_copy_conv.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
namespace aurora::gfx::copy_quantize4_diagnostic {
constexpr unsigned Cases = 1277;
constexpr unsigned Bytes = Cases * sizeof(uint32_t);
struct Job {
  wgpu::Buffer output, readback;
  std::array<uint32_t, Cases> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if (!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal = false;
  if (pending->status.load() == 1) {
    const auto* data = pending->readback.GetConstMappedRange(0, Bytes);
    equal = data && std::memcmp(data, pending->expected.data(), Bytes) == 0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr, "GPU_COPY_QUANTIZE4_%s raw_cases=256 filtered_cases=1021 compared_bytes=%u scope=production_wgsl_helper_only\n", equal ? "PASS" : "FAIL", Bytes);
  pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd, detail::FramePacket& frame) {
  static bool started = false;
  if (started || !std::getenv("AURORA_DIAGNOSTIC_COPY_QUANTIZE4")) return;
  started = true;
  auto job = std::make_shared<Job>();
  for (unsigned i = 0; i < Cases; ++i)
    job->expected[i] = i < 256 ? (i >> 4) * 17 : ((i - 256) / 64) * 17;
  const wgpu::BufferDescriptor outDesc{.label="Copy quantizer output", .usage=wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc, .size=Bytes};
  job->output = webgpu::g_device.CreateBuffer(&outDesc);
  const wgpu::BufferDescriptor readDesc{.label="Copy quantizer readback", .usage=wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, .size=Bytes};
  job->readback = webgpu::g_device.CreateBuffer(&readDesc);
  std::string shader(tex_copy_conv::diagnostic_quantize4_wgsl());
  shader += R"(
@group(0) @binding(0) var<storage,read_write> outputs: array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
  if (id.x >= 1277u) { return; }
  var value: f32;
  if (id.x < 256u) { value = f32(id.x) / 255.0; }
  else { value = f32(id.x - 256u) / 1020.0; }
  outputs[id.x] = u32(round(clamp(quantize4(value), 0.0, 1.0) * 255.0));
})";
  wgpu::ShaderSourceWGSL source{}; source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source, .label="Production copy quantizer"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production copy quantizer", .compute={.module=module, .entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const wgpu::BindGroupEntry entry{.binding=0, .buffer=job->output, .size=Bytes};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0), .entryCount=1, .entries=&entry};
  const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();
  pass.SetPipeline(pipeline); pass.SetBindGroup(0,bind); pass.DispatchWorkgroups((Cases+63)/64); pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,Bytes); pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {
    job->readback.MapAsync(wgpu::MapMode::Read,0,Bytes,wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView) { job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release); });
  });
  std::fprintf(stderr,"GPU_COPY_QUANTIZE4_QUEUED cases=%u compared_bytes=%u\n",Cases,Bytes);
}
}
