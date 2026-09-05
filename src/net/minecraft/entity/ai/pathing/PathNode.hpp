#pragma once
#include <bit>
#include <cstdint>
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::entity::ai::pathing {
class PathNode {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 PathNode() = default;
 PathNode(int xIn, int yIn, int zIn) : x(xIn), y(yIn), z(zIn), hashCode_(hash(xIn, yIn, zIn)) {
 }
 [[nodiscard]] static constexpr int hash(int x, int y, int z) noexcept {
  // see third_party/mcp/net/minecraft/src/PathPoint.java:24
  const std::uint32_t packed = (static_cast<std::uint32_t>(y) & 0xFFu) |
                               ((static_cast<std::uint32_t>(x) & 0x7FFFu) << 8) |
                               ((static_cast<std::uint32_t>(z) & 0x7FFFu) << 24) | (x < 0 ? 0x80000000u : 0u) |
                               (z < 0 ? 0x8000u : 0u);
  return std::bit_cast<int>(packed);
 }
 [[nodiscard]] static constexpr std::uint64_t keyOf(int x, int y, int z) noexcept {
  return ((static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) + 0x2000000u) << 34) |
         ((static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) & 0xFFu) << 26) |
         ((static_cast<std::uint64_t>(static_cast<std::uint32_t>(z)) + 0x2000000u) & 0x3FFFFFFu);
 }
 [[nodiscard]] float getDistance(const PathNode& node) const {
  const float dx = static_cast<float>(node.x - x);
  const float dy = static_cast<float>(node.y - y);
  const float dz = static_cast<float>(node.z - z);
  return MathHelper::sqrt(dx * dx + dy * dy + dz * dz);
 }
 [[nodiscard]] bool equals(const PathNode& other) const {
  return hashCode_ == other.hashCode_ && x == other.x && y == other.y && z == other.z;
 }
 [[nodiscard]] bool isInHeap() const {
  return heapIndex >= 0;
 }
 int heapIndex = -1;
 float penalizedPathLength = 0.0f;
 float distanceToNearestTarget = 0.0f;
 float heapWeight = 0.0f;
 PathNode* previous = nullptr;
 bool visited = false;

 private:
 int hashCode_ = 0;
};
} // namespace net::minecraft::entity::ai::pathing
