#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace net::minecraft::world::light {

struct SunLight {
  float directionX = 0.0f;
  float directionY = 1.0f;
  float directionZ = 0.0f;
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
  float intensity = 1.0f;
};

class UnifiedLightRegistry {
 public:
  static constexpr std::size_t kBlockProfileCount = 256;

  static void setBlockEmission(int blockId, int emission) noexcept;
  [[nodiscard]] static int blockEmission(int blockId) noexcept;
  static void setBlockLightColor(int blockId, float red, float green, float blue) noexcept;
  static void blockLightColor(int blockId, float& red, float& green, float& blue) noexcept;

  void setSun(const SunLight& sun) noexcept { sun_ = sun; }
  [[nodiscard]] const SunLight& sun() const noexcept { return sun_; }

 private:
  inline static std::array<std::atomic<std::uint8_t>, kBlockProfileCount> blockEmission_{};
  inline static std::array<std::atomic<std::uint32_t>, kBlockProfileCount> blockColor_{};
  SunLight sun_{};
};

} // namespace net::minecraft::world::light
