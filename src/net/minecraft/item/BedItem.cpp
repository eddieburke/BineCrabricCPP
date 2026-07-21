#include "net/minecraft/item/BedItem.hpp"
#include <string>
#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item {
BedItem::BedItem() : Item(kRawId, RegistrationMode::Deferred) {
}
bool BedItem::useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) {
 if(stack == nullptr || user == nullptr || world == nullptr || side != 1 || Block::BED == nullptr) {
  return false;
 }
 ++y;
 const int direction = MathHelper::floor(static_cast<double>(user->yaw * 4.0f / 360.0f) + 0.5) & 3;
 int dx = 0;
 int dz = 0;
 if(direction == 0) {
  dz = 1;
 } else if(direction == 1) {
  dx = -1;
 } else if(direction == 2) {
  dz = -1;
 } else if(direction == 3) {
  dx = 1;
 }
 if(world->isAir(x, y, z) && world->isAir(x + dx, y, z + dz) && world->shouldSuffocate(x, y - 1, z) &&
    world->shouldSuffocate(x + dx, y - 1, z + dz)) {
  world->setBlock(x, y, z, Block::BED->id, static_cast<std::uint8_t>(direction));
  world->setBlock(x + dx, y, z + dz, Block::BED->id, static_cast<std::uint8_t>(direction + 8));
  --stack->count;
  return true;
 }
 return false;
}
void BedItem::registerClass() {
 static BedItem instance;
 instance.setMaxCount(1)->setTexturePosition(13, 2)->setTranslationKey("bed");
 Item::registerInItemsArray(&instance);
}
void BedItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Item::byRawId(99), 1),
                               {std::string("###"), std::string("XXX"), '#', Block::WOOL, 'X', Block::PLANKS});
}
MC_REGISTER_ITEM(BedItem)
} // namespace net::minecraft::item
