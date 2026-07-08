#pragma once
#include "net/minecraft/entity/data/DataTracker.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <typeindex>
#include <vector>
namespace net::minecraft::block::material {
class Material;
}
namespace net::minecraft {
class World;
struct ItemStack;
} // namespace net::minecraft
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::entity {
using World = ::net::minecraft::World;
using data::DataTracker;
using util::math::kPiF;
class LivingEntity;
class ItemEntity;
class Entity {
public:
  static int nextId();
  explicit Entity(World* world = nullptr);
  virtual ~Entity() = default;
  [[nodiscard]] virtual std::type_index runtimeType() const {
    return typeid(*this);
  }
  int id = 0;
  double renderDistanceMultiplier = 1.0;
  bool blocksSameBlockSpawning = false;
  Entity* passenger = nullptr;
  Entity* vehicle = nullptr;
  World* world = nullptr;
  double prevX = 0.0;
  double prevY = 0.0;
  double prevZ = 0.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double velocityX = 0.0;
  double velocityY = 0.0;
  double velocityZ = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float prevYaw = 0.0f;
  float prevPitch = 0.0f;
  Box boundingBox{};
  bool onGround = false;
  bool horizontalCollision = false;
  bool verticalCollision = false;
  bool hasCollided = false;
  bool velocityModified = false;
  bool slowed = false;
  bool keepVelocityOnCollision = true;
  bool dead = false;
  float standingEyeHeight = 0.0f;
  float width = 0.6f;
  float height = 1.8f;
  float prevHorizontalSpeed = 0.0f;
  float horizontalSpeed = 0.0f;
  float fallDistance = 0.0f;
  int nextStepSoundDistance = 1;
  double lastTickX = 0.0;
  double lastTickY = 0.0;
  double lastTickZ = 0.0;
  float cameraOffset = 0.0f;
  float stepHeight = 0.0f;
  bool noClip = false;
  float pushSpeedReduction = 0.0f;
  JavaRandom random{};
  int age = 0;
  int fireImmunityTicks = 1;
  int fireTicks = 0;
  int maxAir = 300;
  bool submergedInWater = false;
  int hearts = 0;
  int air = 300;
  bool firstTick = true;
  std::string skinUrl;
  std::string capeUrl;
  bool slimArms = false;
  bool fireImmune = false;
  DataTracker dataTracker;
  float minBrightness = 0.0f;
  double vehiclePitchDelta = 0.0;
  double vehicleYawDelta = 0.0;
  bool isPersistent = false;
  int chunkX = 0;
  int chunkSlice = 0;
  int chunkZ = 0;
  int trackedPosX = 0;
  int trackedPosY = 0;
  int trackedPosZ = 0;
  bool ignoreFrustumCull = false;
  void setWorld(World* worldIn) noexcept {
    world = worldIn;
  }
  [[nodiscard]] Vec3d position() const noexcept {
    return {x, y, z};
  }
  [[nodiscard]] Vec3d velocity() const noexcept {
    return {velocityX, velocityY, velocityZ};
  }
  [[nodiscard]] DataTracker& getDataTracker() noexcept {
    return dataTracker;
  }
  [[nodiscard]] const DataTracker& getDataTracker() const noexcept {
    return dataTracker;
  }
  virtual void initDataTracker() {}
  virtual void teleportTop();
  void teleport(double xIn, double yIn, double zIn, float yawIn, float pitchIn);
  void nudgeOutOfCollision();
  virtual void markDead() {
    dead = true;
  }
  void setBoundingBoxSpacing(float spacingXZ, float spacingY);
  void setRotation(float yawIn, float pitchIn);
  void setPosition(double xIn, double yIn, double zIn);
  void changeLookDirection(float cursorDeltaX, float cursorDeltaY);
  virtual void tick() {
    baseTick();
  }
  virtual void baseTick();
  virtual void move(double dx, double dy, double dz);
  virtual void fall(double heightDifference, bool onGroundIn);
  virtual void onLanding(float fallDistanceIn) {
    (void)fallDistanceIn;
  }
  void moveNonSolid(float sideways, float forward, float speed);
  void setPositionAndAngles(double xIn, double yIn, double zIn, float yawIn, float pitchIn);
  void setPositionAndAnglesKeepPrevAngles(double xIn, double yIn, double zIn, float yawIn, float pitchIn);
  virtual void setPositionAndAnglesAvoidEntities(double xIn, double yIn, double zIn, float yawIn, float pitchIn,
                                                 int interpolationSteps);
  [[nodiscard]] float getDistance(const Entity& entity) const;
  [[nodiscard]] double getSquaredDistance(double xIn, double yIn, double zIn) const;
  [[nodiscard]] double getDistance(double xIn, double yIn, double zIn) const;
  [[nodiscard]] double getSquaredDistance(const Entity& entity) const;
  [[nodiscard]] virtual float getEyeHeight() const {
    return 0.0f;
  }
  [[nodiscard]] virtual float getBrightnessAtEyes(float tickDelta) const;
  [[nodiscard]] virtual bool isSubmergedInWater() const {
    return submergedInWater;
  }
  [[nodiscard]] virtual bool isTouchingLava() const;
  [[nodiscard]] bool isInFluid(block::material::Material& material) const;
  [[nodiscard]] virtual bool isCollidable() const {
    return false;
  }
  [[nodiscard]] virtual float getTargetingMargin() const {
    return 0.1f;
  }
  [[nodiscard]] virtual bool isPushable() const {
    return false;
  }
  [[nodiscard]] virtual std::optional<Box> getBoundingBox() const {
    return std::nullopt;
  }
  [[nodiscard]] virtual std::optional<Box> getCollisionAgainstShape(Entity* other) const;
  [[nodiscard]] virtual std::string getTexture() const {
    return {};
  }
  virtual void onPlayerInteraction(player::PlayerEntity* player);
  virtual void onCollision(Entity* otherEntity);
  void addVelocity(double vx, double vy, double vz);
  virtual void scheduleVelocityUpdate() {
    velocityModified = true;
  }
  virtual bool damage(Entity* damageSource, int amount);
  [[nodiscard]] virtual bool shouldRender(double distance) const;
  [[nodiscard]] bool shouldRender(const Vec3d& pos) const;
  [[nodiscard]] bool saveSelfNbt(NbtCompound& nbt) const;
  virtual void writeNbt(NbtCompound& nbt) const;
  virtual void readNbt(const NbtCompound& nbt);
  [[nodiscard]] virtual float getShadowRadius() const {
    return height * 0.5f;
  }
  [[nodiscard]] virtual bool isAlive() const {
    return !dead;
  }
  ItemEntity* dropItem(int id, int amount);
  ItemEntity* dropItem(int id, int amount, float yOffset);
  ItemEntity* dropItem(const ItemStack& itemStack, float yOffset);
  [[nodiscard]] virtual bool isInsideWall() const;
  virtual bool interact(player::PlayerEntity* player);
  virtual void tickRiding();
  virtual void updatePassengerPosition();
  [[nodiscard]] double getStandingEyeHeight() const {
    return standingEyeHeight;
  }
  [[nodiscard]] virtual double getPassengerRidingHeight() const {
    return static_cast<double>(height) * 0.75;
  }
  void setVehicle(Entity* entity);
  [[nodiscard]] virtual Vec3d getLookVector() const;
  virtual void tickPortalCooldown() {}
  virtual void setVelocityClient(double xIn, double yIn, double zIn);
  virtual void processServerEntityStatus(std::int8_t status);
  virtual void animateHurt() {}
  virtual void updateCapeUrl() {}
  virtual void setEquipmentStack(int armorSlot, int itemId, int meta);
  [[nodiscard]] virtual bool isOnFire() const;
  [[nodiscard]] virtual bool hasVehicle() const;
  [[nodiscard]] virtual std::vector<ItemStack> getEquipment() const {
    return {};
  }
  [[nodiscard]] virtual bool isSneaking() const;
  virtual void setSneaking(bool sneaking);
  [[nodiscard]] bool getFlag(int index) const;
  void setFlag(int index, bool value);
  virtual void onStruckByLightning(Entity* lightning);
  virtual void updateKilledAchievement(LivingEntity* entityKilled, int score);
  virtual void onKilledOther(LivingEntity* other);
  [[nodiscard]] virtual bool bypassesSteppingEffects() const {
    return true;
  }
  [[nodiscard]] virtual bool checkWaterCollisions();
  [[nodiscard]] bool isWet() const;
  [[nodiscard]] bool getEntitiesInside(double offsetX, double offsetY, double offsetZ);
  virtual void tickInVoid();
  virtual bool pushOutOfBlock(double x, double y, double z);

protected:
  [[nodiscard]] Nbt toNbtList(double a, double b, double c) const;
  [[nodiscard]] Nbt toNbtList(float a, float b) const;
};
} // namespace net::minecraft::entity
