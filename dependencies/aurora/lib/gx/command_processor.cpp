#include "command_processor.hpp"

#include "../gfx/depth_peek.hpp"
#include "../gfx/recording.hpp"
#include "../internal.hpp"
#include "../indexed_vertex_extent.h"
#include "dolphin/gx/GXAurora.h"
#include "gx.hpp"
#include "pipeline.hpp"
#include "regs.hpp"
#include "shader_info.hpp"
#include "../bounded_state_cache.hpp"
#include "../binding_profile.hpp"
#include "../recording_diagnostics.hpp"
#include "texture.hpp"
#include <aurora/diagnostics.h>

#include <tracy/Tracy.hpp>

#include <cstdint>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <span>
#include <set>
#include <tuple>

namespace aurora::gx::fifo {
namespace {
constexpr Module Log{"aurora::gx::fifo"};

struct CommandProfile {
  bool checked = false;
  bool enabled = false;
  uint64_t processNs = 0;
  uint64_t commandNs[8]{};
  uint64_t commandCount[8]{};
  uint64_t drawCalls = 0, mergedDrawCalls = 0;
  uint64_t drawIndexedNs = 0, drawStateNs = 0, drawVertexUploadNs = 0;
  uint64_t drawMergeNs = 0, drawSubmitNs = 0;
  uint64_t drawSubmitCalls = 0, drawSubmitSetupNs = 0, drawPipelineNs = 0;
  uint64_t drawBindingsNs = 0, drawUniformNs = 0, drawCommandNs = 0;
  uint64_t pipelineCacheHits = 0, pipelineRebuilds = 0;
  uint64_t bindingCacheHits = 0, bindingRebuilds = 0;
  uint64_t uniformCacheHits = 0, uniformRebuilds = 0;
  uint64_t shaderInfoReuse = 0, shaderInfoBuilds = 0;
  uint64_t shaderInfoMultiHits = 0, shaderInfoMultiMisses = 0, shaderInfoChecks = 0;
  uint64_t bytes = 0;
  unsigned processCalls = 0;
} sCommandProfile;

struct CommandDiagnostics {
  bool pacingProfile = false;
  bool fifoTrace = false;
  std::atomic<bool> drawUv{false};
  bool renderState = false;
  bool vertexReplayDispatch = false;
  std::atomic<bool> uniformUpload{false};
  bool primitiveHash = false;
  bool drawMergeAudit = false;
  bool disableDrawMerge = false;
  bool verifyShaderInfoReuse = false;
  bool disableShaderInfoCache = true;

