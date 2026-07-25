#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/tool/wooden_pickaxe.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "support/server_event_fixture.hpp"
namespace {
void ensureWinsock() {
 static std::once_flag once;
 std::call_once(once, []() {
  WSADATA data{};
  if(::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
   throw std::runtime_error("WSAStartup failed");
  }
 });
}
SOCKET connectLoopback(std::uint16_t port) {
 ensureWinsock();
 addrinfo hints{};
 hints.ai_family = AF_UNSPEC;
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_protocol = IPPROTO_TCP;
 addrinfo* result = nullptr;
 std::ostringstream portStream;
 portStream << port;
 if(::getaddrinfo("127.0.0.1", portStream.str().c_str(), &hints, &result) != 0 || result == nullptr) {
  return INVALID_SOCKET;
 }
 SOCKET socket = INVALID_SOCKET;
 for(addrinfo* candidate = result; candidate != nullptr; candidate = candidate->ai_next) {
  socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
  if(socket == INVALID_SOCKET) {
   continue;
  }
  if(::connect(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == 0) {
   break;
  }
  ::closesocket(socket);
  socket = INVALID_SOCKET;
 }
 ::freeaddrinfo(result);
 return socket;
}
// Captures the fields of the first PlayerMoveFullPacket received on the wire, exactly as
// a real client's NetworkHandler would via Packet::apply -> onPlayerMove. Verifying through
// a real Connection (real socket, real Packet::write/read) rather than reading the C++
// struct directly is what makes this test catch a field-ordering/swap bug: a construction
// bug that happens to read back correctly in-process can still serialize wrong on the wire.
class CapturingClientHandler : public net::minecraft::NetworkHandler {
 public:
 [[nodiscard]] bool isServerSide() const override {
  return false;
 }
 void onPlayerMove(const net::minecraft::PlayerMovePacket& packet) override {
  const std::lock_guard<std::mutex> lock(mutex_);
  lastFeetY_ = packet.feetY;
  lastStance_ = packet.stance;
  received_.store(true, std::memory_order_release);
 }
 [[nodiscard]] bool received() const {
  return received_.load(std::memory_order_acquire);
 }
 [[nodiscard]] double lastFeetY() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return lastFeetY_;
 }
 [[nodiscard]] double lastStance() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return lastStance_;
 }

