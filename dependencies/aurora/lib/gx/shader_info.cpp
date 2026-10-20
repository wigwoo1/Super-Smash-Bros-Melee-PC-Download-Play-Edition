#include "shader_info.hpp"
#include "texgen_dependencies.hpp"

#include "../gfx/recording.hpp"
#include "../gfx/hash.hpp"
#include <absl/container/flat_hash_set.h>
#include <cstdio>
#include <cstdlib>
#include <atomic>

#include <cmath>

#include <tracy/Tracy.hpp>

namespace aurora::gx {
namespace {
constexpr Module Log{"aurora::gx"};
enum UniformDiagnosticFlag : unsigned {
  MatrixTrace = 1, LayoutTrace = 2, ContentTrace = 4, UvTrace = 8,
};
unsigned read_uniform_diagnostic_flags() noexcept {
  return (std::getenv("AURORA_DIAGNOSTIC_TEXTURE_MATRICES") ? MatrixTrace : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_UNIFORM_LAYOUT") ? LayoutTrace : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_TEXTURE_CONTENT") ? ContentTrace : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") ? UvTrace : 0u);
}
std::atomic<unsigned>& uniform_diagnostic_flags() noexcept {
  static std::atomic<unsigned> flags{read_uniform_diagnostic_flags()};
  return flags;
}

bool is_alpha_bump_channel(GXChannelID id) { return id == GX_ALPHA_BUMP || id == GX_ALPHA_BUMPN; }

Vec4<float> texture_size_bias(const gfx::TextureBind& tex) {
  auto width = static_cast<float>(tex.texObj.width());
  auto height = static_cast<float>(tex.texObj.height());
  float vpBias = 0.f;
  if (tex.ref && tex.ref->hasArbitraryMips) {
    const float viewportScale =
        std::min(g_gxState.renderViewport.width / std::max(g_gxState.logicalViewport.width, 1.f),
                 g_gxState.renderViewport.height / std::max(g_gxState.logicalViewport.height, 1.f));
    const float replacementScale = static_cast<float>(tex.ref->size.width) / std::max(width, 1.f);
    vpBias = std::log2(viewportScale / std::max(replacementScale, 0.001f));
  }
  return {width, height, tex.texObj.lod_bias() + vpBias, 0.0f};
}

void color_arg_reg_info(GXTevColorArg arg, const TevStage& stage, ShaderInfo& info,
                        const std::bitset<MaxTevRegs>& colorWritten,
                        const std::bitset<MaxTevRegs>& alphaWritten) {
  switch (arg) {
  case GX_CC_CPREV:
  case GX_CC_APREV:
    if (!(arg == GX_CC_APREV ? alphaWritten : colorWritten).test(GX_TEVPREV)) {
      info.loadsTevReg.set(GX_TEVPREV);
    }
    break;
  case GX_CC_C0:
  case GX_CC_A0:
    if (!(arg == GX_CC_A0 ? alphaWritten : colorWritten).test(GX_TEVREG0)) {
      info.loadsTevReg.set(GX_TEVREG0);
    }
    break;
  case GX_CC_C1:
  case GX_CC_A1:
    if (!(arg == GX_CC_A1 ? alphaWritten : colorWritten).test(GX_TEVREG1)) {
      info.loadsTevReg.set(GX_TEVREG1);
    }
    break;
  case GX_CC_C2:
  case GX_CC_A2:
    if (!(arg == GX_CC_A2 ? alphaWritten : colorWritten).test(GX_TEVREG2)) {
      info.loadsTevReg.set(GX_TEVREG2);
    }
    break;
  case GX_CC_TEXC:
  case GX_CC_TEXA:
    if (stage.texMapId != GX_TEXMAP_NULL) {
      CHECK(stage.texCoordId != GX_TEXCOORD_NULL, "tex coord not bound");
      info.sampledTexCoords.set(stage.texCoordId);
      info.sampledTextures.set(stage.texMapId);
    }
    break;
  case GX_CC_RASC:
  case GX_CC_RASA:
    if (stage.channelId != GX_COLOR_NULL && stage.channelId != GX_COLOR_ZERO &&
        !is_alpha_bump_channel(stage.channelId)) {
      info.sampledColorChannels.set(color_channel(stage.channelId));
    }
    break;
  case GX_CC_KONST:
    switch (stage.kcSel) {
    case GX_TEV_KCSEL_K0:
    case GX_TEV_KCSEL_K0_R:
    case GX_TEV_KCSEL_K0_G:
    case GX_TEV_KCSEL_K0_B:
    case GX_TEV_KCSEL_K0_A:
      info.sampledKColors.set(0);
      break;
    case GX_TEV_KCSEL_K1:
    case GX_TEV_KCSEL_K1_R:
    case GX_TEV_KCSEL_K1_G:
    case GX_TEV_KCSEL_K1_B:
    case GX_TEV_KCSEL_K1_A:
      info.sampledKColors.set(1);
      break;
    case GX_TEV_KCSEL_K2:
    case GX_TEV_KCSEL_K2_R:
    case GX_TEV_KCSEL_K2_G:
    case GX_TEV_KCSEL_K2_B:
    case GX_TEV_KCSEL_K2_A:
      info.sampledKColors.set(2);
      break;
    case GX_TEV_KCSEL_K3:
    case GX_TEV_KCSEL_K3_R:
    case GX_TEV_KCSEL_K3_G:
    case GX_TEV_KCSEL_K3_B:
    case GX_TEV_KCSEL_K3_A:
      info.sampledKColors.set(3);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

void alpha_arg_reg_info(GXTevAlphaArg arg, const TevStage& stage, ShaderInfo& info,
                        const std::bitset<MaxTevRegs>& alphaWritten) {
  switch (arg) {
  case GX_CA_APREV:
    if (!alphaWritten.test(GX_TEVPREV)) {
      info.loadsTevReg.set(GX_TEVPREV);
    }
    break;
  case GX_CA_A0:
    if (!alphaWritten.test(GX_TEVREG0)) {
      info.loadsTevReg.set(GX_TEVREG0);
    }
    break;
  case GX_CA_A1:
    if (!alphaWritten.test(GX_TEVREG1)) {
      info.loadsTevReg.set(GX_TEVREG1);
    }
    break;
  case GX_CA_A2:
    if (!alphaWritten.test(GX_TEVREG2)) {
      info.loadsTevReg.set(GX_TEVREG2);
    }
    break;
  case GX_CA_TEXA:
    if (stage.texMapId != GX_TEXMAP_NULL) {
      CHECK(stage.texCoordId != GX_TEXCOORD_NULL, "tex coord not bound");
      info.sampledTexCoords.set(stage.texCoordId);
      info.sampledTextures.set(stage.texMapId);
    }
    break;
  case GX_CA_RASA:
    if (stage.channelId != GX_COLOR_NULL && stage.channelId != GX_COLOR_ZERO &&
        !is_alpha_bump_channel(stage.channelId)) {
      info.sampledColorChannels.set(color_channel(stage.channelId));
    }
    break;
  case GX_CA_KONST:
    switch (stage.kaSel) {
    case GX_TEV_KASEL_K0_R:
    case GX_TEV_KASEL_K0_G:
    case GX_TEV_KASEL_K0_B:
    case GX_TEV_KASEL_K0_A:
      info.sampledKColors.set(0);
      break;
    case GX_TEV_KASEL_K1_R:
    case GX_TEV_KASEL_K1_G:
    case GX_TEV_KASEL_K1_B:
    case GX_TEV_KASEL_K1_A:
      info.sampledKColors.set(1);
      break;
    case GX_TEV_KASEL_K2_R:
    case GX_TEV_KASEL_K2_G:
    case GX_TEV_KASEL_K2_B:
    case GX_TEV_KASEL_K2_A:
      info.sampledKColors.set(2);
      break;
    case GX_TEV_KASEL_K3_R:
    case GX_TEV_KASEL_K3_G:
    case GX_TEV_KASEL_K3_B:
    case GX_TEV_KASEL_K3_A:
      info.sampledKColors.set(3);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

f32 tex_offset(GXTexOffset offs) noexcept {
  switch (offs) {
  default:
  case GX_TO_ZERO:
    return 0.f;
  case GX_TO_SIXTEENTH:
    return 1.f / 16.f;
  case GX_TO_EIGHTH:
    return 1.f / 8.f;
  case GX_TO_FOURTH:
    return 1.f / 4.f;
  case GX_TO_HALF:
    return 1.f / 2.f;
  case GX_TO_ONE:
    return 1.f;
  }
}

u32 point_texcoord_mask() noexcept {
  u32 mask = 0;
  for (int i = 0; i < MaxTexCoord; ++i) {
    if (g_gxState.texCoordScales[i].pointOffset) {
      mask |= 1 << i;
    }
  }
  return mask;
}

u32 line_texcoord_mask() noexcept {
  u32 mask = 0;
  for (int i = 0; i < MaxTexCoord; ++i) {
    if (g_gxState.texCoordScales[i].lineOffset) {
      mask |= 1 << i;
    }
  }
  return mask;
}
} // namespace

ShaderInfo build_shader_info(const ShaderConfig& config) noexcept {
  return build_shader_info(config,std::getenv("AURORA_DIAGNOSTIC_UNIFORM_LAYOUT")!=nullptr);
}

ShaderInfo build_shader_info(const ShaderConfig& config,bool diagnosticLayout) noexcept {
  ZoneScoped;

  ShaderInfo info{
      // render/logical viewport size, proj
      .uniformSize = 8 + 8 + 64,
  };
  info.lightingDisabled=config.lightingDisabled!=0;
  if (diagnosticLayout)
    info.diagnosticShaderHash = xxh3_hash(config);

  if (config.lineMode != 0) {
    info.uniformSize += 4 + 4 + 4 + 4; // line_width, line_aspect_y, line_tex_offset, line_texcoord_mask
    info.lineMode = config.lineMode;
  }

  for (int attr = 0; attr < config.attrs.size(); attr++) {
    const auto attrType = config.attrs[attr].attrType;
    if ((attr == GX_VA_PNMTXIDX || (attr >= GX_VA_TEX0MTXIDX && attr <= GX_VA_TEX7MTXIDX)) && attrType == GX_DIRECT) {
      info.indexAttr.set(attr);
    } else if (attrType == GX_INDEX8 || attrType == GX_INDEX16) {
      info.indexAttr.set(attr);
    }
  }

  // 10 position matrices, 10 texture matrices, 10 normal matrices.
  info.uniformSize += sizeof(Mat3x4<float>) * 30;

  std::bitset<MaxTevRegs> colorWritten, alphaWritten;
  for (int i = 0; i < config.tevStageCount; ++i) {
    const auto& stage = config.tevStages[i];
    if (i == z_texture_sample_stage(config)) {
      info.sampledTexCoords.set(stage.texCoordId);
      info.sampledTextures.set(stage.texMapId);
    }
    // Color pass
    color_arg_reg_info(stage.colorPass.a, stage, info, colorWritten, alphaWritten);
    color_arg_reg_info(stage.colorPass.b, stage, info, colorWritten, alphaWritten);
    color_arg_reg_info(stage.colorPass.c, stage, info, colorWritten, alphaWritten);
    color_arg_reg_info(stage.colorPass.d, stage, info, colorWritten, alphaWritten);
    // Alpha pass
    alpha_arg_reg_info(stage.alphaPass.a, stage, info, alphaWritten);
    alpha_arg_reg_info(stage.alphaPass.b, stage, info, alphaWritten);
    alpha_arg_reg_info(stage.alphaPass.c, stage, info, alphaWritten);
    alpha_arg_reg_info(stage.alphaPass.d, stage, info, alphaWritten);
    // Both passes read the register state from before this stage.
    colorWritten.set(stage.colorOp.outReg);
    alphaWritten.set(stage.alphaOp.outReg);
    info.writesTevReg.set(stage.colorOp.outReg);
    info.writesTevReg.set(stage.alphaOp.outReg);
  }
  for (int i = 0; i < config.tevStageCount; ++i) {
    const auto& stage = config.tevStages[i];
    // Skip if not enabled
    if (stage.indTexStage >= config.numIndStages) {
      continue;
    }
    const bool usesIndStage = stage.indTexMtxId != GX_ITM_OFF || is_alpha_bump_channel(stage.channelId);
    const bool usesTevTexCoord = stage.indTexMtxId != GX_ITM_OFF || stage.indTexWrapS != GX_ITW_OFF ||
                                 stage.indTexWrapT != GX_ITW_OFF || stage.indTexAddPrev;
    if (usesTevTexCoord && stage.texCoordId != GX_TEXCOORD_NULL) {
      info.sampledTexCoords.set(stage.texCoordId);
    }
    if (usesTevTexCoord && stage.texMapId != GX_TEXMAP_NULL &&
        (stage.indTexMtxId != GX_ITM_OFF || stage.indTexAddPrev)) {
      info.sampledTextures.set(stage.texMapId);
    }
    if (!usesIndStage) {
      continue;
    }
    info.usedIndStages.set(stage.indTexStage);
    const auto& indStage = config.indStages[stage.indTexStage];
    info.sampledTextures.set(indStage.texMapId);
    info.sampledTexCoords.set(indStage.texCoordId);
    info.sampledIndTextures.set(indStage.texMapId);
    // Track which indirect matrix is used
    if (stage.indTexMtxId >= GX_ITM_0 && stage.indTexMtxId <= GX_ITM_2) {
      info.usedIndTexMtxs.set(stage.indTexMtxId - GX_ITM_0);
    } else if (stage.indTexMtxId >= GX_ITM_S0 && stage.indTexMtxId <= GX_ITM_S2) {
      info.usedIndTexMtxs.set(stage.indTexMtxId - GX_ITM_S0);
    } else if (stage.indTexMtxId >= GX_ITM_T0 && stage.indTexMtxId <= GX_ITM_T2) {
      info.usedIndTexMtxs.set(stage.indTexMtxId - GX_ITM_T0);
    }
  }

  // Color-generated coordinates consume XF lighting even when TEV reads no
  // raster channel. Include indirect coordinate dependencies before layout.
  resolve_texgen_dependencies(config,info);

  info.uniformSize += info.loadsTevReg.count() * sizeof(Vec4<float>);
  for (int i = 0; i < info.sampledColorChannels.size(); ++i) {
    if (info.sampledColorChannels.test(i)) {
      const auto& cc = config.colorChannels[i];
      const auto& cca = config.colorChannels[i + GX_ALPHA0];
      if (cc.lightingEnabled || cca.lightingEnabled) {
        info.lightingEnabled = true;
      }
    }
  }
  if (info.lightingEnabled) {
    // Lights + light state for all channels
    info.uniformSize += 16 + sizeof(Light) * GX::MaxLights;
  }
  for (int i = 0; i < info.sampledColorChannels.size(); ++i) {
    if (info.sampledColorChannels.test(i)) {
      const auto& cc = config.colorChannels[i];
      if (cc.lightingEnabled && cc.ambSrc == GX_SRC_REG) {
        info.uniformSize += sizeof(Vec4<float>);
      }
      if (cc.matSrc == GX_SRC_REG) {
        info.uniformSize += sizeof(Vec4<float>);
      }
      const auto& cca = config.colorChannels[i + GX_ALPHA0];
      if (cca.lightingEnabled && cca.ambSrc == GX_SRC_REG) {
        info.uniformSize += sizeof(Vec4<float>);
      }
      if (cca.matSrc == GX_SRC_REG) {
        info.uniformSize += sizeof(Vec4<float>);
      }
    }
  }
  info.uniformSize += info.sampledKColors.count() * sizeof(Vec4<float>);
  for (int i = 0; i < info.sampledTexCoords.size(); ++i) {
    if (!info.sampledTexCoords.test(i)) {
      continue;
    }
    const auto& tcg = config.tcgs[i];
    if ((tcg.type==GX_TG_MTX2x4||tcg.type==GX_TG_MTX3x4) && tcg.postMtx != GX_PTIDENTITY) {
      u32 postMtxIdx = (tcg.postMtx - GX_PTTEXMTX0) / 3;
      info.usesPTTexMtx.set(postMtxIdx);
    }
  }
  if (info.usesPTTexMtx.any()) {
    info.uniformSize += sizeof(Mat3x4<float>) * MaxPTTexMtx;
  }
  if (config.fogType != GX_FOG_NONE) {
    info.usesFog = true;
    info.uniformSize += sizeof(Fog);
  }
  info.uniformSize += MaxTexCoord * sizeof(Vec4<float>);
  if (info.usedIndTexMtxs.any()) {
    info.uniformSize += MaxIndTexMtxs * sizeof(Mat2x4<float>);
  }
  info.uniformSize += info.sampledTextures.count() * sizeof(Vec4<float>);
  info.uniformSize = gfx::align_uniform(info.uniformSize);
  if (info.uniformSize > MaxUniformSize) {
    Log.fatal("Uniform size exceeds maximum: {} > {}", info.uniformSize, MaxUniformSize);
  }
  return info;
}

struct UniformDiagnosticWriter {
  ByteBuffer& storage;
  uint64_t hash;
  bool trace;
  u32 slot = 0;
  void reserve_extra(size_t size) { storage.reserve_extra(size); }
  template<class T> void append(const T& value) {
    const auto offset = storage.size();
    storage.append<T>(value);
    if (trace) std::fprintf(stderr, "UNIFORM_LAYOUT_SLOT hash=%llx slot=%u offset=%zu bytes=%zu\n",
                            static_cast<unsigned long long>(hash), slot++, offset, storage.size() - offset);
  }
};
static void fill_uniform(ByteBuffer& storage, const ShaderInfo& info, unsigned diagnosticFlags) noexcept {
  // Read only, recording owner; arm at original match entry to exclude menus.
  if (diagnosticFlags & MatrixTrace) {
    static absl::flat_hash_set<uint64_t> matrixSnapshots;
    Hasher hash;
    hash.update(g_gxState.texMtxs.data(), sizeof(g_gxState.texMtxs));
    hash.update(g_gxState.ptTexMtxs.data(), sizeof(g_gxState.ptTexMtxs));
    const auto sampledCoords = info.sampledTexCoords.to_ullong();
    const auto activePostMatrices = info.usesPTTexMtx.to_ullong();
    hash.update(&sampledCoords, sizeof(sampledCoords));
    hash.update(&activePostMatrices, sizeof(activePostMatrices));
    // Identical matrices can feed distinct shaders; retain each interpretation.
    hash.update(&info.diagnosticShaderHash, sizeof(info.diagnosticShaderHash));
    const auto key = hash.digest();
    if (matrixSnapshots.size() < 128 && matrixSnapshots.insert(key).second) {
      unsigned nonfinite = 0;
      float maxAbs = 0.f;
      auto inspect = [&](const auto& matrices) {
        for (const auto& matrix : matrices) {
          float values[12];
          static_assert(sizeof(values) == sizeof(matrix));
          std::memcpy(values, &matrix, sizeof(values));
          for (float value : values) {
            if (!std::isfinite(value)) ++nonfinite;
            else maxAbs = std::max(maxAbs, std::abs(value));
          }
        }
      };
      inspect(g_gxState.texMtxs);
      inspect(g_gxState.ptTexMtxs);
      std::fprintf(stderr, "TEXTURE_MATRIX_USAGE hash=%016llx shader=%016llx coords=%llx post=%llx\n",
                   static_cast<unsigned long long>(key),
                   static_cast<unsigned long long>(info.diagnosticShaderHash),
                   sampledCoords, activePostMatrices);
      std::fprintf(stderr, "TEXTURE_MATRIX_SNAPSHOT hash=%016llx nonfinite=%u max_abs=%.9g tex_slots=%u post_slots=%u\n",
                   static_cast<unsigned long long>(key), nonfinite, maxAbs,
                   static_cast<unsigned>(g_gxState.texMtxs.size()),
                   static_cast<unsigned>(g_gxState.ptTexMtxs.size()));
      for (unsigned coord = 0; coord < MaxTexCoord; ++coord) {
        if (!info.sampledTexCoords.test(coord) || info.indexAttr.test(GX_VA_TEX0MTXIDX + coord)) continue;
        const auto& gen = g_gxState.tcgs[coord];
        if (gen.type != GX_TG_MTX2x4 && gen.type != GX_TG_MTX3x4) continue;
        float pre[12]{1,0,0,0, 0,1,0,0, 0,0,1,0};
        float post[12]{1,0,0,0, 0,1,0,0, 0,0,1,0};
        if (gen.mtx != GX_IDENTITY) {
          const unsigned slot = gen.mtx / 3;
          if (slot < MaxPnMtx) std::memcpy(pre, &g_gxState.pnMtx[slot].pos, sizeof(pre));
          else if (slot < MaxPnMtx + MaxTexMtx)
            std::memcpy(pre, &g_gxState.texMtxs[slot - MaxPnMtx], sizeof(pre));
          else continue;
        }
        if (gen.postMtx != GX_PTIDENTITY) {
          const unsigned slot = (gen.postMtx - GX_PTTEXMTX0) / 3;
          if (slot >= MaxPTTexMtx) continue;
          std::memcpy(post, &g_gxState.ptTexMtxs[slot], sizeof(post));
        }
        auto row = fmt::format("TEXTURE_FIXED_ROWS hash={:016x} shader={:016x} coord={} src={} type={} matrix={} post={} normalize={} pre=",
                     key, info.diagnosticShaderHash, coord, unsigned(gen.src), unsigned(gen.type),
                     unsigned(gen.mtx), unsigned(gen.postMtx), unsigned(gen.normalize));
        for (unsigned n=0;n<12;++n) row += fmt::format("{}{:.9g}",n?",":"",pre[n]);
        row += " postrows=";
        for (unsigned n=0;n<12;++n) row += fmt::format("{}{:.9g}",n?",":"",post[n]);
        std::fprintf(stderr,"%s\n",row.c_str());
      }
    }
  }
  // Recording owner only; report each shader once and preserve append semantics.
  static absl::flat_hash_set<uint64_t> seen;
  const bool trace = (diagnosticFlags & LayoutTrace) &&
                     seen.insert(info.diagnosticShaderHash).second;
  UniformDiagnosticWriter buf{storage, info.diagnosticShaderHash, trace};
  buf.reserve_extra(info.uniformSize);

  buf.append<f32>(g_gxState.renderViewport.width);
  buf.append<f32>(g_gxState.renderViewport.height);
  buf.append<f32>(g_gxState.logicalViewport.width);
  buf.append<f32>(g_gxState.logicalViewport.height);
  if (info.lineMode != 0) {
    if (info.lineMode == 3) { // GX_POINTS
      buf.append<f32>(static_cast<f32>(g_gxState.pointSize) / 6.f);
      buf.append<f32>(1.0f);
      buf.append<f32>(tex_offset(g_gxState.pointTexOffset));
      buf.append<u32>(point_texcoord_mask());
    } else { // GX_LINES / GX_LINESTRIP
      buf.append<f32>(static_cast<f32>(g_gxState.lineWidth) / 6.f);
      buf.append<f32>(g_gxState.lineHalfAspect ? 0.5f : 1.f);
      buf.append<f32>(tex_offset(g_gxState.lineTexOffset));
      buf.append<u32>(line_texcoord_mask());
    }
  }
  auto proj = g_gxState.proj;
  if constexpr (UseReversedZ) {
    proj.m2 = proj.m2 * Vec4{-1.f, -1.f, -1.f, -1.f};
  } else {
    proj.m2 = proj.m2 + proj.m3;
  }
  buf.append(proj);

  for (int i = 0; i < MaxPnMtx; i++) {
    buf.append(g_gxState.pnMtx[i].pos);
  }

  for (int i = 0; i < MaxTexMtx; i++) {
    buf.append(g_gxState.texMtxs[i]);
  }

  for (int i = 0; i < MaxPnMtx; i++) {
    buf.append(g_gxState.pnMtx[i].nrm);
  }

  for (int i = 0; i < info.loadsTevReg.size(); ++i) {
    if (info.loadsTevReg.test(i)) {
      buf.append(g_gxState.colorRegs[i]);
    }
  }
  if (info.lightingEnabled) {
    // Lights
    static_assert(sizeof(g_gxState.lights) == 80 * GX::MaxLights);
    buf.append(g_gxState.lights);
    // Light state for all channels
    for (int i = 0; i < 4; ++i) {
      buf.append<u32>(g_gxState.colorChannelState[i].lightMask.to_ulong());
    }
  }
  for (int i = 0; i < info.sampledColorChannels.size(); ++i) {
    if (!info.sampledColorChannels.test(i)) {
      continue;
    }
    const auto& ccc = g_gxState.colorChannelConfig[i];
    const auto& ccs = g_gxState.colorChannelState[i];
    if (!info.lightingDisabled && ccc.lightingEnabled && ccc.ambSrc == GX_SRC_REG) {
      buf.append(ccs.ambColor);
    }
    if (ccc.matSrc == GX_SRC_REG) {
      buf.append(ccs.matColor);
    }
    const auto& ccca = g_gxState.colorChannelConfig[i + GX_ALPHA0];
    const auto& ccsa = g_gxState.colorChannelState[i + GX_ALPHA0];
    if (!info.lightingDisabled && ccca.lightingEnabled && ccca.ambSrc == GX_SRC_REG) {
      buf.append(ccsa.ambColor);
    }
    if (ccca.matSrc == GX_SRC_REG) {
      buf.append(ccsa.matColor);
    }
  }
  for (int i = 0; i < info.sampledKColors.size(); ++i) {
    if (info.sampledKColors.test(i)) {
      buf.append(g_gxState.kcolors[i]);
    }
  }
  if (info.usesPTTexMtx.any()) {
    const auto postOffset = storage.size();
    for (int i = 0; i < info.usesPTTexMtx.size(); ++i) {
      buf.append(g_gxState.ptTexMtxs[i]);
    }
    // Inspect the packed bytes, rather than reprinting the source matrix.
    // Match-armed, bounded, recording-owner only; no uniform mutation.
    if ((diagnosticFlags & (ContentTrace | UvTrace)) == (ContentTrace | UvTrace)) {
      static unsigned records = 0;
      static bool limitReported = false;
      static_assert(sizeof(g_gxState.ptTexMtxs[0]) == 48);
      for (unsigned i = 0; i < info.usesPTTexMtx.size(); ++i) {
        // This diagnostic targets the two live shadow slots observed in the
        // original match. Slot zero is heavily used by unrelated menu effects.
        if (i != 1 && i != 2) continue;
        if (!info.usesPTTexMtx.test(i)) continue;
        if (records >= 64) {
          if (!limitReported) {
            std::fprintf(stderr, "SHADOW_UNIFORM_PACKED_LIMIT records=64\n");
            limitReported = true;
          }
          break;
        }
        const auto offset = postOffset + i * 48;
        float packed[12];
        std::memcpy(packed, storage.data() + offset, sizeof(packed));
        char line[1024];
        int length = std::snprintf(line, sizeof(line),
            "SHADOW_UNIFORM_PACKED shader=%llx post=%u offset=%zu bytes=48",
            static_cast<unsigned long long>(info.diagnosticShaderHash), 64 + i * 3, offset);
        for (unsigned n = 0; n < 12; ++n)
          length += std::snprintf(line + length, sizeof(line) - length,
                                  " matrix%u=%.9g", n, packed[n]);
        std::fprintf(stderr, "%s\n", line);
        ++records;
      }
    }
  }
  if (info.usesFog) {
    const auto& state = g_gxState.fog;
    const float logicalWidth = std::max(g_gxState.logicalViewport.width, 1.f);
    const float renderWidth = std::max(g_gxState.renderViewport.width, 1.f);
    Fog fog{
        .color = state.color,
        .a = state.a,
        .b = state.b,
        .c = state.c,
        .rangeCenter = ((static_cast<float>(state.rangeCenter) - g_gxState.logicalViewport.left) / logicalWidth) * 2.f -
                       1.f + (g_gxState.renderViewport.left / renderWidth) * 2.f,
    };
    for (u32 i = 0; i < state.rangeK.size(); ++i) {
      const u32 source = (i & ~1u) | (1u - (i & 1u));
      fog.rangeK[i / 4][i % 4] = static_cast<float>(state.rangeK[source]) / 64.f;
    }
    fog.rangeK[2][2] = fog.rangeK[2][1];
    fog.rangeK[2][3] = fog.rangeK[2][1];
    buf.append(fog);
  }
  for (const auto& scale : g_gxState.texCoordScales) {
    buf.append(Vec4{static_cast<f32>(scale.scaleS) + 1.0f, static_cast<f32>(scale.scaleT) + 1.0f, 0.0f, 0.0f});
  }
  if (info.usedIndTexMtxs.any()) {
    for (u32 i = 0; i < MaxIndTexMtxs; ++i) {
      const auto& mtx = g_gxState.indTexMtxs[i];
      buf.append(Vec4{mtx.mtx.m0.x, mtx.mtx.m0.y, mtx.mtx.m1.x, mtx.mtx.m1.y});
      buf.append(Vec4{mtx.mtx.m2.x, mtx.mtx.m2.y, std::exp2f(mtx.scaleExp), 0.0f});
    }
  }
  for (int i = 0; i < info.sampledTextures.size(); ++i) {
    if (!info.sampledTextures.test(i)) {
      continue;
    }
    buf.append(texture_size_bias(get_texture(static_cast<GXTexMapID>(i))));
  }
  if (trace) std::fprintf(stderr, "UNIFORM_LAYOUT_END hash=%llx bytes=%zu aligned=%u\n",
                          static_cast<unsigned long long>(info.diagnosticShaderHash), storage.size(), info.uniformSize);
}

void refresh_uniform_diagnostic_flags() noexcept {
  uniform_diagnostic_flags().store(read_uniform_diagnostic_flags(), std::memory_order_relaxed);
}

gfx::Range build_uniform(const ShaderInfo& info) noexcept {
  ZoneScoped;
  static ByteBuffer buf;
  buf.clear();
  const unsigned flags = uniform_diagnostic_flags().load(std::memory_order_relaxed);
  fill_uniform(buf, info, flags);
  static const bool verify = std::getenv("AURORA_DIAGNOSTIC_VERIFY_UNIFORM_FLAGS") != nullptr;
  if (verify) {
    const unsigned freshFlags = read_uniform_diagnostic_flags();
    AURORA_ASSERT(flags == freshFlags, "Cached uniform diagnostic flags are stale");
    static ByteBuffer reference;
    reference.clear();
    fill_uniform(reference, info, freshFlags);
    AURORA_ASSERT(reference.size() == buf.size() &&
                  std::memcmp(reference.data(), buf.data(), buf.size()) == 0,
                  "Cached diagnostic flags changed uniform bytes");
    static uint64_t checks = 0;
    if (++checks % 4096 == 0)
      std::fprintf(stderr, "NATIVE_UNIFORM_FLAGS_VERIFIED checks=%llu flags=%u bytes_equal=1\n",
                   static_cast<unsigned long long>(checks), flags);
  }
  return gfx::push_uniform(buf.data(), buf.size());
}
} // namespace aurora::gx
