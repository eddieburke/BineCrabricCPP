#pragma once
#include "net/minecraft/entity/mob/ZombieEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::mob {
class PigZombieEntity : public ZombieEntity {
 public:
 static constexpr int kEntityId = 57;
 static constexpr const char* kEntityName = "PigZombie";
 explicit PigZombieEntity(World* world = nullptr);
 void tick() override;
 void writeNbt(NbtCompound& nbt) const override;
 void readNbt(const NbtCompound& nbt) override;
 bool damage(Entity* damageSource, int amount) override;
 [[nodiscard]] bool canSpawn() const override;
 [[nodiscard]] std::string getRandomSound() override {
  return "mob.zombiepig.zpig";
 }
 [[nodiscard]] std::string getHurtSound() const override {
  return "mob.zombiepig.zpighurt";
 }
 [[nodiscard]] std::string getDeathSound() const override {
  return "mob.zombiepig.zpigdeath";
 }
 [[nodiscard]] int getDroppedItemId() const override;
 [[nodiscard]] ItemStack getHeldItem() const override {
  return ItemStack(Item::byRawId(27), 1);
 }

 protected:
 Entity* getTargetInRange() override;

 private:
 int anger = 0;
 int angrySoundDelay = 0;
 void makeAngry(Entity* source);
};
} // namespace net::minecraft::entity::mob
