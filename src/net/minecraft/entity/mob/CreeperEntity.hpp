#pragma once
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
namespace net::minecraft::entity::mob {
class CreeperEntity : public MonsterEntity {
public:
  static constexpr int kEntityId = 50;
  static constexpr const char* kEntityName = "Creeper";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit CreeperEntity(World* world = nullptr) : MonsterEntity(world) {
    initDataTracker();
    texture = "/mob/creeper.png";
  }
  int fuseTime = 0;
  int lastFuseTime = 0;
  void initDataTracker() override {
    LivingEntity::initDataTracker();
    dataTracker.startTracking(16, static_cast<std::int8_t>(-1));
    dataTracker.startTracking(17, static_cast<std::int8_t>(0));
  }
  void tick() override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  void onKilledBy(Entity* adversary) override;
  void onStruckByLightning(Entity* lightning) override;
  [[nodiscard]] float getScale(float tickDelta) const {
    return (static_cast<float>(lastFuseTime) + static_cast<float>(fuseTime - lastFuseTime) * tickDelta) / 28.0f;
  }
  [[nodiscard]] bool isCharged() const {
    return dataTracker.getByte(17) == 1;
  }
  [[nodiscard]] std::string getHurtSound() const override {
    return "mob.creeper";
  }
  [[nodiscard]] std::string getDeathSound() const override {
    return "mob.creeperdeath";
  }
  [[nodiscard]] int getDroppedItemId() const override;

protected:
  void attack(Entity* other, float distance) override;
  void resetAttack(Entity* other, float distance) override;

private:
  [[nodiscard]] int getFuseSpeed() const {
    return dataTracker.getByte(16);
  }
  void setFuseSpeed(int fuseSpeed) {
    dataTracker.set(16, static_cast<std::int8_t>(fuseSpeed));
  }
};
} // namespace net::minecraft::entity::mob
