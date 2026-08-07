#pragma once
#include <cstddef>
#include <cstdint>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/PacketItems.hpp"
namespace net::minecraft {
class HealthUpdateS2CPacket : public Packet {
 public:
 int health = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  health = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(health));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onHealthUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 2;
 }
};
class PlayerMovePacket : public Packet {
 public:
 double x = 0.0;
 double feetY = 0.0;
 double z = 0.0;
 double stance = 0.0;
 float yaw = 0.0f;
 float pitch = 0.0f;
 bool onGround = false;
 bool changePosition = false;
 bool changeLook = false;
 void setMove(double xIn, double feetYIn, double stanceIn, double zIn, float yawIn, float pitchIn, bool onGroundIn) {
  x = xIn;
  feetY = feetYIn;
  stance = stanceIn;
  z = zIn;
  yaw = yawIn;
  pitch = pitchIn;
  onGround = onGroundIn;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  onGround = packetio::readU8(src, end) != 0;
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, onGround ? 1U : 0U);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerMove(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 1;
 }
};
class PlayerMovePositionAndOnGroundPacket : public PlayerMovePacket {
 public:
 PlayerMovePositionAndOnGroundPacket() {
  changePosition = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readDoubleBE(src, end);
  feetY = packetio::readDoubleBE(src, end);
  stance = packetio::readDoubleBE(src, end);
  z = packetio::readDoubleBE(src, end);
  PlayerMovePacket::read(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeDoubleBE(dest, end, x);
  packetio::writeDoubleBE(dest, end, feetY);
  packetio::writeDoubleBE(dest, end, stance);
  packetio::writeDoubleBE(dest, end, z);
  PlayerMovePacket::write(dest, end);
 }
 [[nodiscard]] std::size_t size() const override {
  return 33;
 }
};
class PlayerMoveLookAndOnGroundPacket : public PlayerMovePacket {
 public:
 PlayerMoveLookAndOnGroundPacket() {
  changeLook = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  yaw = packetio::readFloatBE(src, end);
  pitch = packetio::readFloatBE(src, end);
  PlayerMovePacket::read(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeFloatBE(dest, end, yaw);
  packetio::writeFloatBE(dest, end, pitch);
  PlayerMovePacket::write(dest, end);
 }
 [[nodiscard]] std::size_t size() const override {
  return 9;
 }
};
class PlayerMoveFullPacket : public PlayerMovePacket {
 public:
 PlayerMoveFullPacket() {
  changeLook = true;
  changePosition = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readDoubleBE(src, end);
  feetY = packetio::readDoubleBE(src, end);
  stance = packetio::readDoubleBE(src, end);
  z = packetio::readDoubleBE(src, end);
  yaw = packetio::readFloatBE(src, end);
  pitch = packetio::readFloatBE(src, end);
  PlayerMovePacket::read(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeDoubleBE(dest, end, x);
  packetio::writeDoubleBE(dest, end, feetY);
  packetio::writeDoubleBE(dest, end, stance);
  packetio::writeDoubleBE(dest, end, z);
  packetio::writeFloatBE(dest, end, yaw);
  packetio::writeFloatBE(dest, end, pitch);
  PlayerMovePacket::write(dest, end);
 }
 [[nodiscard]] std::size_t size() const override {
  return 41;
 }
};
class PlayerActionC2SPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 int direction = 0;
 int action = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  action = packetio::readU8(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readU8(src, end);
  z = packetio::readI32BE(src, end);
  direction = packetio::readU8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(action));
  packetio::writeI32BE(dest, end, x);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(direction));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handlePlayerAction(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 11;
 }
};
class PlayerInteractBlockC2SPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 int side = 0;
 ItemStack stack;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readU8(src, end);
  z = packetio::readI32BE(src, end);
  side = packetio::readU8(src, end);
  stack = packetitems::readOptionalItemStack(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(side));
  packetitems::writeOptionalItemStack(dest, end, stack);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerInteractBlock(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 15;
 }
};
class PlayerInteractEntityC2SPacket : public Packet {
 public:
 int playerId = 0;
 int entityId = 0;
 int isLeftClick = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  playerId = packetio::readI32BE(src, end);
  entityId = packetio::readI32BE(src, end);
  isLeftClick = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, playerId);
  packetio::writeI32BE(dest, end, entityId);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(isLeftClick));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handleInteractEntity(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 9;
 }
};
class UpdateSelectedSlotC2SPacket : public Packet {
 public:
 int selectedSlot = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  selectedSlot = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(selectedSlot));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onUpdateSelectedSlot(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 2;
 }
};
class PlayerSleepUpdateS2CPacket : public Packet {
 public:
 int id = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int status = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  status = packetio::readI8(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI8(src, end);
  z = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(status));
  packetio::writeI32BE(dest, end, x);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(y));
  packetio::writeI32BE(dest, end, z);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerSleepUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 14;
 }
};
class PlayerInputC2SPacket : public Packet {
 public:
 float sideways = 0.0f;
 float forward = 0.0f;
 bool jumping = false;
 bool sneaking = false;
 float pitch = 0.0f;
 float yaw = 0.0f;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  sideways = packetio::readFloatBE(src, end);
  forward = packetio::readFloatBE(src, end);
  pitch = packetio::readFloatBE(src, end);
  yaw = packetio::readFloatBE(src, end);
  jumping = packetio::readBoolean(src, end);
  sneaking = packetio::readBoolean(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeFloatBE(dest, end, sideways);
  packetio::writeFloatBE(dest, end, forward);
  packetio::writeFloatBE(dest, end, pitch);
  packetio::writeFloatBE(dest, end, yaw);
  packetio::writeBoolean(dest, end, jumping);
  packetio::writeBoolean(dest, end, sneaking);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerInput(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 18;
 }
};
class ClientCommandC2SPacket : public Packet {
 public:
 int entityId = 0;
 int mode = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  entityId = packetio::readI32BE(src, end);
  mode = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, entityId);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(mode));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handleClientCommand(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 5;
 }
};
} // namespace net::minecraft
