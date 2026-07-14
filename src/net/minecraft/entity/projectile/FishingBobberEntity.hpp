#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::projectile {
class FishingBobberEntity : public Entity {
public:
  static constexpr int kEntityId = 96;
  static constexpr const char* kEntityName = "FishingBobber";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit FishingBobberEntity(World* world = nullptr);
  FishingBobberEntity(World* world, player::PlayerEntity* thrower);
  void tick() override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] int use();
  [[nodiscard]] float getShadowRadius() const override {
    return 0.0f;
  }
  [[nodiscard]] bool ignoresOwnerForHook() const {
    return inAirTime < 5;
  }
  player::PlayerEntity* owner = nullptr;
  Entity* hookedEntity = nullptr;
  int shake = 0;

private:
  int blockX = -1;
  int blockY = -1;
  int blockZ = -1;
  int blockId = 0;
  bool inGround = false;
  int removalTimer = 0;
  int inAirTime = 0;
  int hookCountdown = 0;
};
} // namespace net::minecraft::entity::projectile
