#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include <exception>
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/network/packet/LuaModSyncPacket.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::lua {
LuaModEntity::LuaModEntity(entity::World* world) : Entity(world) {
 getDataTracker().startTracking(kRegistryIdTrackerKey, std::string());
 getDataTracker().startTracking(kDataTrackerKey, entity::data::DataTrackerByteArray());
 width = 0.6f;
 height = 1.0f;
}
void LuaModEntity::setRegistryId(std::string id) {
 registryId_ = std::move(id);
 getDataTracker().set(kRegistryIdTrackerKey, registryId_);
 dirty_ = true;
}
void LuaModEntity::setData(const NbtCompound& value) {
 data_ = value;
 Nbt encoded = Nbt::compound();
 encoded.asCompound() = data_.storage().asCompound();
 getDataTracker().set(kDataTrackerKey, encoded.toBytes());
 dirty_ = true;
}
bool LuaModEntity::takeDirty() {
 bool d = dirty_;
 dirty_ = false;
 return d;
}
std::unique_ptr<Packet> LuaModEntity::createUpdatePacket() const {
 LuaModSnapshot snapshot;
 snapshot.id = id;
 snapshot.registryId = registryId_;
 snapshot.x = MathHelper::floor(x * 32.0);
 snapshot.y = MathHelper::floor(y * 32.0);
 snapshot.z = MathHelper::floor(z * 32.0);
 snapshot.yaw = MathHelper::floor(yaw * 256.0f / 360.0f);
 snapshot.pitch = MathHelper::floor(pitch * 256.0f / 360.0f);
 snapshot.data = data_;
 return std::make_unique<LuaModSyncPacket>(makeLuaModSnapshotPacket(snapshot, LuaModSyncKind::Entity));
}
void LuaModEntity::writeNbt(NbtCompound& nbt) const {
 Entity::writeNbt(nbt);
 nbt.putString("ModId", registryId_);
 nbt.put("Data", data_.storage());
}
void LuaModEntity::readNbt(const NbtCompound& nbt) {
 Entity::readNbt(nbt);
 setRegistryId(nbt.getString("ModId"));
 const Nbt* stored = nbt.storage().get("Data");
 if(stored != nullptr && stored->isCompound()) {
  setData(NbtCompound(*stored));
 } else {
  setData(NbtCompound());
 }
}
void LuaModEntity::onTrackedDataUpdated(int key) {
 if(key == kRegistryIdTrackerKey) {
  registryId_ = getDataTracker().getString(kRegistryIdTrackerKey);
  return;
 }
 if(key != kDataTrackerKey) {
  return;
 }
 try {
  const Nbt decoded = Nbt::read(getDataTracker().getByteArray(kDataTrackerKey));
  if(decoded.isCompound()) {
   data_ = NbtCompound(decoded);
  }
 } catch(const std::exception&) {
 }
}
void LuaModEntity::tick() {
 // Client replica: it is packet-driven, so simulating physics here would fight
 // the server's authoritative motion. Just glide toward the last interpolation
 // targets (MinecartEntity::tick's remote branch, ported verbatim in shape).
 if(world != nullptr && world->isRemote()) {
  prevX = x;
  prevY = y;
  prevZ = z;
  prevYaw = yaw;
  prevPitch = pitch;
  if(clientInterpolationSteps_ <= 0) {
   return;
  }
  const double interpX = x + (clientX_ - x) / static_cast<double>(clientInterpolationSteps_);
  const double interpY = y + (clientY_ - y) / static_cast<double>(clientInterpolationSteps_);
  const double interpZ = z + (clientZ_ - z) / static_cast<double>(clientInterpolationSteps_);
  double yawDelta = clientTargetYaw_ - static_cast<double>(yaw);
  while(yawDelta < -180.0) {
   yawDelta += 360.0;
  }
  while(yawDelta >= 180.0) {
   yawDelta -= 360.0;
  }
  yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta / static_cast<double>(clientInterpolationSteps_));
  pitch = static_cast<float>(static_cast<double>(pitch) + (clientTargetPitch_ - static_cast<double>(pitch)) /
                                                              static_cast<double>(clientInterpolationSteps_));
  --clientInterpolationSteps_;
  setPosition(interpX, interpY, interpZ);
  setRotation(yaw, pitch);
  return;
 }
 // Server: authoritative simulation.
 Entity::tick();
 prevX = x;
 prevY = y;
 prevZ = z;
 velocityY -= 0.04;
 move(velocityX, velocityY, velocityZ);
 velocityX *= 0.6;
 velocityZ *= 0.6;
 if(onGround) {
  velocityY = 0.0;
 } else {
  velocityY *= 0.98;
 }
}
void LuaModEntity::setPositionAndAnglesAvoidEntities(
    double xIn, double yIn, double zIn, float yawIn, float pitchIn, int interpolationSteps) {
 clientX_ = xIn;
 clientY_ = yIn;
 clientZ_ = zIn;
 clientTargetYaw_ = static_cast<double>(yawIn);
 clientTargetPitch_ = static_cast<double>(pitchIn);
 clientInterpolationSteps_ = interpolationSteps + 2;
}
void registerLuaModEntityType() {
 static bool registered = false;
 if(registered) {
  return;
 }
 registered = true;
 entity::EntityRegistry::registerType(typeid(LuaModEntity), "lua_mod_entity", 1000, [](entity::World* world) {
  return std::make_unique<LuaModEntity>(world);
 });
}
std::unique_ptr<net::minecraft::Packet> LuaModBlockEntity::createUpdatePacket() const {
 LuaModSnapshot snapshot;
 snapshot.x = x;
 snapshot.y = y;
 snapshot.z = z;
 snapshot.id = 0;
 snapshot.yaw = 0;
 snapshot.pitch = 0;
 snapshot.registryId = registryId_;
 snapshot.data = data_;
 return std::make_unique<LuaModSyncPacket>(makeLuaModSnapshotPacket(snapshot, LuaModSyncKind::BlockEntitySnapshot));
}
} // namespace net::minecraft::mod::lua
