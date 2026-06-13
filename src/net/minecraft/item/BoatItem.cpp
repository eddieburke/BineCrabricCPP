#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/BoatItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/world/World.hpp"

#include <optional>

namespace net::minecraft::item {

BoatItem::BoatItem(int rawId) : Item(rawId)
{
    setMaxCount(1);
}

ItemStack* BoatItem::use(ItemStack* stack, World* world, PlayerEntity* user)
{
    if (world == nullptr || user == nullptr) {
        return stack;
    }
    Vec3d start;
    Vec3d end;
    detail::playerLookRay(user, 1.0f, start, end);
    std::optional<HitResult> hit = world->raycast(start, end, true);
    if (!hit.has_value() || hit->type != HitResultType::BLOCK) {
        return stack;
    }
    int y = hit->blockY;
    if (Block::SNOW != nullptr && world->getBlockId(hit->blockX, y, hit->blockZ) == Block::SNOW->id) {
        --y;
    }
    if (!world->isRemote()) {
        auto* boat = new entity::vehicle::BoatEntity(world);
        boat->setPosition(static_cast<double>(hit->blockX) + 0.5, static_cast<double>(y) + 1.0, static_cast<double>(hit->blockZ) + 0.5);
        world->spawnEntity(boat);
    }
    if (stack != nullptr) {
        --stack->count;
    }
    return stack;
}

void BoatItem::registerClass()
{
    static BoatItem BOAT(77);
    BOAT.setTexturePosition(8, 8)->setTranslationKey("boat");
}

void BoatItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(77)),
        {std::string("# #"), std::string("###"), '#', Block::PLANKS});
}





MC_REGISTER_ITEM(BoatItem)
} // namespace net::minecraft::item
