#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/WorkbenchBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

int WorkbenchBlock::getTexture(int side) const
{
    const int planks = Block::PLANKS != nullptr ? Block::PLANKS->getTexture(0) : 4;
    if (side == FACE_EAST || side == FACE_NORTH) {
        return textureId + 1;
    }
    return Block::textureForSide(side, textureId, planks, textureId - 16);
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
namespace {

void registerWorkbenchBlock()
{
    Block::CRAFTING_TABLE = (new WorkbenchBlock(58))->setHardness(2.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("workbench");
}

MINECRAFT_REGISTER_BLOCK(registerWorkbenchBlock, 58);

} // namespace
} // namespace net::minecraft::block

