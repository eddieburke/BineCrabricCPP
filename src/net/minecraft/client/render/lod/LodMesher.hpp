#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/lod/LodData.hpp"
namespace net::minecraft::client::render::lod {
inline constexpr int kRegionChunks = 8;
inline constexpr int kRegionBlocks = kRegionChunks * 16;
inline constexpr int kGroupRegions = 16;
inline constexpr int kGroupBlocks = kGroupRegions * kRegionBlocks;
inline constexpr int kMaxLodLevel = 4;
struct LodMeshJob {
  int regionX = 0;
  int regionZ = 0;
  int level = 0;
  long long stamp = 0;
  float originX = 0.0f;
  float originZ = 0.0f;
  std::array<const std::array<std::uint32_t, 256>*, 2> colors{nullptr, nullptr};
  std::vector<LodChunk> chunks;
  std::vector<std::uint8_t> present;
  std::vector<TessellatorVertex> vertices;
  float minY = 0.0f;
  float maxY = 0.0f;
  bool failed = false;
  static void build(LodMeshJob& job);
};
} // namespace net::minecraft::client::render::lod
