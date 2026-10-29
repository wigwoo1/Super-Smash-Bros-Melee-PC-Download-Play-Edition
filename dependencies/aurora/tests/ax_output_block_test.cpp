#include "../lib/audio/ax_block_mixer.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace aurora::audio;

static std::array<uint8_t,512> pcm;
static bool read_pcm(void*,uint32_t at,uint8_t* out,uint32_t count) {
  if(at>pcm.size() || count>pcm.size()-at)return false;
  std::memcpy(out,pcm.data()+at,count);return true;
}
static AXVPB voice() {
  AXVPB p{};p.pb.state=1;p.pb.srcSelect=2;p.pb.src.ratioHi=1;
  p.pb.addr.format=10;p.pb.addr.endAddressLo=255;p.pb.addr.loopFlag=1;
  p.pb.ve.currentVolume=32767;return p;
}
static int16_t clip(double value) {return static_cast<int16_t>(std::clamp(std::floor(value),-32768.,32767.));}
static int64_t ref_gain(int sample,unsigned gain) {return clip(double(sample)*gain/32768.);}
struct State {
  std::array<AXVPB,2> voices{};
  std::array<AxVoiceMixer,2> mixers{};
  std::array<bool,2> used{},reserved{};
  std::array<AuxRing,2> aux{};
  bool rendering=false;
  AxBlockResult mix(int16_t* out,unsigned frames,unsigned channels) {
    return ax_mix_block(voices,mixers,used,reserved,aux,read_pcm,nullptr,out,frames,channels,rendering);
  }
};
struct Effect {unsigned calls=0;bool* rendering=nullptr;bool inject=false;};
static void effect(void* data,void* context) {
  auto& e=*static_cast<Effect*>(context);assert(e.rendering && *e.rendering);++e.calls;
  auto& b=*static_cast<AXFX_BUFFERUPDATE*>(data);
  for(unsigned i=0;i<160;++i){b.left[i]*=2;b.right[i]*=2;b.surround[i]=e.inject?1234:b.surround[i]*2;}
}
static void refusal_unchanged(State& state,unsigned frames,unsigned channels,AxBlockResult expected) {
  const auto before=state;
  std::array<int16_t,1442> output;output.fill(321);
  assert(state.mix(output.data()+1,frames,channels)==expected);
  assert(std::memcmp(before.voices.data(),state.voices.data(),sizeof(state.voices))==0);
  assert(std::memcmp(before.mixers.data(),state.mixers.data(),sizeof(state.mixers))==0);
  assert(std::memcmp(before.aux.data(),state.aux.data(),sizeof(state.aux))==0);
  for(auto sample:output)assert(sample==321);
  assert(!state.rendering);
}
int main() {
  constexpr int16_t wave[]={16384,-16384,32767,-32768,1,-1,0,8192};
  for(unsigned i=0;i<256;++i){auto s=uint16_t(wave[i%8]);pcm[i*2]=s>>8;pcm[i*2+1]=s;}
  unsigned checked=0;
  for(unsigned channels:{2u,6u})for(unsigned gain:{0u,1u,16384u,32768u,65535u}) {
    State s;s.used={true,true};s.voices={voice(),voice()};
    s.voices[0].pb.mix.vL=gain;s.voices[1].pb.mix.vR=gain;
    if(channels==6){s.voices[0].pb.mix.vS=gain;s.voices[1].pb.mix.vS=gain;}
    std::array<int16_t,1442> output;output.fill(123);
    assert(s.mix(output.data()+1,240,channels)==AxBlockResult::ok);
    assert(output[0]==123 && output[1+240*channels]==123);
    for(unsigned frame=0;frame<240;++frame){
      const int envelope=clip(double(wave[frame%8])*32767./32768.);
      const int64_t sample=ref_gain(envelope,gain);
      const auto* p=output.data()+1+frame*channels;
      assert(p[0]==sample && p[1]==sample);
      if(channels==6){
        assert(p[2]==0 && p[3]==0);
        const auto rear=clip(double(sample*2)*23170./32768.);
        assert(p[4]==rear && p[5]==rear);
      }
      ++checked;
    }
  }
  State invalid;invalid.used[0]=true;invalid.voices[0]=voice();invalid.voices[0].pb.mix.vS=1;
  refusal_unchanged(invalid,160,2,AxBlockResult::invalid);
  refusal_unchanged(invalid,160,4,AxBlockResult::invalid);
  refusal_unchanged(invalid,241,6,AxBlockResult::invalid);
  invalid.voices[0].pb.mix.vS=0;invalid.used[1]=true;invalid.voices[1]=voice();
  invalid.voices[1].pb.addr.format=99;
  refusal_unchanged(invalid,160,6,AxBlockResult::voice_rejected);
  invalid.voices[1]=voice();invalid.voices[0].pb.mix.vAuxAS=32768;
  refusal_unchanged(invalid,160,6,AxBlockResult::invalid);
  State aux;aux.used[0]=true;aux.voices[0]=voice();
  aux.voices[0].pb.mix.vAuxAS=32768;
  Effect e{0,&aux.rendering,false};aux.aux[0].callback=effect;aux.aux[0].context=&e;
  std::array<int16_t,960> output;
  for(unsigned block=0;block<3;++block){
    assert(aux.mix(output.data(),160,6)==AxBlockResult::ok);
    for(unsigned i=0;i<160;++i){
      for(unsigned c=0;c<4;++c)assert(output[i*6+c]==0);
      const int envelope=clip(double(wave[i%8])*32767./32768.);
      const auto rear=block<2?0:clip(double(envelope*2)*23170./32768.);
      assert(output[i*6+4]==rear && output[i*6+5]==rear);++checked;
    }
  }
  assert(e.calls==3 && !aux.rendering);
  State stereo;Effect injected{0,&stereo.rendering,true};
  stereo.aux[0].callback=effect;stereo.aux[0].context=&injected;
  refusal_unchanged(stereo,160,2,AxBlockResult::surround_return_rejected);
  assert(injected.calls==1); // Caller must fault lifetime; never replay this callback.
  assert(ax_frame_bytes(2)==4 && ax_frame_bytes(6)==12 && ax_frame_bytes(4)==0);
  std::printf("AX_OUTPUT_BLOCK_PASS samples=%u channels=2,6 rear_gain_q15=23170 center_lfe_silent=1 aux_delay_blocks=2 atomic_refusals=1 callback_owner_guard=1 speaker_output_verified=0\n",checked);
}
