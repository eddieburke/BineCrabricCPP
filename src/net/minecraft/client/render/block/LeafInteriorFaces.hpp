#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

// Interior faces of a leaf blob.
//
// Fancy leaves are alpha-tested, so looking into a canopy you see straight past
// the outer shell - and with every leaf<->leaf face culled (the vanilla rule in
// TransparentBlock) there is nothing behind it to look at. Emitting both sides
// of each boundary is not the fix either: the two quads are exactly coplanar, so
// any pass that runs without back-face culling (the shadow map, or a pack that
// sets backFace.cutout=false) rasterises both and they Z-fight over the whole
// plane.
//
// So each boundary emits exactly one quad. The tie-break is the face direction:
// only the -Y / -Z / -X faces are emitted, which means the boundary quad always
// comes from the block on the positive side and each shared plane is covered
// once. Nothing is ever coincident, in any pass, at any cull state. The single
// quad is drawn double-sided (terrain_layer::CutoutInterior renders with cull
// off) so it is there whichever side you approach it from.
//
// Note this hands the pack a back face for half the viewing angles: its normal
// points away from the viewer and its tangent handedness is computed for the
// opposite winding, so normal mapping and any DIR_SHADING term see the wrong
// basis on those fragments. That is the accepted cost of one quad per boundary
// instead of a back-to-back pair.

// True when this block's shared faces participate at all. Fast leaves are opaque
// cubes with nothing to see through, so they never do.
[[nodiscard]] inline bool hasLeafInteriorFaces(const net::minecraft::block::Block& block,
                                               int leafInteriorFaces) noexcept {
 return leafInteriorFaces > 0 && &block.material == &net::minecraft::block::material::Material::LEAVES;
}

namespace leaf_interior_detail {
// A block is on the shell when at least one of its six neighbours is something
// other than more of the same leaf and does not block vision - i.e. there is a
// line of sight into it from outside the blob.
[[nodiscard]] inline bool touchesOpenSpace(const net::minecraft::BlockView& blockView,
                                           int blockId,
                                           int x,
                                           int y,
                                           int z) noexcept {
 for(const auto& offset : kFaceOffsets) {
  const int neighborId = blockView.getBlockId(x + offset[0], y + offset[1], z + offset[2]);
  if(neighborId == blockId) {
   continue;
  }
  if(neighborId <= 0 || neighborId >= net::minecraft::block::Block::BLOCK_COUNT) {
   return true;
  }
  if(net::minecraft::block::Block::BLOCKS_ALLOW_VISION[static_cast<std::size_t>(neighborId)]) {
   return true;
  }
 }
 return false;
}
} // namespace leaf_interior_detail

// `side` is the face being considered (0=-Y 1=+Y 2=-Z 3=+Z 4=-X 5=+X) and
// (neighborX, neighborY, neighborZ) the block it looks at, matching
// Block::isSideVisibleForBounds. The block emitting the face is therefore one
// step back along `side`.
[[nodiscard]] inline bool leafInteriorFaceVisible(const net::minecraft::BlockView* blockView,
                                                  const net::minecraft::block::Block& block,
                                                  int neighborX,
                                                  int neighborY,
                                                  int neighborZ,
                                                  int side,
                                                  int leafInteriorFaces) noexcept {
 if(blockView == nullptr || !hasLeafInteriorFaces(block, leafInteriorFaces)) {
  return false;
 }
 // Tie-break: the negative-direction faces only, so each shared plane is
 // emitted by exactly one of the two blocks that touch it.
 if(side != 0 && side != 2 && side != 4) {
  return false;
 }
 // Only a boundary against the same leaf type is an interior face; a leaf
 // against air or stone is an ordinary exterior face and the cutout layer
 // already drew it.
 if(blockView->getBlockId(neighborX, neighborY, neighborZ) != block.id) {
  return false;
 }
 if(leafInteriorFaces >= 2) {
  return true;
 }
 // Shell mode: keep the boundary only if it is reachable by sight. Deep inside
 // a solid clump both blocks are walled in by more leaves and the quad could
 // never be seen through the texture's holes, so it is not worth meshing.
 const int emitterX = side == 4 ? neighborX + 1 : neighborX;
 const int emitterY = side == 0 ? neighborY + 1 : neighborY;
 const int emitterZ = side == 2 ? neighborZ + 1 : neighborZ;
 return leaf_interior_detail::touchesOpenSpace(*blockView, block.id, emitterX, emitterY, emitterZ) ||
        leaf_interior_detail::touchesOpenSpace(*blockView, block.id, neighborX, neighborY, neighborZ);
}

} // namespace net::minecraft::client::render::block
