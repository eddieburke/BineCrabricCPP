#include "net/minecraft/item/SnowballItem.hpp"
#include <memory>
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item {
SnowballItem::SnowballItem(int rawId) : Item(rawId) {
 setMaxCount(16);
}
ItemStack* SnowballItem::use(ItemStack* stack, World* world, PlayerEntity* user) {
 if(stack == nullptr || stack->count <= 0) {
  return stack;
 }
 --stack->count;
 if(world != nullptr && user != nullptr) {
  world->playSound(user, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
  if(!world->isRemote()) {
   auto projectile = std::make_unique<entity::projectile::thrown::SnowballEntity>(world, user);
   if(world->spawnEntity(projectile.get())) {
    projectile.release();
   }
  }
 }
 return stack;
}
void SnowballItem::registerClass() {
 static SnowballItem SNOWBALL(76);
 SNOWBALL.setTexturePosition(14, 0)->setTranslationKey("snowball");
}
MC_REGISTER_ITEM(SnowballItem)
} // namespace net::minecraft::item
