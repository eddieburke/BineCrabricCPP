#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
namespace net::minecraft::client::util {
class Timer {
public:
  explicit Timer(float ticksPerSecond)
      : tps(ticksPerSecond), prevTickTime_(currentTimeMillis()), prevCorrectionTime_(nanoTimeMillis()) {
  }
  void advance() {
    const std::int64_t nowMillis = currentTimeMillis();
    const std::int64_t tickElapsed = nowMillis - prevTickTime_;
    const std::int64_t nowNanoMillis = nanoTimeMillis();
    const double nowSeconds = static_cast<double>(nowNanoMillis) / 1000.0;
    if(tickElapsed > 1000 || tickElapsed < 0) {
      timeSec_ = nowSeconds;
    } else {
      cooldownTickTime_ += tickElapsed;
      if(cooldownTickTime_ > 1000) {
        const std::int64_t correctionElapsed = nowNanoMillis - prevCorrectionTime_;
        if(correctionElapsed != 0) {
          const double measuredCorrection =
              static_cast<double>(cooldownTickTime_) / static_cast<double>(correctionElapsed);
          tickTimeCorrection_ += (measuredCorrection - tickTimeCorrection_) * 0.2;
        }
        prevCorrectionTime_ = nowNanoMillis;
        cooldownTickTime_ = 0;
      }
      if(cooldownTickTime_ < 0) {
        prevCorrectionTime_ = nowNanoMillis;
      }
    }
    prevTickTime_ = nowMillis;
    double frameDelta = (nowSeconds - timeSec_) * tickTimeCorrection_;
    timeSec_ = nowSeconds;
    frameDelta = std::clamp(frameDelta, 0.0, 1.0);
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
  [[nodiscard]] static std::int64_t currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
  [[nodiscard]] static std::int64_t nanoTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }
  double timeSec_ = 0.0;
  std::int64_t prevTickTime_ = 0;
  std::int64_t prevCorrectionTime_ = 0;
  std::int64_t cooldownTickTime_ = 0;
  double tickTimeCorrection_ = 1.0;
};
} // namespace net::minecraft::client::util
