#include <dolphin/os.h>
#include <atomic>
#include <cassert>
#include <cstdint>

namespace {
// Session preference only. No SRAM or memory-card persistence.
std::atomic<u32> soundMode{OS_SOUND_MODE_STEREO};
std::atomic<u32> progressiveMode{0};
void check_range(void* address, u32 bytes) {
  const auto start = reinterpret_cast<uintptr_t>(address);
  assert(!bytes || (address && start <= UINTPTR_MAX - bytes));
}
}

extern "C" {
u32 OSGetProgressiveMode(){return progressiveMode.load(std::memory_order_acquire);}
void OSSetProgressiveMode(u32 mode){
  assert(mode<=1);
  if(mode<=1)progressiveMode.store(mode,std::memory_order_release);
}
u32 OSGetSoundMode() { return soundMode.load(std::memory_order_acquire); }
void OSSetSoundMode(u32 mode) {
  assert(mode == OS_SOUND_MODE_MONO || mode == OS_SOUND_MODE_STEREO);
  if (mode <= OS_SOUND_MODE_STEREO) soundMode.store(mode, std::memory_order_release);
}

// Native samples use coherent CPU memory, not the GameCube DSP DMA cache.
// These are CPU ordering points only; GPU uploads and audio device submission
// must use their own explicit ownership/completion protocols.
void DCFlushRange(void* address,u32 bytes) {
  check_range(address,bytes);
  std::atomic_thread_fence(std::memory_order_seq_cst);
}
void DCStoreRange(void* address, u32 bytes) {
  check_range(address, bytes);
  std::atomic_thread_fence(std::memory_order_seq_cst);
}
void DCInvalidateRange(void* address, u32 bytes) {
  check_range(address, bytes);
  std::atomic_thread_fence(std::memory_order_seq_cst);
}
}
