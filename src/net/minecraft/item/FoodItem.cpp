#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/FoodItem.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::item {
FoodItem::FoodItem(int rawId, int healAmount, bool wolfFood)
    : Item(rawId, RegistrationMode::Deferred), healAmount_(healAmount), wolfFood_(wolfFood) {
  setMaxCount(1);
}
ItemStack* FoodItem::use(ItemStack* stack, World* /*world*/, PlayerEntity* user) {
  if(stack != nullptr) {
    --stack->count;
  }
  if(user != nullptr) {
    user->heal(healAmount_);
  }
  return stack;
}
} // namespace net::minecraft::item
