#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/ShearsItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

namespace net::minecraft::item {

ShearsItem::ShearsItem(int rawId) : Item(rawId)
{
    setMaxCount(1);
    setMaxDamage(238);
}

bool ShearsItem::postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* miner)
{
    (void)x;
    (void)y;
    (void)z;
    (void)miner;
    if (stack != nullptr && ((Block::LEAVES != nullptr && blockId == Block::LEAVES->id)
            || (Block::COBWEB != nullptr && blockId == Block::COBWEB->id))) {
        stack->applyDamage(1);
    }
    return Item::postMine(stack, blockId, x, y, z, miner);
}

bool ShearsItem::isSuitableFor(Block* block) const
{
    return block != nullptr && Block::COBWEB != nullptr && block->id == Block::COBWEB->id;
}

float ShearsItem::getMiningSpeedMultiplier(ItemStack* stack, Block* block) const
{
    (void)stack;
    if (block == nullptr) {
        return Item::getMiningSpeedMultiplier(stack, block);
    }
    if ((Block::COBWEB != nullptr && block->id == Block::COBWEB->id) || (Block::LEAVES != nullptr && block->id == Block::LEAVES->id)) {
        return 15.0f;
    }
    if (Block::WOOL != nullptr && block->id == Block::WOOL->id) {
        return 5.0f;
    }
    return Item::getMiningSpeedMultiplier(stack, block);
}

void ShearsItem::registerClass()
{
    static ShearsItem SHEARS(103);
    SHEARS.setTexturePosition(13, 5)->setTranslationKey("shears");
}

void ShearsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(103)),
        {std::string(" #"), std::string("# "), '#', Item::byRawId(9)});
}




namespace {static ::net::minecraft::registry::RegisterItem<ShearsItem> autoReg(103); } // namespace

} // namespace net::minecraft::item
