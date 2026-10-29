#include <dolphin/ax.h>
#include <dolphin/ai.h>
#include "../audio/voice_pool.hpp"
#include "../audio/ax_block_mixer.hpp"
#include "../audio/ax_output_layout.hpp"
#include "../audio/ax_schedule.hpp"
#include "../audio/stream_gain.hpp"
#include "../audio/aux_ring.hpp"
#include <aurora/audio.h>
#include "../internal.hpp"
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <atomic>
static aurora::Module Log("aurora::ax");
static aurora::audio::VoicePool pool;
static SDL_AudioStream* stream=nullptr;
static std::array<aurora::audio::AxVoiceMixer,AX_MAX_VOICES> mixers;
static std::array<uint64_t,AX_MAX_VOICES> generations{};
static aurora::audio::AxSchedule schedule;
static uint64_t lastServiceNs=0,lastServiceElapsedNs=0;
static uint32_t lastServiceBlocks=0,maskedServiceCalls=0,queuePressureStops=0;
static uint64_t serviceCallbackNs=0,serviceMixNs=0,serviceDeviceNs=0;
static uint64_t serviceCalls=0,serviceBlocks=0,serviceElapsedNs=0,maxServiceElapsedNs=0;
// Observe device requests only. SDL holds its stream lock here: never mix,
// invoke game callbacks, log, or perform another stream query on this thread.
static std::atomic<uint64_t> devicePullCalls{0},devicePullRequested{0},devicePullShortage{0};
static std::atomic<uint64_t> devicePullLastNs{0},devicePullMaxGapNs{0};
static void SDLCALL observe_device_pull(void*,SDL_AudioStream*,int additional,int total){
  const auto now=SDL_GetTicksNS(),previous=devicePullLastNs.exchange(now,std::memory_order_relaxed);
  if(previous && now>=previous){auto gap=now-previous,old=devicePullMaxGapNs.load(std::memory_order_relaxed);
    while(gap>old && !devicePullMaxGapNs.compare_exchange_weak(old,gap,std::memory_order_relaxed)){} }
  devicePullCalls.fetch_add(1,std::memory_order_relaxed);
  if(total>0)devicePullRequested.fetch_add(static_cast<uint64_t>(total),std::memory_order_relaxed);
  if(additional>0)devicePullShortage.fetch_add(static_cast<uint64_t>(additional),std::memory_order_relaxed);
}
static void (*userCallback)()=nullptr;
static int (*callbackGate)()=nullptr;
static bool servicing=false,serviceFailed=false,catchupReported=false;
static bool renderingEffects=false;
static std::array<aurora::audio::AuxRing,2> auxRings;
static bool has_aux(){return auxRings[0].callback || auxRings[1].callback;}
static std::atomic<uint32_t> sourceRate{32000};
static aurora::audio::AxOutputLayout outputLayout;
static bool stereoUpmix=false;
static int device_channels(){
  SDL_AudioSpec spec{};
  return stream && SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream),&spec,nullptr)?spec.channels:0;
}
struct SdlOutputStream {
  int device_channels(){return ::device_channels();}
  bool flush(){return SDL_FlushAudioStream(stream);}
  int queued(){return SDL_GetAudioStreamQueued(stream);}
  int available(){return SDL_GetAudioStreamAvailable(stream);}
  uint64_t now_ms(){return SDL_GetTicks();}
  bool set_format(uint32_t channels,uint32_t rate){
    const SDL_AudioSpec next{SDL_AUDIO_S16,static_cast<int>(channels),static_cast<int>(rate)};
    SDL_AudioSpec actual{};
    return SDL_SetAudioStreamFormat(stream,&next,nullptr) &&
      SDL_GetAudioStreamFormat(stream,&actual,nullptr) && actual.channels==next.channels &&
      actual.freq==next.freq && actual.format==next.format;
  }
};
static int apply_output_request(){
  SdlOutputStream adapter;
  const int result=outputLayout.service(adapter,sourceRate.load());
  if(result<0)serviceFailed=true;
  return result;
}
static std::atomic<uint8_t> streamLeft{255},streamRight{255};
static std::atomic_bool aiInitialized{false};
static void require_voice_impl(AXVPB* voice,const char* api){
  const int index=pool.index(voice);
  AURORA_ASSERT(!renderingEffects && pool.owns(voice),
                "AX configuration requires live owner outside effect processing: api={} voice=0x{:X} index={} owner={} used={} reserved={} effects={}",
                api,reinterpret_cast<uintptr_t>(voice),index,pool.on_owner(),
                index>=0?pool.used[index]:false,index>=0?pool.reserved[index]:false,renderingEffects);
}
#define require_voice(voice) require_voice_impl(voice,__func__)
extern "C" {
void AXInit(){
  if(stream){AURORA_ASSERT(pool.on_owner(),"AXInit requires audio owner");return;}
  AURORA_ASSERT(SDL_InitSubSystem(SDL_INIT_AUDIO),"Native AX audio subsystem failed");
  const char* driver=SDL_GetCurrentAudioDriver();AURORA_ASSERT(driver && std::strcmp(driver,"dummy") && std::strcmp(driver,"disk"),"AX requires real playback driver");
  SDL_AudioSpec spec{SDL_AUDIO_S16,2,32000};
  stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
  AURORA_ASSERT(stream,"Native AX playback device could not open");
  AURORA_ASSERT(pool.initialize(),"Native AX owner initialization failed");
  AURORA_ASSERT(schedule.initialize(SDL_GetTicksNS()),"AX clock initialization failed");
  lastServiceNs=lastServiceElapsedNs=0;lastServiceBlocks=maskedServiceCalls=queuePressureStops=0;
  serviceCalls=serviceBlocks=serviceElapsedNs=maxServiceElapsedNs=0;catchupReported=false;
  devicePullCalls=0;devicePullRequested=0;devicePullShortage=0;devicePullLastNs=0;devicePullMaxGapNs=0;
  AURORA_ASSERT(SDL_SetAudioStreamGetCallback(stream,observe_device_pull,nullptr),"AX device observation registration failed");
  if(std::getenv("AURORA_DIAGNOSTIC_MUTE_AUDIO") || std::getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
    AURORA_ASSERT(SDL_SetAudioStreamGain(stream,0.f),"Diagnostic audio mute failed");
    AURORA_ASSERT(SDL_GetAudioStreamGain(stream)==0.f,"Diagnostic audio mute verification failed");
    std::fprintf(stderr,"NATIVE_DIAGNOSTIC_AUDIO_MUTED gain=0 mixing_active=1\n");
  }
  AURORA_ASSERT(SDL_ResumeAudioStreamDevice(stream),"Native AX playback device could not resume");
  if(const char* channels=std::getenv("AURORA_DIAGNOSTIC_AUDIO_CHANNELS")){
    AURORA_ASSERT(!std::strcmp(channels,"2") || !std::strcmp(channels,"6"),"Invalid diagnostic audio output request");
    AURORA_ASSERT(aurora_ax_request_output_channels(!std::strcmp(channels,"6")?6:2),"Diagnostic audio request rejected");
    std::fprintf(stderr,"NATIVE_AUDIO_OUTPUT_START requested=%u effective=%u device=%d status=%d physical_51_verified=0\n",
        outputLayout.requested,aurora_ax_output_channels(),device_channels(),outputLayout.status);
  }
  if(const char* upmix=std::getenv("AURORA_DIAGNOSTIC_STEREO_UPMIX")){
    AURORA_ASSERT(!std::strcmp(upmix,"0") || !std::strcmp(upmix,"1"),"Invalid diagnostic upmix request");
    AURORA_ASSERT(aurora_ax_set_stereo_upmix(!std::strcmp(upmix,"1")),"Diagnostic upmix request rejected");
  }
}
void AXQuit(){
  if(!stream)return;
  AURORA_ASSERT(!servicing && !renderingEffects,"AXQuit cannot run inside audio callbacks");
  AURORA_ASSERT(pool.shutdown(),"AXQuit requires owner outside voice drop callback");
  SDL_DestroyAudioStream(stream);stream=nullptr;SDL_QuitSubSystem(SDL_INIT_AUDIO);
  mixers={};generations={};
  schedule={};userCallback=nullptr;callbackGate=nullptr;serviceFailed=false;
  sourceRate=32000;
  aiInitialized=false;
  auxRings={};outputLayout={};stereoUpmix=false;
}
AXVPB* AXAcquireVoice(u32 priority,void(*callback)(void*),u32 context){
  return stream && !renderingEffects?pool.acquire(priority,callback,context):nullptr;
}
void AXFreeVoice(AXVPB* voice){AURORA_ASSERT(!renderingEffects && pool.release(voice),"AXFreeVoice requires live owner outside effects");}
void AXSetVoicePriority(AXVPB* voice,u32 priority){
  AURORA_ASSERT(!renderingEffects && pool.set_priority(voice,priority),"AXSetVoicePriority requires live owner outside effects and priority 1..31");
}
void AXSetVoiceAddr(AXVPB* p,AXPBADDR* addr){
  require_voice(p);AURORA_ASSERT(addr && (addr->format==0 || addr->format==10 || addr->format==25),"Unknown AX sample format");
  AURORA_ASSERT(addr->loopFlag<=1,"Invalid AX loop flag");
  if(addr->format==0)AURORA_ASSERT((addr->loopAddressLo&15)>1 && (addr->endAddressLo&15)>1 && (addr->currentAddressLo&15)>1,"AX ADPCM address on frame header");
  p->pb.addr=*addr;
  if(addr->format!=0){p->pb.adpcm={};p->pb.adpcm.gain=addr->format==10?0x800:0x100;}
  p->sync&=~(AX_SYNC_FLAG_COPYLOOP|AX_SYNC_FLAG_COPYLOOPADDR|AX_SYNC_FLAG_COPYENDADDR|AX_SYNC_FLAG_COPYCURADDR);
  p->sync|=AX_SYNC_FLAG_COPYADDR|AX_SYNC_FLAG_COPYADPCM;
}
void AXSetVoiceLoop(AXVPB* p,u16 loop){require_voice(p);AURORA_ASSERT(loop<=1,"Invalid AX loop flag");p->pb.addr.loopFlag=loop;p->sync|=AX_SYNC_FLAG_COPYLOOP;}
void AXSetVoiceLoopAddr(AXVPB* p,u32 addr){require_voice(p);p->pb.addr.loopAddressHi=addr>>16;p->pb.addr.loopAddressLo=addr;p->sync|=AX_SYNC_FLAG_COPYLOOPADDR;}
void AXSetVoiceEndAddr(AXVPB* p,u32 addr){require_voice(p);p->pb.addr.endAddressHi=addr>>16;p->pb.addr.endAddressLo=addr;p->sync|=AX_SYNC_FLAG_COPYENDADDR;}
void AXSetVoiceCurrentAddr(AXVPB* p,u32 addr){require_voice(p);p->pb.addr.currentAddressHi=addr>>16;p->pb.addr.currentAddressLo=addr;p->sync|=AX_SYNC_FLAG_COPYCURADDR;}
void AXSetVoiceAdpcm(AXVPB* p,AXPBADPCM* adpcm){require_voice(p);AURORA_ASSERT(adpcm,"Null AX ADPCM configuration");p->pb.adpcm=*adpcm;p->sync|=AX_SYNC_FLAG_COPYADPCM;}
void AXSetVoiceAdpcmLoop(AXVPB* p,AXPBADPCMLOOP* loop){require_voice(p);AURORA_ASSERT(loop,"Null AX ADPCM loop configuration");p->pb.adpcmLoop=*loop;p->sync|=AX_SYNC_FLAG_COPYADPCMLOOP;}
void AXSetVoiceSrc(AXVPB* p,AXPBSRC* src){require_voice(p);AURORA_ASSERT(src,"Null AX SRC configuration");p->pb.src=*src;p->sync&=~AX_SYNC_FLAG_COPYRATIO;p->sync|=AX_SYNC_FLAG_COPYSRC;}
void AXSetVoiceSrcRatio(AXVPB* p,float ratio){
  require_voice(p);AURORA_ASSERT(std::isfinite(ratio) && ratio>=0,"Invalid AX SRC ratio");
  const u32 fixed=static_cast<u32>(std::min(ratio,4.0f)*65536.0f);
  p->pb.src.ratioHi=fixed>>16;p->pb.src.ratioLo=fixed;p->sync|=AX_SYNC_FLAG_COPYRATIO;
}
void AXSetVoiceSrcType(AXVPB* p,u32 type){
  require_voice(p);AURORA_ASSERT(type<=AX_SRC_TYPE_4TAP_16K,"Unknown AX SRC type");
  p->pb.srcSelect=type==AX_SRC_TYPE_NONE?2:type==AX_SRC_TYPE_LINEAR?1:0;
  if(type>=AX_SRC_TYPE_4TAP_8K)p->pb.coefSelect=type-AX_SRC_TYPE_4TAP_8K;
  p->sync|=AX_SYNC_FLAG_COPYSELECT;
}
void AXSetVoiceState(AXVPB* p,u16 state){require_voice(p);AURORA_ASSERT(state<=1,"Invalid AX voice state");if(p->pb.state!=state)mixers[p->index].ready=false;p->pb.state=state;p->sync|=AX_SYNC_FLAG_COPYSTATE;if(!state)p->depop=1;}
void AXSetVoiceMix(AXVPB* p,AXPBMIX* mix){
  require_voice(p);AURORA_ASSERT(mix,"Null AX mix");p->pb.mix=*mix;
  p->pb.mixerCtrl=(mix->vAuxAL || mix->vAuxAR?1:0)|(mix->vAuxBL || mix->vAuxBR?2:0)|(mix->vS || mix->vAuxAS || mix->vAuxBS?4:0)|(mix->vDeltaL || mix->vDeltaR || mix->vDeltaS || mix->vDeltaAuxAL || mix->vDeltaAuxAR || mix->vDeltaAuxAS || mix->vDeltaAuxBL || mix->vDeltaAuxBR || mix->vDeltaAuxBS?8:0);
  p->sync|=AX_SYNC_FLAG_COPYAXPBMIX|AX_SYNC_FLAG_COPYMXRCTRL;
}
void AXSetVoiceVe(AXVPB* p,AXPBVE* ve){require_voice(p);AURORA_ASSERT(ve,"Null AX envelope");p->pb.ve=*ve;p->sync|=AX_SYNC_FLAG_COPYVOL;}
void AXSetVoiceVeDelta(AXVPB* p,s16 delta){require_voice(p);p->pb.ve.currentDelta=delta;p->sync|=AX_SYNC_FLAG_SWAPVOL;}
void AXSetVoiceItdOn(AXVPB* p){require_voice(p);p->pb.itd.flag=1;p->pb.itd.shiftL=p->pb.itd.shiftR=p->pb.itd.targetShiftL=p->pb.itd.targetShiftR=0;p->sync&=~AX_SYNC_FLAG_COPYTSHIFT;p->sync|=AX_SYNC_FLAG_COPYITD;}
void AXSetVoiceItdTarget(AXVPB* p,u16 l,u16 r){require_voice(p);AURORA_ASSERT(l<=31 && r<=31,"AX interaural delay outside 32-sample buffer");p->pb.itd.targetShiftL=l;p->pb.itd.targetShiftR=r;p->sync|=AX_SYNC_FLAG_COPYTSHIFT;}
int aurora_ax_request_output_channels(uint32_t channels){
  if(!stream || !pool.on_owner() || servicing || renderingEffects || serviceFailed ||
     !aurora::audio::ax_output_channels_valid(channels))return 0;
  if(!outputLayout.request(channels))return 0;
  const int result=apply_output_request();
  if(std::getenv("AURORA_DIAGNOSTIC_AUDIO_OUTPUT"))
    std::fprintf(stderr,"NATIVE_AUDIO_OUTPUT requested=%u source=%u device=%d status=%d pending=%d\n",
                 outputLayout.requested,outputLayout.channels,device_channels(),outputLayout.status,outputLayout.pending);
  return result>=0;
}
uint32_t aurora_ax_requested_output_channels(){return stream && pool.on_owner()?outputLayout.requested:0;}
uint32_t aurora_ax_output_channels(){return stream && pool.on_owner()?(outputLayout.channels==6 && device_channels()==6?6:2):0;}
int aurora_ax_output_status(){return stream && pool.on_owner()?outputLayout.status:AURORA_AUDIO_OUTPUT_ERROR;}
int aurora_ax_surround_available(){return stream && pool.on_owner() && device_channels()==6;}
int aurora_ax_set_stereo_upmix(int enabled){
  if(!stream || !pool.on_owner() || servicing || renderingEffects || serviceFailed || (enabled!=0 && enabled!=1))return 0;
  stereoUpmix=enabled!=0;
  if(std::getenv("AURORA_DIAGNOSTIC_AUDIO_OUTPUT"))
    std::fprintf(stderr,"NATIVE_AUDIO_UPMIX enabled=%d source=%u device=%d effective=%d\n",
        stereoUpmix,outputLayout.channels,device_channels(),stereoUpmix && aurora_ax_output_channels()==6);
  return 1;
}
int aurora_ax_stereo_upmix_enabled(){return stream && pool.on_owner() && stereoUpmix;}
int aurora_ax_render_output_frames(int16_t* samples,uint32_t frames,uint32_t capacity){
  if(!stream || !pool.on_owner() || !samples || !frames || frames>240 ||
     capacity<frames*outputLayout.channels || renderingEffects || serviceFailed || outputLayout.pending)return 0;
  const bool effects=has_aux();
  if(effects && (frames!=160 || sourceRate!=32000 || !callbackGate || !callbackGate()))return 0;
  auto next=mixers;auto tags=generations;
  for(unsigned i=0;i<AX_MAX_VOICES;i++)if(tags[i]!=pool.age[i]){next[i]={};tags[i]=pool.age[i];}
  const auto result=aurora::audio::ax_mix_block(pool.voices,next,pool.used,pool.reserved,
      auxRings,aurora::audio::read_aram_sample_bytes,nullptr,samples,frames,outputLayout.channels,renderingEffects,stereoUpmix);
  if(result!=aurora::audio::AxBlockResult::ok){
    if(result==aurora::audio::AxBlockResult::surround_return_rejected)serviceFailed=true;
    Log.error("AX output block rejected: reason={} channels={} frames={} effects={}",
              static_cast<int>(result),outputLayout.channels,frames,effects);
    return 0;
  }
  mixers=next;generations=tags;return 1;
}
int aurora_ax_render_frames(int16_t* stereo,uint32_t frames){
  return outputLayout.channels==2?aurora_ax_render_output_frames(stereo,frames,frames<=240?frames*2:0):0;
}
int aurora_ax_submit_frames(uint32_t frames){
  if(!stream || !pool.on_owner() || !frames || frames>240)return 0;
  const unsigned bytes=aurora::audio::ax_frame_bytes(outputLayout.channels);
  const int queued=SDL_GetAudioStreamQueued(stream);if(outputLayout.pending || queued<0 || queued+frames*bytes>sourceRate*bytes/10)return 0;
  const auto voices=pool.voices;const auto previous=mixers;const auto tags=generations;std::array<int16_t,240*6> output;
  const uint64_t mixStart=SDL_GetTicksNS();
  if(!aurora_ax_render_output_frames(output.data(),frames,output.size()))return 0;
  const uint64_t deviceStart=SDL_GetTicksNS();if(servicing)serviceMixNs+=deviceStart-mixStart;
  const bool submitted=SDL_PutAudioStreamData(stream,output.data(),frames*bytes);
  if(servicing)serviceDeviceNs+=SDL_GetTicksNS()-deviceStart;
  if(!submitted){
    // Effect contexts cannot be rolled back. A failed device submission after
    // an effect callback is terminal, so its observed callback is never replayed.
    if(has_aux())serviceFailed=true;
    else {pool.voices=voices;mixers=previous;generations=tags;}
    return 0;
  }return 1;
}
int aurora_ax_queued_bytes(){return stream && pool.on_owner()?SDL_GetAudioStreamQueued(stream):-1;}
int aurora_ax_flush_pending(){return stream && pool.on_owner() && SDL_FlushAudioStream(stream);}
int aurora_ax_set_callback_gate(int (*provider)()){
  if(!stream || !pool.on_owner() || !provider || callbackGate || userCallback || servicing)return 0;
  callbackGate=provider;return 1;
}
void AXRegisterCallback(void (*callback)()){
  AURORA_ASSERT(stream && pool.on_owner() && !renderingEffects && (!callback || callbackGate),"AX callback requires live owner outside effects and interrupt gate");
  userCallback=callback;
}
static void register_aux(unsigned bus,void (*callback)(void*,void*),void* context){
  AURORA_ASSERT(stream && pool.on_owner() && !renderingEffects && (!callback || (callbackGate && sourceRate==32000)),"AX aux callback requires 32kHz owner and interrupt gate");
  // Replacing or disabling an effect clears its pending native returns; never
  // retain callbacks/context pointers after the game releases an effect.
  auxRings[bus]={};auxRings[bus].callback=callback;auxRings[bus].context=context;
}
void AXRegisterAuxACallback(void (*callback)(void*,void*),void* context){register_aux(0,callback,context);}
void AXRegisterAuxBCallback(void (*callback)(void*,void*),void* context){register_aux(1,callback,context);}
int aurora_ax_service(){
  if(!stream || !pool.on_owner() || servicing || renderingEffects || serviceFailed)return 0;
  // A device changing away from six-channel output must not remain advertised
  // as surround. Drain and revert to stereo, retaining the session request.
  if(outputLayout.channels==6)outputLayout.device_changed(device_channels());
  const int outputReady=apply_output_request();
  if(outputReady<0)return 0;
  if(!outputReady)return 1;
  // Masked work remains pending, including its clock ticks.
  if((userCallback || has_aux()) && (!callbackGate || !callbackGate())){++maskedServiceCalls;return 1;}
  const uint64_t now=SDL_GetTicksNS();
  uint32_t blocks;if(!schedule.due(now,blocks)){
    Log.error("AX service clock regression rejected: now={} consumedThrough={} overdue_ns={} queued_bytes={}",now,schedule.consumedThrough,now>=schedule.consumedThrough?now-schedule.consumedThrough:0,SDL_GetAudioStreamQueued(stream));
    Log.error("AX service history: gap_ns={} previous_elapsed_ns={} previous_blocks={} masked_calls={} queue_pressure_stops={}",lastServiceNs && now>=lastServiceNs?now-lastServiceNs:0,lastServiceElapsedNs,lastServiceBlocks,maskedServiceCalls,queuePressureStops);
    Log.error("AX previous service phases: callback_ns={} mix_ns={} device_submit_ns={}",serviceCallbackNs,serviceMixNs,serviceDeviceNs);
    SDL_AudioSpec input{},output{};const bool formats=SDL_GetAudioStreamFormat(stream,&input,&output);
    Log.error("AX service totals: calls={} blocks={} elapsed_ns={} max_elapsed_ns={} source_hz={} input_hz={} output_hz={} formats_valid={} device_paused={}",serviceCalls,serviceBlocks,serviceElapsedNs,maxServiceElapsedNs,sourceRate.load(),input.freq,output.freq,formats,SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(stream)));
    const auto lastPull=devicePullLastNs.load(std::memory_order_relaxed);
    Log.error("AX device pull history: calls={} requested_input_bytes_estimate={} shortage_input_bytes_estimate={} last_gap_ns={} max_gap_ns={} available_output_bytes={}",devicePullCalls.load(),devicePullRequested.load(),devicePullShortage.load(),lastPull && now>=lastPull?now-lastPull:0,devicePullMaxGapNs.load(),SDL_GetAudioStreamAvailable(stream));
    serviceFailed=true;return 0;
  }
  if(!catchupReported && now-schedule.consumedThrough>=21*aurora::audio::AxSchedule::blockNs){
    Log.warn("AX bounded catch-up: overdue_ns={} blocks_this_call={} queued_bytes={} no_ticks_discarded=1",now-schedule.consumedThrough,blocks,SDL_GetAudioStreamQueued(stream));catchupReported=true;
  }
  servicing=true;
  lastServiceNs=now;lastServiceBlocks=0;serviceCallbackNs=serviceMixNs=serviceDeviceNs=0;
  for(unsigned i=0;i<blocks;i++){
    if((userCallback || has_aux()) && !callbackGate())break;
    const int queued=SDL_GetAudioStreamQueued(stream);
    if(queued<0){Log.error("AX device queue query failed: {}",SDL_GetError());serviceFailed=true;break;}
    const uint32_t blockFrames=sourceRate/200;
    const unsigned bytes=aurora::audio::ax_frame_bytes(outputLayout.channels);
    if(queued+blockFrames*bytes>sourceRate*bytes/10){++queuePressureStops;break;}
    if(userCallback){const uint64_t callbackStart=SDL_GetTicksNS();userCallback();serviceCallbackNs+=SDL_GetTicksNS()-callbackStart;}
    // A failed callback-configured render is terminal for this AX lifetime:
    // never retry its already-observed game callback as if it had not run.
    if(!aurora_ax_submit_frames(blockFrames)){
      Log.error("AX block submission rejected: frames={} queued_bytes={} effects={} sdl_error={}",blockFrames,SDL_GetAudioStreamQueued(stream),has_aux(),SDL_GetError());serviceFailed=true;break;
    }
    if(!schedule.commit()){Log.error("AX schedule commit overflow");serviceFailed=true;break;}
    ++lastServiceBlocks;
  }
  lastServiceElapsedNs=SDL_GetTicksNS()-now;
  ++serviceCalls;serviceBlocks+=lastServiceBlocks;serviceElapsedNs+=lastServiceElapsedNs;
  maxServiceElapsedNs=std::max(maxServiceElapsedNs,lastServiceElapsedNs);
  servicing=false;return !serviceFailed;
}
void AISetDSPSampleRate(u32 rate){
  AURORA_ASSERT(stream && pool.on_owner() && !servicing && !renderingEffects && !outputLayout.pending && rate<=AI_SAMPLERATE_48KHZ,"AI DSP rate requires live idle audio owner");
  const uint32_t hz=rate==AI_SAMPLERATE_32KHZ?32000:48000;if(hz==sourceRate)return;
  AURORA_ASSERT(!has_aux(),"AI DSP rate change requires unregistered 32kHz effects");
  AURORA_ASSERT(SDL_GetAudioStreamQueued(stream)==0,"AI DSP rate change requires drained queue");
  for(unsigned i=0;i<AX_MAX_VOICES;i++)AURORA_ASSERT(!pool.used[i] || !pool.voices[i].pb.state,"AI DSP rate change requires stopped voices");
  SDL_AudioSpec spec{SDL_AUDIO_S16,static_cast<int>(outputLayout.channels),static_cast<int>(hz)};
  AURORA_ASSERT(SDL_SetAudioStreamFormat(stream,&spec,nullptr),"AI DSP stream rate change failed");
  SDL_AudioSpec actual;AURORA_ASSERT(SDL_GetAudioStreamFormat(stream,&actual,nullptr) && actual.freq==static_cast<int>(hz) && actual.channels==static_cast<int>(outputLayout.channels),"AI DSP stream format did not change");
  sourceRate=hz;mixers={};generations={};AURORA_ASSERT(schedule.initialize(SDL_GetTicksNS()),"AI DSP rate clock reset failed");
}
u32 AIGetDSPSampleRate(){return sourceRate==32000?AI_SAMPLERATE_32KHZ:AI_SAMPLERATE_48KHZ;}
void AIInit(u8* stack){
  // Native callbacks execute on the installed owner; a console callback stack
  // cannot be used as a Windows stack. The original game supplies nullptr.
  AURORA_ASSERT(!stack,"AI alternate callback stacks are unsupported on native hosts");
  AXInit();
  AURORA_ASSERT(pool.on_owner() && !servicing && !renderingEffects,"AIInit requires idle audio owner");
  if(aiInitialized.load())return;
  AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);
  streamLeft=0;streamRight=0;
  // Success means a real resumed playback device exists. Disc-stream decoding,
  // DMA and stream-trigger callbacks are separate unfinished runtime systems.
  aiInitialized=true;
}
BOOL AICheckInit(){return aiInitialized.load()?TRUE:FALSE;}
void AIReset(){
  AURORA_ASSERT(stream && pool.on_owner() && !servicing && !renderingEffects,"AIReset requires idle audio owner");
  aiInitialized=false;
}
void AISetStreamVolLeft(u8 volume){streamLeft.store(volume);}
void AISetStreamVolRight(u8 volume){streamRight.store(volume);}
u8 AIGetStreamVolLeft(){return streamLeft.load();}
u8 AIGetStreamVolRight(){return streamRight.load();}
uint32_t aurora_ax_source_rate(){return stream && pool.on_owner()?sourceRate.load():0;}
int aurora_ai_apply_stream_volume(const int16_t* input,int16_t* output,uint32_t frames){
  if(!stream || !pool.on_owner() || !input || !output || !frames || frames>240)return 0;
  std::array<int16_t,480> scaled;const auto l=streamLeft.load(),r=streamRight.load();
  for(unsigned i=0;i<frames;i++){scaled[2*i]=aurora::audio::stream_gain(input[2*i],l);scaled[2*i+1]=aurora::audio::stream_gain(input[2*i+1],r);}
  std::copy_n(scaled.data(),frames*2,output);return 1;
}
}
