#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::render::chunk {
[[nodiscard]] int terrainLodLevel(int distanceChunks, int previousLevel, bool enabled = true) noexcept;
[[nodiscard]] bool terrainColumnInRange(int x, int z, int cameraX, int cameraZ, int radius) noexcept;
std::size_t simplifyTerrainMesh(TessellatorMesh& mesh, int level, std::span<const std::uint8_t> eligible);
}
