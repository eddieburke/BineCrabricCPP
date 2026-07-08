#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
}  // namespace net::minecraft

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class BucketItem : public Item {
   public:
    static constexpr int kRawId = 69;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    BucketItem(int rawId, int fluidBlockId);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;

   private:
    int fluidBlockId_ = 0;
};
}  // namespace net::minecraft::item
