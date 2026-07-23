#pragma once
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
class WorldTimeUpdateS2CPacket : public Packet {
 public:
 std::int64_t time = 0;
 void read(std::istream& input) override {
  time = packetio::readI64BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI64BE(output, time);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onWorldTimeUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 8;
 }
};
class PlayerSpawnPositionS2CPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 void read(std::istream& input) override {
  x = packetio::readI32BE(input);
  y = packetio::readI32BE(input);
  z = packetio::readI32BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, x);
  packetio::writeI32BE(output, y);
  packetio::writeI32BE(output, z);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerSpawnPosition(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 12;
 }
};
class PlayerRespawnPacket : public Packet {
 public:
 std::int8_t dimensionRawId = 0;
 void read(std::istream& input) override {
  dimensionRawId = packetio::readI8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI8(output, dimensionRawId);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayerRespawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 1;
 }
};
class GameStateChangeS2CPacket : public Packet {
 public:
 static constexpr const char* REASONS[3] = {"tile.bed.notValid", nullptr, nullptr};
 int reason = 0;
 void read(std::istream& input) override {
  reason = packetio::readI8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI8(output, static_cast<std::int8_t>(reason));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onGameStateChange(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 1;
 }
};
class WorldEventS2CPacket : public Packet {
 public:
 int eventId = 0;
 int data = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 void read(std::istream& input) override {
  eventId = packetio::readI32BE(input);
  x = packetio::readI32BE(input);
  y = packetio::readU8(input);
  z = packetio::readI32BE(input);
  data = packetio::readI32BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, eventId);
  packetio::writeI32BE(output, x);
  packetio::writeU8(output, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(output, z);
  packetio::writeI32BE(output, data);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onWorldEvent(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 20;
 }
};
class GlobalEntitySpawnS2CPacket : public Packet {
 public:
 int id = 0;
 int type = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 void read(std::istream& input) override {
  id = packetio::readI32BE(input);
  type = packetio::readI8(input);
  x = packetio::readI32BE(input);
  y = packetio::readI32BE(input);
  z = packetio::readI32BE(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, id);
  packetio::writeI8(output, static_cast<std::int8_t>(type));
  packetio::writeI32BE(output, x);
  packetio::writeI32BE(output, y);
  packetio::writeI32BE(output, z);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onLightningEntitySpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 17;
 }
};
} // namespace net::minecraft
