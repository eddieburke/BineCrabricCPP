#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/WorkbenchBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/Item.hpp"

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
void WorkbenchBlock::registerClass()
{
    Block::CRAFTING_TABLE = (new WorkbenchBlock(kBlockId))->setHardness(2.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("workbench");
}
void WorkbenchBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::CRAFTING_TABLE),
        {std::string("##"), std::string("##"), '#', Block::PLANKS});
}





namespace {static ::net::minecraft::registry::RegisterBlock<WorkbenchBlock> autoReg;} // namespace
} // namespace net::minecraft::block

