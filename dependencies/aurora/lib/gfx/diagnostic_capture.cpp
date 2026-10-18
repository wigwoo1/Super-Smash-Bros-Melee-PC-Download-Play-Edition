#include "diagnostic_capture.hpp"
#include "draw_readiness_diagnostic.hpp"
#include "../webgpu/gpu.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <atomic>
#include <vector>
#include <string>
namespace aurora::gfx::diagnostic_capture {
namespace {
struct Capture { wgpu::Buffer buffer; uint32_t width,height,pitch; uint64_t size; bool bgra; std::string path; std::atomic<bool> ready{false}; bool success=false,mapping=false; std::vector<unsigned char> rgb; };
std::shared_ptr<Capture> pending;
bool requested=false;
bool armed=false;
uint64_t armedFrame=0;
}
void encode(const wgpu::CommandEncoder& cmd,const wgpu::Texture& texture,uint64_t frameId,
            bool viActive,bool viBlack,uint64_t viFrame,uint64_t renderFrame,bool compositedPresent) {
  const bool wantComposited=std::getenv("AURORA_DIAGNOSTIC_CAPTURE_COMPOSITED_PRESENT")!=nullptr;
  if(wantComposited!=compositedPresent) return;
  const char* path=std::getenv("AURORA_DIAGNOSTIC_COLOR_CAPTURE");
  if (!path || !*path || requested || !texture) return;
  if (!armed) { armed=true; armedFrame=frameId; }
  /* A scene can arm capture after thousands of title frames. Delay relative
   * to that request so the previous scene's presented texture can retire. */
  if(const char* setting=std::getenv("AURORA_DIAGNOSTIC_CAPTURE_DELAY")) {
    char* end=nullptr; const auto delay=std::strtoull(setting,&end,10);
    if(end!=setting && *end==0 && delay<=10000 &&
       (frameId<armedFrame || frameId-armedFrame<delay)) return;
  }
  uint64_t minimumFrame=20;
  if(const char* setting=std::getenv("AURORA_DIAGNOSTIC_CAPTURE_FRAME")) {
    char* end=nullptr; const auto value=std::strtoull(setting,&end,10);
    if(end!=setting && *end==0 && value>=20 && value<=100000) minimumFrame=value;
  }
  if (!path || !*path || requested || frameId<minimumFrame || !texture) return;
  // Controlled presentation comparisons must not consume a fallback texture.
  if(std::getenv("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_VI") && (!viActive || viBlack || !viFrame)) {
    static bool logged=false;
    if(!logged){std::fprintf(stderr,"COLOR_CAPTURE_WAITING_VI\n");logged=true;}
    return;
  }
  draw_readiness_diagnostic::capture(renderFrame?renderFrame:frameId);
  std::fprintf(stderr,"COLOR_CAPTURE_RENDER_SOURCE frame=%llu render_frame=%llu\n",static_cast<unsigned long long>(frameId),static_cast<unsigned long long>(renderFrame));
  requested=true;
  const auto format=texture.GetFormat();
  const bool bgra=format==wgpu::TextureFormat::BGRA8Unorm || format==wgpu::TextureFormat::BGRA8UnormSrgb;
  if ((!bgra && format!=wgpu::TextureFormat::RGBA8Unorm && format!=wgpu::TextureFormat::RGBA8UnormSrgb) || texture.GetSampleCount()!=1) {
    std::fprintf(stderr,"COLOR_CAPTURE_FAILED unsupported format/sample count\n"); return;
  }
  const uint32_t width=texture.GetWidth(),height=texture.GetHeight();
  std::fprintf(stderr,"COLOR_CAPTURE_QUEUED frame=%llu armed_frame=%llu width=%u height=%u format=%u\n",
    static_cast<unsigned long long>(frameId),static_cast<unsigned long long>(armedFrame),width,height,unsigned(format));
  std::fprintf(stderr,"COLOR_CAPTURE_VI_SOURCE frame=%llu vi_active=%u vi_black=%u vi_frame=%llu scope=exact_capture_ticket\n",
    static_cast<unsigned long long>(frameId),unsigned(viActive),unsigned(viBlack),static_cast<unsigned long long>(viFrame));
  if (!width || !height || width>4096 || height>4096) { std::fprintf(stderr,"COLOR_CAPTURE_FAILED extent\n"); return; }
  const uint32_t pitch=(width*4+255)&~255u;
  const uint64_t size=uint64_t(pitch)*height;
  const wgpu::BufferDescriptor descriptor{.label="Diagnostic color readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=size};
  auto buffer=webgpu::g_device.CreateBuffer(&descriptor);
  if (!buffer) { std::fprintf(stderr,"COLOR_CAPTURE_FAILED buffer\n"); return; }
  const wgpu::TexelCopyTextureInfo source{.texture=texture};
  const wgpu::TexelCopyBufferInfo dest{.layout={.offset=0,.bytesPerRow=pitch,.rowsPerImage=height},.buffer=buffer};
  const wgpu::Extent3D extent{.width=width,.height=height,.depthOrArrayLayers=1};
  cmd.CopyTextureToBuffer(&source,&dest,&extent);
  pending=std::make_shared<Capture>(); pending->buffer=buffer; pending->width=width; pending->height=height; pending->pitch=pitch; pending->size=size; pending->bgra=bgra; pending->path=path; pending->rgb.resize(uint64_t(width)*height*3);
}
void after_submit() {
  if (!pending) return;
  auto capture=pending;
  if(capture->ready.load(std::memory_order_acquire)) {
    bool ok=capture->success;
    if(ok) {
      capture->buffer.Unmap();
      FILE* file=std::fopen(capture->path.c_str(),"wb");
      ok=file && std::fprintf(file,"P6\n%u %u\n255\n",capture->width,capture->height)>0;
      if(ok) ok=std::fwrite(capture->rgb.data(),1,capture->rgb.size(),file)==capture->rgb.size();
      if(file) ok=std::fclose(file)==0 && ok;
    }
    pending.reset();
    std::fprintf(stderr,ok?"COLOR_CAPTURE_OK\n":"COLOR_CAPTURE_FAILED mapping/file\n");
    return;
  }
  if(capture->mapping) return;
  capture->mapping=true;
  capture->buffer.MapAsync(wgpu::MapMode::Read,0,capture->size,wgpu::CallbackMode::AllowSpontaneous,
    [capture](wgpu::MapAsyncStatus status,wgpu::StringView) {
      if(status==wgpu::MapAsyncStatus::Success) {
        const auto* pixels=static_cast<const unsigned char*>(capture->buffer.GetConstMappedRange(0,capture->size));
        if(pixels) {
          for(uint32_t y=0;y<capture->height;y++) for(uint32_t x=0;x<capture->width;x++) {
            const auto* p=pixels+uint64_t(y)*capture->pitch+x*4;
            auto* out=capture->rgb.data()+(uint64_t(y)*capture->width+x)*3;
            out[0]=p[capture->bgra?2:0];out[1]=p[1];out[2]=p[capture->bgra?0:2];
          }
          capture->success=true;
        }
      }
      capture->ready.store(true,std::memory_order_release);
    });
}
}

