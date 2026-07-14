#pragma once
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
namespace net::minecraft::item {
class BlockItem : public Item {
public:
  explicit BlockItem(int rawId) : Item(rawId), itemId_(rawId + 256) {
    Block* block =
        itemId_ >= 0 && itemId_ < Block::BLOCK_COUNT ? Block::BLOCKS[static_cast<std::size_t>(itemId_)] : nullptr;
    if(block != nullptr) {
      setTextureId(block->getTexture(2));
    }
  }
  bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override {
    if(world == nullptr || itemId_ <= 0 || itemId_ >= Block::BLOCK_COUNT) {
      return false;
    }
    int tx = x;
    int ty = y;
    int tz = z;
    int clickedSide = side;
    detail::offsetPlacementPos(world, tx, ty, tz, clickedSide);
    Block* block = Block::BLOCKS[static_cast<std::size_t>(itemId_)];
    if(ty == 127 && block != nullptr && block->material.isSolid()) {
      return false;
    }
    return detail::placeBlockItem(stack,
                                  user,
                                  world,
                                  itemId_,
                                  getPlacementMetadata(stack != nullptr ? stack->getDamage() : 0),
                                  x,
                                  y,
                                  z,
                                  side,
                                  false);
  }
  [[nodiscard]] std::string getTranslationKey(const ItemStack* /*stack*/) const override {
    Block* block =
        itemId_ >= 0 && itemId_ < Block::BLOCK_COUNT ? Block::BLOCKS[static_cast<std::size_t>(itemId_)] : nullptr;
    return block != nullptr ? block->getTranslationKey() : Item::getTranslationKey();
  }

protected:
  int itemId_ = 0;
};
} // namespace net::minecraft::item
