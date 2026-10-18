#pragma once
#include "types.hpp"
namespace aurora::gfx::diagnostic_capture {
void encode(const wgpu::CommandEncoder& cmd, const wgpu::Texture& texture, uint64_t frameId,
            bool viActive=false, bool viBlack=false, uint64_t viFrame=0,uint64_t renderFrame=0,
            bool compositedPresent=false);
void after_submit();
}
