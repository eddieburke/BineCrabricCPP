#pragma once
#include <algorithm>

namespace net::minecraft {
// Weather state extracted from World: rain/thunder gradients, lightning
// counters, and the master enable flag. Tick *control flow* stays in
// World/ClientWorld (server and client cycles differ); this module owns the
// state and the shared gradient math so World no longer exposes raw fields.
class WorldWeather {
   public:
    // Screen-flash counter: LightningEntity sets it to 2, Minecraft's render
    // loop decrements it, sky/cloud color blend toward white while > 0.
    int lightningTicks = 0;
    // Set to 2 when a strike spawns; decremented each weather tick.
    int ticksSinceLightning = 0;

    [[nodiscard]] bool enabled() const noexcept {
        return enabled_;
    }

    void setEnabled(bool value) noexcept {
        enabled_ = value;
    }

    [[nodiscard]] float rainGradient(float partialTicks) const noexcept {
        if (!enabled_) {
            return 0.0f;
        }
        return rainGradientPrev_ + (rainGradient_ - rainGradientPrev_) * partialTicks;
    }

    [[nodiscard]] float thunderGradient(float partialTicks) const noexcept {
        if (!enabled_) {
            return 0.0f;
        }
        return (thunderGradientPrev_ + (thunderGradient_ - thunderGradientPrev_) * partialTicks) *
               rainGradient(partialTicks);
    }

    void setRainGradient(float value) noexcept {
        rainGradientPrev_ = rainGradient_ = value;
    }

    void resetGradients() noexcept {
        rainGradient_ = rainGradientPrev_ = 0.0f;
        thunderGradient_ = thunderGradientPrev_ = 0.0f;
    }

    // World load with active weather: snap gradients to fully formed.
    void beginActiveWeather(bool thundering) noexcept {
        rainGradient_ = 1.0f;
        if (thundering) {
            thunderGradient_ = 1.0f;
        }
    }

    // One weather tick: gradients chase the raining/thundering flags by 0.01.
    void tickGradients(bool raining, bool thundering) noexcept {
        rainGradientPrev_ = rainGradient_;
        rainGradient_ = static_cast<float>(static_cast<double>(rainGradient_) + (raining ? 0.01 : -0.01));
        rainGradient_ = std::clamp(rainGradient_, 0.0f, 1.0f);
        thunderGradientPrev_ = thunderGradient_;
        thunderGradient_ = static_cast<float>(static_cast<double>(thunderGradient_) + (thundering ? 0.01 : -0.01));
        thunderGradient_ = std::clamp(thunderGradient_, 0.0f, 1.0f);
    }

    // Weather disabled: gradients only decay toward zero.
    void decayGradients() noexcept {
        rainGradientPrev_ = rainGradient_;
        if (rainGradient_ > 0.0f) {
            rainGradient_ = static_cast<float>(static_cast<double>(rainGradient_) - 0.01);
        }
        thunderGradientPrev_ = thunderGradient_;
        if (thunderGradient_ > 0.0f) {
            thunderGradient_ = static_cast<float>(static_cast<double>(thunderGradient_) - 0.01);
        }
        rainGradient_ = std::clamp(rainGradient_, 0.0f, 1.0f);
        thunderGradient_ = std::clamp(thunderGradient_, 0.0f, 1.0f);
    }

   private:
    float rainGradientPrev_ = 0.0f;
    float rainGradient_ = 0.0f;
    float thunderGradientPrev_ = 0.0f;
    float thunderGradient_ = 0.0f;
    bool enabled_ = true;
};
}  // namespace net::minecraft
