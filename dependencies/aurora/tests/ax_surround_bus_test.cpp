#include "../lib/audio/ax_voice_mixer.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace aurora::audio;
static constexpr int16_t waveform[]={16384,-16384,8192,0,32767,-32768,1,-1};
static std::array<uint8_t,512> pcm;
static bool read_sample(void*,uint32_t at,uint8_t* output,uint32_t count){
  if(at>pcm.size()||count>pcm.size()-at)return false;
  std::memcpy(output,pcm.data()+at,count);return true;
}
static AXVPB make_voice(){
  AXVPB p{};p.pb.state=1;p.pb.srcSelect=1;p.pb.src.ratioHi=1;
  p.pb.addr.format=10;p.pb.addr.endAddressLo=255;p.pb.ve.currentVolume=32767;return p;
}
static int64_t reference(int64_t sample,uint16_t gain){
  const auto value=static_cast<int64_t>(std::floor(double(sample)*double(gain)/32768.0));
  return std::min(int64_t(32767),std::max(int64_t(-32768),value));
}
template<class T>static auto snapshot(const T& value){
  std::array<uint8_t,sizeof(T)> bytes;std::memcpy(bytes.data(),&value,sizeof(T));return bytes;
}
static void refused_unchanged(AXVPB& p,AxVoiceMixer& mixer,int64_t& l,int64_t& r,
                              AxAuxSample* aux=nullptr,int64_t* s=nullptr){
  const auto pb=snapshot(p);const auto state=snapshot(mixer);const auto old_l=l,old_r=r,old_s=s?*s:0;
  assert(!ax_mix_voice(p,mixer,read_sample,nullptr,l,r,aux,s));
  assert(std::memcmp(&p,pb.data(),sizeof(p))==0&&std::memcmp(&mixer,state.data(),sizeof(mixer))==0);
  assert(l==old_l&&r==old_r&&(!s||*s==old_s));
}
int main(){
  for(unsigned i=0;i<256;i++){
    const auto sample=uint16_t(waveform[i%8]);pcm[2*i]=sample>>8;pcm[2*i+1]=sample;
  }
  unsigned cases=0;
  for(uint16_t initial:{uint16_t(0),uint16_t(1),uint16_t(32767),uint16_t(32768),uint16_t(65535)})
    for(uint16_t delta:{uint16_t(0),uint16_t(1),uint16_t(65535),uint16_t(32768),uint16_t(257)})
      for(unsigned mask=0;mask<8;mask++){
        auto p=make_voice();AxVoiceMixer mixer;
        p.pb.mix.vL=(mask&1)?32768:0;p.pb.mix.vR=(mask&2)?16384:0;
        p.pb.mix.vS=(mask&4)?initial:0;p.pb.mix.vDeltaS=delta;
        auto gain=p.pb.mix.vS;
        for(unsigned frame=0;frame<16;frame++){
          int64_t l=17,r=23,s=31;
          const auto envelope=static_cast<int64_t>(std::floor(double(waveform[frame%8])*32767.0/32768.0));
          assert(ax_mix_voice(p,mixer,read_sample,nullptr,l,r,nullptr,&s));
          assert(l==17+reference(envelope,(mask&1)?32768:0));
          assert(r==23+reference(envelope,(mask&2)?16384:0));
          assert(s==31+reference(envelope,gain));
          gain=uint16_t(gain+delta);assert(p.pb.mix.vS==gain);++cases;
        }
      }
  assert(cases==3200);
  auto p=make_voice();AxVoiceMixer mixer;int64_t l=17,r=23,s=31;
  p.pb.mix.vS=32768;refused_unchanged(p,mixer,l,r);
  p.pb.mix.vS=0;p.pb.mix.vDeltaS=1;refused_unchanged(p,mixer,l,r);
  p.pb.mix.vS=32768;p.pb.mix.vDeltaS=0;refused_unchanged(p,mixer,l,r,nullptr,&l);
  refused_unchanged(p,mixer,l,r,nullptr,&r);
  p.pb.mix.vAuxAS=32768;refused_unchanged(p,mixer,l,r,nullptr,&s);
  AxAuxSample buses;l=r=s=0;p.pb.mix.vAuxBS=16384;
  assert(ax_mix_voice(p,mixer,read_sample,nullptr,l,r,&buses,&s));
  assert(l==0&&r==0&&s==16383&&buses.a[2]==16383&&buses.b[2]==8191);
  p=make_voice();mixer={};p.pb.mix.vL=p.pb.mix.vR=p.pb.mix.vS=32768;
  p.pb.itd.flag=1;p.pb.itd.shiftL=p.pb.itd.targetShiftL=2;p.sync|=AX_SYNC_FLAG_COPYITD;
  l=r=s=0;assert(ax_mix_voice(p,mixer,read_sample,nullptr,l,r,nullptr,&s));
  assert(l==0&&r==16383&&s==16383); // Main S uses the undelayed mono envelope.
  std::puts("AX_SURROUND_BUS_PASS frames=3200 independent_reference=1 sink_required=1 refusals_unchanged=1 aux_separate=1 itd_separate=1 speaker_output_51=0");
}
