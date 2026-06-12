#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/TntBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/flint_and_steel.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

namespace net::minecraft::block {

TntBlock::TntBlock(int id, int textureId) : Block(id, textureId, material::Material::TNT) {}

int TntBlock::getTexture(int side) const
{
    return Block::textureForSide(side, textureId, textureId + 2, textureId + 1);
}

void TntBlock::onPlaced(World* world, int x, int y, int z)
{
    Block::onPlaced(world, x, y, z);
    if (world != nullptr && world->isEmittingRedstonePower(x, y, z)) {
        onMetadataChange(world, x, y, z, 1);
        world->setBlock(x, y, z, 0);
    }
}

void TntBlock::neighborUpdate(World* world, int x, int y, int z, int blockId)
{
    if (world == nullptr) {
        return;
    }
    if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
        && Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower() && world->isEmittingRedstonePower(x, y, z)) {
        onMetadataChange(world, x, y, z, 1);
        world->setBlock(x, y, z, 0);
    }
}

void TntBlock::onMetadataChange(World* world, int x, int y, int z, int meta)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    if ((meta & 1) == 0) {
        dropStack(world, x, y, z, ItemStack(id, 1));
    } else {
        auto* tnt = new TntEntity(world, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f);
        world->spawnEntity(tnt);
        world->playSound(tnt, "random.fuse", 1.0f, 1.0f);
    }
}

void TntBlock::onDestroyedByExplosion(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    auto* tnt = new TntEntity(world, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f);
    tnt->fuse = world->random().nextInt(tnt->fuse / 4) + tnt->fuse / 8;
    world->spawnEntity(tnt);
}

void TntBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (world != nullptr && player != nullptr && Item::byRawId(3) != nullptr) {
        const ItemStack hand = player->getHand();
        if (hand.itemId == Item::byRawId(3)->id) {
            world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, 1);
        }
    }
    Block::onBlockBreakStart(world, x, y, z, player);
}

bool TntBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    return Block::onUse(world, x, y, z, player);
}
void TntBlock::registerClass()
{
    Block::TNT = (new TntBlock(kBlockId, 8))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("tnt");
}
void TntBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::TNT),
        {std::string("X#X"), std::string("#X#"), std::string("X#X"), 'X', Item::byRawId(33), '#', Block::SAND});
}





namespace {static ::net::minecraft::registry::RegisterBlock<TntBlock> autoReg;} // namespace
} // namespace net::minecraft::block

