#pragma once
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/ai/pathing/Path.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <memory>
namespace net::minecraft::entity {
class MobEntity : public LivingEntity {
public:
  explicit MobEntity(World* world = nullptr) : LivingEntity(world) {}
  Entity* target = nullptr;
  bool movementBlocked = false;
  [[nodiscard]] virtual float getPathfindingFavor(int x, int y, int z) const {
    (void)x;
    (void)y;
    (void)z;
    return 0.0f;
  }
  [[nodiscard]] bool canSpawn() const override {
    const int bx = MathHelper::floor(x);
    const int by = MathHelper::floor(boundingBox.minY);
    const int bz = MathHelper::floor(z);
    return LivingEntity::canSpawn() && getPathfindingFavor(bx, by, bz) >= 0.0f;
  }
  [[nodiscard]] bool hasPath() const {
    return path != nullptr;
  }
  void setPath(std::unique_ptr<ai::pathing::Path> newPath) {
    path = std::move(newPath);
  }
  [[nodiscard]] Entity* getTarget() const {
    return target;
  }
  void setTarget(Entity* newTarget) {
    target = newTarget;
  }
  void tickLiving() override;

protected:
  [[nodiscard]] virtual bool isMovementBlocked() const {
    return false;
  }
  [[nodiscard]] virtual Entity* getTargetInRange() {
    return nullptr;
  }
  virtual void attack(Entity* other, float distance);
  virtual void resetAttack(Entity* other, float distance);
  void pathingUpdate();
  std::unique_ptr<ai::pathing::Path> path{};
};
} // namespace net::minecraft::entity
