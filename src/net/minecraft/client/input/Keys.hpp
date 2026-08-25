#pragma once
#include <string>
namespace net::minecraft::client::input::keys {
inline constexpr int kEscape = 1;
inline constexpr int kHotbar1 = 2;
inline constexpr int kHotbar9 = 10;
inline constexpr int kBackspace = 14;
inline constexpr int kLCtrl = 29;
inline constexpr int kC = 46;
inline constexpr int kS = 31;
inline constexpr int kV = 47;
inline constexpr int kEnter = 28;
inline constexpr int kLShift = 42;
inline constexpr int kRShift = 54;
inline constexpr int kLAlt = 56;
inline constexpr int kRAlt = 184;
inline constexpr int kRCtrl = 157;
inline constexpr int kF1 = 59;
inline constexpr int kF2 = 60;
inline constexpr int kF3 = 61;
inline constexpr int kF4 = 62;
inline constexpr int kF5 = 63;
inline constexpr int kF6 = 64;
inline constexpr int kF7 = 65;
inline constexpr int kF8 = 66;
inline constexpr int kF11 = 87;
inline constexpr int kUp = 200;
inline constexpr int kPageUp = 201;
inline constexpr int kDown = 208;
inline constexpr int kPageDown = 209;
inline int hotbarSlotFromKey(int key) noexcept {
 if(key >= kHotbar1 && key <= kHotbar9) {
  return key - kHotbar1;
 }
 return -1;
}
}
namespace net::minecraft::client::input {
[[nodiscard]] std::string keyDisplayName(int key);
}
