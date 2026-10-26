#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace {
class TestMinecraft final : public net::minecraft::client::Minecraft {
 public:
 using net::minecraft::client::Minecraft::Minecraft;
 void handleCrash(const net::minecraft::util::crash::CrashReport&) override {
 }
};
std::filesystem::path makeTempWorldRoot(const std::string& name) {
 static std::atomic<int> counter{0};
 const std::filesystem::path root = std::filesystem::temp_directory_path() / "minecraft_native_client_tests" / (name + "_" + std::to_string(++counter));
 std::error_code ec;
 std::filesystem::remove_all(root, ec);
 std::filesystem::create_directories(root, ec);
 return root;
}
} // namespace
namespace net::minecraft::test {
TEST(ServerProcessCoordinator, LocalWorldCanTransferAndRemoteWorldCannot) {
 const std::filesystem::path root = makeTempWorldRoot("server_process_coordinator");
 TestMinecraft client(854, 480, false);
 client.worldSession().ownedWorldStorageMut() =
     std::make_unique<net::minecraft::AlphaWorldStorage>(root, "ServerWorld", true);
 client.worldSession().ownedWorldMut() =
     std::make_unique<net::minecraft::World>(client.worldSession().ownedWorldStorage(), "ServerWorld", 12345, false);
 client.world = client.worldSession().ownedWorld();
 EXPECT_TRUE(client.serverProcessCoordinator().canStartServer());
 net::minecraft::ClientWorld remoteWorld(nullptr, 12345, 0);
 EXPECT_FALSE(net::minecraft::client::host::ServerProcessCoordinator::canHostWorld(&remoteWorld));
}
TEST(ServerProcessCoordinator, FailedHandoffRetainsPlayerPayloadAndRemovesOldEntity) {
 const auto root = makeTempWorldRoot("failed_host_inventory");
 TestMinecraft client(854, 480, false);
 auto& session = client.worldSession();
 session.ownedWorldStorageMut() = std::make_unique<AlphaWorldStorage>(root, "World", true);
 session.ownedWorldMut() = std::make_unique<World>(session.ownedWorldStorage(), "World", 42, true);
 client.world = session.ownedWorld();
 session.ownedPlayerMut() = std::make_unique<entity::player::ClientPlayerEntity>(
     &client, client.world, net::minecraft::client::util::Session("WorldHost", ""), client.options, 0);
 client.player = session.ownedPlayer();
 client.player->inventory.main[0] = ItemStack(264, 7);
 client.world->players.push_back(client.player);
 World* parked = client.world;
 ASSERT_TRUE(session.parkLocalWorldForRemoteHandoff(client));
 EXPECT_TRUE(parked->players.empty());
 ASSERT_NE(parked->getProperties().getPlayerNbt(), nullptr);
 entity::player::PlayerEntity restored(parked);
 restored.readNbt(*parked->getProperties().getPlayerNbt());
 EXPECT_EQ(restored.inventory.main[0].itemId, 264);
 EXPECT_EQ(restored.inventory.main[0].count, 7);
 session.commitRemoteHandoff();
}
} // namespace net::minecraft::test
