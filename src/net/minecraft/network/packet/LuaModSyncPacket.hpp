#pragma once
#include <cstddef>
#include <cstdint>
#include <sstream>
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
 BlockEntitySnapshot,
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
 void read(std::istream& input) override {
  kind = static_cast<LuaModSyncKind>(packetio::readU8(input));
  const std::int32_t length = packetio::readI32BE(input);
  if(length < 0 || static_cast<std::size_t>(length) > maxPayloadBytes) {
   throw std::runtime_error("Invalid Lua mod sync payload length");
  }
  payload = packetio::readBytes(input, static_cast<std::size_t>(length));
 }
 void write(std::ostream& output) const override {
  if(payload.size() > maxPayloadBytes) {
   throw std::runtime_error("Lua mod sync payload is too large");
  }
  packetio::writeU8(output, static_cast<std::uint8_t>(kind));
  packetio::writeI32BE(output, static_cast<std::int32_t>(payload.size()));
  packetio::writeBytes(output, payload);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onLuaModSync(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 5U + payload.size();
 }
};
inline LuaModSyncPacket makeLuaModSnapshotPacket(const LuaModSnapshot& snapshot, LuaModSyncKind kind) {
 std::ostringstream out(std::ios::binary);
 packetio::writeI32BE(out, snapshot.x);
 packetio::writeI32BE(out, snapshot.y);
 packetio::writeI32BE(out, snapshot.z);
 packetio::writeI32BE(out, snapshot.id);
 packetio::writeI32BE(out, snapshot.yaw);
 packetio::writeI32BE(out, snapshot.pitch);
 packetio::writeUtfString(out, snapshot.registryId);
 Nbt data = Nbt::compound();
 data.asCompound() = snapshot.data.storage().asCompound();
 data.write(out);
 const std::string bytes = out.str();
 LuaModSyncPacket packet;
 packet.kind = kind;
 packet.payload.assign(bytes.begin(), bytes.end());
 return packet;
}
inline LuaModSnapshot readLuaModSnapshotPacket(const LuaModSyncPacket& packet) {
 if(packet.kind != LuaModSyncKind::BlockEntitySnapshot && packet.kind != LuaModSyncKind::Entity) {
  throw std::runtime_error("Expected Lua mod snapshot packet");
 }
 const std::string bytes(packet.payload.begin(), packet.payload.end());
 std::istringstream in(bytes, std::ios::binary);
 LuaModSnapshot snapshot;
 snapshot.x = packetio::readI32BE(in);
 snapshot.y = packetio::readI32BE(in);
 snapshot.z = packetio::readI32BE(in);
 snapshot.id = packetio::readI32BE(in);
 snapshot.yaw = packetio::readI32BE(in);
 snapshot.pitch = packetio::readI32BE(in);
 snapshot.registryId = packetio::readUtfString(in);
 const Nbt data = Nbt::read(in);
 if(data.isCompound()) {
  snapshot.data = NbtCompound(data);
 }
 return snapshot;
}
} // namespace net::minecraft
