#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/PaintingItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

PaintingItem::PaintingItem(int rawId) : Item(rawId) {}

bool PaintingItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side)
{
    if (world == nullptr || stack == nullptr || side == 0 || side == 1) {
        return false;
    }
    int facing = 0;
    if (side == 4) {
        facing = 1;
    } else if (side == 3) {
        facing = 2;
    } else if (side == 5) {
        facing = 3;
    }
    if (!world->isRemote()) {
        auto* painting = new entity::decoration::painting::PaintingEntity(world);
        painting->facing = facing;
        painting->setPosition(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
        world->spawnEntity(painting);
    }
    --stack->count;
    return true;
}

void PaintingItem::registerClass()
{
    static PaintingItem PAINTING(65);
    PAINTING.setTexturePosition(10, 1)->setTranslationKey("painting");
}

void PaintingItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(65)),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Item::byRawId(24), 'X', Block::WOOL});
}




namespace {static ::net::minecraft::registry::RegisterItem<PaintingItem> autoReg(65); } // namespace

} // namespace net::minecraft::item
