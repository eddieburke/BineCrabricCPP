#pragma once
#include <cstdint>
#include <string>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity {
class LivingEntity : public Entity {
public:
  static constexpr int kEntityId = 48;
  static constexpr const char* kEntityName = "Mob";
  explicit LivingEntity(World* world = nullptr);
  int maxHealth = 20;
  float field1010 = 0.0f;
  float field1011 = 0.0f;
  float bodyYaw = 0.0f;
  float lastBodyYaw = 0.0f;
  float lastWalkProgress = 0.0f;
  float walkProgress = 0.0f;
  float field1016 = 0.0f;
  float field1017 = 0.0f;
  bool field1018 = true;
  std::string texture = "/mob/char.png";
  bool field1020 = true;
  float rotationOffset = 0.0f;
  std::string modelName;
  float field1023 = 1.0f;
  int scoreAmount = 0;
  float damageAmount = 0.0f;
  bool interpolateOnly = false;
  float lastSwingAnimationProgress = 0.0f;
  float swingAnimationProgress = 0.0f;
  int health = 10;
  int lastHealth = 0;
  int ambientSoundTimer = 0;
  int hurtTime = 0;
  int damagedTime = 0;
  float damagedSwingDir = 0.0f;
  int deathTime = 0;
  int attackCooldown = 0;
  float prevTilt = 0.0f;
  float tilt = 0.0f;
  bool killedByOtherEntity = false;
  int field1046 = -1;
  float field1047 = 0.0f;
  float lastWalkAnimationSpeed = 0.0f;
  float walkAnimationSpeed = 0.0f;
  float walkAnimationProgress = 0.0f;
  int bodyTrackingIncrements = 0;
  double lerpX = 0.0;
  double lerpY = 0.0;
  double lerpZ = 0.0;
  double lerpYaw = 0.0;
  double lerpPitch = 0.0;
  float field1057 = 0.0f;
  int prevHealth = 0;
  int despawnCounter = 0;
  float sidewaysSpeed = 0.0f;
  float forwardSpeed = 0.0f;
  float rotationSpeed = 0.0f;
  bool jumping = false;
  float defaultPitch = 0.0f;
  float movementSpeed = 0.7f;
  Entity* lookTarget = nullptr;
  int lookTimer = 0;
  [[nodiscard]] std::string getTexture() const override;
  [[nodiscard]] bool isCollidable() const override;
  [[nodiscard]] bool isPushable() const override;
  [[nodiscard]] float getEyeHeight() const override;
  [[nodiscard]] virtual int getMinAmbientSoundDelay() const;
  virtual void makeSound();
  void baseTick() override;
  void tick() override;
  void tickRiding() override;
  void setPositionAndAnglesAvoidEntities(
      double xIn, double yIn, double zIn, float yawIn, float pitchIn, int interpolationSteps) override;
  virtual void initializeSpawnState(JavaRandom& random);
  void heal(int amount);
  bool damage(Entity* damageSource, int amount) override;
  void animateHurt() override;
  void animateSpawn();
  virtual void applyDamage(int amount);
  [[nodiscard]] virtual float getSoundVolume() const;
  [[nodiscard]] virtual std::string getRandomSound();
  [[nodiscard]] virtual std::string getHurtSound() const;
  [[nodiscard]] virtual std::string getDeathSound() const;
  virtual void applyKnockback(Entity* attacker, int amount, double dx, double dz);
  virtual void onKilledBy(Entity* adversary);
  [[nodiscard]] bool canSee(Entity* entity) const;
  // Single item id (0-2 rolls): override getDroppedItemId(). Stacks/meta or multiple items: override dropItems().
  virtual void dropItems();
  [[nodiscard]] virtual int getDroppedItemId() const;
  void onLanding(float fallDistanceIn);
  virtual void travel(float sideways, float forward);
  [[nodiscard]] virtual bool isOnLadder() const;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] bool isAlive() const override;
  [[nodiscard]] virtual bool canBreatheInWater() const;
  virtual void tickMovement();
  [[nodiscard]] virtual bool isImmobile() const;
  virtual void jump();
  [[nodiscard]] virtual bool canDespawn() const;
  virtual void tryDespawn();
  virtual void tickLiving();
  [[nodiscard]] virtual int getMaxLookPitchChange() const;
  virtual void lookAt(Entity* target, float maxPitch, float maxYaw);
  [[nodiscard]] bool hasLookTarget() const;
  [[nodiscard]] Entity* getLookTarget() const;
  virtual void beforeRemove();
  [[nodiscard]] virtual bool canSpawn() const;
  void tickInVoid() override;
  [[nodiscard]] Vec3d getPosition(float tickDelta) const;
  [[nodiscard]] Vec3d getLookVector(float tickDelta) const;
  [[nodiscard]] Vec3d getLookVector() const override;
  [[nodiscard]] virtual int getLimitPerChunk() const;
  [[nodiscard]] virtual ItemStack getHeldItem() const;
  void processServerEntityStatus(std::int8_t status) override;
  [[nodiscard]] virtual bool isSleeping() const;
  [[nodiscard]] virtual int getItemStackTextureId(const ItemStack& stack) const;
  [[nodiscard]] float getHandSwingProgress(float tickDelta) const;

private:
  [[nodiscard]] float lerpRotation(float from, float to, float maxChange) const;
};
} // namespace net::minecraft::entity
