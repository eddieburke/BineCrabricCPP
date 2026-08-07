#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
class ChunkStatusUpdateS2CPacket : public Packet {
 public:
 int x = 0;
 int z = 0;
 bool load = false;
 ChunkStatusUpdateS2CPacket() {
  worldPacket = false;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  load = packetio::readU8(src, end) != 0;
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, load ? 1U : 0U);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onChunkStatusUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 9;
 }
};
class ChunkDataS2CPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 int sizeX = 0;
 int sizeY = 0;
 int sizeZ = 0;
 std::vector<std::uint8_t> chunkData;
 mutable int chunkDataSize = 0;
 ChunkDataS2CPacket() {
  worldPacket = true;
 }
 // Match Java ChunkDataS2CPacket(World): compress before enqueue so size() and sendQueueSize stay accurate.
 void compressForSend() {
  if(chunkDataSize > 0 || chunkData.empty()) {
   return;
  }
  const std::vector<std::uint8_t> compressed = zlibCompress(chunkData);
  chunkDataSize = static_cast<int>(compressed.size());
  chunkData = std::move(compressed);
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readI16BE(src, end);
  z = packetio::readI32BE(src, end);
  sizeX = packetio::readU8(src, end) + 1;
  sizeY = packetio::readU8(src, end) + 1;
  sizeZ = packetio::readU8(src, end) + 1;
  chunkDataSize = packetio::readI32BE(src, end);
  const std::vector<std::uint8_t> compressed =
      packetio::readBytes(src, end, static_cast<std::size_t>(chunkDataSize));
  chunkData = zlibDecompress(compressed);
  const std::size_t expectedMax = static_cast<std::size_t>(sizeX * sizeY * sizeZ * 5 / 2);
  if(chunkData.size() > expectedMax) {
   throw std::runtime_error("Chunk data decompression overflow");
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(y));
  packetio::writeI32BE(dest, end, z);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(sizeX - 1));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(sizeY - 1));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(sizeZ - 1));
  packetio::writeI32BE(dest, end, chunkDataSize);
  if(chunkDataSize > 0) {
   packetio::writeBytes(dest, end, chunkData.data(), static_cast<std::size_t>(chunkDataSize));
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handleChunkData(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return static_cast<std::size_t>(17 + chunkDataSize);
 }
};
class ChunkDeltaUpdateS2CPacket : public Packet {
 public:
 int x = 0;
 int z = 0;
 int entryCount = 0;
 std::vector<std::int16_t> positions;
 std::vector<std::int8_t> blockRawIds;
 std::vector<std::int8_t> blockMetadata;
 ChunkDeltaUpdateS2CPacket() {
  worldPacket = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  z = packetio::readI32BE(src, end);
  entryCount = packetio::readU16BE(src, end);
  positions.resize(static_cast<std::size_t>(entryCount));
  blockRawIds.resize(static_cast<std::size_t>(entryCount));
  blockMetadata.resize(static_cast<std::size_t>(entryCount));
  for(int i = 0; i < entryCount; ++i) {
   positions[static_cast<std::size_t>(i)] = packetio::readI16BE(src, end);
  }
  for(int i = 0; i < entryCount; ++i) {
   blockRawIds[static_cast<std::size_t>(i)] = packetio::readI8(src, end);
  }
  for(int i = 0; i < entryCount; ++i) {
   blockMetadata[static_cast<std::size_t>(i)] = packetio::readI8(src, end);
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI32BE(dest, end, z);
  packetio::writeU16BE(dest, end, static_cast<std::uint16_t>(entryCount));
  for(const std::int16_t position : positions) {
   packetio::writeI16BE(dest, end, position);
  }
  for(const std::int8_t blockRawId : blockRawIds) {
   packetio::writeI8(dest, end, blockRawId);
  }
  for(const std::int8_t metadata : blockMetadata) {
   packetio::writeI8(dest, end, metadata);
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onChunkDeltaUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return static_cast<std::size_t>(10 + entryCount * 4);
 }
};
} // namespace net::minecraft
