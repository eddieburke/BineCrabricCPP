#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SignBlock.hpp"

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

SignBlock::SignBlock(int id, bool standingIn) : BlockWithEntity(id, material::Material::WOOD)
{
    standing = standingIn;
    textureId = 4;
    const float halfWidth = 0.25f;
    const float height = 1.0f;
    setBoundingBox(0.5f - halfWidth, 0.0f, 0.5f - halfWidth, 0.5f + halfWidth, height, 0.5f + halfWidth);
}

void SignBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (standing) {
        return;
    }
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box SignBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (standing) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }

    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    constexpr float yMin = 0.28125f;
    constexpr float yMax = 0.78125f;
    constexpr float thickness = 0.125f;
    if (meta == 2) {
        return {0.0f, yMin, 1.0f - thickness, 1.0f, yMax, 1.0f};
    }
    if (meta == 3) {
        return {0.0f, yMin, 0.0f, 1.0f, yMax, thickness};
    }
    if (meta == 4) {
        return {1.0f - thickness, yMin, 0.0f, 1.0f, yMax, 1.0f};
    }
    if (meta == 5) {
        return {0.0f, yMin, 0.0f, thickness, yMax, 1.0f};
    }
    return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
}

void SignBlock::neighborUpdate(World* world, int x, int y, int z, int id)
{
    if (world == nullptr) {
        return;
    }

    bool shouldBreak = false;
    if (standing) {
        if (!world->getMaterial(x, y - 1, z).isSolid()) {
            shouldBreak = true;
        }
    } else {
        const int meta = world->getBlockMeta(x, y, z);
        shouldBreak = true;
        if (meta == 2 && world->getMaterial(x, y, z + 1).isSolid()) {
            shouldBreak = false;
        }
        if (meta == 3 && world->getMaterial(x, y, z - 1).isSolid()) {
            shouldBreak = false;
        }
        if (meta == 4 && world->getMaterial(x + 1, y, z).isSolid()) {
            shouldBreak = false;
        }
        if (meta == 5 && world->getMaterial(x - 1, y, z).isSolid()) {
            shouldBreak = false;
        }
    }

    if (shouldBreak) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
    BlockWithEntity::neighborUpdate(world, x, y, z, id);
}

int SignBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Item::SIGN != nullptr ? Item::SIGN->id : 323;
}
namespace {

void registerSignBlocks()
{
    Block::SIGN = (new SignBlock(63, true))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
    Block::WALL_SIGN = (new SignBlock(68, false))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerSignBlocks, 63);

} // namespace
} // namespace net::minecraft::block

