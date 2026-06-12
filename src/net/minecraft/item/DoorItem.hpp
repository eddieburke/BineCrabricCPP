#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block::material {
class Material;
} // namespace net::minecraft::block::material

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class DoorItem : public Item {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kRawId = 68;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    DoorItem(int rawId, block::material::Material& material);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;

private:
    block::material::Material* material_ = nullptr;
};

} // namespace net::minecraft::item
