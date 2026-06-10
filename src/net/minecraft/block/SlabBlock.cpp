#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SlabBlock.hpp"

#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/SlabBlockItem.hpp"

namespace net::minecraft::block {

SlabBlock::SlabBlock(int id, bool doubleSlabIn) : Block(id, 6, material::Material::STONE)
{
    doubleSlab = doubleSlabIn;
    if (!doubleSlab) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    }
    setOpacity(255);
}

int SlabBlock::getTexture(int side, int meta) const
{
    if (meta == 0) {
        if (side <= 1) {
            return 6;
        }
        return 5;
    }
    if (meta == 1) {
        if (side == 0) {
            return 208;
        }
        if (side == 1) {
            return 176;
        }
        return 192;
    }
    if (meta == 2) {
        return 4;
    }
    if (meta == 3) {
        return 16;
    }
    return 6;
}

void SlabBlock::onPlaced(World* world, int x, int y, int z)
{
    if (Block::SLAB != nullptr && this != Block::SLAB) {
        Block::onPlaced(world, x, y, z);
    }
    if (world == nullptr || Block::SLAB == nullptr || Block::DOUBLE_SLAB == nullptr) {
        return;
    }

    const int belowId = world->getBlockId(x, y - 1, z);
    const int meta = world->getBlockMeta(x, y, z);
    const int belowMeta = world->getBlockMeta(x, y - 1, z);
    if (meta != belowMeta) {
        return;
    }
    if (belowId == Block::SLAB->id) {
        world->setBlock(x, y, z, 0);
        world->setBlock(x, y - 1, z, Block::DOUBLE_SLAB->id, static_cast<std::uint8_t>(meta));
    }
}

int SlabBlock::getDroppedItemCount(JavaRandom& /*random*/) const
{
    return doubleSlab ? 2 : 1;
}

int SlabBlock::getDroppedItemMeta(int blockMeta) const
{
    return blockMeta;
}

bool SlabBlock::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const
{
    if (Block::SLAB != nullptr && this != Block::SLAB) {
        return Block::isSideVisible(blockView, x, y, z, side);
    }
    if (side == 1) {
        return true;
    }
    if (!Block::isSideVisible(blockView, x, y, z, side)) {
        return false;
    }
    if (side == 0) {
        return true;
    }
    return blockView == nullptr || blockView->getBlockId(x, y, z) != id;
}
void SlabBlock::registerClass()
{
    Block::DOUBLE_SLAB = (new SlabBlock(43, true))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneSlab");
    Block::SLAB = (new SlabBlock(44, false))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneSlab");
}
void SlabBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 3), {std::string("###"), '#', Block::COBBLESTONE});
    recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 0), {std::string("###"), '#', Block::STONE});
    recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 1), {std::string("###"), '#', Block::SANDSTONE});
    recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 2), {std::string("###"), '#', Block::PLANKS});
}





void SlabBlock::registerBlockItems()
{
    (new item::SlabBlockItem(44 - 256))->setTranslationKey("stoneSlab");
}

namespace {static ::net::minecraft::registry::RegisterBlock<SlabBlock> autoReg(43);} // namespace
} // namespace net::minecraft::block

