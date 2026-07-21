#include "net/minecraft/entity/Entity.hpp"
#include <algorithm>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity {
int Entity::nextId() {
 static int id = 0;
 return id++;
}
Entity::Entity(World* worldIn) : id(nextId()), world(worldIn) {
 // Java: new Random() per entity; default seed 0 made every particle share prevU/prevV.
 random.setSeed(static_cast<std::uint64_t>(id));
 setPosition(0.0, 0.0, 0.0);
 dataTracker.startTracking(0, static_cast<std::int8_t>(0));
 initDataTracker();
}
void Entity::setBoundingBoxSpacing(float spacingXZ, float spacingY) {
 width = spacingXZ;
 height = spacingY;
}
void Entity::setRotation(float yawIn, float pitchIn) {
 yaw = std::fmod(yawIn, 360.0f);
 pitch = std::fmod(pitchIn, 360.0f);
}
void Entity::setPosition(double xIn, double yIn, double zIn) {
 x = xIn;
 y = yIn;
 z = zIn;
 const float halfWidth = width * 0.5f;
 boundingBox = {
     x - static_cast<double>(halfWidth),
     y - static_cast<double>(standingEyeHeight) + static_cast<double>(cameraOffset),
     z - static_cast<double>(halfWidth),
     x + static_cast<double>(halfWidth),
     y - static_cast<double>(standingEyeHeight) + static_cast<double>(cameraOffset) + static_cast<double>(height),
     z + static_cast<double>(halfWidth)};
}
void Entity::teleport(double xIn, double yIn, double zIn, float yawIn, float pitchIn) {
 mod::EntityTeleportEvent event;
 event.entity = this;
 event.world = world;
 event.fromX = x;
 event.fromY = y;
 event.fromZ = z;
 event.x = xIn;
 event.y = yIn;
 event.z = zIn;
 event.yaw = yawIn;
 event.pitch = pitchIn;
  net::minecraft::mod::runtime::luaHookEntityTeleport(event);
  if(event.canceled) {
   return;
  }
  setPositionAndAngles(event.x, event.y, event.z, event.yaw, event.pitch);
}
void Entity::changeLookDirection(float cursorDeltaX, float cursorDeltaY) {
 const float oldPitch = pitch;
 const float oldYaw = yaw;
 yaw = static_cast<float>(static_cast<double>(yaw) + static_cast<double>(cursorDeltaX) * 0.15);
 pitch = static_cast<float>(static_cast<double>(pitch) - static_cast<double>(cursorDeltaY) * 0.15);
 pitch = std::clamp(pitch, -90.0f, 90.0f);
 prevPitch += pitch - oldPitch;
 prevYaw += yaw - oldYaw;
}
void Entity::fall(double heightDifference, bool onGroundIn) {
 if(onGroundIn) {
  if(fallDistance > 0.0f) {
   onLanding(fallDistance);
   fallDistance = 0.0f;
  }
 } else if(heightDifference < 0.0) {
  fallDistance = static_cast<float>(static_cast<double>(fallDistance) - heightDifference);
 }
}
void Entity::moveNonSolid(float sideways, float forward, float speed) {
 float magnitude = MathHelper::sqrt(sideways * sideways + forward * forward);
 if(magnitude < 0.01f) {
  return;
 }
 if(magnitude < 1.0f) {
  magnitude = 1.0f;
 }
 magnitude = speed / magnitude;
 const float sinYaw = MathHelper::sin(yaw * kPiF / 180.0f);
 const float cosYaw = MathHelper::cos(yaw * kPiF / 180.0f);
 velocityX += static_cast<double>((sideways *= magnitude) * cosYaw - (forward *= magnitude) * sinYaw);
 velocityZ += static_cast<double>(forward * cosYaw + sideways * sinYaw);
}
void Entity::setPositionAndAngles(double xIn, double yIn, double zIn, float yawIn, float pitchIn) {
 prevX = x = xIn;
 prevY = y = yIn;
 prevZ = z = zIn;
 prevYaw = yaw = yawIn;
 prevPitch = pitch = pitchIn;
 cameraOffset = 0.0f;
 double deltaYaw = static_cast<double>(prevYaw - yawIn);
 if(deltaYaw < -180.0) {
  prevYaw += 360.0f;
 }
 if(deltaYaw >= 180.0) {
  prevYaw -= 360.0f;
 }
 setPosition(x, y, z);
 setRotation(yaw, pitch);
}
void Entity::setPositionAndAnglesKeepPrevAngles(double xIn, double yIn, double zIn, float yawIn, float pitchIn) {
 prevX = x = xIn;
 lastTickX = x;
 prevY = y = yIn + static_cast<double>(standingEyeHeight);
 lastTickY = y;
 prevZ = z = zIn;
 lastTickZ = z;
 yaw = yawIn;
 pitch = pitchIn;
 setPosition(x, y, z);
}
float Entity::getDistance(const Entity& entity) const {
 return MathHelper::sqrt(static_cast<float>((x - entity.x) * (x - entity.x) + (y - entity.y) * (y - entity.y) +
                                            (z - entity.z) * (z - entity.z)));
}
double Entity::getSquaredDistance(double xIn, double yIn, double zIn) const {
 const double dx = x - xIn;
 const double dy = y - yIn;
 const double dz = z - zIn;
 return dx * dx + dy * dy + dz * dz;
}
double Entity::getDistance(double xIn, double yIn, double zIn) const {
 return std::sqrt(getSquaredDistance(xIn, yIn, zIn));
}
double Entity::getSquaredDistance(const Entity& entity) const {
 return getSquaredDistance(entity.x, entity.y, entity.z);
}
std::optional<Box> Entity::getCollisionAgainstShape(Entity* /*other*/) const {
 return std::nullopt;
}
void Entity::onPlayerInteraction(player::PlayerEntity* player) {
 (void)player;
}
void Entity::onCollision(Entity* otherEntity) {
 if(otherEntity == nullptr || otherEntity->passenger == this || otherEntity->vehicle == this) {
  return;
 }
 double dx = otherEntity->x - x;
 double dz = otherEntity->z - z;
 double distance = MathHelper::absMax(dx, dz);
 if(distance < 0.01) {
  return;
 }
 distance = MathHelper::sqrt(distance);
 dx /= distance;
 dz /= distance;
 double push = 1.0 / distance;
 if(push > 1.0) {
  push = 1.0;
 }
 dx *= push;
 dz *= push;
 dx *= 0.05;
 dz *= 0.05;
 addVelocity(-(dx *= static_cast<double>(1.0f - pushSpeedReduction)),
             0.0,
             -(dz *= static_cast<double>(1.0f - pushSpeedReduction)));
 otherEntity->addVelocity(dx, 0.0, dz);
}
void Entity::addVelocity(double vx, double vy, double vz) {
 velocityX += vx;
 velocityY += vy;
 velocityZ += vz;
}
bool Entity::damage(Entity* damageSource, int amount) {
 (void)damageSource;
 (void)amount;
 scheduleVelocityUpdate();
 return false;
}
bool Entity::shouldRender(double distance) const {
 const double sideLength = std::max({boundingBox.maxX - boundingBox.minX,
                                     boundingBox.maxY - boundingBox.minY,
                                     boundingBox.maxZ - boundingBox.minZ});
 const double scaled = sideLength * 64.0 * renderDistanceMultiplier;
 return distance < scaled * scaled;
}
bool Entity::shouldRender(const Vec3d& pos) const {
 const double dx = x - pos.x;
 const double dy = y - pos.y;
 const double dz = z - pos.z;
 return shouldRender(dx * dx + dy * dy + dz * dz);
}
void Entity::writeNbt(NbtCompound& nbt) const {
 nbt.put("Pos", toNbtList(x, y, z));
 nbt.put("Motion", toNbtList(velocityX, velocityY, velocityZ));
 nbt.put("Rotation", toNbtList(yaw, pitch));
 nbt.putFloat("FallDistance", fallDistance);
 nbt.putShort("Fire", static_cast<std::int16_t>(fireTicks));
 nbt.putShort("Air", static_cast<std::int16_t>(air));
 nbt.putBoolean("OnGround", onGround);
}
void Entity::readNbt(const NbtCompound& nbt) {
 const Nbt& storage = nbt.storage();
 if(const Nbt* pos = storage.get("Pos"); pos != nullptr && pos->isList() && pos->asList().size() >= 3) {
  const auto& posList = pos->asList();
  lastTickX = x = posList[0].asDouble();
  prevX = x;
  lastTickY = y = posList[1].asDouble();
  prevY = y;
  lastTickZ = z = posList[2].asDouble();
  prevZ = z;
 }
 if(const Nbt* motion = storage.get("Motion");
    motion != nullptr && motion->isList() && motion->asList().size() >= 3) {
  const auto& motionList = motion->asList();
  velocityX = std::clamp(motionList[0].asDouble(), -10.0, 10.0);
  velocityY = std::clamp(motionList[1].asDouble(), -10.0, 10.0);
  velocityZ = std::clamp(motionList[2].asDouble(), -10.0, 10.0);
 }
 if(const Nbt* rotation = storage.get("Rotation");
    rotation != nullptr && rotation->isList() && rotation->asList().size() >= 2) {
  const auto& rotationList = rotation->asList();
  prevYaw = yaw = rotationList[0].asFloat();
  prevPitch = pitch = rotationList[1].asFloat();
 }
 fallDistance = nbt.getFloat("FallDistance");
 fireTicks = nbt.getShort("Fire");
 air = nbt.getShort("Air");
 onGround = nbt.getBoolean("OnGround");
 setPosition(x, y, z);
 setRotation(yaw, pitch);
}
bool Entity::interact(player::PlayerEntity* player) {
 (void)player;
 return false;
}
void Entity::tickRiding() {
 if(vehicle == nullptr) {
  return;
 }
 if(vehicle->dead) {
  vehicle = nullptr;
  return;
 }
 velocityX = 0.0;
 velocityY = 0.0;
 velocityZ = 0.0;
 tick();
 if(vehicle == nullptr) {
  return;
 }
 vehicle->updatePassengerPosition();
 vehicleYawDelta += static_cast<double>(vehicle->yaw - vehicle->prevYaw);
 vehiclePitchDelta += static_cast<double>(vehicle->pitch - vehicle->prevPitch);
 while(vehicleYawDelta >= 180.0) {
  vehicleYawDelta -= 360.0;
 }
 while(vehicleYawDelta < -180.0) {
  vehicleYawDelta += 360.0;
 }
 while(vehiclePitchDelta >= 180.0) {
  vehiclePitchDelta -= 360.0;
 }
 while(vehiclePitchDelta < -180.0) {
  vehiclePitchDelta += 360.0;
 }
 double yawDelta = std::clamp(vehicleYawDelta * 0.5, -10.0, 10.0);
 double pitchDelta = std::clamp(vehiclePitchDelta * 0.5, -10.0, 10.0);
 vehicleYawDelta -= yawDelta;
 vehiclePitchDelta -= pitchDelta;
 yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta);
 pitch = static_cast<float>(static_cast<double>(pitch) + pitchDelta);
}
void Entity::updatePassengerPosition() {
 if(passenger != nullptr) {
  passenger->setPosition(x, y + getPassengerRidingHeight() + passenger->getStandingEyeHeight(), z);
 }
}
void Entity::setVehicle(Entity* entity) {
 vehiclePitchDelta = 0.0;
 vehicleYawDelta = 0.0;
 if(entity == nullptr) {
  if(vehicle != nullptr) {
   setPositionAndAnglesKeepPrevAngles(
       vehicle->x, vehicle->boundingBox.minY + static_cast<double>(vehicle->height), vehicle->z, yaw, pitch);
   vehicle->passenger = nullptr;
  }
  vehicle = nullptr;
  return;
 }
 if(vehicle == entity) {
  vehicle->passenger = nullptr;
  vehicle = nullptr;
  setPositionAndAnglesKeepPrevAngles(
      entity->x, entity->boundingBox.minY + static_cast<double>(entity->height), entity->z, yaw, pitch);
  return;
 }
 if(vehicle != nullptr) {
  vehicle->passenger = nullptr;
 }
 if(entity->passenger != nullptr) {
  entity->passenger->vehicle = nullptr;
 }
 vehicle = entity;
 entity->passenger = this;
}
Vec3d Entity::getLookVector() const {
 const float cosYaw = MathHelper::cos(-yaw * (kPiF / 180.0f) - kPiF);
 const float sinYaw = MathHelper::sin(-yaw * (kPiF / 180.0f) - kPiF);
 const float cosPitch = -MathHelper::cos(-pitch * (kPiF / 180.0f));
 const float sinPitch = MathHelper::sin(-pitch * (kPiF / 180.0f));
 return {sinYaw * cosPitch, sinPitch, cosYaw * cosPitch};
}
void Entity::setVelocityClient(double xIn, double yIn, double zIn) {
 velocityX = xIn;
 velocityY = yIn;
 velocityZ = zIn;
}
void Entity::processServerEntityStatus(std::int8_t status) {
 (void)status;
}
void Entity::setEquipmentStack(int armorSlot, int itemId, int meta) {
 (void)armorSlot;
 (void)itemId;
 (void)meta;
}
bool Entity::isOnFire() const {
 return fireTicks > 0 || getFlag(0);
}
bool Entity::hasVehicle() const {
 return vehicle != nullptr || getFlag(2);
}
bool Entity::isSneaking() const {
 return getFlag(1);
}
void Entity::setSneaking(bool sneaking) {
 setFlag(1, sneaking);
}
bool Entity::getFlag(int index) const {
 return (dataTracker.getByte(0) & (1 << index)) != 0;
}
void Entity::setFlag(int index, bool value) {
 std::int8_t flags = dataTracker.getByte(0);
 if(value) {
  dataTracker.set(0, static_cast<std::int8_t>(flags | (1 << index)));
 } else {
  dataTracker.set(0, static_cast<std::int8_t>(flags & ~(1 << index)));
 }
}
void Entity::onStruckByLightning(Entity* lightning) {
 (void)lightning;
 damage(nullptr, 5);
 ++fireTicks;
 if(fireTicks == 0) {
  fireTicks = 300;
 }
}
void Entity::updateKilledAchievement(LivingEntity* entityKilled, int score) {
 (void)entityKilled;
 (void)score;
}
void Entity::onKilledOther(LivingEntity* other) {
 (void)other;
}
Nbt Entity::toNbtList(double a, double b, double c) const {
 Nbt list = Nbt::list();
 auto& entries = list.asList();
 entries.emplace_back(a);
 entries.emplace_back(b);
 entries.emplace_back(c);
 return list;
}
Nbt Entity::toNbtList(float a, float b) const {
 Nbt list = Nbt::list();
 auto& entries = list.asList();
 entries.emplace_back(a);
 entries.emplace_back(b);
 return list;
}
ItemEntity* Entity::dropItem(int id, int amount) {
 return dropItem(id, amount, 0.0f);
}
ItemEntity* Entity::dropItem(int id, int amount, float yOffset) {
 return dropItem(ItemStack(id, amount, 0), yOffset);
}
ItemEntity* Entity::dropItem(const ItemStack& itemStack, float yOffset) {
 if(world == nullptr || itemStack.empty()) {
  return nullptr;
 }
 auto* itemEntity = new ItemEntity(world, x, y + static_cast<double>(yOffset), z, itemStack);
 itemEntity->pickupDelay = 10;
 world->spawnEntity(itemEntity);
 return itemEntity;
}
bool Entity::saveSelfNbt(NbtCompound& nbt) const {
 const std::string id = EntityRegistry::getId(*this);
 if(dead || id.empty()) {
  return false;
 }
 nbt.putString("id", id);
 writeNbt(nbt);
 return true;
}
void Entity::baseTick() {
 if(vehicle != nullptr && vehicle->dead) {
  vehicle = nullptr;
 }
 ++age;
 prevHorizontalSpeed = horizontalSpeed;
 prevX = x;
 prevY = y;
 prevZ = z;
 prevPitch = pitch;
 prevYaw = yaw;
 if(checkWaterCollisions()) {
  if(!submergedInWater && !firstTick && world != nullptr) {
   float splashVolume =
       MathHelper::sqrt(static_cast<float>(velocityX * velocityX * 0.2 + velocityY * velocityY +
                                           velocityZ * velocityZ * 0.2)) *
       0.2f;
   if(splashVolume > 1.0f) {
    splashVolume = 1.0f;
   }
   world->playSound(
       this, "random.splash", splashVolume, 1.0f + (random.nextFloat() - random.nextFloat()) * 0.4f);
   const float surfaceY = static_cast<float>(MathHelper::floor(boundingBox.minY));
   for(int i = 0; i < 1 + static_cast<int>(width * 20.0f); ++i) {
    const float offsetX = (random.nextFloat() * 2.0f - 1.0f) * width;
    const float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * width;
    world->addParticle("bubble",
                       x + static_cast<double>(offsetX),
                       static_cast<double>(surfaceY + 1.0f),
                       z + static_cast<double>(offsetZ),
                       velocityX,
                       velocityY - static_cast<double>(random.nextFloat() * 0.2f),
                       velocityZ);
   }
   for(int i = 0; i < 1 + static_cast<int>(width * 20.0f); ++i) {
    const float offsetX = (random.nextFloat() * 2.0f - 1.0f) * width;
    const float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * width;
    world->addParticle("splash",
                       x + static_cast<double>(offsetX),
                       static_cast<double>(surfaceY + 1.0f),
                       z + static_cast<double>(offsetZ),
                       velocityX,
                       velocityY,
                       velocityZ);
   }
  }
  fallDistance = 0.0f;
  submergedInWater = true;
  fireTicks = 0;
 } else {
  submergedInWater = false;
 }
 if(world != nullptr && world->isRemote()) {
  fireTicks = 0;
 } else if(fireTicks > 0) {
  if(fireImmune) {
   fireTicks -= 4;
   if(fireTicks < 0) {
    fireTicks = 0;
   }
  } else {
   if(fireTicks % 20 == 0) {
    damage(nullptr, 1);
   }
   --fireTicks;
  }
 }
 if(isTouchingLava()) {
  if(!fireImmune) {
   damage(nullptr, 4);
   fireTicks = 600;
  }
 }
 if(y < -64.0) {
  tickInVoid();
 }
 if(world == nullptr || !world->isRemote()) {
  setFlag(0, fireTicks > 0);
  setFlag(2, vehicle != nullptr);
 }
 firstTick = false;
}
void Entity::nudgeOutOfCollision() {
 if(world == nullptr) {
  return;
 }
 while(y > 0.0) {
  setPosition(x, y, z);
  if(world->getEntityCollisions(this, boundingBox).empty()) {
   break;
  }
  y += 1.0;
 }
}
void Entity::teleportTop() {
 nudgeOutOfCollision();
 velocityX = 0.0;
 velocityY = 0.0;
 velocityZ = 0.0;
 pitch = 0.0f;
}
bool Entity::checkWaterCollisions() {
 if(world == nullptr) {
  return false;
 }
 return world->updateMovementInFluid(
     boundingBox.expand(0.0, -0.4, 0.0).contract(0.001, 0.001, 0.001), block::material::Material::WATER, this);
}
bool Entity::isWet() const {
 if(world == nullptr) {
  return false;
 }
 return submergedInWater || world->isRaining(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z));
}
bool Entity::getEntitiesInside(double offsetX, double offsetY, double offsetZ) {
 if(world == nullptr) {
  return false;
 }
 const Box probe = boundingBox.offset(offsetX, offsetY, offsetZ);
 if(!world->getEntityCollisions(this, probe).empty()) {
  return false;
 }
 return !world->isBoxSubmergedInFluid(probe);
}
bool Entity::isTouchingLava() const {
 if(world == nullptr) {
  return false;
 }
 return world->isMaterialInBox(boundingBox.expand(-0.1f), block::material::Material::LAVA);
}
bool Entity::isInsideWall() const {
 if(world == nullptr) {
  return false;
 }
 for(int i = 0; i < 8; ++i) {
  const float offsetX = (static_cast<float>((i >> 0) % 2) - 0.5f) * width * 0.9f;
  const float offsetY = (static_cast<float>((i >> 1) % 2) - 0.5f) * 0.1f;
  const float offsetZ = (static_cast<float>((i >> 2) % 2) - 0.5f) * width * 0.9f;
  const int bx = MathHelper::floor(x + static_cast<double>(offsetX));
  const int by = MathHelper::floor(y + static_cast<double>(getEyeHeight()) + static_cast<double>(offsetY));
  const int bz = MathHelper::floor(z + static_cast<double>(offsetZ));
  if(world->shouldSuffocate(bx, by, bz)) {
   return true;
  }
 }
 return false;
}
float Entity::getBrightnessAtEyes(float tickDelta) const {
 (void)tickDelta;
 if(world == nullptr) {
  return minBrightness;
 }
 const int blockX = MathHelper::floor(x);
 const double eyeSpan = (boundingBox.maxY - boundingBox.minY) * 0.66;
 const int blockY = MathHelper::floor(y - static_cast<double>(standingEyeHeight) + eyeSpan);
 const int blockZ = MathHelper::floor(z);
 if(!world->isRegionLoaded(MathHelper::floor(boundingBox.minX),
                           MathHelper::floor(boundingBox.minY),
                           MathHelper::floor(boundingBox.minZ),
                           MathHelper::floor(boundingBox.maxX),
                           MathHelper::floor(boundingBox.maxY),
                           MathHelper::floor(boundingBox.maxZ))) {
  return minBrightness;
 }
 float brightness = world->getLightBrightness(blockX, blockY, blockZ);
 if(brightness < minBrightness) {
  brightness = minBrightness;
 }
 return brightness;
}
void Entity::tickInVoid() {
 markDead();
}
bool Entity::pushOutOfBlock(double px, double py, double pz) {
 if(world == nullptr) {
  return false;
 }
 const int blockX = MathHelper::floor(px);
 const int blockY = MathHelper::floor(py);
 const int blockZ = MathHelper::floor(pz);
 const double offsetX = px - static_cast<double>(blockX);
 const double offsetY = py - static_cast<double>(blockY);
 const double offsetZ = pz - static_cast<double>(blockZ);
 if(!world->shouldSuffocate(blockX, blockY, blockZ)) {
  return false;
 }
 const bool westFree = !world->shouldSuffocate(blockX - 1, blockY, blockZ);
 const bool eastFree = !world->shouldSuffocate(blockX + 1, blockY, blockZ);
 const bool downFree = !world->shouldSuffocate(blockX, blockY - 1, blockZ);
 const bool upFree = !world->shouldSuffocate(blockX, blockY + 1, blockZ);
 const bool northFree = !world->shouldSuffocate(blockX, blockY, blockZ - 1);
 const bool southFree = !world->shouldSuffocate(blockX, blockY, blockZ + 1);
 int direction = -1;
 double closestDistance = 9999.0;
 if(westFree && offsetX < closestDistance) {
  closestDistance = offsetX;
  direction = 0;
 }
 if(eastFree && 1.0 - offsetX < closestDistance) {
  closestDistance = 1.0 - offsetX;
  direction = 1;
 }
 if(downFree && offsetY < closestDistance) {
  closestDistance = offsetY;
  direction = 2;
 }
 if(upFree && 1.0 - offsetY < closestDistance) {
  closestDistance = 1.0 - offsetY;
  direction = 3;
 }
 if(northFree && offsetZ < closestDistance) {
  closestDistance = offsetZ;
  direction = 4;
 }
 if(southFree && 1.0 - offsetZ < closestDistance) {
  direction = 5;
 }
 const float impulse = random.nextFloat() * 0.2f + 0.1f;
 if(direction == 0) {
  velocityX = -static_cast<double>(impulse);
 } else if(direction == 1) {
  velocityX = static_cast<double>(impulse);
 } else if(direction == 2) {
  velocityY = -static_cast<double>(impulse);
 } else if(direction == 3) {
  velocityY = static_cast<double>(impulse);
 } else if(direction == 4) {
  velocityZ = -static_cast<double>(impulse);
 } else if(direction == 5) {
  velocityZ = static_cast<double>(impulse);
 }
 return false;
}
void Entity::setPositionAndAnglesAvoidEntities(
    double xIn, double yIn, double zIn, float yawIn, float pitchIn, int interpolationSteps) {
 (void)interpolationSteps;
 setPosition(xIn, yIn, zIn);
 setRotation(yawIn, pitchIn);
 if(world == nullptr) {
  return;
 }
 const std::vector<Box> collisions = world->getEntityCollisions(this, boundingBox.contract(0.03125, 0.0, 0.03125));
 if(collisions.empty()) {
  return;
 }
 double highestCollision = 0.0;
 for(const Box& box : collisions) {
  if(box.maxY > highestCollision) {
   highestCollision = box.maxY;
  }
 }
 setPosition(xIn, yIn + highestCollision - boundingBox.minY, zIn);
}
} // namespace net::minecraft::entity
