#include <gtest/gtest.h>
#include <sstream>
#include "net/minecraft/entity/data/DataTracker.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/network/packet/LuaModSyncPacket.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
namespace net::minecraft::test {
TEST(PacketRegistry, KeepAliveRoundTrip) {
 Packet::ensureRegistered();
 KeepAlivePacket original;
 std::ostringstream out;
 Packet::write(original, out);
 std::istringstream in(out.str());
 const std::unique_ptr<Packet> decoded = Packet::read(in, true);
 ASSERT_NE(decoded, nullptr);
 EXPECT_EQ(decoded->rawId(), 0);
}
TEST(PacketRegistry, LoginHelloRoundTrip) {
 Packet::ensureRegistered();
 LoginHelloPacket original("Steve", 14);
 original.worldSeed = 12345ULL;
 original.dimensionId = 0;
 std::ostringstream out;
 Packet::write(original, out);
 std::istringstream in(out.str());
 const std::unique_ptr<Packet> decoded = Packet::read(in, true);
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
  std::ostringstream out;
  Packet::write(original, out);
  std::istringstream in(out.str());
  const std::unique_ptr<Packet> decoded = Packet::read(in, serverSide);
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
 std::ostringstream out;
 Packet::write(original, out);
 std::istringstream in(out.str());
 const std::unique_ptr<Packet> decoded = Packet::read(in, true);
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
 LuaModSnapshot blockEntity;
 blockEntity.x = -20;
 blockEntity.y = 72;
 blockEntity.z = 144;
 blockEntity.registryId = "test:block_entity";
 blockEntity.data.putString("name", "snapshot");
 const LuaModSnapshot decodedBlockEntity =
     readLuaModSnapshotPacket(makeLuaModSnapshotPacket(blockEntity, LuaModSyncKind::BlockEntitySnapshot));
 EXPECT_EQ(decodedBlockEntity.x, blockEntity.x);
 EXPECT_EQ(decodedBlockEntity.y, blockEntity.y);
 EXPECT_EQ(decodedBlockEntity.z, blockEntity.z);
 EXPECT_EQ(decodedBlockEntity.registryId, blockEntity.registryId);
 EXPECT_EQ(decodedBlockEntity.data.getString("name"), "snapshot");
}
TEST(PacketRegistry, DataTrackerByteArrayRoundTrip) {
 entity::data::DataTracker source;
 source.startTracking(1, entity::data::DataTrackerByteArray{1, 2, 3, 5, 8});
 std::ostringstream out;
 entity::data::DataTracker::writeEntries(source.snapshotEntries(), out);
 std::istringstream in(out.str());
 const std::vector<entity::data::DataTrackerEntry> entries = entity::data::DataTracker::readEntries(in);
 entity::data::DataTracker target;
 target.startTracking(1, entity::data::DataTrackerByteArray{});
 EXPECT_EQ(target.writeUpdatedEntries(entries), std::vector<int>{1});
 EXPECT_EQ(target.getByteArray(1), (entity::data::DataTrackerByteArray{1, 2, 3, 5, 8}));
}
TEST(PacketRegistry, ClientboundPlayerMoveKeepsFeetAndStanceDistinct) {
 Packet::ensureRegistered();
 PlayerMoveFullPacket original;
 original.setMove(12.5, 70.62, 69.0, -4.25, 90.0f, 12.0f, false);
 std::ostringstream out;
 Packet::write(original, out);
 std::istringstream in(out.str());
 const std::unique_ptr<Packet> decoded = Packet::read(in, true);
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
 std::ostringstream out;
 Packet::write(original, out);
 std::istringstream in(out.str());
 const std::unique_ptr<Packet> decoded = Packet::read(in, true);
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
