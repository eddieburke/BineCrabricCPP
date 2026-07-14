#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"
namespace net::minecraft::item {
class SwordItem : public Item {
public:
protected:
  SwordItem(int rawId, ToolMaterial material)
      : Item(rawId, RegistrationMode::Deferred), damage_(4 + toolMaterialAttackDamage(material) * 2) {
    setMaxCount(1);
    setMaxDamage(toolMaterialDurability(material));
  }
  [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* /*stack*/, Block* block) const override {
    if(block != nullptr && Block::COBWEB != nullptr && block->id == Block::COBWEB->id) {
      return 15.0f;
    }
    return 1.5f;
  }
  bool postHit(ItemStack* stack, LivingEntity* /*target*/, LivingEntity* /*attacker*/) override {
    if(stack != nullptr) {
      stack->applyDamage(1);
    }
    return true;
  }
  bool postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* /*miner*/) override {
    (void)blockId;
    (void)x;
    (void)y;
    (void)z;
    if(stack != nullptr) {
      stack->applyDamage(2);
    }
    return true;
  }
  [[nodiscard]] int getAttackDamage(Entity* /*attacked*/) const override {
    return damage_;
  }
  [[nodiscard]] bool isSuitableFor(Block* block) const override {
    return block != nullptr && Block::COBWEB != nullptr && block->id == Block::COBWEB->id;
  }
  [[nodiscard]] bool isHandheld() const {
    return true;
  }

private:
  int damage_ = 1;
};
} // namespace net::minecraft::item
