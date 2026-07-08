#include "net/minecraft/block/LadderBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"

namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}

namespace net::minecraft::block {
LadderBlock::LadderBlock(int blockId, int textureId) : Block(blockId, textureId, material::Material::PISTON_BREAKABLE) {
}

std::optional<net::minecraft::Box> LadderBlock::collisionShapeForMeta(int meta, int x, int y, int z) {
    constexpr double thickness = 0.125;
    switch (meta) {
        case 2:
            return net::minecraft::Box{static_cast<double>(x),
                                       static_cast<double>(y),
                                       static_cast<double>(z + 1) - thickness,
                                       static_cast<double>(x + 1),
                                       static_cast<double>(y + 1),
                                       static_cast<double>(z + 1)};
        case 3:
            return net::minecraft::Box{static_cast<double>(x),
                                       static_cast<double>(y),
                                       static_cast<double>(z),
                                       static_cast<double>(x + 1),
                                       static_cast<double>(y + 1),
                                       static_cast<double>(z) + thickness};
        case 4:
            return net::minecraft::Box{static_cast<double>(x + 1) - thickness,
                                       static_cast<double>(y),
                                       static_cast<double>(z),
                                       static_cast<double>(x + 1),
                                       static_cast<double>(y + 1),
                                       static_cast<double>(z + 1)};
        case 5:
            return net::minecraft::Box{static_cast<double>(x),
                                       static_cast<double>(y),
                                       static_cast<double>(z),
                                       static_cast<double>(x) + thickness,
                                       static_cast<double>(y + 1),
                                       static_cast<double>(z + 1)};
        default:
            return std::nullopt;
    }
}

std::optional<net::minecraft::Box> LadderBlock::getCollisionShape(World* world, int x, int y, int z) const {
    const int meta = world != nullptr ? world->getBlockMeta(x, y, z) : 0;
    return collisionShapeForMeta(meta, x, y, z);
}

void LadderBlock::applyBoundsForMeta(int meta) {
    if (meta >= 2 && meta <= 5) {
        setBoundingBox(boundsForMeta(meta));
    }
}

net::minecraft::Box LadderBlock::boundsForMeta(int meta) const {
    constexpr float thickness = 0.125f;
    if (meta == 2) {
        return {0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f};
    }
    if (meta == 3) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness};
    }
    if (meta == 4) {
        return {1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    if (meta == 5) {
        return {0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f};
    }
    return {minX, minY, minZ, maxX, maxY, maxZ};
}

void LadderBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}

net::minecraft::Box LadderBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    return boundsForMeta(meta);
}

bool LadderBlock::canPlaceAt(World* world, int x, int y, int z) const {
    if (world == nullptr) {
        return false;
    }
    if (world->shouldSuffocate(x - 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x + 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x, y, z - 1)) {
        return true;
    }
    return world->shouldSuffocate(x, y, z + 1);
}

void LadderBlock::onPlaced(World* world, int x, int y, int z, int direction) {
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    if ((meta == 0 || direction == 2) && world->shouldSuffocate(x, y, z + 1)) {
        meta = 2;
    }
    if ((meta == 0 || direction == 3) && world->shouldSuffocate(x, y, z - 1)) {
        meta = 3;
    }
    if ((meta == 0 || direction == 4) && world->shouldSuffocate(x + 1, y, z)) {
        meta = 4;
    }
    if ((meta == 0 || direction == 5) && world->shouldSuffocate(x - 1, y, z)) {
        meta = 5;
    }
    world->setBlockMeta(x, y, z, meta);
}

void LadderBlock::neighborUpdate(World* world, int x, int y, int z, int id) {
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    bool attached = false;
    if (meta == 2 && world->shouldSuffocate(x, y, z + 1)) {
        attached = true;
    }
    if (meta == 3 && world->shouldSuffocate(x, y, z - 1)) {
        attached = true;
    }
    if (meta == 4 && world->shouldSuffocate(x + 1, y, z)) {
        attached = true;
    }
    if (meta == 5 && world->shouldSuffocate(x - 1, y, z)) {
        attached = true;
    }
    if (!attached) {
        dropStacks(world, x, y, z, meta);
        world->setBlock(x, y, z, 0);
    }
    Block::neighborUpdate(world, x, y, z, id);
}

void LadderBlock::registerClass() {
    Block::LADDER = (new LadderBlock(kBlockId, 83))
                        ->setHardness(0.4f)
                        ->setSoundGroup(&kWoodSound)
                        ->setTranslationKey("ladder")
                        ->ignoreMetaUpdates();
}

void LadderBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Block::LADDER, 2),
                                  {std::string("# #"), std::string("###"), std::string("# #"), '#', Item::byRawId(24)});
}
MC_REGISTER_BLOCK(LadderBlock)
}  // namespace net::minecraft::block
