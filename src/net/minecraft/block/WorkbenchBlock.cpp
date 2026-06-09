#include "net/minecraft/block/WorkbenchBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

int WorkbenchBlock::getTexture(int side) const
{
    if (side == 1) {
        return textureId - 16;
    }
    if (side == 0) {
        return Block::PLANKS != nullptr ? Block::PLANKS->getTexture(0) : 4;
    }
    if (side == 2 || side == 4) {
        return textureId + 1;
    }
    return textureId;
}

bool WorkbenchBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (world == nullptr || player == nullptr) {
        return false;
    }
    if (world->isRemote()) {
        return true;
    }
    player->openCraftingScreen(x, y, z);
    return true;
}

} // namespace net::minecraft::block
