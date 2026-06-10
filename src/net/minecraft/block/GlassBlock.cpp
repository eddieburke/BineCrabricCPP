#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GlassBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

namespace net::minecraft::block {
void GlassBlock::registerClass()
{
    namespace mat = material;
    Block::GLASS = (new GlassBlock(20, 49, mat::Material::GLASS, false))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("glass");
}

void GlassBlock::registerSmeltingRecipes()
{
    if (Block::SAND != nullptr && Block::GLASS != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(Block::SAND->id, ItemStack(Block::GLASS));
    }
}




namespace {static ::net::minecraft::registry::RegisterBlock<GlassBlock> autoReg(20);} // namespace
} // namespace net::minecraft::block

