#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include <algorithm>
namespace net::minecraft::world::light {
void UnifiedLightRegistry::setBlockEmission(int blockId, int emission) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  blockEmission_[static_cast<std::size_t>(blockId)].store(
      static_cast<std::uint8_t>(std::clamp(emission, 0, 15)), std::memory_order_relaxed);
 }
}
int UnifiedLightRegistry::blockEmission(int blockId) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  return blockEmission_[static_cast<std::size_t>(blockId)].load(std::memory_order_relaxed);
 }
 return 0;
}
void UnifiedLightRegistry::setBlockLightColor(int blockId, float red, float green, float blue) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  const float rc = std::clamp(red, 0.0f, 1.0f);
  const float gc = std::clamp(green, 0.0f, 1.0f);
  const float bc = std::clamp(blue, 0.0f, 1.0f);
  const auto r = static_cast<std::uint32_t>(rc * 255.0f);
  const auto g = static_cast<std::uint32_t>(gc * 255.0f);
  const auto b = static_cast<std::uint32_t>(bc * 255.0f);
  blockColor_[static_cast<std::size_t>(blockId)].store((r << 16) | (g << 8) | b, std::memory_order_relaxed);
 }
}
void UnifiedLightRegistry::blockLightColor(int blockId, float& red, float& green, float& blue) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  const std::uint32_t packed = blockColor_[static_cast<std::size_t>(blockId)].load(std::memory_order_relaxed);
  if(packed != 0) {
   red = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
   green = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
   blue = static_cast<float>(packed & 0xFF) / 255.0f;
   return;
  }
 }
 red = 1.0f;
 green = 1.0f;
 blue = 1.0f;
}
void UnifiedLightRegistry::blockEmissionRGB(int blockId, float& r, float& g, float& b) noexcept {
 blockLightColor(blockId, r, g, b);
 const float level = static_cast<float>(blockEmission(blockId)) / 15.0f;
 r *= level;
 g *= level;
 b *= level;
}
} // namespace net::minecraft::world::light
