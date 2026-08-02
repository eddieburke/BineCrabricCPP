#pragma once
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
inline constexpr int Count = 3;
} // namespace terrain_layer

// Legacy Block::getRenderLayer() is only opaque(0) / translucent(1). Pack
// block.properties layer.* overrides use the mesh indices above. Without an
// override, non-opaque / alpha-tested shapes go to cutout so packs that ship
// gbuffers_terrain_solid without discard do not write transparent texels.
[[nodiscard]] inline int resolveTerrainMeshLayer(const net::minecraft::block::Block& block, int blockId,
                                                 const std::unordered_map<int, int>& packOverrides) {
 const auto found = packOverrides.find(blockId);
 if(found != packOverrides.end()) {
  return found->second;
 }
 if(blockId == 18) {
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

} // namespace net::minecraft::client::render::chunk
