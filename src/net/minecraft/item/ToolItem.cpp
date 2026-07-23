#include "net/minecraft/item/ToolItem.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::item {
bool ToolItem::postHit(ItemStack* stack, LivingEntity* /*target*/, LivingEntity* /*attacker*/) {
 if(stack != nullptr) {
  stack->applyDamage(2);
 }
 return true;
}
bool ToolItem::postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* /*miner*/) {
 (void)blockId;
 (void)x;
 (void)y;
 (void)z;
 if(stack != nullptr) {
  stack->applyDamage(1);
 }
 return true;
}
} // namespace net::minecraft::item
