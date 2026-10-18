#pragma once
#include "ax_sample_cursor.hpp"
#include "stereo_mixer.hpp"
#include "itd_delay.hpp"
#include "ax_fir_resampler.hpp"
namespace aurora::audio {
struct AxVoiceMixer {MixerVoice sample;AxFirState fir;ItdDelay itd;bool ready=false;};
struct AxAuxSample { std::array<int64_t,3> a{},b{}; };
inline int64_t floor_q15(int64_t value){return value>=0?value/32768:-((-value+32767)/32768);}
inline int16_t clamp_sample(int64_t value){return static_cast<int16_t>(std::clamp(value,int64_t{-32768},int64_t{32767}));}
inline bool ax_mix_voice(AXVPB& voice,AxVoiceMixer& mixer,SampleRead read,void* context,int64_t& left,int64_t& right,AxAuxSample* aux=nullptr,int64_t* surround=nullptr){
  // Original AX has a distinct main S bus. A future multi-channel block caller
  // must supply it explicitly; never fold it into front channels or drop it.
  if(surround==&left || surround==&right)return false;
  auto& pb=voice.pb;
  if(pb.state==0){mixer.ready=false;return true;}
  // A caller must provide real aux buses when sends are configured. The host
  // stereo-only path still refuses these until block routing is installed.
  const bool sends=pb.mix.vAuxAL || pb.mix.vAuxAR || pb.mix.vAuxAS || pb.mix.vAuxBL || pb.mix.vAuxBR || pb.mix.vAuxBS || pb.mix.vDeltaAuxAL || pb.mix.vDeltaAuxAR || pb.mix.vDeltaAuxAS || pb.mix.vDeltaAuxBL || pb.mix.vDeltaAuxBR || pb.mix.vDeltaAuxBS;
  if(pb.state!=1 || pb.srcSelect>2 || (pb.srcSelect==0 && pb.coefSelect>=3) || pb.itd.flag>1 || ((pb.mix.vS || pb.mix.vDeltaS) && !surround) || (sends && !aux))return false;
  constexpr uint32_t seek=AX_SYNC_FLAG_COPYADDR|AX_SYNC_FLAG_COPYCURADDR|AX_SYNC_FLAG_COPYADPCM|AX_SYNC_FLAG_COPYSRC;
  if(!mixer.ready || (voice.sync&seek)){
    MixerVoice sample;if(!ax_sample_cursor(pb,sample.cursor))return false;
    sample.phase=pb.src.currentAddressFrac;mixer.sample=sample;
    mixer.fir.phase=pb.src.currentAddressFrac;
    for(unsigned i=0;i<4;++i)mixer.fir.history[i]=std::bit_cast<int16_t>(pb.src.last_samples[i]);if(!mixer.ready)mixer.itd={};mixer.ready=true;
  }
  auto& v=mixer.sample;
  v.ratio=pb.srcSelect==2?65536:ax_address(pb.src.ratioHi,pb.src.ratioLo);
  if(v.ratio>4*65536)return false;
  v.cursor.end=ax_address(pb.addr.endAddressHi,pb.addr.endAddressLo);
  v.cursor.loop=ax_address(pb.addr.loopAddressHi,pb.addr.loopAddressLo);v.cursor.looping=pb.addr.loopFlag==1;
  v.cursor.loopPredictorScale=pb.adpcmLoop.loop_pred_scale;
  v.cursor.loopHistory={std::bit_cast<int16_t>(pb.adpcmLoop.loop_yn1),std::bit_cast<int16_t>(pb.adpcmLoop.loop_yn2)};
  int16_t mono,unused;
  if(pb.srcSelect==0){
    if(!ax_fir_frame(v.cursor,mixer.fir,v.ratio,pb.coefSelect,read,context,mono))return false;
    v.phase=mixer.fir.phase;v.active=v.cursor.running;
    for(unsigned i=0;i<4;++i)pb.src.last_samples[i]=static_cast<uint16_t>(mixer.fir.history[i]);
  }else if(!mix_stereo_frame(std::span(&v,1),read,context,mono,unused))return false;
  const auto envelope=clamp_sample(floor_q15(int64_t(mono)*std::bit_cast<int16_t>(pb.ve.currentVolume)));
  int16_t l=envelope,r=envelope;
  if(pb.itd.flag && !mixer.itd.apply(pb.itd,(voice.sync&AX_SYNC_FLAG_COPYITD)!=0,envelope,l,r))return false;
  left+=clamp_sample(floor_q15(int64_t(l)*pb.mix.vL));right+=clamp_sample(floor_q15(int64_t(r)*pb.mix.vR));
  if(surround)*surround+=clamp_sample(floor_q15(int64_t(envelope)*pb.mix.vS));
  if(aux){
    const auto send=[](int16_t sample,uint16_t& volume,uint16_t delta,int64_t& bus){
      bus+=clamp_sample(floor_q15(int64_t(sample)*volume));
      volume=static_cast<uint16_t>(volume+delta);
    };
    send(l,pb.mix.vAuxAL,pb.mix.vDeltaAuxAL,aux->a[0]);
    send(r,pb.mix.vAuxAR,pb.mix.vDeltaAuxAR,aux->a[1]);
    send(envelope,pb.mix.vAuxAS,pb.mix.vDeltaAuxAS,aux->a[2]);
    send(l,pb.mix.vAuxBL,pb.mix.vDeltaAuxBL,aux->b[0]);
    send(r,pb.mix.vAuxBR,pb.mix.vDeltaAuxBR,aux->b[1]);
    send(envelope,pb.mix.vAuxBS,pb.mix.vDeltaAuxBS,aux->b[2]);
  }
  pb.ve.currentVolume=static_cast<uint16_t>(pb.ve.currentVolume+pb.ve.currentDelta);
  pb.mix.vL=static_cast<uint16_t>(pb.mix.vL+pb.mix.vDeltaL);pb.mix.vR=static_cast<uint16_t>(pb.mix.vR+pb.mix.vDeltaR);
  pb.mix.vS=static_cast<uint16_t>(pb.mix.vS+pb.mix.vDeltaS);
  pb.addr.currentAddressHi=v.cursor.current>>16;pb.addr.currentAddressLo=v.cursor.current;
  pb.adpcm.yn1=static_cast<uint16_t>(v.cursor.history.recent);pb.adpcm.yn2=static_cast<uint16_t>(v.cursor.history.older);pb.adpcm.pred_scale=v.cursor.predictorScale;
  pb.src.currentAddressFrac=v.phase;pb.state=v.active?1:0;voice.sync=0;return true;
}
}