 private:
 mutable std::mutex mutex_;
 double lastFeetY_ = 0.0;
 double lastStance_ = 0.0;
 std::atomic<bool> received_{false};
};
// Connection's constructor takes NetworkHandler& by reference and starts its reader/writer
// threads immediately, so it needs a live handler before ServerPlayNetworkHandler (which
// needs the Connection*) can exist. Production code (ServerLoginNetworkHandler) resolves the
// same chicken-and-egg by constructing the Connection against a placeholder handler and
// letting ServerPlayNetworkHandler's constructor retarget it via connection_->setNetworkHandler(*this).
class BootstrapServerHandler : public net::minecraft::NetworkHandler {
 public:
 [[nodiscard]] bool isServerSide() const override {
  return true;
 }
};
} // namespace
namespace net::minecraft::test {
TEST(MultiplayerParityUpdates, StanceCalculationIsFeetPlusEyeHeight) {
 PlayerMoveFullPacket packet;
 const double feetY = 64.0;
 const double stance = feetY + 1.62;
 packet.setMove(8.0, feetY, stance, 8.0, 0.0f, 0.0f, true);
 const double stanceDelta = packet.stance - packet.feetY;
 EXPECT_NEAR(stanceDelta, 1.62, 0.001);
 EXPECT_GT(stanceDelta, 0.1);
 EXPECT_LT(stanceDelta, 1.65);
}
TEST(MultiplayerParityUpdates, StanceNeverZeroForNormalPosition) {
 PlayerMovePositionAndOnGroundPacket packet;
 const double feetY = 72.5;
 const double stance = feetY + 1.62;
 packet.setMove(0.0, feetY, stance, 0.0, 0.0f, 0.0f, true);
 EXPECT_DOUBLE_EQ(packet.stance, 74.12);
 EXPECT_NE(packet.stance, packet.feetY);
}
// Wire-format regression test for the "50/50 chunk load" bug: the server's teleport
// confirmation handshake (see ServerTeleportSendsFeetYAndStanceCorrectly below) requires
// the client's echoed feetY to be bit-exact with boundingBox.minY. Since the entity
// coordinate refactor player.y is EYE level, not feet -- echoing player.y as feetY (as an
// earlier version of makePlayerMoveResponsePacket did) put it ~1.62 blocks off, the
// server's `teleported` flag never latched, and ServerPlayNetworkHandler.playerTick(true)
// (the only thing that drains pendingChunkUpdates into ChunkDataS2CPacket) never ran.
TEST(MultiplayerParityUpdates, MovementAckUsesActualPlayerY) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
 player.cameraOffset = 0.2f;
 player.setPosition(8.5, 65.0, 8.5);
 PlayerMoveFullPacket inbound;
 inbound.setMove(8.5, 65.0, 64.0, 8.5, 0.0f, 0.0f, false);
 const std::unique_ptr<Packet> response = client::multiplayer::makePlayerMoveResponsePacket(inbound, player);
 const auto* move = dynamic_cast<const PlayerMoveFullPacket*>(response.get());
 ASSERT_NE(move, nullptr);
 EXPECT_DOUBLE_EQ(move->feetY, player.boundingBox.minY);
 EXPECT_DOUBLE_EQ(move->stance, player.getEyeY());
 // cameraOffset/standingEyeHeight make these genuinely different values here (63.58 vs
 // 65.0); a regression back to echoing player.y as feetY would pass every other assertion
 // in this test but silently reintroduce the join-failure bug, so pin the inequality too.
 EXPECT_NE(move->feetY, player.y);
}
TEST(MultiplayerParityUpdates, HealthDecreaseTriggersHurtAnimation) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
 player.health = 20;
 player.damageTo(15);
 EXPECT_EQ(player.health, 15);
 EXPECT_EQ(player.hurtTime, 10);
 EXPECT_EQ(player.damagedTime, 10);
}
TEST(MultiplayerParityUpdates, HealthIncreaseDoesNotTriggerHurt) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
 player.health = 10;
 player.hurtTime = 0;
 player.damagedTime = 0;
 player.damageTo(15);
 EXPECT_EQ(player.health, 15);
 EXPECT_EQ(player.hurtTime, 0);
 EXPECT_EQ(player.damagedTime, 0);
}
TEST(MultiplayerParityUpdates, PredictedClientBlockBreakResetsWithoutAuthority) {
 block::initializeBlocks();
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 ASSERT_NE(block::Block::DIRT, nullptr);
 ASSERT_TRUE(world.setBlockWithMetaFromPacket(8, 64, 8, block::Block::DIRT->id, 0));
 ASSERT_TRUE(world.setBlock(8, 64, 8, 0));
 EXPECT_EQ(world.getBlockId(8, 64, 8), 0);
 for(int tick = 0; tick < 80; ++tick) {
  world.tick();
 }
 EXPECT_EQ(world.getBlockId(8, 64, 8), block::Block::DIRT->id);
}
TEST(MultiplayerParityUpdates, AuthoritativeBlockUpdateClearsPredictedReset) {
 block::initializeBlocks();
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 ASSERT_NE(block::Block::DIRT, nullptr);
 ASSERT_TRUE(world.setBlockWithMetaFromPacket(8, 64, 8, block::Block::DIRT->id, 0));
 ASSERT_TRUE(world.setBlock(8, 64, 8, 0));
 (void)world.setBlockWithMetaFromPacket(8, 64, 8, 0, 0);
 for(int tick = 0; tick < 80; ++tick) {
  world.tick();
 }
 EXPECT_EQ(world.getBlockId(8, 64, 8), 0);
}
TEST(MultiplayerParityUpdates, ChunkDeltaAppliesBeforeFullChunkData) {
 block::initializeBlocks();
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 handler.world = &world;
 world.updateChunk(20, 20, true);
 ASSERT_FALSE(world.isChunkDataReady(20, 20));
 ASSERT_NE(block::Block::DIRT, nullptr);
 ChunkDeltaUpdateS2CPacket packet;
 packet.x = 20;
 packet.z = 20;
 packet.entryCount = 1;
 packet.positions.push_back(static_cast<std::int16_t>((1 << 12) | (2 << 8) | 64));
 packet.blockRawIds.push_back(static_cast<std::int8_t>(block::Block::DIRT->id));
 packet.blockMetadata.push_back(0);
 handler.onChunkDeltaUpdate(packet);
 EXPECT_EQ(world.getBlockId(321, 64, 322), block::Block::DIRT->id);
}
TEST(MultiplayerParityUpdates, BlockBreakActionPacketFieldLayout) {
 PlayerActionC2SPacket packet;
 packet.action = 0;
 packet.x = 5;
 packet.y = 64;
 packet.z = 5;
 packet.direction = 1;
 EXPECT_EQ(packet.action, 0);
 PlayerActionC2SPacket stopPacket;
 stopPacket.action = 2;
 stopPacket.x = 5;
 stopPacket.y = 64;
 stopPacket.z = 5;
 stopPacket.direction = 1;
 EXPECT_EQ(stopPacket.action, 2);
 EXPECT_EQ(stopPacket.x, packet.x);
 EXPECT_EQ(stopPacket.y, packet.y);
 EXPECT_EQ(stopPacket.z, packet.z);
}
TEST(MultiplayerParityUpdates, AbortActionPacketUsesAction1) {
 PlayerActionC2SPacket abortPacket;
 abortPacket.action = 1;
 abortPacket.x = 10;
 abortPacket.y = 70;
 abortPacket.z = 10;
 abortPacket.direction = 0;
 EXPECT_EQ(abortPacket.action, 1);
}
TEST(MultiplayerParityUpdates, HostedOwnerBreaksSpawnBlockAndReceivesDrop) {
 block::initializeBlocks();
 ServerEventFixture fixture;
 fixture.world.setSpawnPos(Vec3i{8, 64, 8});
 fixture.world.setEventProcessingEnabled(true);
 (void)fixture.world.getChunk(0, 0);
 fixture.world.setEventProcessingEnabled(false);
 ASSERT_NE(block::Block::DIRT, nullptr);
 ASSERT_NE(block::Block::TORCH, nullptr);
 const int targetY = fixture.world.getTopY(8, 8);
 ASSERT_GT(targetY, 0);
 ASSERT_LT(targetY, 127);
 if(fixture.world.getBlockId(8, targetY - 1, 8) != block::Block::DIRT->id) {
  ASSERT_TRUE(fixture.world.setBlock(8, targetY - 1, 8, block::Block::DIRT->id));
 }
 ASSERT_EQ(fixture.world.getBlockId(8, targetY, 8), 0);
 ASSERT_TRUE(fixture.world.setBlock(8, targetY, 8, block::Block::TORCH->id));
 server::network::ServerPlayerInteractionManager interactionManager(&fixture.world);
 entity::player::ServerPlayerEntity player(&fixture.server, &fixture.world, "WorldHost", &interactionManager);
 player.setPosition(8.5, static_cast<double>(targetY + 1), 8.5);
 server::network::ServerPlayNetworkHandler handler(&fixture.server, nullptr, &player);
 PlayerActionC2SPacket packet;
 packet.action = 0;
 packet.x = 8;
 packet.y = targetY;
 packet.z = 8;
 packet.direction = 1;
 handler.handlePlayerAction(packet);
 EXPECT_EQ(fixture.world.getBlockId(8, targetY, 8), block::Block::TORCH->id);
 fixture.server.playerManager.addTransientOperator(player.name);
 ASSERT_TRUE(fixture.server.playerManager.isOperator(player.name));
 handler.handlePlayerAction(packet);
 EXPECT_EQ(fixture.world.getBlockId(8, targetY, 8), 0);
 bool foundDrop = false;
 for(Entity* entity : fixture.world.entities()) {
  if(dynamic_cast<ItemEntity*>(entity) != nullptr) {
   foundDrop = true;
   break;
  }
 }
 EXPECT_TRUE(foundDrop);
}
TEST(MultiplayerParityUpdates, ServerBlockBreakPersistsHeldToolDamage) {
 block::initializeBlocks();
 ServerEventFixture fixture;
 (void)fixture.world.getChunk(0, 0);
 ASSERT_NE(block::Block::STONE, nullptr);
 ASSERT_TRUE(fixture.world.setBlock(8, 64, 8, block::Block::STONE->id));
 server::network::ServerPlayerInteractionManager interactionManager(&fixture.world);
 entity::player::ServerPlayerEntity player(&fixture.server, &fixture.world, "miner", &interactionManager);
 Item* pickaxe = Item::byRawId(item::WoodenPickaxeItem::kRawId);
 ASSERT_NE(pickaxe, nullptr);
 player.inventory.main[0] = ItemStack(pickaxe);
 player.inventory.selectedSlot = 0;
 ASSERT_TRUE(player.interactionManager.tryBreakBlock(8, 64, 8));
 EXPECT_EQ(player.inventory.main[0].damage, 1);
}
TEST(MultiplayerParityUpdates, ServerTicksPlayerOncePerNetworkTick) {
 block::initializeBlocks();
 ServerEventFixture fixture;
 (void)fixture.world.getChunk(0, 0);
 ASSERT_NE(block::Block::WATER, nullptr);
 ASSERT_TRUE(fixture.world.setBlock(8, 65, 8, block::Block::WATER->id));
 server::network::ServerPlayerInteractionManager interactionManager(&fixture.world);
 entity::player::ServerPlayerEntity player(&fixture.server, &fixture.world, "swimmer", &interactionManager);
 player.setPosition(8.5, 64.0, 8.5);
 server::network::ServerPlayNetworkHandler handler(&fixture.server, nullptr, &player);
 PlayerMovePositionAndOnGroundPacket packet;
 packet.setMove(player.x, player.y, player.y + player.getEyeHeight(), player.z, 0.0f, 0.0f, false);
 player.air = 300;
 handler.onPlayerMove(packet);
 handler.onPlayerMove(packet);
 EXPECT_EQ(player.air, 300);
 handler.tick();
 EXPECT_EQ(player.air, 299);
}
TEST(MultiplayerParityUpdates, WorldTimeUpdateAppliesUnconditionally) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 handler.world = &world;
 world.setTime(100);
 WorldTimeUpdateS2CPacket packet;
 packet.time = 95;
 handler.onWorldTimeUpdate(packet);
 EXPECT_EQ(world.time(), 95ULL);
}

