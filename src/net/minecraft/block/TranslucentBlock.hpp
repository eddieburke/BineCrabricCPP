#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::block {
class TranslucentBlock : public Block {
 public:
 bool transparent = false;

 public:
 TranslucentBlock(int id, int textureId, Material& material, bool transparent) : Block(id, textureId, material) {
  this->transparent = transparent;
 }
 [[nodiscard]] bool isOpaque() const override {
  return false;
 }
 [[nodiscard]] bool isSideVisibleForBounds(const BlockView* blockView,
                                           int x,
                                           int y,
                                           int z,
                                           int side,
                                           const net::minecraft::Box& bounds) const override {
  if(blockView != nullptr && !transparent && blockView->getBlockId(x, y, z) == id) {
   return false;
  }
  return Block::isSideVisibleForBounds(blockView, x, y, z, side, bounds);
 }
};
} // namespace net::minecraft::block