#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/DyeItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

DyeItem::DyeItem(int rawId) : Item(rawId)
{
    setHasSubtypes(true);
    setMaxDamage(0);
}

int DyeItem::getTextureId(int damage) const
{
    return textureId_ + damage % 8 * 16 + damage / 8;
}

std::string DyeItem::getTranslationKey(const ItemStack* stack) const
{
    const int damage = stack != nullptr ? (stack->getDamage() & 0xF) : 0;
    return Item::getTranslationKey() + "." + names[static_cast<std::size_t>(damage)];
}

bool DyeItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int /*side*/)
{
    if (stack == nullptr || world == nullptr || stack->getDamage() != 15) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    if (Block::SAPLING != nullptr && blockId == Block::SAPLING->id) {
        if (!world->isRemote()) {
            if (auto* sapling = dynamic_cast<block::SaplingBlock*>(Block::SAPLING)) {
                sapling->generate(world, x, y, z, world->random());
            }
            --stack->count;
        }
        return true;
    }
    if (Block::WHEAT != nullptr && blockId == Block::WHEAT->id) {
        if (!world->isRemote()) {
            world->setBlockMeta(x, y, z, 7);
            --stack->count;
        }
        return true;
    }
    if (Block::GRASS_BLOCK != nullptr && blockId == Block::GRASS_BLOCK->id) {
        if (!world->isRemote()) {
            --stack->count;
            for (int i = 0; i < 128; ++i) {
                int px = x;
                int py = y + 1;
                int pz = z;
                bool blocked = false;
                for (int j = 0; j < i / 16; ++j) {
                    px += random.nextInt(3) - 1;
                    py += (random.nextInt(3) - 1) * random.nextInt(3) / 2;
                    pz += random.nextInt(3) - 1;
                    if (world->getBlockId(px, py - 1, pz) != Block::GRASS_BLOCK->id || world->shouldSuffocate(px, py, pz)) {
                        blocked = true;
                        break;
                    }
                }
                if (blocked || world->getBlockId(px, py, pz) != 0) {
                    continue;
                }
                if (random.nextInt(10) != 0 && Block::GRASS != nullptr) {
                    world->setBlock(px, py, pz, Block::GRASS->id, 1);
                } else if (random.nextInt(3) != 0 && Block::DANDELION != nullptr) {
                    world->setBlock(px, py, pz, Block::DANDELION->id);
                } else if (Block::ROSE != nullptr) {
                    world->setBlock(px, py, pz, Block::ROSE->id);
                }
            }
        }
        return true;
    }
    return false;
}

void DyeItem::useOnEntity(ItemStack* stack, LivingEntity* entity)
{
    auto* sheep = dynamic_cast<entity::passive::SheepEntity*>(entity);
    if (stack == nullptr || sheep == nullptr || sheep->isSheared()) {
        return;
    }
    const int color = block::WoolBlock::getItemMeta(stack->getDamage());
    if (sheep->getColor() != color) {
        sheep->setColor(color);
        --stack->count;
    }
}

void DyeItem::registerClass()
{
    static DyeItem DYE(95);
    DYE.setTexturePosition(14, 4)->setTranslationKey("dyePowder");
}

void DyeItem::registerSmeltingRecipes()
{
    if (Block::CACTUS != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(
            Block::CACTUS->id, ItemStack(Item::byRawId(95), 1, 2));
    }
}

void DyeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    if (Block::WOOL != nullptr) {
        for (int i = 0; i < 16; ++i) {
            recipeManager.addShapelessRecipe(
                ItemStack(Block::WOOL, 1, block::WoolBlock::getItemMeta(i)),
                {ItemStack(Item::byRawId(95), 1, i), ItemStack(Block::WOOL, 1, 0)});
        }
    }
    if (Block::DANDELION != nullptr) {
        recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 11), {Block::DANDELION});
    }
    if (Block::ROSE != nullptr) {
        recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 1), {Block::ROSE});
    }
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 3, 15), {Item::byRawId(96)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 9),
        {ItemStack(Item::byRawId(95), 1, 1), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 14),
        {ItemStack(Item::byRawId(95), 1, 1), ItemStack(Item::byRawId(95), 1, 11)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 10),
        {ItemStack(Item::byRawId(95), 1, 2), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 8),
        {ItemStack(Item::byRawId(95), 1, 0), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 7),
        {ItemStack(Item::byRawId(95), 1, 8), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 3, 7),
        {ItemStack(Item::byRawId(95), 1, 0), ItemStack(Item::byRawId(95), 1, 15), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 12),
        {ItemStack(Item::byRawId(95), 1, 4), ItemStack(Item::byRawId(95), 1, 15)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 6),
        {ItemStack(Item::byRawId(95), 1, 4), ItemStack(Item::byRawId(95), 1, 2)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 5),
        {ItemStack(Item::byRawId(95), 1, 4), ItemStack(Item::byRawId(95), 1, 1)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 2, 13),
        {ItemStack(Item::byRawId(95), 1, 5), ItemStack(Item::byRawId(95), 1, 9)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 3, 13),
        {ItemStack(Item::byRawId(95), 1, 4), ItemStack(Item::byRawId(95), 1, 1), ItemStack(Item::byRawId(95), 1, 9)});
    recipeManager.addShapelessRecipe(ItemStack(Item::byRawId(95), 4, 13),
        {ItemStack(Item::byRawId(95), 1, 4), ItemStack(Item::byRawId(95), 1, 1), ItemStack(Item::byRawId(95), 1, 1),
            ItemStack(Item::byRawId(95), 1, 15)});
    if (Block::LAPIS_BLOCK != nullptr) {
        recipeManager.addShapedRecipe(ItemStack(Block::LAPIS_BLOCK),
            {std::string("###"), std::string("###"), std::string("###"), '#', ItemStack(Item::byRawId(95), 9, 4)});
        recipeManager.addShapedRecipe(ItemStack(Item::byRawId(95), 9, 4),
            {std::string("#"), '#', Block::LAPIS_BLOCK});
    }
}




namespace {static ::net::minecraft::registry::RegisterItem<DyeItem> autoReg; } // namespace

} // namespace net::minecraft::item
