#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::entity::projectile {
class ArrowEntity : public Entity {
 public:
 static constexpr int kEntityId = 10;
 static constexpr const char* kEntityName = "Arrow";
 struct ClientRenderer {
  static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
 };
 explicit ArrowEntity(World* world = nullptr) : Entity(world) {
  setBoundingBoxSpacing(0.5f, 0.5f);
 }
 ArrowEntity(World* world, double x, double y, double z) : Entity(world) {
  setBoundingBoxSpacing(0.5f, 0.5f);
  setPosition(x, y, z);
  standingEyeHeight = 0.0f;
 }
 ArrowEntity(World* world, LivingEntity* ownerIn);
 void tick() override;
 void onPlayerInteraction(player::PlayerEntity* player) override;
 [[nodiscard]] float getShadowRadius() const override {
  return 0.0f;
 }
 void writeNbt(NbtCompound& nbt) const override {
  Entity::writeNbt(nbt);
  nbt.putShort("xTile", static_cast<std::int16_t>(blockX));
  nbt.putShort("yTile", static_cast<std::int16_t>(blockY));
  nbt.putShort("zTile", static_cast<std::int16_t>(blockZ));
  nbt.putByte("inTile", static_cast<std::int8_t>(blockId));
  nbt.putByte("inData", static_cast<std::int8_t>(blockMeta));
  nbt.putByte("shake", static_cast<std::int8_t>(shake));
  nbt.putByte("inGround", static_cast<std::int8_t>(inGround ? 1 : 0));
  nbt.putBoolean("player", pickupAllowed);
 }
 void readNbt(const NbtCompound& nbt) override {
  Entity::readNbt(nbt);
  blockX = nbt.getShort("xTile");
  blockY = nbt.getShort("yTile");
  blockZ = nbt.getShort("zTile");
  blockId = static_cast<int>(nbt.getByte("inTile")) & 0xFF;
  blockMeta = static_cast<int>(nbt.getByte("inData")) & 0xFF;
  shake = static_cast<int>(nbt.getByte("shake")) & 0xFF;
  inGround = nbt.getByte("inGround") == 1;
  pickupAllowed = nbt.getBoolean("player");
 }
 LivingEntity* owner = nullptr;
 int blockX = -1;
 int blockY = -1;
 int blockZ = -1;
 int blockId = 0;
 int blockMeta = 0;
 bool inGround = false;
 int life = 0;
 int inAirTime = 0;
 int shake = 0;
 bool pickupAllowed = false;
};
} // namespace net::minecraft::entity::projectile
