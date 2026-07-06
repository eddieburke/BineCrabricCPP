#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item {
class SeedsItem : public Item {
public:
  static constexpr int kRawId = 39;
  static void registerClass();
  SeedsItem(int rawId, int cropBlockId) : Item(rawId, RegistrationMode::Deferred), cropBlockId_(cropBlockId) {}
  bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side) override {
    if(stack == nullptr || world == nullptr || side != 1 || Block::FARMLAND == nullptr) {
      return false;
    }
    if(world->getBlockId(x, y, z) == Block::FARMLAND->id && world->isAir(x, y + 1, z)) {
      world->setBlock(x, y + 1, z, cropBlockId_);
      --stack->count;
      return true;
    }
    return false;
  }

private:
  int cropBlockId_ = 0;
};
} // namespace net::minecraft::item
