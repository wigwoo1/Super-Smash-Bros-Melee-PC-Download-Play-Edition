#include "../lib/audio/ax_block_mixer.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace aurora::audio;
static int16_t clipped(double value) {return static_cast<int16_t>(std::clamp(std::floor(value),-32768.,32767.));}
static void reference(int64_t l,int64_t r,int64_t s,int16_t* out) {
  const double left=std::clamp(double(l),-32768.,32767.);
  const double right=std::clamp(double(r),-32768.,32767.);
  const double root=std::sqrt(0.5);
  out[0]=clipped(left*root);out[1]=clipped(right*root);
  out[2]=clipped((left+right)*0.5);out[3]=0;
  const double surround=std::floor(double(s)*root);
  out[4]=clipped(std::floor((left-right)*root*0.5)+surround);
  out[5]=clipped(std::floor((right-left)*root*0.5)+surround);
}
static unsigned checked=0;
static void check(int64_t l,int64_t r,int64_t s) {
  std::array<int16_t,8> actual;actual.fill(12345);
  int16_t expected[6];reference(l,r,s,expected);
  ax_pack_buses(l,r,s,6,actual.data()+1,true);
  assert(actual.front()==12345 && actual.back()==12345);
  for(unsigned i=0;i<6;++i)assert(std::abs(int(actual[i+1])-int(expected[i]))<=2);
  assert(actual[4]==0); // No unfiltered LFE.
  int16_t stereo[2];ax_pack_buses(l,r,s,2,stereo,true);
  assert(stereo[0]==clamp_sample(l) && stereo[1]==clamp_sample(r));
  if(!s) {
    const double left=clamp_sample(l),right=clamp_sample(r);
    double power=0;for(unsigned i=1;i<=6;++i)power+=double(actual[i])*actual[i];
    const double original=left*left+right*right;
    // Integer coefficient quantization and signed flooring only; each sample
    // is independently checked above against the exact orthonormal matrix.
    assert(std::abs(power-original)<=8*(std::abs(left)+std::abs(right)+1));
  }
  ++checked;
}
static std::array<uint8_t,512> pcm;
static bool read_pcm(void*,uint32_t at,uint8_t* output,uint32_t count) {
  if(at>pcm.size() || count>pcm.size()-at)return false;
  std::memcpy(output,pcm.data()+at,count);return true;
}
static AXVPB voice(unsigned start,unsigned end) {
  AXVPB p{};p.pb.state=1;p.pb.srcSelect=2;p.pb.src.ratioHi=1;
  p.pb.addr.format=10;p.pb.addr.loopFlag=1;p.pb.addr.currentAddressLo=start;
  p.pb.addr.loopAddressLo=start;p.pb.addr.endAddressLo=end;
  p.pb.ve.currentVolume=32767;return p;
}
int main() {
  constexpr int values[]={-2097088,-32768,-32767,-12000,-1,0,1,8192,30000,32767,2097088};
  for(auto l:values)for(auto r:values)for(auto s:{0,-32768,32767,-2097088,2097088})check(l,r,s);
  for(int sample=-32768;sample<=32767;sample+=127) {
    check(sample,sample,0);check(sample,-sample,0);check(sample,0,0);check(0,sample,0);
  }
  constexpr int16_t left[]={0,32767,-32768,10000,-10000,1,-1,8192};
  constexpr int16_t right[]={32767,0,16384,-20000,20000,-1,1,-8192};
  for(unsigned i=0;i<256;++i) {
    const auto value=uint16_t(i<128?left[i%8]:right[i%8]);
    pcm[i*2]=value>>8;pcm[i*2+1]=value;
  }
  std::array<AXVPB,2> voices{voice(0,127),voice(128,255)};
  voices[0].pb.mix.vL=32768;voices[1].pb.mix.vR=32768;
  std::array<AxVoiceMixer,2> mixers{};
  std::array<bool,2> used{true,true},reserved{};
  std::array<AuxRing,2> aux{};bool effects=false;
  std::array<int16_t,1442> output;
  for(unsigned block=0;block<20;++block) {
    output.fill(12345);
    assert(ax_mix_block(voices,mixers,used,reserved,aux,read_pcm,nullptr,
                       output.data()+1,240,6,effects,true)==AxBlockResult::ok);
    assert(output.front()==12345 && output.back()==12345 && !effects);
    for(unsigned i=0;i<240;++i) {
      int16_t expected[6];
      const auto l=clipped(double(left[i%8])*32767./32768.);
      const auto r=clipped(double(right[i%8])*32767./32768.);
      reference(l,r,0,expected);
      for(unsigned c=0;c<6;++c)assert(std::abs(int(output[1+i*6+c])-int(expected[c]))<=2);
    }
  }
  std::printf("AX_STEREO_UPMIX_PASS matrix_cases=%u production_pcm_frames=4800 independent_float_oracle=1 power_bound=1 stereo_unchanged=1 no_lfe=1 no_authored_3d_claim=1 physical_output_verified=0\n",checked);
}
