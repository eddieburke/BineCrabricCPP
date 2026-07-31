#pragma once
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::util {
// fpsLimit 0 = Max → Unlimited; 1/2 = Balanced/PowerSaver → VSync (Adaptive if smoothFps).
[[nodiscard]] inline gl::SwapPacing swapPacingFromOptions(int fpsLimit, bool smoothFps) {
 if(fpsLimit == 0) {
  return gl::SwapPacing::Unlimited;
 }
 return smoothFps ? gl::SwapPacing::Adaptive : gl::SwapPacing::VSync;
}
} // namespace net::minecraft::client::util
