#pragma once

#include "frame_packet.hpp"

namespace aurora::gfx {

void bind_texture_group(BindGroupRef ref, const wgpu::RenderPassEncoder& pass);

bool bind_pipeline(PipelineRef ref, const wgpu::RenderPassEncoder& pass);

namespace detail {
void encode_op(wgpu::CommandEncoder& encoder, FramePacket& frame, const FrameOp& op);
}

} // namespace aurora::gfx
