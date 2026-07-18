#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::render::terrain {
struct TerrainMeshResult {
  std::vector<TessellatorVertex> vertices{};
  float minY = 0.0f;
  float maxY = 0.0f;
};
void buildTerrainMesh(std::span<const std::uint8_t> snapshot,
                      int level,
                      const std::array<std::uint32_t, 256>& topColors,
                      const std::array<std::uint32_t, 256>& sideColors,
                      TerrainMeshResult& result);
}
