#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
namespace net::minecraft::entity::projectile::thrown {
class EggEntity : public Entity {
public:
  static constexpr int kEntityId = 98;
  static constexpr const char* kEntityName = "Egg";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit EggEntity(World* world = nullptr) : Entity(world) {
    setBoundingBoxSpacing(0.25f, 0.25f);
  }
  EggEntity(World* world, LivingEntity* ownerIn) : Entity(world) {
    owner = ownerIn;
    setBoundingBoxSpacing(0.25f, 0.25f);
    if(owner != nullptr) {
      setPositionAndAnglesKeepPrevAngles(owner->x, owner->y + static_cast<double>(owner->getEyeHeight()),
                                         owner->z, owner->yaw, owner->pitch);
      x -= static_cast<double>(MathHelper::cos(yaw / 180.0f * kPiF) * 0.16f);
      y -= 0.1;
      z -= static_cast<double>(MathHelper::sin(yaw / 180.0f * kPiF) * 0.16f);
      setPosition(x, y, z);
      standingEyeHeight = 0.0f;
      constexpr float spread = 0.4f;
      velocityX = -MathHelper::sin(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * spread;
      velocityZ = MathHelper::cos(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * spread;
      velocityY = -MathHelper::sin(pitch / 180.0f * kPiF) * spread;
      setProjectileVelocity(*this, velocityX, velocityY, velocityZ, 1.5f, 1.0f);
    }
  }
  EggEntity(World* world, double xIn, double yIn, double zIn) : Entity(world) {
    setBoundingBoxSpacing(0.25f, 0.25f);
    setPosition(xIn, yIn, zIn);
    standingEyeHeight = 0.0f;
  }
  void tick() override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] float getShadowRadius() const override {
    return 0.0f;
  }
  LivingEntity* owner = nullptr;
  int inAirTime = 0;
  int shake = 0;

private:
  int blockX = -1;
  int blockY = -1;
  int blockZ = -1;
  int blockId = 0;
  bool inGround = false;
  int removalTimer = 0;
};
} // namespace net::minecraft::entity::projectile::thrown
