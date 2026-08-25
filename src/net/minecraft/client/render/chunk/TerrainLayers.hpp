#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
namespace net::minecraft::client::render::chunk {
// Mesh / draw indices for world terrain.
// https://shaders.properties/current/reference/miscellaneous/block_properties/
namespace terrain_layer {
inline constexpr int Solid = 0;
inline constexpr int Cutout = 1;
inline constexpr int Translucent = 2;
inline constexpr int CutoutInterior = 3;
inline constexpr int Count = 4;
} // namespace terrain_layer
[[nodiscard]] inline int resolveTerrainMeshLayer(const net::minecraft::block::Block& block, int blockId,
                                                 const std::unordered_map<int, int>& packOverrides) {
 const auto found = packOverrides.find(blockId);
 if(found != packOverrides.end()) {
  return found->second;
 }
 if(blockId == 2 || blockId == 18) {
  return terrain_layer::Cutout;
 }
 if(block.getRenderLayer() == 1) {
  return terrain_layer::Translucent;
 }
 using namespace block::BlockRenderType;
 switch(block.getRenderType()) {
 case CROSS:
 case CROP:
 case TORCH:
 case FIRE:
 case REDSTONE_DUST:
 case LADDER:
 case RAIL:
  return terrain_layer::Cutout;
 default:
  break;
 }
 if(!block.isOpaque()) {
  return terrain_layer::Cutout;
 }
 return terrain_layer::Solid;
}
// The layer decision baked per block id: resolveTerrainMeshLayer is a hash lookup and
// up to three virtual calls, and the mesher asked it once per block per layer pass for
// an answer that depends on nothing but the id. Unregistered ids keep layer 0; every
// caller has already null-checked Block::BLOCKS before it reads this.
using TerrainLayerTable = std::array<std::uint8_t, net::minecraft::block::Block::BLOCK_COUNT>;
[[nodiscard]] inline TerrainLayerTable buildTerrainLayerTable(const std::unordered_map<int, int>& packOverrides) {
 TerrainLayerTable table{};
 for(int blockId = 0; blockId < net::minecraft::block::Block::BLOCK_COUNT; ++blockId) {
  net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block != nullptr) {
   table[static_cast<std::size_t>(blockId)] =
       static_cast<std::uint8_t>(resolveTerrainMeshLayer(*block, blockId, packOverrides));
  }
 }
 return table;
}
} // namespace net::minecraft::client::render::chunk
