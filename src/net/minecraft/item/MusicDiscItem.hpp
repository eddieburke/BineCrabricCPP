#pragma once
#include <string>
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft
namespace net::minecraft::item {
class MusicDiscItem : public Item {
public:
  static constexpr int kRawId = 2000;
  static void registerClass();
  MusicDiscItem(int rawId, std::string sound);
  bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
  std::string sound;
};
} // namespace net::minecraft::item