  CommandDiagnostics() noexcept {
    pacingProfile = std::getenv("MELEE_DIAGNOSTIC_PACING_PROFILE") != nullptr;
    fifoTrace = std::getenv("AURORA_DIAGNOSTIC_Z_FIFO") != nullptr;
    drawUv.store(std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") != nullptr, std::memory_order_relaxed);
    renderState = std::getenv("AURORA_DIAGNOSTIC_RENDER_STATE") != nullptr;
    vertexReplayDispatch = std::getenv("AURORA_DIAGNOSTIC_VERTEX_REPLAY_DISPATCH") != nullptr;
    uniformUpload.store(std::getenv("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD") != nullptr, std::memory_order_relaxed);
    primitiveHash = std::getenv("AURORA_DIAGNOSTIC_PRIMITIVE_HASH") != nullptr;
    drawMergeAudit = std::getenv("AURORA_DIAGNOSTIC_DRAW_MERGE_AUDIT") != nullptr;
    disableDrawMerge = std::getenv("AURORA_DIAGNOSTIC_DISABLE_DRAW_MERGE") != nullptr;
    verifyShaderInfoReuse = std::getenv("AURORA_DIAGNOSTIC_VERIFY_SHADER_INFO_REUSE") != nullptr;
    // Matched controls showed no consistent frame-rate benefit. Keep the
    // bounded experiment opt-in until broader measurements justify enabling it.
    disableShaderInfoCache = std::getenv("AURORA_DIAGNOSTIC_SHADER_INFO_CACHE") == nullptr ||
                             std::getenv("AURORA_DIAGNOSTIC_DISABLE_SHADER_INFO_CACHE") != nullptr;
  }
};

CommandDiagnostics& diagnostics() noexcept {
  static CommandDiagnostics value;
  return value;
}

void refresh_dynamic_diagnostic_flags() noexcept {
  refresh_uniform_diagnostic_flags();
  auto& value = diagnostics();
  value.drawUv.store(std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") != nullptr, std::memory_order_relaxed);
  value.uniformUpload.store(std::getenv("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD") != nullptr,
                            std::memory_order_relaxed);
  binding_diagnostics::refresh();
  recording_diagnostics::refresh();
  if(binding_profile::enabled())
    std::fprintf(stderr,"NATIVE_BINDING_DIAGNOSTICS_REFRESH flags=%u\n",binding_diagnostics::snapshot().flags);
}

uint64_t profile_now_ns() noexcept {
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}

unsigned profile_category(u8 cmd, u8 opcode) noexcept;

class Reader {
public:
  explicit Reader(std::span<const u8> data) noexcept : mData{data} {}

  [[nodiscard]] bool empty() const noexcept { return mPos == mData.size(); }
  [[nodiscard]] size_t offset() const noexcept { return mPos; }
  [[nodiscard]] size_t size() const noexcept { return mData.size(); }
  [[nodiscard]] size_t remaining() const noexcept { return mData.size() - mPos; }
  [[nodiscard]] const u8* data() const noexcept { return mData.data(); }

  template <typename T>
    requires(std::is_arithmetic_v<T>)
  T read() noexcept {
    const auto bytes = take(sizeof(T));
    return read_bits<T>(bytes.data());
  }

  std::span<const u8> take(size_t count) noexcept {
    AURORA_ASSERT(count <= remaining(), "FIFO read overrun: need {} bytes at offset {}, have {}", count, mPos,
                  remaining());
    const auto bytes = mData.subspan(mPos, count);
    mPos += count;
    return bytes;
  }

  void skip(size_t count) noexcept {
    AURORA_ASSERT(count <= remaining(), "FIFO read overrun: need {} bytes at offset {}, have {}", count, mPos,
                  remaining());
    mPos += count;
  }

  std::string read_string() noexcept {
    const auto len = read<uint16_t>();
    const auto bytes = take(len);
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
  }

private:
  std::span<const u8> mData;
  size_t mPos = 0;
};

u16 prepare_idx_buffer(ByteBuffer& buf, GXPrimitive prim, u16 vtxStart, u16 vtxCount) noexcept {
  u16 numIndices = 0;
  if (prim == GX_QUADS) {
    buf.reserve_extra((vtxCount / 4) * 6 * sizeof(u16));

    for (u16 v = 0; v < vtxCount; v += 4) {
      u16 idx0 = vtxStart + v;
      u16 idx1 = vtxStart + v + 1;
      u16 idx2 = vtxStart + v + 2;
      u16 idx3 = vtxStart + v + 3;

      buf.append(idx0);
      buf.append(idx1);
      buf.append(idx2);
      numIndices += 3;

      buf.append(idx2);
      buf.append(idx3);
      buf.append(idx0);
      numIndices += 3;
    }
  } else if (prim == GX_TRIANGLES) {
    buf.reserve_extra(vtxCount * sizeof(u16));
    for (u16 v = 0; v < vtxCount; ++v) {
      const u16 idx = vtxStart + v;
      buf.append(idx);
      ++numIndices;
    }
  } else if (prim == GX_TRIANGLEFAN) {
    buf.reserve_extra(((u32(vtxCount) - 3) * 3 + 3) * sizeof(u16));
    for (u16 v = 0; v < vtxCount; ++v) {
      const u16 idx = vtxStart + v;
      if (v < 3) {
        buf.append(idx);
        ++numIndices;
        continue;
      }
      buf.append(std::array{vtxStart, static_cast<u16>(idx - 1), idx});
      numIndices += 3;
    }
  } else if (prim == GX_TRIANGLESTRIP) {
    buf.reserve_extra(((static_cast<u32>(vtxCount) - 3) * 3 + 3) * sizeof(u16));
    for (u16 v = 0; v < vtxCount; ++v) {
      const u16 idx = vtxStart + v;
      if (v < 3) {
        buf.append(idx);
        ++numIndices;
        continue;
      }
      if ((v & 1) == 0) {
        buf.append(std::array{static_cast<u16>(idx - 2), static_cast<u16>(idx - 1), idx});
      } else {
        buf.append(std::array{static_cast<u16>(idx - 1), static_cast<u16>(idx - 2), idx});
      }
      numIndices += 3;
    }
  } else if (prim == GX_LINES || prim == GX_LINESTRIP || prim == GX_POINTS) {
    buf.reserve_extra(6 * sizeof(u16));
    buf.append<u16>(0);
    buf.append<u16>(1);
    buf.append<u16>(3);
    buf.append<u16>(3);
    buf.append<u16>(2);
    buf.append<u16>(0);
    numIndices = 6;
  } else
    UNLIKELY FATAL("unsupported primitive type {}", static_cast<u32>(prim));
  return numIndices;
}

// GX FIFO opcodes - use CP_ prefix to avoid clashing with GXCommandList.h macros
constexpr u8 CP_CMD_NOP = GX_NOP;
constexpr u8 CP_CMD_LOAD_CP_REG = GX_LOAD_CP_REG;
constexpr u8 CP_CMD_LOAD_XF_REG = GX_LOAD_XF_REG;
constexpr u8 CP_CMD_LOAD_INDX_A = GX_LOAD_INDX_A;
constexpr u8 CP_CMD_LOAD_INDX_B = GX_LOAD_INDX_B;
constexpr u8 CP_CMD_LOAD_INDX_C = GX_LOAD_INDX_C;
constexpr u8 CP_CMD_LOAD_INDX_D = GX_LOAD_INDX_D;
constexpr u8 CP_CMD_CALL_DL = GX_CMD_CALL_DL;
constexpr u8 CP_CMD_INVAL_VTX = GX_CMD_INVL_VC;
constexpr u8 CP_CMD_LOAD_BP_REG = GX_LOAD_BP_REG & GX_OPCODE_MASK;

// Primitive type mask
constexpr u8 CP_OPCODE_MASK = GX_OPCODE_MASK;
constexpr u8 CP_VAT_MASK = GX_VAT_MASK;

unsigned profile_category(u8 cmd, u8 opcode) noexcept {
  if (opcode == CP_CMD_NOP) return 0;
  if (opcode == CP_CMD_LOAD_BP_REG) return 1;
  if (opcode == CP_CMD_LOAD_CP_REG) return 2;
  if (opcode == CP_CMD_LOAD_XF_REG) return 3;
  if (opcode == CP_CMD_LOAD_INDX_A || opcode == CP_CMD_LOAD_INDX_B ||
      opcode == CP_CMD_LOAD_INDX_C || opcode == CP_CMD_LOAD_INDX_D) return 4;
  if (cmd >= 0x80) return 5;
  if (opcode == GX_AURORA) return 6;
  return 7;
}

struct DrawCache {
  PipelineConfig config{};
  ShaderInfo shaderInfo{};
  gfx::PipelineRef pipelineRef{};
  GXBindGroups bindGroups{};
  uint64_t bindGeneration = 0;
  GXVtxFmt fmt = GX_MAX_VTXFMT;
  u8 lineMode = 0;
  bool hasPipeline = false;
  bool shaderLayoutHashEnabled = false;
  u32 shaderUniformAlignment = 0;
  gfx::Range uniformRange{};
  GXVtxFmt lastDrawFmt = GX_MAX_VTXFMT;
};
DrawCache sDrawCache;
struct ShaderInfoKey {
  ShaderConfig config{};
  u32 alignment=0;
  bool diagnosticLayout=false;
  bool operator==(const ShaderInfoKey& other) const {
    return alignment==other.alignment && diagnosticLayout==other.diagnosticLayout && config==other.config;
  }
};
// FIFO-reader-owned, bounded plain metadata only: no GPU/resource references.
// Alignment is device-dependent and belongs in the key across reinitialization.
BoundedStateCache<ShaderInfoKey,ShaderInfo> sShaderInfoCache;

u8 line_mode_for_prim(GXPrimitive prim) noexcept {
  switch (prim) {
  case GX_LINES:
    return 1;
  case GX_LINESTRIP:
    return 2;
  case GX_POINTS:
    return 3;
  default:
    return 0;
  }
}
} // namespace

static void handle_draw(u8 cmd, Reader& reader) noexcept;
static void handle_aurora(Reader& reader) noexcept;

void process(const u8* data, u32 size) noexcept {
  ZoneScoped;
  const graphics_quality::ProcessingContext qualityContext;
  if (!sCommandProfile.checked) {
    sCommandProfile.enabled = diagnostics().pacingProfile;
    sCommandProfile.checked = true;
  }
  const bool profile = sCommandProfile.enabled;
  const uint64_t processStartNs = profile ? profile_now_ns() : 0;
  Reader reader{{data, size}};
  static unsigned traceCalls;
  const bool trace=diagnostics().fifoTrace && traceCalls++<32;
  unsigned traceCommands=0;
  if(trace) std::fprintf(stderr,"Z_FIFO_BEGIN bytes=%u\n",size);

  while (!reader.empty()) {
    const uint64_t commandStartNs = profile ? profile_now_ns() : 0;
    const u8 cmd = reader.read<u8>();
    if(trace && traceCommands++<128) std::fprintf(stderr,"Z_FIFO_COMMAND offset=%zu opcode=%02x\n",reader.offset()-1,cmd);
    u8 opcode = cmd & CP_OPCODE_MASK;

    switch (opcode) {
    case CP_CMD_NOP:
      break;

    case CP_CMD_LOAD_BP_REG: {
      const u32 value = reader.read<u32>();
      const bool traceBp = trace && traceCommands <= 128;
      if (traceBp) std::fprintf(stderr, "Z_FIFO_BP_BEGIN value=%08x\n", value);
      handle_bp(value);
      if (traceBp) std::fprintf(stderr, "Z_FIFO_BP_END value=%08x\n", value);
      break;
    }

    case CP_CMD_LOAD_CP_REG: {
      const u8 addr = reader.read<u8>();
      handle_cp(addr, reader.read<u32>());
      break;
    }

    case CP_CMD_LOAD_XF_REG: {
      const u32 header = reader.read<u32>();
      const u32 count = ((header >> 16) & 0xFFFF) + 1;
      const u16 addr = header & 0xFFFF;
      handle_xf(addr, reader.take(count * sizeof(u32)));
      break;
    }

    case CP_CMD_LOAD_INDX_A:
    case CP_CMD_LOAD_INDX_B:
    case CP_CMD_LOAD_INDX_C:
    case CP_CMD_LOAD_INDX_D: {
      ZoneScopedN("LOAD_INDX");
      const u32 arrayType = GX_POS_MTX_ARRAY + (opcode - CP_CMD_LOAD_INDX_A) / 0x08;
      const u16 srcArrayIdx = reader.read<u16>();
      const u16 addrLen = reader.read<u16>();

      const u16 len = (addrLen >> 12) + 1;
      const u16 dstAddr = addrLen & 0x0FFF;
      auto const& array = g_gxState.arrays[arrayType];
      const u32 srcOffset = static_cast<u32>(srcArrayIdx) * array.stride;
      const u32 srcSize = static_cast<u32>(len) * sizeof(u32);
      AURORA_ASSERT(array.data != nullptr, "indexed XF load from unmapped array {}", arrayType);
      AURORA_ASSERT(srcOffset <= array.size && srcSize <= array.size - srcOffset,
                    "indexed XF load outside array {}: offset={}, size={}, array size={}", arrayType, srcOffset,
                    srcSize, array.size);
      auto const* srcData = static_cast<const u8*>(array.data) + srcOffset;
      if (!copy_xf_data(dstAddr, srcData, len, array.le ? std::endian::little : std::endian::big)) {
#ifndef NDEBUG
        Log.debug("Unimplemented indexed XF load (opcode 0x{:02X}, dstAddr=%04x)", opcode, dstAddr);
#endif
      }
      break;
    }

    case CP_CMD_CALL_DL: {
      // Call display list: 8 bytes (address + size)
      Log.warn("Ignoring nested GX_CMD_CALL_DL");
      reader.skip(8);
      break;
    }

    case CP_CMD_INVAL_VTX: {
      // Invalidate vertex cache
      break;
    }

    case GX_AURORA: {
      handle_aurora(reader);
      break;
    }

    // Draw commands: 0x80-0xBF
    case GX_DRAW_QUADS:
    case GX_DRAW_TRIANGLES:
    case GX_DRAW_TRIANGLE_STRIP:
    case GX_DRAW_TRIANGLE_FAN:
    case GX_DRAW_LINES:
    case GX_DRAW_LINE_STRIP:
    case GX_DRAW_POINTS: {
      handle_draw(cmd, reader);
      break;
    }

    default:
      // Check if it's a draw command (0x80-0xBF range)
      if (cmd >= 0x80) {
        handle_draw(cmd, reader);
      } else {
        // Hex dump surrounding bytes for debugging
        {
          const size_t pos = reader.offset();
          size_t dumpStart = (pos > 17) ? pos - 17 : 0;
          size_t dumpEnd = (pos + 16 < size) ? pos + 16 : size;
          std::string hex;
          for (size_t i = dumpStart; i < dumpEnd; i++) {
            if (i == pos - 1)
              hex += fmt::format("[{:02x}]", data[i]);
            else
              hex += fmt::format(" {:02x}", data[i]);
          }
          Log.error("  hex dump (pos {}-{}):{}", dumpStart, dumpEnd - 1, hex);
        }
        FATAL("command_processor: unknown opcode 0x{:02X} at pos {}", cmd, reader.offset() - 1);
      }
      break;
    }

    if (profile) {
      const unsigned category = profile_category(cmd, opcode);
      ++sCommandProfile.commandCount[category];
      sCommandProfile.commandNs[category] += profile_now_ns() - commandStartNs;
    }
  }
  if (profile) {
    sCommandProfile.processNs += profile_now_ns() - processStartNs;
    sCommandProfile.bytes += size;
    if (++sCommandProfile.processCalls == 60) {
      static constexpr const char* names[] = {"nop", "bp", "cp", "xf", "indexed", "draw", "aurora", "misc"};
      std::fprintf(stderr, "NATIVE_GX_COMMAND_PROFILE calls=%u bytes_avg=%llu process_avg_us=%llu",
                   sCommandProfile.processCalls,
                   static_cast<unsigned long long>(sCommandProfile.bytes / sCommandProfile.processCalls),
                   static_cast<unsigned long long>(sCommandProfile.processNs / 1000 / sCommandProfile.processCalls));
      for (unsigned i = 0; i < 8; ++i) {
        std::fprintf(stderr, " %s_count=%llu %s_avg_us=%llu", names[i],
                     static_cast<unsigned long long>(sCommandProfile.commandCount[i] / sCommandProfile.processCalls),
                     names[i],
                     static_cast<unsigned long long>(sCommandProfile.commandNs[i] / 1000 / sCommandProfile.processCalls));
      }
      std::fputc('\n', stderr);
      std::fprintf(stderr,
          "NATIVE_GX_DRAW_PROFILE calls_avg=%llu merged_avg=%llu indexed_avg_us=%llu "
          "state_avg_us=%llu vertex_upload_avg_us=%llu merge_avg_us=%llu submit_avg_us=%llu\n",
          static_cast<unsigned long long>(sCommandProfile.drawCalls / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.mergedDrawCalls / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawIndexedNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawStateNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawVertexUploadNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawMergeNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawSubmitNs / 1000 / sCommandProfile.processCalls));
      std::fprintf(stderr,
          "NATIVE_GX_DRAW_SUBMIT_PROFILE calls_avg=%llu setup_avg_us=%llu pipeline_avg_us=%llu "
          "bindings_avg_us=%llu uniform_avg_us=%llu command_avg_us=%llu\n",
          static_cast<unsigned long long>(sCommandProfile.drawSubmitCalls / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawSubmitSetupNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawPipelineNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawBindingsNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawUniformNs / 1000 / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.drawCommandNs / 1000 / sCommandProfile.processCalls));
      std::fprintf(stderr,
          "NATIVE_GX_DRAW_CACHE_PROFILE pipeline_hits_avg=%llu pipeline_rebuilds_avg=%llu "
          "binding_hits_avg=%llu binding_rebuilds_avg=%llu uniform_hits_avg=%llu uniform_rebuilds_avg=%llu "
          "shader_info_reuse_avg=%llu shader_info_builds_avg=%llu\n",
          static_cast<unsigned long long>(sCommandProfile.pipelineCacheHits / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.pipelineRebuilds / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.bindingCacheHits / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.bindingRebuilds / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.uniformCacheHits / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.uniformRebuilds / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.shaderInfoReuse / sCommandProfile.processCalls),
          static_cast<unsigned long long>(sCommandProfile.shaderInfoBuilds / sCommandProfile.processCalls));
      std::fprintf(stderr,"NATIVE_SHADER_INFO_CACHE hits=%llu misses=%llu checks=%llu capacity=%zu bytes=%zu disabled=%u\n",
          (unsigned long long)sCommandProfile.shaderInfoMultiHits,
          (unsigned long long)sCommandProfile.shaderInfoMultiMisses,
          (unsigned long long)sCommandProfile.shaderInfoChecks,
          decltype(sShaderInfoCache)::capacity,sizeof(sShaderInfoCache),unsigned(diagnostics().disableShaderInfoCache));
      binding_profile::report(sCommandProfile.drawBindingsNs);
      sCommandProfile.processNs = sCommandProfile.bytes = 0;
      sCommandProfile.drawCalls = sCommandProfile.mergedDrawCalls = 0;
      sCommandProfile.drawIndexedNs = sCommandProfile.drawStateNs = 0;
      sCommandProfile.drawVertexUploadNs = sCommandProfile.drawMergeNs = sCommandProfile.drawSubmitNs = 0;
      sCommandProfile.drawSubmitCalls = sCommandProfile.drawSubmitSetupNs = 0;
      sCommandProfile.drawPipelineNs = sCommandProfile.drawBindingsNs = 0;
      sCommandProfile.drawUniformNs = sCommandProfile.drawCommandNs = 0;
      sCommandProfile.pipelineCacheHits = sCommandProfile.pipelineRebuilds = 0;
      sCommandProfile.bindingCacheHits = sCommandProfile.bindingRebuilds = 0;
      sCommandProfile.uniformCacheHits = sCommandProfile.uniformRebuilds = 0;
      sCommandProfile.shaderInfoReuse = sCommandProfile.shaderInfoBuilds = 0;
      sCommandProfile.shaderInfoMultiHits = sCommandProfile.shaderInfoMultiMisses = sCommandProfile.shaderInfoChecks = 0;
      sCommandProfile.processCalls = 0;
      for (unsigned i = 0; i < 8; ++i) {
        sCommandProfile.commandNs[i] = 0;
        sCommandProfile.commandCount[i] = 0;
      }
    }
  }
}

[[noreturn]] static void handle_draw_overrun(size_t totalVtxBytes, const Reader& reader) noexcept {
  // Hex dump around the draw command for debugging
  const size_t pos = reader.offset();
  const size_t size = reader.size();
  const u8* data = reader.data();
  size_t cmdPos = pos - 2 - 1; // opcode byte position (before vtxCount and pos++)
  size_t dumpStart = (cmdPos > 16) ? cmdPos - 16 : 0;
  size_t dumpEnd = (cmdPos + 32 < size) ? cmdPos + 32 : size;
  std::string hex;
  for (size_t i = dumpStart; i < dumpEnd; i++) {
    if (i == cmdPos)
      hex += fmt::format("[{:02x}]", data[i]);
    else
      hex += fmt::format(" {:02x}", data[i]);
  }
  Log.error("  hex dump around draw cmd (pos {}-{}):{}", dumpStart, dumpEnd - 1, hex);
  FATAL("draw vertex data overrun: need {} bytes at pos {}, have {}", totalVtxBytes, pos, reader.remaining());
}

static u32 calc_vtx_size(GXVtxFmt fmt) noexcept {
  u32 vtxSize = 0;
  const auto& vtxFmt = g_gxState.vtxFmts[fmt];
  for (int i = GX_VA_PNMTXIDX; i <= GX_VA_TEX7; ++i) {
    const auto& attrFmt = vtxFmt.attrs[i];
    switch (g_gxState.vtxDesc[i]) {
    case GX_NONE:
      break;
    case GX_DIRECT: {
      const auto attr = static_cast<GXAttr>(i);
      vtxSize += comp_type_size(attr, attrFmt.type) * comp_cnt_count(attr, attrFmt.cnt);
      break;
    }
    case GX_INDEX8:
      vtxSize += i == GX_VA_NRM && attrFmt.cnt == GX_NRM_NBT3 ? 3 : 1;
      break;
    case GX_INDEX16:
      vtxSize += i == GX_VA_NRM && attrFmt.cnt == GX_NRM_NBT3 ? 6 : 2;
      break;
    }
  }
  g_gxState.lastVtxFmt = fmt;
  g_gxState.lastVtxSize = vtxSize;
  return vtxSize;
}

/* Derive each indexed array's upload prefix from this draw's packed indices.
 * Existing ranges stay immutable for already recorded draws. Growing a range
 * changes immediate offsets, so that draw must not merge with the previous one. */
static bool prepare_indexed_storage(GXVtxFmt fmt,u16 count,u32 vertexStride,
                                   std::span<const uint8_t> vertices) {
  bool changed=false;
  u32 offset=0;
  const auto& formats=g_gxState.vtxFmts[fmt];
  for(int i=GX_VA_PNMTXIDX;i<=GX_VA_TEX7;++i){
    const auto type=g_gxState.vtxDesc[i];
    if(type==GX_NONE)continue;
    const auto attr=static_cast<GXAttr>(i);
    const auto& format=formats.attrs[i];
    const u32 component=comp_type_size(attr,format.type);
    const u32 components=comp_cnt_count(attr,format.cnt);
    if(type==GX_DIRECT){
      if (attr >= GX_VA_TEX0MTXIDX && attr <= GX_VA_TEX7MTXIDX && count &&
          diagnostics().drawUv.load(std::memory_order_relaxed)) {
        static std::set<std::tuple<u32, u32, u32, u32>> selectors;
        u32 maskLo = 0, maskHi = 0, invalid = 0;
        AURORA_ASSERT(component == 1 && components == 1 &&
                      uint64_t(count - 1) * vertexStride + offset < vertices.size(),
                      "Texture matrix selector extent");
        for (u32 vertex = 0; vertex < count; ++vertex) {
          const u32 selector = vertices[vertex * vertexStride + offset] & 63u;
          if (selector < 32) maskLo |= 1u << selector;
          else maskHi |= 1u << (selector - 32);
          if (selector > 60 || selector % 3 != 0) ++invalid;
        }
        const auto key = std::make_tuple(u32(i), maskLo, maskHi, invalid);
        if (selectors.size() < 128 && selectors.insert(key).second)
          std::fprintf(stderr,
              "DRAW_TEXMTX_SELECTORS attr=%d count=%u low=%u high=%u invalid=%u\n",
              i, count, maskLo, maskHi, invalid);
      }
      offset+=component*components;continue;
    }
    const u32 width=type==GX_INDEX8?1:2;
    const bool nbt3=attr==GX_VA_NRM&&format.cnt==GX_NRM_NBT3;
    const u32 indices=nbt3?3:1;
    auto& array=g_gxState.arrays[i];
    u32 needed=0;
    AURORA_ASSERT(MeleePc_IndexedVertexSliceExtent(vertices.data(),vertices.size(),count,
       vertexStride,offset,width,indices,array.stride,component*(nbt3?3:components),
       nbt3?component*3:0,array.size,&needed),
       "indexed vertex range outside bound attr {} fmt {} count {} vertexStride {} offset {} indexWidth {} indices {} arrayStride {} elementBytes {} sliceStep {} bound {}",
       i,static_cast<u32>(fmt),count,vertexStride,offset,width,indices,array.stride,
       component*(nbt3?3:components),nbt3?component*3:0,array.size);
    if (count && offset <= vertices.size() && width <= vertices.size() - offset &&
        i >= GX_VA_TEX0 && i <= GX_VA_TEX7 &&
        diagnostics().drawUv.load(std::memory_order_relaxed)) {
      static std::set<std::tuple<uintptr_t, u32, u32, u32>> reported;
      const auto key = std::make_tuple(reinterpret_cast<uintptr_t>(array.data),
                                      needed, u32(i), u32(fmt));
      if (reported.size() < 256 && reported.insert(key).second) {
        uint64_t sourceHash = 14695981039346656037ull;
        const auto* bytes = static_cast<const uint8_t*>(array.data);
        for (u32 n = 0; n < needed; ++n)
          sourceHash = (sourceHash ^ bytes[n]) * 1099511628211ull;
        const u32 firstIndex = width == 1 ? vertices[offset] :
            (u32(vertices[offset]) << 8) | vertices[offset + 1];
        std::fprintf(stderr,
            "DRAW_UV_RANGE fmt=%u attr=%d count=%u packed_stride=%u packed_offset=%u index_width=%u first_index=%u array_stride=%u component_type=%u components=%u fraction=%u needed=%u bound=%u cached_before=%u source_hash=%llu\n",
            static_cast<u32>(fmt), i, count, vertexStride, offset, width,
            firstIndex, array.stride, static_cast<u32>(format.type), components,
            static_cast<unsigned>(format.frac), needed, array.size,
            array.cachedRange.size, static_cast<unsigned long long>(sourceHash));
      }
    }
    if (count && offset <= vertices.size() && width <= vertices.size() - offset &&
        i == GX_VA_POS &&
        diagnostics().drawUv.load(std::memory_order_relaxed)) {
      static std::set<std::tuple<uintptr_t, u32, u32, u32>> reported;
      const auto key = std::make_tuple(reinterpret_cast<uintptr_t>(array.data),
                                      needed, u32(i), u32(fmt));
      if (reported.size() < 256 && reported.insert(key).second) {
        uint64_t sourceHash = 14695981039346656037ull;
        const auto* bytes = static_cast<const uint8_t*>(array.data);
        for (u32 n = 0; n < needed; ++n)
          sourceHash = (sourceHash ^ bytes[n]) * 1099511628211ull;
        const u32 firstIndex = width == 1 ? vertices[offset] :
            (u32(vertices[offset]) << 8) | vertices[offset + 1];
        std::fprintf(stderr,
            "DRAW_POS_RANGE fmt=%u attr=%d count=%u packed_stride=%u packed_offset=%u index_width=%u first_index=%u array_stride=%u component_type=%u components=%u fraction=%u needed=%u bound=%u cached_before=%u source_hash=%llu\n",
            static_cast<u32>(fmt), i, count, vertexStride, offset, width,
            firstIndex, array.stride, static_cast<u32>(format.type), components,
            static_cast<unsigned>(format.frac), needed, array.size,
            array.cachedRange.size, static_cast<unsigned long long>(sourceHash));
      }
    }
    offset+=width*indices;
    if(needed && array.cachedRange.size<needed){
      /* Shader storage fetches address u32 words. Pad the last word without
       * reading beyond the registered source allocation. */
      const u32 padded=(needed+3u)&~3u;
      AURORA_ASSERT(padded>=needed,"indexed vertex range padding overflow");
      if(padded<=array.size){
        array.cachedRange=gfx::push_storage(static_cast<const uint8_t*>(array.data),padded);
      }else{
        std::vector<uint8_t> copy(padded,0);
        std::memcpy(copy.data(),array.data,needed);
        array.cachedRange=gfx::push_storage(copy.data(),copy.size());
      }
      changed=true;
    }
    if (i == GX_VA_POS && needed && diagnostics().drawUv.load(std::memory_order_relaxed)) {
      static u32 samples = 0;
      if (samples < 512) {
        const bool equal = gfx::diagnostic_storage_range_matches(
            array.cachedRange, static_cast<const uint8_t*>(array.data), needed);
        std::fprintf(stderr,
            "DRAW_POS_STAGING sample=%u count=%u needed=%u offset=%u size=%u source_equal=%u\n",
            ++samples, count, needed, array.cachedRange.offset, array.cachedRange.size, equal ? 1u : 0u);
      }
    }
  }
  AURORA_ASSERT(offset==vertexStride,"indexed vertex layout stride mismatch");
  return changed;
}

static void push_gx_draw(GXPrimitive prim, GXVtxFmt fmt, u16 vtxCount, gfx::Range vertRange, gfx::Range idxRange,
                         u32 numIndices) noexcept {
  const bool profile = sCommandProfile.enabled;
  uint64_t phaseStartNs = profile ? profile_now_ns() : 0;
  auto& state = g_gxState;
  auto& cache = sDrawCache;

  DrawImmediateData immediates{.vtxStart = vertRange.offset, .currentPnMtx = state.currentPnMtx};
  for (int i = GX_VA_POS; i <= GX_VA_TEX7; ++i) {
    if (state.vtxDesc[i] != GX_INDEX8 && state.vtxDesc[i] != GX_INDEX16) {
      continue;
    }
    auto& array = state.arrays[i];
    if (array.cachedRange.size == 0) {
      AURORA_ASSERT(vtxCount==0,"indexed storage range not prepared");
    }
    immediates.arrayStart[i - GX_VA_POS] = array.cachedRange.offset;
  }
  if (profile) {
    sCommandProfile.drawSubmitSetupNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
    ++sCommandProfile.drawSubmitCalls;
  }

  const u8 lineMode = line_mode_for_prim(prim);
  const bool pipelineValid = cache.hasPipeline && (state.dirty & DirtyPipeline) == 0 && cache.fmt == fmt &&
                             cache.config.shaderConfig.lightingDisabled == !aurora_get_effective_lighting_enabled() &&
                             cache.lineMode == lineMode && cache.config.msaaSamples == gfx::get_sample_count();
  if (profile) {
    if (pipelineValid) ++sCommandProfile.pipelineCacheHits;
    else ++sCommandProfile.pipelineRebuilds;
  }
  if (!pipelineValid) {
    const bool trace=diagnostics().fifoTrace;
    if(trace) std::fprintf(stderr,"Z_FIFO_DRAW_CONFIG_BEGIN stages=%u op=%u early=%u\n",state.numTevStages,state.zTextureOp,state.zCompLocBeforeTex);
    const bool hadPipeline = cache.hasPipeline;
    const auto prevShaderConfig = cache.config.shaderConfig;
    const auto prevSampledTextures = cache.shaderInfo.sampledTextures;
    const auto prevSampledIndTextures = cache.shaderInfo.sampledIndTextures;
    populate_pipeline_config(cache.config, prim, fmt);
    if(trace) std::fprintf(stderr,"Z_FIFO_DRAW_CONFIG_END\n");
    // Blend/depth/raster state can dirty the pipeline without changing shader
    // analysis. Reuse only on full ShaderConfig equality (not a hash match).
    // Diagnostic layout hashing may be toggled by the native test harness.
    const bool layoutHashEnabled = std::getenv("AURORA_DIAGNOSTIC_UNIFORM_LAYOUT") != nullptr;
    const u32 uniformAlignment=gfx::align_uniform(1);
    bool reuseShaderInfo = hadPipeline && prevShaderConfig == cache.config.shaderConfig &&
        layoutHashEnabled == cache.shaderLayoutHashEnabled && uniformAlignment == cache.shaderUniformAlignment;
    if (!reuseShaderInfo) {
      if(!diagnostics().disableShaderInfoCache){
        const ShaderInfoKey key{cache.config.shaderConfig,uniformAlignment,layoutHashEnabled};
        const uint64_t hash=uint64_t(xxh3_hash(key.config)) ^ (uint64_t(uniformAlignment)<<32) ^
            (layoutHashEnabled ? UINT64_C(0x9e3779b97f4a7c15) : 0);
        if(const auto* info=sShaderInfoCache.find(hash,key)){
          cache.shaderInfo=*info;reuseShaderInfo=true;
          if(profile)++sCommandProfile.shaderInfoMultiHits;
        }else{
          cache.shaderInfo=build_shader_info(key.config,layoutHashEnabled);
          sShaderInfoCache.insert(hash,key,cache.shaderInfo);
          if(profile)++sCommandProfile.shaderInfoMultiMisses;
        }
      }else cache.shaderInfo=build_shader_info(cache.config.shaderConfig,layoutHashEnabled);
    }
    if (reuseShaderInfo && diagnostics().verifyShaderInfoReuse) {
      AURORA_ASSERT(cache.shaderInfo == build_shader_info(cache.config.shaderConfig,layoutHashEnabled),
                    "Reused shader metadata differs from fresh analysis");
      if(profile)++sCommandProfile.shaderInfoChecks;
    }
    cache.shaderLayoutHashEnabled=layoutHashEnabled;
    cache.shaderUniformAlignment=uniformAlignment;
    if (profile) {
      if (reuseShaderInfo) ++sCommandProfile.shaderInfoReuse;
      else ++sCommandProfile.shaderInfoBuilds;
    }
    if(trace) std::fprintf(stderr,"Z_FIFO_DRAW_INFO_END\n");
    cache.pipelineRef = gfx::pipeline_ref(cache.config);
    if(trace) std::fprintf(stderr,"Z_FIFO_DRAW_PIPELINE_END\n");
    cache.fmt = fmt;
    cache.lineMode = lineMode;
    cache.hasPipeline = true;
    state.dirty = (state.dirty & ~DirtyPipeline) | DirtyUniform;
    if (!hadPipeline || prevSampledTextures != cache.shaderInfo.sampledTextures ||
        prevSampledIndTextures != cache.shaderInfo.sampledIndTextures) {
      cache.bindGeneration = 0;
    }
  }
  if (profile) {
    sCommandProfile.drawPipelineNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
  }

  const bool bindGroupsValid =
      (state.dirty & DirtyTextures) == 0 && cache.bindGeneration == texture::current_bind_generation();
  if (profile) {
    if (bindGroupsValid) ++sCommandProfile.bindingCacheHits;
    else ++sCommandProfile.bindingRebuilds;
  }
  if (!bindGroupsValid) {
    const auto prevBindGroup = cache.bindGroups.textureBindGroup;
    resolve_sampled_textures(cache.shaderInfo);
    cache.bindGroups = build_bind_groups(cache.shaderInfo);
    cache.bindGeneration = texture::current_bind_generation();
    state.dirty &= ~DirtyTextures;
    // For texture_size_bias uniform
    if (cache.bindGroups.textureBindGroup != prevBindGroup) {
      state.dirty |= DirtyUniform;
    }
  }
  if (profile) {
    sCommandProfile.drawBindingsNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
  }

  // Reflection/shadow texgens can use normal/position arrays without UV0.
  const GXAttr diagnosticSource = diagnostics().drawUv.load(std::memory_order_relaxed) ? [&] {
    for (const auto attr : {GX_VA_TEX0, GX_VA_NRM, GX_VA_POS}) {
      if ((state.vtxDesc[attr] == GX_INDEX8 || state.vtxDesc[attr] == GX_INDEX16) &&
          state.arrays[attr].data && state.arrays[attr].size >= 64) return attr;
    }
    return GX_VA_NULL;
  }() : GX_VA_NULL;
  if (diagnosticSource != GX_VA_NULL) {
    uint64_t sourceHash = 14695981039346656037ull;
    const auto* bytes = static_cast<const uint8_t*>(state.arrays[diagnosticSource].data);
    for (u32 n = 0; n < 64; ++n) sourceHash = (sourceHash ^ bytes[n]) * 1099511628211ull;
    uint64_t layoutHash = 14695981039346656037ull;
    for (u32 slot = 0; slot < MaxTextures; ++slot) {
      if (!cache.shaderInfo.sampledTextures.test(slot) &&
          !cache.shaderInfo.sampledIndTextures.test(slot)) continue;
      const auto& tex = state.textures[slot];
      for (u32 value : {slot, tex ? tex.ref->size.width : 0,
                       tex ? tex.ref->size.height : 0})
        layoutHash = (layoutHash ^ value) * 1099511628211ull;
    }
    static std::set<std::tuple<uint64_t, uint64_t, uint64_t, unsigned>> configurations;
    const auto shader = xxh3_hash(cache.config.shaderConfig);
    if (configurations.size() < 512 && configurations.emplace(sourceHash, shader, layoutHash, unsigned(diagnosticSource)).second) {
      std::fprintf(stderr,"DRAW_TEXTURE_SOURCE source_hash=%llu shader=%llu attr=%u bytes=%u\n",
        static_cast<unsigned long long>(sourceHash),static_cast<unsigned long long>(shader),
        u32(diagnosticSource),u32(state.arrays[diagnosticSource].size));
      // Hash the declared uploaded span only in diagnostics; never export vertex bytes.
      const auto sourceSize = std::min(state.arrays[diagnosticSource].cachedRange.size,
                                       state.arrays[diagnosticSource].size);
      if (sourceSize && sourceSize <= 1024u * 1024u) {
        uint64_t fullHash = 14695981039346656037ull;
        for (u32 n=0;n<sourceSize;++n) fullHash=(fullHash^bytes[n])*1099511628211ull;
        std::fprintf(stderr,"DRAW_TEXTURE_ARRAY_HASH source_hash=%llu shader=%llu attr=%u bytes=%u full_hash=%llu\n",
          static_cast<unsigned long long>(sourceHash),static_cast<unsigned long long>(shader),
          u32(diagnosticSource),u32(sourceSize),static_cast<unsigned long long>(fullHash));
      }
      for (u32 coord = 0; coord < MaxTexCoord; ++coord) {
        if (!cache.shaderInfo.sampledTexCoords.test(coord)) continue;
        const auto& gen = cache.config.shaderConfig.tcgs[coord];
        std::fprintf(stderr,
          "DRAW_TEXTURE_GEN source_hash=%llu shader=%llu coord=%u src=%u type=%u matrix=%u post=%u normalize=%u indexed=%u\n",
          static_cast<unsigned long long>(sourceHash), static_cast<unsigned long long>(shader),
          coord, u32(gen.src), u32(gen.type), u32(gen.mtx), u32(gen.postMtx), u32(gen.normalize),
          u32(cache.shaderInfo.indexAttr.test(GX_VA_TEX0MTXIDX + coord)));
      }
      for (u32 stage = 0; stage < state.numTevStages; ++stage) {
        const auto& layer = cache.config.shaderConfig.tevStages[stage];
        const u32 map = u32(layer.texMapId);
        if (map >= MaxTextures) continue;
        const auto& tex = state.textures[map];
        const u32 coord = u32(layer.texCoordId);
        const u32 scaleS = coord < MaxTexCoord ? u32(state.texCoordScales[coord].scaleS) + 1 : 0;
        const u32 scaleT = coord < MaxTexCoord ? u32(state.texCoordScales[coord].scaleT) + 1 : 0;
        std::fprintf(stderr,
          "DRAW_TEXTURE_LAYER source_hash=%llu shader=%llu stage=%u coord=%u map=%u resolved=%u width=%u height=%u scale_s=%u scale_t=%u original_width=%u original_height=%u ind_matrix=%u wrap_s=%u wrap_t=%u add_prev=%u\n",
          static_cast<unsigned long long>(sourceHash), static_cast<unsigned long long>(shader),
          stage, coord, map, u32(bool(tex)),
          tex ? tex.ref->size.width : 0, tex ? tex.ref->size.height : 0,
          scaleS, scaleT, tex ? tex.texObj.width() : 0, tex ? tex.texObj.height() : 0,
          u32(layer.indTexMtxId), u32(layer.indTexWrapS), u32(layer.indTexWrapT), u32(layer.indTexAddPrev));
      }
    }
  }
  const bool uniformValid = (state.dirty & DirtyUniform) == 0 && cache.uniformRange.size != 0;
  if (profile) {
    if (uniformValid) ++sCommandProfile.uniformCacheHits;
    else ++sCommandProfile.uniformRebuilds;
  }
  if (!uniformValid) {
    cache.uniformRange = build_uniform(cache.shaderInfo);
    state.dirty &= ~DirtyUniform;
  }
  if (profile) {
    sCommandProfile.drawUniformNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
  }

  state.dirty &= ~DirtyImmediates;

  uint32_t instanceCount = 1;
  if (prim == GX_LINES) {
    instanceCount = vtxCount / 2;
  } else if (prim == GX_LINESTRIP) {
    instanceCount = vtxCount - 1;
  } else if (prim == GX_POINTS) {
    instanceCount = vtxCount;
  }
  cache.lastDrawFmt = fmt;
  if(diagnostics().renderState && diagnostics().drawUv.load(std::memory_order_relaxed)) {
    static std::set<uint64_t> seenStates;
    const auto stateHash=xxh3_hash(cache.config);
    if(seenStates.size()<256 && seenStates.insert(stateHash).second) {
      const auto& c=cache.config;
      std::fprintf(stderr,"GX_RENDER_STATE shader=%llu pipeline=%llu samples=%u depth_compare=%u depth_update=%u depth_func=%u cull=%u blend=%u src=%u dst=%u logic=%u color_write=%u alpha_write=%u dst_alpha=%u\n",
          static_cast<unsigned long long>(xxh3_hash(c.shaderConfig)),static_cast<unsigned long long>(stateHash),c.msaaSamples,unsigned(c.depthCompare),unsigned(c.depthUpdate),unsigned(c.depthFunc),unsigned(c.cullMode),unsigned(c.blendMode),unsigned(c.blendFacSrc),unsigned(c.blendFacDst),unsigned(c.blendOp),unsigned(c.colorUpdate),unsigned(c.alphaUpdate),c.dstAlpha);
    }
  }
  // Capture immutable pipeline state on the recording owner, outside the inline draw payload.
  std::array<uint32_t, 16> diagnosticState{};
  if (diagnostics().renderState && diagnostics().drawUv.load(std::memory_order_relaxed)) {
    const auto& c = cache.config;
    diagnosticState = {c.msaaSamples, uint32_t(c.depthCompare), uint32_t(c.depthUpdate),
        uint32_t(c.depthFunc), uint32_t(c.cullMode), uint32_t(c.blendMode),
        uint32_t(c.blendFacSrc), uint32_t(c.blendFacDst), uint32_t(c.blendOp),
        uint32_t(c.colorUpdate), uint32_t(c.alphaUpdate), c.dstAlpha,
        c.polygonOffsetBits, c.polygonOffsetScaleBits, c.polygonOffsetClampBits, GXPipelineConfigVersion};
  }
  gfx::push_draw_command(DrawData{
      .pipeline = cache.pipelineRef,
      .vertRange = vertRange,
      .idxRange = idxRange,
      .uniformRange = cache.uniformRange,
      .immediateData = immediates,
      .vtxCount = vtxCount,
      .indexCount = numIndices,
      .instanceCount = instanceCount,
      .bindGroups = cache.bindGroups,
      .dstAlpha = cache.config.dstAlpha,
      .diagnosticShader = diagnostics().vertexReplayDispatch ? xxh3_hash(cache.config.shaderConfig) : 0,
      .diagnosticUniformShader = diagnostics().uniformUpload.load(std::memory_order_relaxed)
          ? xxh3_hash(cache.config.shaderConfig) : 0,
  }, diagnosticState);
  if (profile) sCommandProfile.drawCommandNs += profile_now_ns() - phaseStartNs;
}

static void handle_draw_unmerged(GXPrimitive prim, GXVtxFmt fmt, u16 vtxCount, gfx::Range vertRange) noexcept {
  ZoneScoped;
  u32 numIndices = 0;
  gfx::Range idxRange;

  if (prim != GX_TRIANGLES) {
    ZoneScopedN("build idx buffer");
    static ByteBuffer idxBuf;
    numIndices = prepare_idx_buffer(idxBuf, prim, 0, vtxCount);
    idxRange = gfx::push_indices(idxBuf.data(), idxBuf.size(), 4);
    idxBuf.clear();
  }

  push_gx_draw(prim, fmt, vtxCount, vertRange, idxRange, numIndices);
}

static void draw_prim(GXPrimitive prim, GXVtxFmt fmt, u16 vtxCount, Reader& reader) noexcept {
  ZoneScoped;
  const bool profile = sCommandProfile.enabled;
  uint64_t phaseStartNs = profile ? profile_now_ns() : 0;
  u32 vtxSize;
  if (g_gxState.lastVtxFmt == fmt)
    LIKELY { vtxSize = g_gxState.lastVtxSize; }
  else
    UNLIKELY { vtxSize = calc_vtx_size(fmt); }

  u32 totalVtxBytes = vtxCount * vtxSize;
  if (totalVtxBytes > reader.remaining())
    UNLIKELY { handle_draw_overrun(totalVtxBytes, reader); }

  const auto vertexData = reader.take(totalVtxBytes);
  const auto tracePrimitive = [&] {
    if (!diagnostics().drawUv.load(std::memory_order_relaxed) || !diagnostics().primitiveHash) return;
    uint64_t hash = 14695981039346656037ull;
    for (const auto byte : vertexData) hash = (hash ^ byte) * 1099511628211ull;
    const auto shader = xxh3_hash(sDrawCache.config.shaderConfig);
    static std::set<std::tuple<uint64_t,uint64_t,unsigned,unsigned,unsigned>> seen;
    if (seen.size() < 1024 && seen.emplace(hash,shader,unsigned(prim),unsigned(fmt),vtxCount).second)
      std::fprintf(stderr,"DRAW_PRIMITIVE_HASH hash=%llu shader=%llu primitive=%u format=%u vertices=%u bytes=%u\n",
        static_cast<unsigned long long>(hash),static_cast<unsigned long long>(shader),
        unsigned(prim),unsigned(fmt),unsigned(vtxCount),totalVtxBytes);
  };
  const bool storageChanged=prepare_indexed_storage(fmt,vtxCount,vtxSize,vertexData);
  if (profile) {
    sCommandProfile.drawIndexedNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
  }
  const bool cleanState = !storageChanged && g_gxState.dirty == 0 && fmt == sDrawCache.lastDrawFmt && sDrawCache.lineMode == 0 &&
                          prim != GX_LINES && prim != GX_LINESTRIP && prim != GX_POINTS;
  auto* lastDraw = cleanState ? gfx::get_last_draw_command<DrawData>() : nullptr;
  if(lastDraw && diagnostics().drawMergeAudit && diagnostics().drawUv.load(std::memory_order_relaxed)) {
    static unsigned samples=0;
    if(samples<512) {
      unsigned mask=0;
      if(lastDraw->immediateData.currentPnMtx!=g_gxState.currentPnMtx) mask|=1;
      for(int i=GX_VA_POS;i<=GX_VA_TEX7;++i)
        if((g_gxState.vtxDesc[i]==GX_INDEX8 || g_gxState.vtxDesc[i]==GX_INDEX16) &&
           lastDraw->immediateData.arrayStart[i-GX_VA_POS]!=g_gxState.arrays[i].cachedRange.offset) mask|=2;
      if(lastDraw->vertRange.size!=lastDraw->vtxCount*vtxSize ||
         lastDraw->immediateData.vtxStart!=lastDraw->vertRange.offset) mask|=4;
      PipelineConfig current{};populate_pipeline_config(current,prim,fmt);
      if(xxh3_hash(current)!=xxh3_hash(sDrawCache.config) || lastDraw->pipeline!=sDrawCache.pipelineRef) mask|=8;
      const auto fresh=build_uniform(sDrawCache.shaderInfo);
      if(!gfx::detail::diagnostic_uniform_ranges_equal(lastDraw->uniformRange,fresh)) mask|=16;
      if(lastDraw->bindGroups.textureBindGroup!=sDrawCache.bindGroups.textureBindGroup) mask|=32;
      if(sDrawCache.bindGeneration!=texture::current_bind_generation()) mask|=64;
      std::fprintf(stderr,"DRAW_MERGE_AUDIT sample=%u mask=%u previous_vertices=%u incoming_vertices=%u stride=%u primitive=%u\n",
                   ++samples,mask,lastDraw->vtxCount,unsigned(vtxCount),vtxSize,unsigned(prim));
    }
  }
  const bool canMerge = lastDraw != nullptr && lastDraw->instanceCount == 1 &&
                        !diagnostics().disableDrawMerge;
  if (profile) {
    sCommandProfile.drawStateNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
  }

  // Push raw vertex data to buffer. Merged draws must remain contiguous with the previous range.

  gfx::Range vertRange = gfx::push_verts(vertexData.data(), vertexData.size(), canMerge ? 0 : 4);
  if (profile) {
    sCommandProfile.drawVertexUploadNs += profile_now_ns() - phaseStartNs;
    phaseStartNs = profile_now_ns();
    ++sCommandProfile.drawCalls;
  }

  // Try to merge with previous draw call
  if (canMerge) {
    const uint32_t originalFirstVertex=lastDraw->vtxCount;
    u32 numIndices = 0;
    gfx::Range idxRange;
    static ByteBuffer idxBuf;
    const bool hadIndexRange = lastDraw->idxRange.size != 0;
    if (lastDraw->indexCount == 0 && prim != GX_TRIANGLES) {
      // Generate triangle index buffer for previous draw
      lastDraw->indexCount = prepare_idx_buffer(idxBuf, GX_TRIANGLES, 0, lastDraw->vtxCount);
    }
    if (lastDraw->indexCount != 0) {
      numIndices += prepare_idx_buffer(idxBuf, prim, lastDraw->vtxCount, vtxCount);
      idxRange = gfx::push_indices(idxBuf.data(), idxBuf.size(), hadIndexRange ? 0 : 4);
      idxBuf.clear();
    }
    CHECK(lastDraw->vertRange.offset + lastDraw->vertRange.size == vertRange.offset,
          "Non-consecutive vertex ranges ({} < {})", lastDraw->vertRange.offset + lastDraw->vertRange.size,
          vertRange.offset);
    if (hadIndexRange) {
      CHECK(lastDraw->idxRange.offset + lastDraw->idxRange.size == idxRange.offset,
            "Non-consecutive index ranges ({} < {})", lastDraw->idxRange.offset + lastDraw->idxRange.size,
            idxRange.offset);
    }
    lastDraw->vertRange.size += vertRange.size;
    if (lastDraw->idxRange.size == 0) {
      lastDraw->idxRange = idxRange;
    } else {
      lastDraw->idxRange.size += idxRange.size;
    }
    lastDraw->vtxCount += vtxCount;
    lastDraw->indexCount += numIndices;
    gfx::detail::diagnostic_update_merged_draw(*lastDraw);
    gfx::detail::diagnostic_original_mesh_primitive(*lastDraw,sDrawCache.config.shaderConfig,vertexData.data(),totalVtxBytes,originalFirstVertex);
    gfx::detail::increment_merged_draw_count();
    tracePrimitive();
    if (profile) {
      sCommandProfile.drawMergeNs += profile_now_ns() - phaseStartNs;
      ++sCommandProfile.mergedDrawCalls;
    }
    return;
  }

  handle_draw_unmerged(prim, fmt, vtxCount, vertRange);
  if(auto* originalDraw=gfx::get_last_draw_command<DrawData>())
    gfx::detail::diagnostic_original_mesh_primitive(*originalDraw,sDrawCache.config.shaderConfig,vertexData.data(),totalVtxBytes,0);
  if (profile) sCommandProfile.drawSubmitNs += profile_now_ns() - phaseStartNs;
  tracePrimitive();
}

static void handle_draw(u8 cmd, Reader& reader) noexcept {
  const auto fmt = static_cast<GXVtxFmt>(cmd & CP_VAT_MASK);
  const auto prim = static_cast<GXPrimitive>(cmd & CP_OPCODE_MASK);
  draw_prim(prim, fmt, reader.read<u16>(), reader);
}

void handle_aurora(Reader& reader) noexcept {
  ZoneScoped;
  const u16 subCmd = reader.read<u16>();

  if (subCmd == GX_AURORA_HUD_GRAPHICS_PUSH) {
    // State changes must execute in Release builds, where CHECK is compiled out.
    AURORA_ASSERT(gfx::detail::push_hud_graphics(), "Invalid FIFO HUD graphics push");
  } else if (subCmd == GX_AURORA_HUD_GRAPHICS_POP) {
    AURORA_ASSERT(gfx::detail::pop_hud_graphics(), "Invalid FIFO HUD graphics pop");
  } else if (subCmd == GX_AURORA_LOAD_VIEWPORT_RENDER) {
    const f32 left = reader.read<f32>();
    const f32 top = reader.read<f32>();
    const f32 width = reader.read<f32>();
    const f32 height = reader.read<f32>();
    const f32 nearZ = reader.read<f32>();
    const f32 farZ = reader.read<f32>();
    set_render_viewport({
        .left = left,
        .top = top,
        .width = width,
        .height = height,
        .znear = nearZ,
        .zfar = farZ,
    });
  } else if (subCmd == GX_AURORA_LOAD_SCISSOR_RENDER) {
    const s32 left = reader.read<s32>();
    const s32 top = reader.read<s32>();
    const s32 width = reader.read<s32>();
    const s32 height = reader.read<s32>();
    set_render_scissor({left, top, width, height});
  } else if (subCmd == GX_AURORA_LOAD_PROJECTION_FULL) {
    auto& proj = g_gxState.proj;
    proj = {};
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        proj[r][c] = reader.read<f32>();
      }
    }
    g_gxState.dirty |= DirtyUniform;
    // Invalidate projection XF regs
    for (u32 reg = 0x20; reg <= 0x26; ++reg) {
      g_gxState.xfRegValid.reset(reg);
    }
  } else if (subCmd >= GX_AURORA_LOAD_ARRAYBASE && subCmd <= (GX_AURORA_LOAD_ARRAYBASE | 0x0f)) {
    const u32 attrIdx = subCmd - GX_AURORA_LOAD_ARRAYBASE + GX_VA_POS;
    const u64 arrayAddr = reader.read<u64>();
    const u32 arraySize = reader.read<u32>();
    const bool le = reader.read<u8>() == 1;

    auto& array = g_gxState.arrays[attrIdx];
    const auto newData = reinterpret_cast<void*>(arrayAddr);
    if (array.data != newData || array.size != arraySize || array.le != le) {
      if (array.le != le) {
        // Endianness is baked into the shader
        g_gxState.dirty |= DirtyPipeline;
      }
      array.data = newData;
      array.size = arraySize;
      array.le = le;
      array.cachedRange = {};
      g_gxState.dirty |= DirtyImmediates;
    }
  } else if (subCmd == GX_AURORA_LOAD_TEXOBJ) {
    const auto texMapId = reader.read<u8>();
    CHECK(texMapId < MaxTextures, "invalid texture map id {}", texMapId);
    auto& slot = g_gxState.loadedTextures[texMapId];
    const auto newData = reinterpret_cast<const void*>(reader.read<u64>());
    const u32 newWidth = reader.read<u32>();
    const u32 newHeight = reader.read<u32>();
    const auto newFormat = static_cast<GXTexFmt>(reader.read<u32>());
    const auto newTlut = static_cast<GXTlut>(reader.read<u32>());
    u8 newFlags = slot.flags & ~0x80u; // Reset no-cache flag
    if (reader.read<u8>() != 0) {
      newFlags |= 1u;
    } else {
      newFlags &= ~1u;
    }
    const u32 newTexObjId = reader.read<u32>();
    const u32 newTexDataVersion = reader.read<u32>();
    if (slot.data != newData || slot.mWidth != newWidth || slot.mHeight != newHeight ||
        slot.mFormat != static_cast<u32>(newFormat) || slot.tlut != newTlut || slot.flags != newFlags ||
        slot.texObjId != newTexObjId || slot.texDataVersion != newTexDataVersion) {
      slot.data = newData;
      slot.mWidth = newWidth;
      slot.mHeight = newHeight;
      slot.mFormat = newFormat;
      slot.tlut = newTlut;
      slot.flags = newFlags;
      slot.texObjId = newTexObjId;
      slot.texDataVersion = newTexDataVersion;
      g_gxState.dirty |= DirtyTextures;
    }
  } else if (subCmd == GX_AURORA_LOAD_TLUT) {
    const auto idx = reader.read<u8>();
    CHECK(idx < MaxTluts, "invalid tlut slot {}", idx);
    auto& slot = g_gxState.loadedTluts[idx];
    const auto newData = reinterpret_cast<const void*>(reader.read<u64>());
    const auto newFormat = static_cast<GXTlutFmt>(reader.read<u32>());
    const u16 newNumEntries = reader.read<u16>();
    const u32 newTlutObjId = reader.read<u32>();
    const u32 newTlutDataVersion = reader.read<u32>();
    const u8 newFlags = slot.flags & ~0x80u; // Reset no-cache flag
    if (slot.data != newData || slot.format != newFormat || slot.numEntries != newNumEntries ||
        slot.tlutObjId != newTlutObjId || slot.tlutDataVersion != newTlutDataVersion || slot.flags != newFlags) {
      if (slot.tlutObjId != newTlutObjId || slot.tlutDataVersion != newTlutDataVersion) {
        texture::invalidate_bindings();
      }
      slot.data = newData;
      slot.format = newFormat;
      slot.numEntries = newNumEntries;
      slot.tlutObjId = newTlutObjId;
      slot.tlutDataVersion = newTlutDataVersion;
      slot.flags = newFlags;
      g_gxState.dirty |= DirtyTextures;
    }
  } else if (subCmd == GX2_SET_POLYGON_OFFSET) {
    const f32 frontOffset = reader.read<f32>();
    const f32 frontScale = reader.read<f32>();
    const f32 backOffset = reader.read<f32>();
    const f32 backScale = reader.read<f32>();
    const f32 clamp = reader.read<f32>();
    if (g_gxState.frontOffset != frontOffset || g_gxState.frontScale != frontScale ||
        g_gxState.backOffset != backOffset || g_gxState.backScale != backScale || g_gxState.clamp != clamp) {
      g_gxState.frontOffset = frontOffset;
      g_gxState.frontScale = frontScale;
      g_gxState.backOffset = backOffset;
      g_gxState.backScale = backScale;
      g_gxState.clamp = clamp;
      g_gxState.dirty |= DirtyPipeline;
    }
  } else if (subCmd == GX_AURORA_LOAD_COPY_SRC) {
    const s32 left = reader.read<s32>();
    const s32 top = reader.read<s32>();
    const s32 width = reader.read<s32>();
    const s32 height = reader.read<s32>();
    g_gxState.texCopySrc = {left, top, width, height};
  } else if (subCmd == GX_AURORA_LOAD_COPY_DST) {
    g_gxState.texCopyDstWidth = reader.read<u32>();
    g_gxState.texCopyDstHeight = reader.read<u32>();
    g_gxState.texCopyFmt = static_cast<GXTexFmt>(reader.read<u32>());
    reader.skip(1); // mipmap is not implemented, but remains part of the command payload
    g_gxState.texCopyDstWide = true;
  } else if (subCmd == GX_AURORA_LOAD_COPY_DEST) {
    g_gxState.texCopyDest = reinterpret_cast<const void*>(reader.read<u64>());
  } else if (subCmd == GX_AURORA_REQUEST_DEPTH_SNAPSHOT) {
    gfx::depth_peek::request_snapshot();
  } else if (subCmd == GX_AURORA_BEGIN_OFFSCREEN) {
    const u32 width = reader.read<u32>();
    const u32 height = reader.read<u32>();
    gfx::begin_offscreen(width, height);
  } else if (subCmd == GX_AURORA_END_OFFSCREEN) {
    gfx::end_offscreen();
  } else if (subCmd == GX_AURORA_DESTROY_TEXOBJ) {
    evict_texture_object(reader.read<u32>());
  } else if (subCmd == GX_AURORA_DESTROY_TLUT) {
    evict_tlut_object(reader.read<u32>());
  } else if (subCmd == GX_AURORA_DESTROY_COPY_TEX) {
    evict_copy_texture(reinterpret_cast<const void*>(reader.read<u64>()));
  } else if (subCmd == GX_AURORA_DESTROY_COPY_TEX_RANGE) {
    const u64 low = reader.read<u64>();
    const u64 high = reader.read<u64>();
    evict_copy_texture_range(low, high);
  } else if (subCmd == GX_AURORA_DRAW_SIZED) {
    const u8 cmd = reader.read<u8>();
    const u32 byteLen = reader.read<u32>();
    const GXVtxFmt fmt = static_cast<GXVtxFmt>(cmd & CP_VAT_MASK);
    const GXPrimitive prim = static_cast<GXPrimitive>(cmd & CP_OPCODE_MASK);
    if (byteLen != 0) {
      u32 vtxSize;
      if (g_gxState.lastVtxFmt == fmt) {
        vtxSize = g_gxState.lastVtxSize;
      } else {
        vtxSize = calc_vtx_size(fmt);
      }
      AURORA_ASSERT(vtxSize != 0 && byteLen % vtxSize == 0,
                    "GX_AURORA_DRAW_SIZED: {} bytes is not a whole number of size-{} vertices", byteLen, vtxSize);
      u32 vtxCount = byteLen / vtxSize;
      AURORA_ASSERT(vtxCount <= 0xFFFF, "GX_AURORA_DRAW_SIZED: too many vertices ({})", vtxCount);
      draw_prim(prim, fmt, static_cast<u16>(vtxCount), reader);
    }
  } else if (subCmd == GX_AURORA_DRAW_INDEXED) {
    ZoneScopedN("DRAW_INDEXED");
    const u8 cmd = reader.read<u8>();
    const u16 vtxCount = reader.read<u16>();
    const u32 indexCount = reader.read<u32>();
    const GXVtxFmt fmt = static_cast<GXVtxFmt>(cmd & CP_VAT_MASK);
    const GXPrimitive prim = static_cast<GXPrimitive>(cmd & CP_OPCODE_MASK);
    AURORA_ASSERT(prim == GX_TRIANGLES, "GX_AURORA_DRAW_INDEXED: primitive must be GX_TRIANGLES, got {}",
                  static_cast<u32>(prim));
    const size_t idxBytes = static_cast<size_t>(indexCount) * sizeof(u16);
    // Index data is always host-endian; push it to the GPU buffer as-is
    const auto indexData = reader.take(idxBytes);
    const gfx::Range idxRange = gfx::push_indices(indexData.data(), indexData.size(), 4);
    u32 vtxSize;
    if (g_gxState.lastVtxFmt == fmt) {
      vtxSize = g_gxState.lastVtxSize;
    } else {
      vtxSize = calc_vtx_size(fmt);
    }
    const u32 totalVtxBytes = vtxCount * vtxSize;
    const auto vertexData = reader.take(totalVtxBytes);
    if (indexCount != 0) {
      prepare_indexed_storage(fmt, vtxCount, vtxSize, vertexData);
    }
    const gfx::Range vertRange = gfx::push_verts(vertexData.data(), vertexData.size(), 4);
    if (indexCount != 0) {
      push_gx_draw(prim, fmt, vtxCount, vertRange, idxRange, indexCount);
    }
  } else if (subCmd == GX_AURORA_DEBUG_GROUP_PUSH) {
    auto label = reader.read_string();
    gfx::push_debug_group(std::move(label));
  } else if (subCmd == GX_AURORA_DEBUG_GROUP_POP) {
    pop_debug_group();
  } else if (subCmd == GX_AURORA_DEBUG_MARKER_INSERT) {
    auto label = reader.read_string();
    gfx::insert_debug_marker(std::move(label));
  }

  else {
    Log.error("Unknown Aurora subcommand: {:04X}", subCmd);
  }
}

void clear_draw_cache() noexcept {
  sDrawCache.bindGeneration = 0;
  sDrawCache.uniformRange = {};
}

extern "C" void aurora_diagnostic_refresh_command_flags(void) {
  refresh_dynamic_diagnostic_flags();
}

} // namespace aurora::gx::fifo
