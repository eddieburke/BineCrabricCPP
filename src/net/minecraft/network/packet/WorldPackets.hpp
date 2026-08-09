#pragma once
#include <cstddef>
#include <cstdint>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
class WorldTimeUpdateS2CPacket : public Packet {
 public:
 std::int64_t time = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  time = packetio::readI64BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI64BE(dest, end, time);
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
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
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
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  dimensionRawId = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI8(dest, end, dimensionRawId);
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
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  reason = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI8(dest, end, static_cast<std::int8_t>(reason));
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
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  eventId = packetio::readI32BE(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readU8(src, end);
  z = packetio::readI32BE(src, end);
  data = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, eventId);
  packetio::writeI32BE(dest, end, x);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeI32BE(dest, end, data);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onWorldEvent(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 17;
 }
};
class GlobalEntitySpawnS2CPacket : public Packet {
 public:
 int id = 0;
 int type = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  id = packetio::readI32BE(src, end);
  type = packetio::readI8(src, end);
  x = packetio::readI32BE(src, end);
  y = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, id);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(type));
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, y);
  packetio::writeI32BE(dest, end, z);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onLightningEntitySpawn(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 17;
 }
};
} // namespace net::minecraft
