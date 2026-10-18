#pragma once
#include <bitset>
#include <cstddef>
#include "keyboard_state.hpp"
namespace aurora::input {
// Owner-thread only. Preserve a down/up pair drained between PAD samples for
// one keyboard button/axis sample. Held keys still come from SDL; repeats
// create no new edge. This does not reconstruct historical input timing.
template<size_t Count>
inline bool keyboard_sample_pressed(const bool* states,int count,int scancode,
                                    const std::bitset<Count>& presses) noexcept {
  return keyboard_key_pressed(states,count,scancode) ||
      (scancode>=0 && size_t(scancode)<Count && presses[size_t(scancode)]);
}
template<size_t Count> class KeyboardPressLatch {
  std::bitset<Count> pending;
public:
  void press(int scancode,bool repeat=false) noexcept {
    if(!repeat && scancode>=0 && size_t(scancode)<Count)pending.set(size_t(scancode));
  }
  std::bitset<Count> take() noexcept { auto result=pending;pending.reset();return result; }
  void clear() noexcept { pending.reset(); }
};
} // namespace aurora::input
