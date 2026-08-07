#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
enum class LuaModSyncKind : std::uint8_t {
 ClientModList,
 Entity
};
struct LuaModSnapshot {
 int x = 0;
 int y = 0;
 int z = 0;
 int id = 0;
 int yaw = 0;
 int pitch = 0;
 std::string registryId;
 NbtCompound data;
};
class LuaModSyncPacket final : public Packet {
 public:
  static constexpr std::size_t maxPayloadBytes = 1024U * 1024U;
  LuaModSyncKind kind = LuaModSyncKind::ClientModList;
  std::vector<std::uint8_t> payload;
  void read(const std::uint8_t*& src, const std::uint8_t* end) override {
   kind = static_cast<LuaModSyncKind>(packetio::readU8(src, end));
   const std::int32_t length = packetio::readI32BE(src, end);
   if(length < 0 || static_cast<std::size_t>(length) > maxPayloadBytes) {
    throw std::runtime_error("Invalid Lua mod sync payload length");
   }
   payload = packetio::readBytes(src, end, static_cast<std::size_t>(length));
  }
  void write(std::uint8_t*& dest, std::uint8_t* end) const override {
   if(payload.size() > maxPayloadBytes) {
    throw std::runtime_error("Lua mod sync payload is too large");
   }
   packetio::writeU8(dest, end, static_cast<std::uint8_t>(kind));
   packetio::writeI32BE(dest, end, static_cast<std::int32_t>(payload.size()));
   packetio::writeBytes(dest, end, payload);
  }
  void apply(NetworkHandler& networkHandler) const override {
   networkHandler.onLuaModSync(*this);
  }
  [[nodiscard]] std::size_t size() const override {
   return 5U + payload.size();
  }
};
inline LuaModSyncPacket makeLuaModSnapshotPacket(const LuaModSnapshot& snapshot, LuaModSyncKind kind) {
  const std::vector<std::uint8_t> nbtBytes = snapshot.data.storage().toBytes();
  std::vector<std::uint8_t> bytes;
  bytes.resize(32U + snapshot.registryId.size() * 3U + nbtBytes.size());
  std::uint8_t* dest = bytes.data();
  std::uint8_t* const end = bytes.data() + bytes.size();
  packetio::writeI32BE(dest, end, snapshot.x);
  packetio::writeI32BE(dest, end, snapshot.y);
  packetio::writeI32BE(dest, end, snapshot.z);
  packetio::writeI32BE(dest, end, snapshot.id);
  packetio::writeI32BE(dest, end, snapshot.yaw);
  packetio::writeI32BE(dest, end, snapshot.pitch);
  packetio::writeUtfString(dest, end, snapshot.registryId);
  packetio::writeBytes(dest, end, nbtBytes.data(), nbtBytes.size());
  bytes.resize(static_cast<std::size_t>(dest - bytes.data()));
  LuaModSyncPacket packet;
  packet.kind = kind;
  packet.payload = std::move(bytes);
  return packet;
}
inline LuaModSnapshot readLuaModSnapshotPacket(const LuaModSyncPacket& packet) {
  if(packet.kind != LuaModSyncKind::Entity) {
   throw std::runtime_error("Expected Lua mod snapshot packet");
  }
  const std::uint8_t* src = packet.payload.data();
  const std::uint8_t* end = src + packet.payload.size();
  LuaModSnapshot snapshot;
  snapshot.x = packetio::readI32BE(src, end);
  snapshot.y = packetio::readI32BE(src, end);
  snapshot.z = packetio::readI32BE(src, end);
  snapshot.id = packetio::readI32BE(src, end);
  snapshot.yaw = packetio::readI32BE(src, end);
  snapshot.pitch = packetio::readI32BE(src, end);
  snapshot.registryId = packetio::readUtfString(src, end);
  const Nbt data = Nbt::read(std::vector<std::uint8_t>(src, end));
  if(data.isCompound()) {
   snapshot.data = NbtCompound(data);
  }
  return snapshot;
}
} // namespace net::minecraft
