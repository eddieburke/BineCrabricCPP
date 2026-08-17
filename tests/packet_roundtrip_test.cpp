#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "net/minecraft/entity/data/DataTracker.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
namespace net::minecraft::test {
namespace {
std::vector<std::uint8_t> serialize(const Packet& packet) {
 std::vector<std::uint8_t> bytes(packet.size() + 1);
 std::uint8_t* dest = bytes.data();
 std::uint8_t* const end = bytes.data() + bytes.size();
 Packet::write(packet, dest, end);
 bytes.resize(static_cast<std::size_t>(dest - bytes.data()));
 return bytes;
}
std::unique_ptr<Packet> deserialize(const std::vector<std::uint8_t>& bytes, bool serverSide) {
 const std::uint8_t* src = bytes.data();
 const std::uint8_t* const end = bytes.data() + bytes.size();
 return Packet::read(src, end, serverSide);
}
std::size_t serializedPayloadSize(const Packet& packet) {
 std::vector<std::uint8_t> bytes(packet.size() + 65537U);
 std::uint8_t* dest = bytes.data();
 Packet::write(packet, dest, bytes.data() + bytes.size());
 return static_cast<std::size_t>(dest - bytes.data()) - 1U;
}
} // namespace
TEST(PacketRegistry, EveryRegisteredPacketReportsItsExactWireSize) {
 Packet::ensureRegistered();
 for(int rawId = 0; rawId <= 255; ++rawId) {
  const std::unique_ptr<Packet> packet = Packet::create(rawId);
  if(packet == nullptr) {
   continue;
  }
  EXPECT_EQ(serializedPayloadSize(*packet), packet->size()) << "packet id " << rawId;
 }
}
TEST(PacketRegistry, VariablePayloadSizesMatchTheirWireEncoding) {
 PlayerSpawnS2CPacket playerSpawn;
 playerSpawn.name = "Steve";
 EXPECT_EQ(serializedPayloadSize(playerSpawn), playerSpawn.size());
 ScreenHandlerSlotUpdateS2CPacket slotUpdate;
 slotUpdate.stack = ItemStack(1, 2, 3);
 EXPECT_EQ(serializedPayloadSize(slotUpdate), slotUpdate.size());
 InventoryS2CPacket inventory;
 inventory.contents = {ItemStack{}, ItemStack(1, 2, 3)};
 EXPECT_EQ(serializedPayloadSize(inventory), inventory.size());
 PlayerInteractBlockC2SPacket interactBlock;
 interactBlock.stack = ItemStack(1, 2, 3);
 EXPECT_EQ(serializedPayloadSize(interactBlock), interactBlock.size());
 UpdateSignPacket sign;
 sign.text = {"North", "South", "East", "West"};
 EXPECT_EQ(serializedPayloadSize(sign), sign.size());
 MapUpdateS2CPacket map;
 map.updateData = {1, 2, 3, 4, 5};
 EXPECT_EQ(serializedPayloadSize(map), map.size());
}
TEST(PacketRegistry, KeepAliveRoundTrip) {
 Packet::ensureRegistered();
 KeepAlivePacket original;
 const std::unique_ptr<Packet> decoded = deserialize(serialize(original), true);
 ASSERT_NE(decoded, nullptr);
 EXPECT_EQ(decoded->rawId(), 0);
}
TEST(PacketRegistry, LoginHelloRoundTrip) {
 Packet::ensureRegistered();
 LoginHelloPacket original("Steve", 14);
 original.worldSeed = 12345ULL;
 original.dimensionId = 0;
 const std::unique_ptr<Packet> decoded = deserialize(serialize(original), true);
 ASSERT_NE(decoded, nullptr);
 const auto* hello = dynamic_cast<const LoginHelloPacket*>(decoded.get());
 ASSERT_NE(hello, nullptr);
 EXPECT_EQ(hello->protocolVersion, 14);
 EXPECT_EQ(hello->username, "Steve");
 EXPECT_EQ(hello->worldSeed, 12345ULL);
 EXPECT_EQ(hello->dimensionId, 0);
}
TEST(PacketRegistry, ChatMessageRoundTripInBothDirections) {
 Packet::ensureRegistered();
 ChatMessagePacket original;
 original.chatMessage = "LAN chat parity";
 for(const bool serverSide : {false, true}) {
  const std::unique_ptr<Packet> decoded = deserialize(serialize(original), serverSide);
  ASSERT_NE(decoded, nullptr);
  const auto* chat = dynamic_cast<const ChatMessagePacket*>(decoded.get());
  ASSERT_NE(chat, nullptr);
  EXPECT_EQ(chat->rawId(), 3);
  EXPECT_EQ(chat->chatMessage, original.chatMessage);
 }
}
TEST(PacketRegistry, LuaModSyncRoundTrip) {
 Packet::ensureRegistered();
 LuaModSyncPacket original;
 original.kind = LuaModSyncKind::Entity;
 original.payload = {1, 3, 3, 7};
 const std::unique_ptr<Packet> decoded = deserialize(serialize(original), true);
 ASSERT_NE(decoded, nullptr);
 const auto* sync = dynamic_cast<const LuaModSyncPacket*>(decoded.get());
 ASSERT_NE(sync, nullptr);
 EXPECT_EQ(sync->kind, LuaModSyncKind::Entity);
 EXPECT_EQ(sync->payload, original.payload);
}
TEST(PacketRegistry, LuaModSnapshotsRoundTrip) {
 LuaModSnapshot entity;
 entity.id = 41;
 entity.registryId = "test:entity";
 entity.x = 320;
 entity.y = 2048;
 entity.z = -96;
 entity.yaw = 64;
 entity.pitch = -32;
 entity.data.putInt("count", 7);
 const LuaModSnapshot decodedEntity =
     readLuaModSnapshotPacket(makeLuaModSnapshotPacket(entity, LuaModSyncKind::Entity));
 EXPECT_EQ(decodedEntity.id, entity.id);
 EXPECT_EQ(decodedEntity.registryId, entity.registryId);
 EXPECT_EQ(decodedEntity.x, entity.x);
 EXPECT_EQ(decodedEntity.y, entity.y);
 EXPECT_EQ(decodedEntity.z, entity.z);
 EXPECT_EQ(decodedEntity.yaw, entity.yaw);
 EXPECT_EQ(decodedEntity.pitch, entity.pitch);
 EXPECT_EQ(decodedEntity.data.getInt("count"), 7);
}
TEST(PacketRegistry, DataTrackerByteArrayRoundTrip) {
 entity::data::DataTracker source;
 source.startTracking(1, entity::data::DataTrackerByteArray{1, 2, 3, 5, 8});
 const std::vector<entity::data::DataTrackerEntry> snapshot = source.snapshotEntries();
 std::vector<std::uint8_t> bytes(entity::data::DataTracker::sizeOfEntries(snapshot));
 std::uint8_t* dest = bytes.data();
 std::uint8_t* const end = bytes.data() + bytes.size();
 entity::data::DataTracker::writeEntries(snapshot, dest, end);
 const std::uint8_t* src = bytes.data();
 const std::vector<entity::data::DataTrackerEntry> entries = entity::data::DataTracker::readEntries(src, end);
 entity::data::DataTracker target;
 target.startTracking(1, entity::data::DataTrackerByteArray{});
 EXPECT_EQ(target.writeUpdatedEntries(entries), std::vector<int>{1});
 EXPECT_EQ(target.getByteArray(1), (entity::data::DataTrackerByteArray{1, 2, 3, 5, 8}));
}
TEST(PacketRegistry, DataTrackerRejectsOutOfRangeKeys) {
 entity::data::DataTracker tracker;
 EXPECT_THROW(tracker.startTracking(-1, std::int32_t{1}), std::invalid_argument);
 EXPECT_THROW(tracker.startTracking(32, std::int32_t{1}), std::invalid_argument);
}
TEST(PacketRegistry, DataTrackerRejectsTypeChanges) {
 entity::data::DataTracker tracker;
 tracker.startTracking(1, std::int32_t{7});
 EXPECT_THROW(tracker.set(1, 7.0f), std::invalid_argument);
 EXPECT_EQ(tracker.getInt(1), 7);
}
TEST(PacketRegistry, ClientboundPlayerMoveKeepsFeetAndStanceDistinct) {
 Packet::ensureRegistered();
 PlayerMoveFullPacket original;
 original.setMove(12.5, 70.62, 69.0, -4.25, 90.0f, 12.0f, false);
 const std::unique_ptr<Packet> decoded = deserialize(serialize(original), true);
 ASSERT_NE(decoded, nullptr);
 const auto* move = dynamic_cast<const PlayerMoveFullPacket*>(decoded.get());
 ASSERT_NE(move, nullptr);
 EXPECT_DOUBLE_EQ(move->x, 12.5);
 EXPECT_DOUBLE_EQ(move->feetY, 70.62);
 EXPECT_DOUBLE_EQ(move->stance, 69.0);
 EXPECT_DOUBLE_EQ(move->z, -4.25);
 EXPECT_FLOAT_EQ(move->yaw, 90.0f);
 EXPECT_FLOAT_EQ(move->pitch, 12.0f);
 EXPECT_TRUE(move->changePosition);
 EXPECT_TRUE(move->changeLook);
}
TEST(PacketRegistry, ServerboundPlayerMoveKeepsFeetAndStanceDistinct) {
 Packet::ensureRegistered();
 PlayerMoveFullPacket original;
 original.setMove(12.5, 69.0, 70.62, -4.25, 180.0f, -5.0f, true);
 const std::unique_ptr<Packet> decoded = deserialize(serialize(original), true);
 ASSERT_NE(decoded, nullptr);
 const auto* move = dynamic_cast<const PlayerMoveFullPacket*>(decoded.get());
 ASSERT_NE(move, nullptr);
 EXPECT_DOUBLE_EQ(move->x, 12.5);
 EXPECT_DOUBLE_EQ(move->feetY, 69.0);
 EXPECT_DOUBLE_EQ(move->stance, 70.62);
 EXPECT_DOUBLE_EQ(move->z, -4.25);
 EXPECT_FLOAT_EQ(move->yaw, 180.0f);
 EXPECT_FLOAT_EQ(move->pitch, -5.0f);
 EXPECT_TRUE(move->changePosition);
 EXPECT_TRUE(move->changeLook);
}
} // namespace net::minecraft::test
