#include <dolphin/ar.h>
#include "../internal.hpp"
#include "dolphin/os.h"
#include <mutex>
#include <array>
#include <SDL3/SDL_thread.h>
#include "../audio/sample_decode.hpp"

static aurora::Module Log("aurora::ar");

static u32 AR_StackPointer;
static u32* AR_BlockLength;
static u32 AR_FreeBlocks;
static BOOL AR_init_flag;

#define ARAM_STACK_START 0x4000

// ARAM emulation: allocate a large buffer to simulate the GameCube's Auxiliary RAM.
// ARAM "addresses" are offsets into this buffer. On GameCube, ARAM is 16 MB starting
// at a base address returned by ARInit. We emulate this by malloc'ing a buffer
// and using a simple bump allocator (matching ARAlloc behavior on real hardware).
static u8* sAramBuffer = nullptr;
static std::mutex aramMutex;
static u32* stackBase;
struct ArqCompletion { ARQRequest* request=nullptr; ARQCallback callback=nullptr; };
static std::array<ArqCompletion,256> arqCompletions;
static unsigned arqHead=0,arqCount=0;
static SDL_ThreadID arqOwner=0;
static int (*arqGate)(void)=nullptr;
static bool arqServicing=false;
int aurora_arq_set_callback_gate(int (*provider)(void)) {
  if(!provider || arqGate || arqCount || !provider())return 0;
  arqOwner=SDL_GetCurrentThreadID();arqGate=provider;return 1;
}
int aurora_arq_service(void) {
  if(!arqGate || SDL_GetCurrentThreadID()!=arqOwner || arqServicing)return 0;
  arqServicing=true;
  for(unsigned count=0;count<64 && arqCount && arqGate();++count){
    auto completion=arqCompletions[arqHead];
    arqCompletions[arqHead]={};arqHead=(arqHead+1)%arqCompletions.size();--arqCount;
    completion.callback(completion.request);
  }
  arqServicing=false;return 1;
}
static bool aramRange(u32 offset,u32 length){return sAramBuffer && offset<=aurora::g_config.mem2Size && length<=aurora::g_config.mem2Size-offset;}

u32 ARAlloc(u32 length) {
  u32 tmp;

  std::lock_guard lock(aramMutex);
  AURORA_ASSERT(AR_init_flag && length && !(length&31) &&
      AR_StackPointer<=aurora::g_config.mem2Size &&
      length<=aurora::g_config.mem2Size-AR_StackPointer && AR_FreeBlocks,
      "ARAlloc invalid size, exhausted storage or uninitialized ARAM");
  tmp = AR_StackPointer;
  AR_StackPointer += length;
  *AR_BlockLength = length;
  AR_BlockLength += 1;
  AR_FreeBlocks -= 1;
  return tmp;
}

u32 ARFree(u32* length) {
  std::lock_guard lock(aramMutex);
  AURORA_ASSERT(AR_init_flag && AR_BlockLength>stackBase,"ARFree has no allocation to release");
  AR_BlockLength -= 1;
  if (length) {
    *length = *AR_BlockLength;
  }
  AR_StackPointer -= *AR_BlockLength;
  AR_FreeBlocks += 1;
  return AR_StackPointer;
}

BOOL ARCheckInit(void) { return AR_init_flag; }

u32 ARInit(u32* stack_index_addr, u32 num_entries) {
  std::lock_guard lock(aramMutex);
  if (aurora::g_config.mem2Size == 0) {
    Log.warn("ARInit called but no mem2Size specified in AuroraConfig. ARAM will not be available!");
    return 0;
  }

  if (AR_init_flag == TRUE) {
    return ARAM_STACK_START;
  }

  AURORA_ASSERT(stack_index_addr && num_entries && aurora::g_config.mem2Size>ARAM_STACK_START,"ARInit requires stack and available storage");
  sAramBuffer = (u8*)calloc(1, aurora::g_config.mem2Size);
  if (sAramBuffer) {
    Log.debug("Initialized 0x{:X} bytes of ARAM!", aurora::g_config.mem2Size);
  } else {
    Log.fatal("Failed to allocate ARAM!");
  }

  AR_StackPointer = ARAM_STACK_START;
  AR_FreeBlocks = num_entries;
  AR_BlockLength = stack_index_addr;
  stackBase=stack_index_addr;

  AR_init_flag = TRUE;
  return AR_StackPointer;
}

u32 ARGetSize(void) { return aurora::g_config.mem2Size; }

#if !defined(_MSC_VER)
#pragma mark ARQ
#endif
void ARQPostRequest(ARQRequest* request, u32 owner, u32 type, u32 priority, uintptr_t source, uintptr_t dest,
                    u32 length, ARQCallback callback) {
  if(callback || arqGate){
    AURORA_ASSERT(arqGate && SDL_GetCurrentThreadID()==arqOwner,
                  "ARQ callback posting requires installed native owner gate");
    for(unsigned i=0;i<arqCount;++i)
      AURORA_ASSERT(arqCompletions[(arqHead+i)%arqCompletions.size()].request!=request,
                    "ARQ request storage reused before completion");
    AURORA_ASSERT(!callback || arqCount<arqCompletions.size(),"ARQ completion queue full; transfer refused");
  }
  {
    std::lock_guard lock(aramMutex);
    const bool toAram=type==ARAM_DIR_MRAM_TO_ARAM;
    const uintptr_t offset=toAram?dest:source;
    const uintptr_t host=toAram?source:dest;
    AURORA_ASSERT(AR_init_flag && request && type<=ARAM_DIR_ARAM_TO_MRAM &&
       offset<=UINT32_MAX && aramRange(static_cast<u32>(offset),length) &&
       (!length || (host && host<=UINTPTR_MAX-length)),
       "ARQ transfer invalid or outside available storage; callback withheld");
    request->owner=owner;request->type=type;request->priority=priority;
    request->source=static_cast<u32>(source);request->dest=static_cast<u32>(dest);
    request->length=length;request->callback=callback;
    if(length){
      if(toAram)std::memcpy(sAramBuffer+offset,reinterpret_cast<void*>(host),length);
      else std::memcpy(reinterpret_cast<void*>(host),sAramBuffer+offset,length);
    }
  }

  if(callback){
    arqCompletions[(arqHead+arqCount)%arqCompletions.size()]={request,callback};++arqCount;
  }
}

void ARQInit() {
  // Nothing to do on PC - ARAM is initialized in ARInit
}

void* ARGetStorageAddress() {
  return sAramBuffer;
}

BOOL ARReadStorage(u32 offset,void* output,u32 length){
  std::lock_guard lock(aramMutex);
  if(!AR_init_flag || !aramRange(offset,length) ||
      (length && (!output || reinterpret_cast<uintptr_t>(output)>UINTPTR_MAX-length)))return FALSE;
  if(length)std::memcpy(output,sAramBuffer+offset,length);
  return TRUE;
}

BOOL ARDecodeADPCMBlock(u32 offset,const s16* coefficients,s16* history,s16* output,u32 output_samples){
  if(!coefficients || !history || !output || output_samples<14)return FALSE;
  std::array<uint8_t,8> bytes;
  if(!ARReadStorage(offset,bytes.data(),bytes.size()))return FALSE;
  std::array<int16_t,16> table;std::copy_n(coefficients,16,table.begin());
  aurora::audio::ADPCMHistory state{history[0],history[1]};
  if(!aurora::audio::decode_adpcm_block(bytes,table,state,std::span<int16_t>(output,output_samples)))return FALSE;
  history[0]=state.recent;history[1]=state.older;return TRUE;
}
