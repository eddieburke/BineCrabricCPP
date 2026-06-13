#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/DoorItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

DoorItem::DoorItem(int rawId, block::material::Material& material)
    : Item(rawId),
      material_(&material)
{
    setMaxCount(1);
}

bool DoorItem::useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side)
{
    if (stack == nullptr || user == nullptr || world == nullptr || side != 1) {
        return false;
    }
    Block* block = material_ == &block::material::Material::WOOD ? Block::DOOR : Block::IRON_DOOR;
    if (block == nullptr || !block->canPlaceAt(world, x, ++y, z)) {
        return false;
    }
    int direction = MathHelper::floor((user->yaw + 180.0f) * 4.0f / 360.0f - 0.5f) & 3;
    int dx = 0;
    int dz = 0;
    if (direction == 0) {
        dz = 1;
    } else if (direction == 1) {
        dx = -1;
    } else if (direction == 2) {
        dz = -1;
    } else if (direction == 3) {
        dx = 1;
    }
    const int left = (world->shouldSuffocate(x - dx, y, z - dz) ? 1 : 0)
        + (world->shouldSuffocate(x - dx, y + 1, z - dz) ? 1 : 0);
    const int right = (world->shouldSuffocate(x + dx, y, z + dz) ? 1 : 0)
        + (world->shouldSuffocate(x + dx, y + 1, z + dz) ? 1 : 0);
    const bool doorLeft = world->getBlockId(x - dx, y, z - dz) == block->id
        || world->getBlockId(x - dx, y + 1, z - dz) == block->id;
    const bool doorRight = world->getBlockId(x + dx, y, z + dz) == block->id
        || world->getBlockId(x + dx, y + 1, z + dz) == block->id;
    if ((doorLeft && !doorRight) || right > left) {
        direction = (direction - 1) & 3;
        direction += 4;
    }
    world->pauseTicking = true;
    world->setBlock(x, y, z, block->id, static_cast<std::uint8_t>(direction));
    world->setBlock(x, y + 1, z, block->id, static_cast<std::uint8_t>(direction + 8));
    world->pauseTicking = false;
    world->notifyNeighbors(x, y, z, block->id);
    world->notifyNeighbors(x, y + 1, z, block->id);
    --stack->count;
    return true;
}

void DoorItem::registerClass()
{
    static DoorItem WOODEN_DOOR(68, block::material::Material::WOOD);
    WOODEN_DOOR.setTexturePosition(11, 2)->setTranslationKey("doorWood");
    static DoorItem IRON_DOOR(74, block::material::Material::METAL);
    IRON_DOOR.setTexturePosition(12, 2)->setTranslationKey("doorIron");
}

void DoorItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(68)),
        {std::string("##"), std::string("##"), std::string("##"), '#', Block::PLANKS});
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(74)),
        {std::string("##"), std::string("##"), std::string("##"), '#', Item::byRawId(9)});
}





MC_REGISTER_ITEM(DoorItem)
} // namespace net::minecraft::item
