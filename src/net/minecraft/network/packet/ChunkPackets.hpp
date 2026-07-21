#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
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
 void read(std::istream& input) override {
  x = packetio::readI32BE(input);
  z = packetio::readI32BE(input);
  load = packetio::readU8(input) != 0;
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, x);
  packetio::writeI32BE(output, z);
  packetio::writeU8(output, load ? 1U : 0U);
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
 void read(std::istream& input) override {
  x = packetio::readI32BE(input);
  y = packetio::readI16BE(input);
  z = packetio::readI32BE(input);
  sizeX = packetio::readU8(input) + 1;
  sizeY = packetio::readU8(input) + 1;
  sizeZ = packetio::readU8(input) + 1;
  chunkDataSize = packetio::readI32BE(input);
  const std::vector<std::uint8_t> compressed =
      packetio::readBytes(input, static_cast<std::size_t>(chunkDataSize));
  chunkData.resize(static_cast<std::size_t>(sizeX * sizeY * sizeZ * 5 / 2));
  const std::vector<std::uint8_t> decompressed = zlibDecompress(compressed);
  if(decompressed.size() > chunkData.size()) {
   throw std::runtime_error("Chunk data decompression overflow");
  }
  std::copy(decompressed.begin(), decompressed.end(), chunkData.begin());
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, x);
  packetio::writeI16BE(output, static_cast<std::int16_t>(y));
  packetio::writeI32BE(output, z);
  packetio::writeU8(output, static_cast<std::uint8_t>(sizeX - 1));
  packetio::writeU8(output, static_cast<std::uint8_t>(sizeY - 1));
  packetio::writeU8(output, static_cast<std::uint8_t>(sizeZ - 1));
  packetio::writeI32BE(output, chunkDataSize);
  if(chunkDataSize > 0) {
   output.write(reinterpret_cast<const char*>(chunkData.data()), static_cast<std::streamsize>(chunkDataSize));
   if(!output) {
    throw std::runtime_error("Failed to write chunk data");
   }
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
 void read(std::istream& input) override {
  x = packetio::readI32BE(input);
  z = packetio::readI32BE(input);
  entryCount = packetio::readU16BE(input);
  positions.resize(static_cast<std::size_t>(entryCount));
  blockRawIds.resize(static_cast<std::size_t>(entryCount));
  blockMetadata.resize(static_cast<std::size_t>(entryCount));
  for(int i = 0; i < entryCount; ++i) {
   positions[static_cast<std::size_t>(i)] = packetio::readI16BE(input);
  }
  for(int i = 0; i < entryCount; ++i) {
   blockRawIds[static_cast<std::size_t>(i)] = packetio::readI8(input);
  }
  for(int i = 0; i < entryCount; ++i) {
   blockMetadata[static_cast<std::size_t>(i)] = packetio::readI8(input);
  }
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, x);
  packetio::writeI32BE(output, z);
  packetio::writeU16BE(output, static_cast<std::uint16_t>(entryCount));
  for(const std::int16_t position : positions) {
   packetio::writeI16BE(output, position);
  }
  for(const std::int8_t blockRawId : blockRawIds) {
   packetio::writeI8(output, blockRawId);
  }
  for(const std::int8_t metadata : blockMetadata) {
   packetio::writeI8(output, metadata);
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
