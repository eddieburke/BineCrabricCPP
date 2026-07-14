#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/item/BowItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::entity::mob {
class SkeletonEntity : public MonsterEntity {
public:
  static constexpr int kEntityId = 51;
  static constexpr const char* kEntityName = "Skeleton";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit SkeletonEntity(World* world = nullptr);
  void tickMovement() override;
  [[nodiscard]] std::string getRandomSound() override {
    return "mob.skeleton";
  }
  [[nodiscard]] std::string getHurtSound() const override {
    return "mob.skeletonhurt";
  }
  [[nodiscard]] std::string getDeathSound() const override {
    return "mob.skeletonhurt";
  }
  [[nodiscard]] ItemStack getHeldItem() const override {
    return ItemStack(Item::byRawId(5), 1);
  }

protected:
  void attack(Entity* other, float distance) override;
  void dropItems() override;
};
} // namespace net::minecraft::entity::mob
