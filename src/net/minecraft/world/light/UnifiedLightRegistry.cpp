#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include <algorithm>
namespace net::minecraft::world::light {
void UnifiedLightRegistry::setBlockEmission(int blockId, int emission) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  blockEmission_[static_cast<std::size_t>(blockId)].store(static_cast<std::uint8_t>(std::clamp(emission, 0, 15)),
                                                          std::memory_order_relaxed);
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
  const auto r = static_cast<std::uint32_t>(std::clamp(red, 0.0f, 1.0f) * 255.0f);
  const auto g = static_cast<std::uint32_t>(std::clamp(green, 0.0f, 1.0f) * 255.0f);
  const auto b = static_cast<std::uint32_t>(std::clamp(blue, 0.0f, 1.0f) * 255.0f);
  const std::uint32_t packed = (r << 16) | (g << 8) | b;
  blockColor_[static_cast<std::size_t>(blockId)].store(packed, std::memory_order_relaxed);
 }
}
void UnifiedLightRegistry::blockLightColor(int blockId, float& red, float& green, float& blue) noexcept {
 if(blockId >= 0 && static_cast<std::size_t>(blockId) < kBlockProfileCount) {
  const std::uint32_t packed = blockColor_[static_cast<std::size_t>(blockId)].load(std::memory_order_relaxed);
  red = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
  green = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
  blue = static_cast<float>(packed & 0xFF) / 255.0f;
  return;
 }
 red = 1.0f;
 green = 1.0f;
 blue = 1.0f;
}
} // namespace net::minecraft::world::light
