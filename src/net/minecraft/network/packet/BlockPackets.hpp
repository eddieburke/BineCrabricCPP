#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class BlockUpdateS2CPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 int blockRawId = 0;
 int blockMetadata = 0;
 BlockUpdateS2CPacket() {
  worldPacket = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readU8(src, end);
  z = packetio::readI32BE(src, end);
  blockRawId = packetio::readU8(src, end);
  blockMetadata = packetio::readU8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(blockRawId));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(blockMetadata));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onBlockUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 11;
 }
};
class PlayNoteSoundS2CPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 int instrument = 0;
 int pitch = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readI16BE(src, end);
  z = packetio::readI32BE(src, end);
  instrument = packetio::readU8(src, end);
  pitch = packetio::readU8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(instrument));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(pitch));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onPlayNoteSound(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 12;
 }
};
class ExplosionS2CPacket : public Packet {
 public:
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 float radius = 0.0f;
 std::vector<Vec3i> affectedBlocks;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readDoubleBE(src, end);
  y = packetio::readDoubleBE(src, end);
  z = packetio::readDoubleBE(src, end);
  radius = packetio::readFloatBE(src, end);
  const int count = packetio::readI32BE(src, end);
  const int originX = static_cast<int>(x);
  const int originY = static_cast<int>(y);
  const int originZ = static_cast<int>(z);
  affectedBlocks.clear();
  affectedBlocks.reserve(static_cast<std::size_t>(count));
  for(int i = 0; i < count; ++i) {
   affectedBlocks.emplace_back(originX + packetio::readI8(src, end),
                               originY + packetio::readI8(src, end),
                               originZ + packetio::readI8(src, end));
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeDoubleBE(dest, end, x);
  packetio::writeDoubleBE(dest, end, y);
  packetio::writeDoubleBE(dest, end, z);
  packetio::writeFloatBE(dest, end, radius);
  packetio::writeI32BE(dest, end, static_cast<std::int32_t>(affectedBlocks.size()));
  const int originX = static_cast<int>(x);
  const int originY = static_cast<int>(y);
  const int originZ = static_cast<int>(z);
  for(const Vec3i& blockPos : affectedBlocks) {
   packetio::writeI8(dest, end, static_cast<std::int8_t>(blockPos.x - originX));
   packetio::writeI8(dest, end, static_cast<std::int8_t>(blockPos.y - originY));
   packetio::writeI8(dest, end, static_cast<std::int8_t>(blockPos.z - originZ));
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onExplosion(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 32U + affectedBlocks.size() * 3U;
 }
};
} // namespace net::minecraft
