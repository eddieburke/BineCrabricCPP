#pragma once

#include "net/minecraft/item/Item.hpp"

#include <string>

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class MusicDiscItem : public Item {
public:
    static void registerClass();
    MusicDiscItem(int rawId, std::string sound);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;

    std::string sound;
};

} // namespace net::minecraft::item
