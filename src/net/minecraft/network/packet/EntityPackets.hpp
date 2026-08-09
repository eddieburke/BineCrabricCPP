#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "net/minecraft/entity/data/DataTracker.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
class EntityAnimationPacket : public Packet {
 public:
 int id = 0;
 int animationId = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  animationId = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(animationId));
 }
  void apply(NetworkHandler& networkHandler) const override {
   networkHandler.onEntityAnimation(*this);
  }
  [[nodiscard]] std::size_t size() const override {
   return 5;
  }
 };
class PlayerSpawnS2CPacket : public Packet {
 public:
 int id = 0;
 std::string name;
 int x = 0;
 int y = 0;
 int z = 0;
 std::int8_t yaw = 0;
 std::int8_t pitch = 0;
 int itemRawId = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  name = Packet::readString(src, end, 16);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  yaw = packetio::readI8(src, end);
  pitch = packetio::readI8(src, end);
  itemRawId = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  Packet::writeString(name, dest, end);
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeI8(dest, end, yaw);
  packetio::writeI8(dest, end, pitch);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemRawId));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerSpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 20U + packetio::javaStringSize(name);
 }
};
class ItemEntitySpawnS2CPacket : public Packet {
 public:
 int id = 0;
 int itemRawId = 0;
 int itemCount = 0;
 int itemDamage = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 std::int8_t velocityX = 0;
 std::int8_t velocityY = 0;
 std::int8_t velocityZ = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  itemRawId = packetio::readI16BE(src, end);
  itemCount = packetio::readI8(src, end);
  itemDamage = packetio::readI16BE(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  velocityX = packetio::readI8(src, end);
  velocityY = packetio::readI8(src, end);
  velocityZ = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemRawId));
  packetio::writeI8(dest, end, static_cast<std::int8_t>(itemCount));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemDamage));
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeI8(dest, end, velocityX);
  packetio::writeI8(dest, end, velocityY);
  packetio::writeI8(dest, end, velocityZ);
 }
  void apply(NetworkHandler& networkHandler) const override {
   networkHandler.onItemEntitySpawn(*this);
  }
  [[nodiscard]] std::size_t size() const override {
   return 24;
  }
 };