// Java ServerPlayNetworkHandler.teleport sends PlayerMoveFullPacket(x, y + 1.62, y, z, ...):
// server->client the y and stance wire fields are swapped relative to the client->server
// direction (see MovementAckUsesActualPlayerY above) -- "y" carries the eye, "stance"
// carries the feet. Drives the packet over a real loopback socket so the assertion is
// against actual wire bytes, not just the in-process struct ServerPlayNetworkHandler built.
TEST(MultiplayerParityUpdates, ServerTeleportSendsFeetYAndStanceCorrectly) {
 block::initializeBlocks();
 ServerEventFixture fixture;
 (void)fixture.world.getChunk(0, 0);
 server::network::ServerPlayerInteractionManager interactionManager(&fixture.world);
 entity::player::ServerPlayerEntity player(&fixture.server, &fixture.world, "test_player", &interactionManager);
 server::network::ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 ASSERT_TRUE(listenSocket.isOpen());
 const std::uint16_t port = listenSocket.boundPort();
 ASSERT_NE(port, 0);
 SOCKET clientSocket = INVALID_SOCKET;
 std::thread connector([&]() { clientSocket = connectLoopback(port); });
 std::string remoteAddress;
 const auto acceptDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 SOCKET serverSocket = INVALID_SOCKET;
 while(std::chrono::steady_clock::now() < acceptDeadline && serverSocket == INVALID_SOCKET) {
  serverSocket = listenSocket.accept(remoteAddress);
  if(serverSocket == INVALID_SOCKET) {
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
 }
 connector.join();
 ASSERT_NE(serverSocket, INVALID_SOCKET);
 ASSERT_NE(clientSocket, INVALID_SOCKET);
 CapturingClientHandler clientHandler;
 Connection clientConnection(clientSocket, "TestClient", clientHandler);
 BootstrapServerHandler bootstrapHandler;
 Connection serverConnection(serverSocket, "TestServer", bootstrapHandler);
 server::network::ServerPlayNetworkHandler serverHandler(&fixture.server, &serverConnection, &player);
 serverHandler.teleport(10.0, 64.0, 10.0, 0.0f, 0.0f);
 // Connection only queues raw reads on its background reader thread; apply() (and thus
 // onPlayerMove) runs from tick(), same as the production main-thread pump.
 const auto receiveDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(std::chrono::steady_clock::now() < receiveDeadline && !clientHandler.received()) {
  clientConnection.tick();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 ASSERT_TRUE(clientHandler.received()) << "client never received the teleport's PlayerMoveFullPacket";
 // teleport(x, y=64, z, ...) calls setMove(x, y + 1.62, y, z, ...): despite the C++ field
 // names (mirroring the client->server struct layout), in this server->client packet the
 // "feetY" wire slot actually carries the eye value and "stance" carries the feet value --
 // that is what Java's ServerPlayNetworkHandler.teleport does byte-for-byte. Asserting the
 // swapped-back (intuitive) mapping here would make this test pass against the bug this was
 // written to catch (see ServerPlayNetworkHandler.cpp:126, "teleport").
 EXPECT_DOUBLE_EQ(clientHandler.lastFeetY(), 65.62);
 EXPECT_DOUBLE_EQ(clientHandler.lastStance(), 64.0);
 clientConnection.disconnect();
 serverConnection.disconnect();
}
} // namespace net::minecraft::test
