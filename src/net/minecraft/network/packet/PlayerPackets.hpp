#pragma once
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/PacketItems.hpp"
namespace net::minecraft {
class HealthUpdateS2CPacket : public Packet {
 public:
 int health = 0;
 void read(std::istream& input) override {
  health = packetio::readI16BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI16BE(output, static_cast<std::int16_t>(health));
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
 // Sets the fields shared by every player-move packet (feet + stance + look).
 // Parameter order matches the game's mental model (feet, then stance), which is
 // reordered onto the wire (x, feetY, z, stance) by the subclass read/write.
 void setMove(double xIn, double feetYIn, double stanceIn, double zIn, float yawIn, float pitchIn, bool onGroundIn) {
  x = xIn;
  feetY = feetYIn;
  stance = stanceIn;
  z = zIn;
  yaw = yawIn;
  pitch = pitchIn;
  onGround = onGroundIn;
 }
 void read(std::istream& input) override {
  onGround = packetio::readU8(input) != 0;
 }
 void write(std::ostream& output) const override {
  packetio::writeU8(output, onGround ? 1U : 0U);
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
 void read(std::istream& input) override {
  x = packetio::readDoubleBE(input);
  feetY = packetio::readDoubleBE(input);
  stance = packetio::readDoubleBE(input);
  z = packetio::readDoubleBE(input);
  PlayerMovePacket::read(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeDoubleBE(output, x);
  packetio::writeDoubleBE(output, feetY);
  packetio::writeDoubleBE(output, stance);
  packetio::writeDoubleBE(output, z);
  PlayerMovePacket::write(output);
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
 void read(std::istream& input) override {
  yaw = packetio::readFloatBE(input);
  pitch = packetio::readFloatBE(input);
  PlayerMovePacket::read(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeFloatBE(output, yaw);
  packetio::writeFloatBE(output, pitch);
  PlayerMovePacket::write(output);
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
 void read(std::istream& input) override {
  x = packetio::readDoubleBE(input);
  feetY = packetio::readDoubleBE(input);
  stance = packetio::readDoubleBE(input);
  z = packetio::readDoubleBE(input);
  yaw = packetio::readFloatBE(input);
  pitch = packetio::readFloatBE(input);
  PlayerMovePacket::read(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeDoubleBE(output, x);
  packetio::writeDoubleBE(output, feetY);
  packetio::writeDoubleBE(output, stance);
  packetio::writeDoubleBE(output, z);
  packetio::writeFloatBE(output, yaw);
  packetio::writeFloatBE(output, pitch);
  PlayerMovePacket::write(output);
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
 void read(std::istream& input) override {
  action = packetio::readU8(input);
  x = packetio::readI32BE(input);
  y = packetio::readU8(input);
  z = packetio::readI32BE(input);
  direction = packetio::readU8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeU8(output, static_cast<std::uint8_t>(action));
  packetio::writeI32BE(output, x);
  packetio::writeU8(output, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(output, z);
  packetio::writeU8(output, static_cast<std::uint8_t>(direction));
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
 void read(std::istream& input) override {
  x = packetio::readI32BE(input);
  y = packetio::readU8(input);
  z = packetio::readI32BE(input);
  side = packetio::readU8(input);
  stack = packetitems::readOptionalItemStack(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, x);
  packetio::writeU8(output, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(output, z);
  packetio::writeU8(output, static_cast<std::uint8_t>(side));
  packetitems::writeOptionalItemStack(output, stack);
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
 void read(std::istream& input) override {
  playerId = packetio::readI32BE(input);
  entityId = packetio::readI32BE(input);
  isLeftClick = packetio::readI8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, playerId);
  packetio::writeI32BE(output, entityId);
  packetio::writeI8(output, static_cast<std::int8_t>(isLeftClick));
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
 void read(std::istream& input) override {
  selectedSlot = packetio::readI16BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI16BE(output, static_cast<std::int16_t>(selectedSlot));
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
 void read(std::istream& input) override {
  id = packetio::readI32BE(input);
  status = packetio::readI8(input);
  x = packetio::readI32BE(input);
  y = packetio::readI8(input);
  z = packetio::readI32BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, id);
  packetio::writeI8(output, static_cast<std::int8_t>(status));
  packetio::writeI32BE(output, x);
  packetio::writeI8(output, static_cast<std::int8_t>(y));
  packetio::writeI32BE(output, z);
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
 void read(std::istream& input) override {
  sideways = packetio::readFloatBE(input);
  forward = packetio::readFloatBE(input);
  pitch = packetio::readFloatBE(input);
  yaw = packetio::readFloatBE(input);
  jumping = packetio::readBoolean(input);
  sneaking = packetio::readBoolean(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeFloatBE(output, sideways);
  packetio::writeFloatBE(output, forward);
  packetio::writeFloatBE(output, pitch);
  packetio::writeFloatBE(output, yaw);
  packetio::writeBoolean(output, jumping);
  packetio::writeBoolean(output, sneaking);
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
 void read(std::istream& input) override {
  entityId = packetio::readI32BE(input);
  mode = packetio::readI8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, entityId);
  packetio::writeI8(output, static_cast<std::int8_t>(mode));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handleClientCommand(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 5;
 }
};
} // namespace net::minecraft
