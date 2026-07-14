#pragma once
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    animationId = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI8(output, static_cast<std::int8_t>(animationId));
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    name = Packet::readString(input, 16);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    yaw = packetio::readI8(input);
    pitch = packetio::readI8(input);
    itemRawId = packetio::readI16BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    Packet::writeString(name, output);
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeI8(output, yaw);
    packetio::writeI8(output, pitch);
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemRawId));
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onPlayerSpawn(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 28;
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    itemRawId = packetio::readI16BE(input);
    itemCount = packetio::readI8(input);
    itemDamage = packetio::readI16BE(input);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    velocityX = packetio::readI8(input);
    velocityY = packetio::readI8(input);
    velocityZ = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemRawId));
    packetio::writeI8(output, static_cast<std::int8_t>(itemCount));
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemDamage));
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeI8(output, velocityX);
    packetio::writeI8(output, velocityY);
    packetio::writeI8(output, velocityZ);
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
  void read(std::istream& input) override {
    entityId = packetio::readI32BE(input);
    collectorEntityId = packetio::readI32BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, entityId);
    packetio::writeI32BE(output, collectorEntityId);
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    entityType = packetio::readI8(input);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    entityData = packetio::readI32BE(input);
    if(entityData > 0) {
      velocityX = packetio::readI16BE(input);
      velocityY = packetio::readI16BE(input);
      velocityZ = packetio::readI16BE(input);
    }
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI8(output, static_cast<std::int8_t>(entityType));
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeI32BE(output, entityData);
    if(entityData > 0) {
      packetio::writeI16BE(output, static_cast<std::int16_t>(velocityX));
      packetio::writeI16BE(output, static_cast<std::int16_t>(velocityY));
      packetio::writeI16BE(output, static_cast<std::int16_t>(velocityZ));
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    entityType = packetio::readI8(input);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    yaw = packetio::readI8(input);
    pitch = packetio::readI8(input);
    trackedValues = entity::data::DataTracker::readEntries(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI8(output, entityType);
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeI8(output, yaw);
    packetio::writeI8(output, pitch);
    entity::data::DataTracker::writeEntries(trackedValues, output);
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onLivingEntitySpawn(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 20;
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    variant = Packet::readString(input, LONGEST_VARIANT_NAME);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    facing = packetio::readI32BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    Packet::writeString(variant, output);
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeI32BE(output, facing);
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onPaintingEntitySpawn(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 24;
  }
};
class EntityVelocityUpdateS2CPacket : public Packet {
public:
  int id = 0;
  int velocityX = 0;
  int velocityY = 0;
  int velocityZ = 0;
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    velocityX = packetio::readI16BE(input);
    velocityY = packetio::readI16BE(input);
    velocityZ = packetio::readI16BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI16BE(output, static_cast<std::int16_t>(velocityX));
    packetio::writeI16BE(output, static_cast<std::int16_t>(velocityY));
    packetio::writeI16BE(output, static_cast<std::int16_t>(velocityZ));
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
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
  void read(std::istream& input) override {
    EntityS2CPacket::read(input);
    deltaX = packetio::readI8(input);
    deltaY = packetio::readI8(input);
    deltaZ = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    EntityS2CPacket::write(output);
    packetio::writeI8(output, deltaX);
    packetio::writeI8(output, deltaY);
    packetio::writeI8(output, deltaZ);
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
  void read(std::istream& input) override {
    EntityS2CPacket::read(input);
    yaw = packetio::readI8(input);
    pitch = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    EntityS2CPacket::write(output);
    packetio::writeI8(output, yaw);
    packetio::writeI8(output, pitch);
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
  void read(std::istream& input) override {
    EntityS2CPacket::read(input);
    deltaX = packetio::readI8(input);
    deltaY = packetio::readI8(input);
    deltaZ = packetio::readI8(input);
    yaw = packetio::readI8(input);
    pitch = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    EntityS2CPacket::write(output);
    packetio::writeI8(output, deltaX);
    packetio::writeI8(output, deltaY);
    packetio::writeI8(output, deltaZ);
    packetio::writeI8(output, yaw);
    packetio::writeI8(output, pitch);
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    x = packetio::readI32BE(input);
    y = packetio::readI32BE(input);
    z = packetio::readI32BE(input);
    yaw = static_cast<std::int8_t>(packetio::readU8(input));
    pitch = static_cast<std::int8_t>(packetio::readU8(input));
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI32BE(output, x);
    packetio::writeI32BE(output, y);
    packetio::writeI32BE(output, z);
    packetio::writeU8(output, static_cast<std::uint8_t>(yaw));
    packetio::writeU8(output, static_cast<std::uint8_t>(pitch));
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onEntityPosition(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 34;
  }
};
class EntityStatusS2CPacket : public Packet {
public:
  int id = 0;
  std::int8_t status = 0;
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    status = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI8(output, status);
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    vehicleId = packetio::readI32BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI32BE(output, vehicleId);
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
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    trackedValues = entity::data::DataTracker::readEntries(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    entity::data::DataTracker::writeEntries(trackedValues, output);
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onEntityTrackerUpdate(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 5;
  }
};
class EntityEquipmentUpdateS2CPacket : public Packet {
public:
  int id = 0;
  int slot = 0;
  int itemRawId = 0;
  int itemDamage = 0;
  void read(std::istream& input) override {
    id = packetio::readI32BE(input);
    slot = packetio::readI16BE(input);
    itemRawId = packetio::readI16BE(input);
    itemDamage = packetio::readI16BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, id);
    packetio::writeI16BE(output, static_cast<std::int16_t>(slot));
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemRawId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemDamage));
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onEntityEquipmentUpdate(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 8;
  }
};
} // namespace net::minecraft