class ItemPickupAnimationS2CPacket : public Packet {
 public:
 int entityId = 0;
 int collectorEntityId = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  entityId = packetio::readI32BE(src, end);
  collectorEntityId = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, entityId);
  packetio::writeI32BE(dest, end, collectorEntityId);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onItemPickupAnimation(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 8;
 }
};
class EntitySpawnS2CPacket : public Packet {
 public:
 int id = 0;
 int entityType = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int velocityX = 0;
 int velocityY = 0;
 int velocityZ = 0;
 int entityData = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  entityType = packetio::readI8(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  entityData = packetio::readI32BE(src, end);
  if(entityData > 0) {
   velocityX = packetio::readI16BE(src, end);
   velocityY = packetio::readI16BE(src, end);
   velocityZ = packetio::readI16BE(src, end);
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(entityType));
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeI32BE(dest, end, entityData);
  if(entityData > 0) {
   packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityX));
   packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityY));
   packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityZ));
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntitySpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return static_cast<std::size_t>(21 + (entityData > 0 ? 6 : 0));
 }
};
class LivingEntitySpawnS2CPacket : public Packet {
 public:
 int id = 0;
 std::int8_t entityType = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 std::int8_t yaw = 0;
 std::int8_t pitch = 0;
 std::vector<entity::data::DataTrackerEntry> trackedValues;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  entityType = packetio::readI8(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  yaw = packetio::readI8(src, end);
  pitch = packetio::readI8(src, end);
  trackedValues = entity::data::DataTracker::readEntries(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, entityType);
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeI8(dest, end, yaw);
  packetio::writeI8(dest, end, pitch);
  entity::data::DataTracker::writeEntries(trackedValues, dest, end);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onLivingEntitySpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 19U + entity::data::DataTracker::sizeOfEntries(trackedValues);
 }
};
class PaintingEntitySpawnS2CPacket : public Packet {
 public:
 static constexpr int LONGEST_VARIANT_NAME = 13;
 int id = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int facing = 0;
 std::string variant;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  variant = Packet::readString(src, end, LONGEST_VARIANT_NAME);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  facing = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  Packet::writeString(variant, dest, end);
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeI32BE(dest, end, facing);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPaintingEntitySpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 20U + packetio::javaStringSize(variant);
 }
};
class EntityVelocityUpdateS2CPacket : public Packet {
 public:
 int id = 0;
 int velocityX = 0;
 int velocityY = 0;
 int velocityZ = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  velocityX = packetio::readI16BE(src, end);
  velocityY = packetio::readI16BE(src, end);
  velocityZ = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityX));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityY));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(velocityZ));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityVelocityUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 10;
 }
};
class EntityDestroyS2CPacket : public Packet {
 public:
 int id = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityDestroy(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4;
 }
};
class EntityS2CPacket : public Packet {
 public:
 int id = 0;
 std::int8_t deltaX = 0;
 std::int8_t deltaY = 0;
 std::int8_t deltaZ = 0;
 std::int8_t yaw = 0;
 std::int8_t pitch = 0;
 bool rotate = false;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntity(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4;
 }
};
class EntityMoveRelativeS2CPacket : public EntityS2CPacket {
 public:
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  EntityS2CPacket::read(src, end);
  deltaX = packetio::readI8(src, end);
  deltaY = packetio::readI8(src, end);
  deltaZ = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  EntityS2CPacket::write(dest, end);
  packetio::writeI8(dest, end, deltaX);
  packetio::writeI8(dest, end, deltaY);
  packetio::writeI8(dest, end, deltaZ);
 }
 [[nodiscard]] std::size_t size() const override {
  return 7;
 }
};
class EntityRotateS2CPacket : public EntityS2CPacket {
 public:
 EntityRotateS2CPacket() {
  rotate = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  EntityS2CPacket::read(src, end);
  yaw = packetio::readI8(src, end);
  pitch = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  EntityS2CPacket::write(dest, end);
  packetio::writeI8(dest, end, yaw);
  packetio::writeI8(dest, end, pitch);
 }
 [[nodiscard]] std::size_t size() const override {
  return 6;
 }
};
class EntityRotateAndMoveRelativeS2CPacket : public EntityS2CPacket {
 public:
 EntityRotateAndMoveRelativeS2CPacket() {
  rotate = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  EntityS2CPacket::read(src, end);
  deltaX = packetio::readI8(src, end);
  deltaY = packetio::readI8(src, end);
  deltaZ = packetio::readI8(src, end);
  yaw = packetio::readI8(src, end);
  pitch = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  EntityS2CPacket::write(dest, end);
  packetio::writeI8(dest, end, deltaX);
  packetio::writeI8(dest, end, deltaY);
  packetio::writeI8(dest, end, deltaZ);
  packetio::writeI8(dest, end, yaw);
  packetio::writeI8(dest, end, pitch);
 }
 [[nodiscard]] std::size_t size() const override {
  return 9;
 }
};
class EntityPositionS2CPacket : public Packet {
 public:
 int id = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 std::int8_t yaw = 0;
 std::int8_t pitch = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  yaw = static_cast<std::int8_t>(packetio::readU8(src, end));
  pitch = static_cast<std::int8_t>(packetio::readU8(src, end));
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(yaw));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(pitch));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityPosition(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 18;
 }
};
class EntityStatusS2CPacket : public Packet {
 public:
 int id = 0;
 std::int8_t status = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  status = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, status);
 }
  void apply(NetworkHandler& networkHandler) const override {
   networkHandler.onEntityStatus(*this);
  }
  [[nodiscard]] std::size_t size() const override {
   return 5;
  }
 };
class EntityVehicleSetS2CPacket : public Packet {
 public:
 int id = 0;
 int vehicleId = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  vehicleId = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI32BE(dest, end, vehicleId);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityVehicleSet(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 8;
 }
};
class EntityTrackerUpdateS2CPacket : public Packet {
 public:
 int id = 0;
 std::vector<entity::data::DataTrackerEntry> trackedValues;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  trackedValues = entity::data::DataTracker::readEntries(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  entity::data::DataTracker::writeEntries(trackedValues, dest, end);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityTrackerUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4U + entity::data::DataTracker::sizeOfEntries(trackedValues);
 }
};
class EntityEquipmentUpdateS2CPacket : public Packet {
 public:
 int id = 0;
 int slot = 0;
 int itemRawId = 0;
 int itemDamage = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  slot = packetio::readI16BE(src, end);
  itemRawId = packetio::readI16BE(src, end);
  itemDamage = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(slot));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemRawId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemDamage));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onEntityEquipmentUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 10;
 }
};
} // namespace net::minecraft
