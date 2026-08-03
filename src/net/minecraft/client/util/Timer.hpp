#pragma once
#include <algorithm>
#include <chrono>
namespace net::minecraft::client::util {
class Timer {
 public:
  explicit Timer(float ticksPerSecond)
      : tps(ticksPerSecond), lastTime_(std::chrono::steady_clock::now()) {}

  void advance() {
   const auto now = std::chrono::steady_clock::now();
   const auto elapsed = std::chrono::duration<double>(now - lastTime_).count();
   lastTime_ = now;

   const double frameDelta = std::clamp(elapsed, 0.0, 1.0);
   tickDelta += static_cast<float>(frameDelta * static_cast<double>(tpsScale) * static_cast<double>(tps));
   ticksThisFrame = static_cast<int>(tickDelta);
   tickDelta -= static_cast<float>(ticksThisFrame);
   if(ticksThisFrame > 10) {
    ticksThisFrame = 10;
   }
   partialTick = tickDelta;
  }

  float tps = 20.0f;
  int ticksThisFrame = 0;
  float partialTick = 0.0f;
  float tpsScale = 1.0f;
  float tickDelta = 0.0f;

 private:
  std::chrono::steady_clock::time_point lastTime_;
};
} // namespace net::minecraft::client::util
