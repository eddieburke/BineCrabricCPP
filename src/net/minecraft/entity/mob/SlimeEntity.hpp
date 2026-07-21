#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/Monster.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
namespace net::minecraft::entity::mob {
class SlimeEntity : public LivingEntity, public Monster {
 public:
 static constexpr int kEntityId = 55;
 static constexpr const char* kEntityName = "Slime";
 struct ClientRenderer {
  static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
 };
 explicit SlimeEntity(World* world = nullptr);
 float lastStretch = 0.0f;
 float stretch = 0.0f;
 void tick() override;
 void markDead() override;
 void onPlayerInteraction(player::PlayerEntity* player) override;
 void writeNbt(NbtCompound& nbt) const override;
 void readNbt(const NbtCompound& nbt) override;
 [[nodiscard]] int getSize() const;
 void setSize(int size);
 void initDataTracker() override {
  LivingEntity::initDataTracker();
  dataTracker.startTracking(16, static_cast<std::int8_t>(1));
 }
 [[nodiscard]] std::string getHurtSound() const override {
  return "mob.slime";
 }
 [[nodiscard]] std::string getDeathSound() const override {
  return "mob.slime";
 }
 [[nodiscard]] float getSoundVolume() const override {
  return 0.6f;
 }
 [[nodiscard]] int getDroppedItemId() const override;
 [[nodiscard]] bool canSpawn() const override;

 protected:
 void tickLiving() override;

 private:
 int ticksUntilJump = 0;
};
} // namespace net::minecraft::entity::mob
