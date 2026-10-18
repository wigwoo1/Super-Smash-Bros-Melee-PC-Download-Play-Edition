#pragma once
#include "../gx/gx.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <span>
namespace aurora::gfx::original_material {
// Diagnostic oracle: regular TEV only. It does not call the WGSL generator,
// texture converter or GPU arithmetic helpers. Unsupported states fail closed.
using Color=std::array<int,4>;
struct Inputs {std::array<Color,4> registers{},konst{};std::array<Color,2> raster{};};
inline int floor_div(int value,int divisor){return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
inline int regular(int a,int b,int c,int d,const gx::TevOp& op){
  a&=255;b&=255;c&=255;const int factor=c+c/128;
  const int scale=op.scale==GX_CS_DIVIDE_2?1:1<<unsigned(op.scale);
  const int rounding=op.scale==GX_CS_DIVIDE_2?0:op.op==GX_TEV_SUB?127:128;
  const int interpolation=((a*(256-factor)+b*factor)*scale+rounding)/256;
  const int bias=op.bias==GX_TB_ADDHALF?128:op.bias==GX_TB_SUBHALF?-128:0;
  int result=(d+bias)*scale+(op.op==GX_TEV_SUB?-interpolation:interpolation);
  if(op.scale==GX_CS_DIVIDE_2)result=floor_div(result,2);
  return std::clamp(result,op.clamp?0:-1024,op.clamp?255:1023);
}
inline bool uniforms(std::span<const uint8_t> bytes,const gx::ShaderConfig& config,const gx::ShaderInfo& info,Inputs& out){
  size_t offset=80+30*48;
  auto read=[&](Color& c){if(offset>bytes.size()||16>bytes.size()-offset)return false;
    for(unsigned n=0;n<4;n++){float f;std::memcpy(&f,bytes.data()+offset+n*4,4);if(!std::isfinite(f)||std::abs(f)>5)return false;c[n]=int(std::round(double(f)*255));}offset+=16;return true;};
  for(unsigned i=0;i<4;i++)if(info.loadsTevReg[i]&&!read(out.registers[i]))return false;
  if(info.lightingEnabled)offset+=8*80+16;
  for(unsigned i=0;i<2;i++)if(info.sampledColorChannels[i])for(unsigned a=0;a<2;a++){
    const auto& c=config.colorChannels[i+a*2];if(c.lightingEnabled&&c.ambSrc==GX_SRC_REG)offset+=16;if(c.matSrc==GX_SRC_REG)offset+=16;
  }
  for(unsigned i=0;i<4;i++)if(info.sampledKColors[i]&&!read(out.konst[i]))return false;
  return offset<=bytes.size();
}
inline bool valid(const gx::ShaderConfig& c,unsigned slot){
  if(!c.tevStageCount||c.tevStageCount>16||c.fogType!=GX_FOG_NONE||c.numIndStages||c.zTextureOp||c.lineMode||gx::UsePerPixelLighting)return false;
  for(unsigned i=0;i<c.tevStageCount;i++){
    const auto& s=c.tevStages[i];
    if(unsigned(s.colorPass.a)>15||unsigned(s.colorPass.b)>15||unsigned(s.colorPass.c)>15||unsigned(s.colorPass.d)>15||
       unsigned(s.alphaPass.a)>7||unsigned(s.alphaPass.b)>7||unsigned(s.alphaPass.c)>7||unsigned(s.alphaPass.d)>7)return false;
    for(const auto& op:{s.colorOp,s.alphaOp})if(unsigned(op.op)>1||unsigned(op.bias)>2||unsigned(op.scale)>3||unsigned(op.outReg)>3)return false;
    if(s.texMapId!=GX_TEXMAP_NULL&&unsigned(s.texMapId)!=slot)return false;
    if(unsigned(s.channelId)>5&&s.channelId!=GX_COLOR_ZERO&&s.channelId!=GX_COLOR_NULL)return false;
    if(unsigned(s.tevSwapTex)>3||unsigned(s.tevSwapRas)>3)return false;
    if((unsigned(s.kcSel)>7&&unsigned(s.kcSel)<12)||unsigned(s.kcSel)>31||
       (unsigned(s.kaSel)>7&&unsigned(s.kaSel)<16)||unsigned(s.kaSel)>31)return false;
  }
  for(const auto& s:c.tevSwapTable)if(unsigned(s.red)>3||unsigned(s.green)>3||unsigned(s.blue)>3||unsigned(s.alpha)>3)return false;
  return true;
}
inline Color swap(Color c,const gx::TevSwap& s){return {c[s.red],c[s.green],c[s.blue],c[s.alpha]};}
inline int konst(unsigned selector,unsigned component,const Inputs& input){
  constexpr int fractions[]{255,223,191,159,128,96,64,32};
  if(selector<8)return fractions[selector];
  if(selector<16)return input.konst[selector-12][component];
  return input.konst[(selector-16)%4][(selector-16)/4];
}
inline bool comparison(unsigned op,int a,int b){switch(op){case 0:return false;case 1:return a<b;case 2:return a==b;case 3:return a<=b;case 4:return a>b;case 5:return a!=b;case 6:return a>=b;case 7:return true;}return false;}
inline bool evaluate(const gx::ShaderConfig& config,unsigned slot,const Inputs& input,Color texture,Color& output,bool& accepted){
  if(!valid(config,slot))return false;auto regs=input.registers;
  for(unsigned n=0;n<config.tevStageCount;n++){
    const auto& s=config.tevStages[n];const auto tex=swap(s.texMapId==GX_TEXMAP_NULL?Color{}:texture,config.tevSwapTable[s.tevSwapTex]);
    const auto ras=swap(unsigned(s.channelId)<=5?input.raster[unsigned(s.channelId)&1]:Color{},config.tevSwapTable[s.tevSwapRas]);
    auto color=[&](unsigned arg,unsigned channel){
      if(arg<8)return regs[arg/2][arg%2?3:channel];
      switch(arg){case 8:return tex[channel];case 9:return tex[3];case 10:return ras[channel];case 11:return ras[3];case 12:return 255;case 13:return 128;case 14:return konst(s.kcSel,channel,input);default:return 0;}
    };
    auto alpha=[&](unsigned arg){if(arg<4)return regs[arg][3];switch(arg){case 4:return tex[3];case 5:return ras[3];case 6:return konst(s.kaSel,3,input);default:return 0;}};
    Color result{};for(unsigned channel=0;channel<3;channel++)result[channel]=regular(color(s.colorPass.a,channel),color(s.colorPass.b,channel),color(s.colorPass.c,channel),color(s.colorPass.d,channel),s.colorOp);
    result[3]=regular(alpha(s.alphaPass.a),alpha(s.alphaPass.b),alpha(s.alphaPass.c),alpha(s.alphaPass.d),s.alphaOp);
    // Both operations read the same old stage registers before either commits.
    for(unsigned channel=0;channel<3;channel++)regs[s.colorOp.outReg][channel]=result[channel];regs[s.alphaOp.outReg][3]=result[3];
  }
  const auto& last=config.tevStages[config.tevStageCount-1];for(unsigned channel=0;channel<4;channel++)output[channel]=regs[channel==3?last.alphaOp.outReg:last.colorOp.outReg][channel]&255;
  const auto& a=config.alphaCompare;const bool first=comparison(a.comp0,output[3],a.ref0),second=comparison(a.comp1,output[3],a.ref1);
  switch(a.op){case GX_AOP_AND:accepted=first&&second;break;case GX_AOP_OR:accepted=first||second;break;case GX_AOP_XOR:accepted=first!=second;break;case GX_AOP_XNOR:accepted=first==second;break;default:return false;}return true;
}
}
