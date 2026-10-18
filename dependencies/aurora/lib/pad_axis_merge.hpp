#pragma once
#include <algorithm>
#include <cstdlib>
namespace aurora::input {
// Inputs are bounded PAD values; ties retain the primary device.
inline int dominant_pad_axis(int primary,int secondary,int low,int high) noexcept {
 return std::clamp(std::abs(secondary)>std::abs(primary)?secondary:primary,low,high);
}
}
