#pragma once
namespace aurora::input {
// SDL owns this array; mappings must never index outside its reported length.
inline bool keyboard_key_pressed(const bool* states,int count,int scancode) noexcept {
  return states && scancode>=0 && scancode<count && states[scancode];
}
}
