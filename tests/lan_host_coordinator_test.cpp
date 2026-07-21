#include <gtest/gtest.h>
#include <filesystem>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
namespace {
class TestMinecraft final : public net::minecraft::client::Minecraft {
 public:
 using net::minecraft::client::Minecraft::Minecraft;
 void handleCrash(const net::minecraft::util::crash::CrashReport&) override {
 }
};
std::filesystem::path makeTempWorldRoot(const std::string& name) {
 const std::filesystem::path root = std::filesystem::temp_directory_path() / "minecraft_native_client_tests" / name;
 std::filesystem::remove_all(root);
 std::filesystem::create_directories(root);
 return root;
}
} // namespace
namespace net::minecraft::test {
TEST(ServerProcessCoordinator, LocalWorldCanTransferAndRemoteWorldCannot) {
 const std::filesystem::path root = makeTempWorldRoot("server_process_coordinator");
 TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
 client.worldSession().ownedWorldStorageMut() =
     std::make_unique<net::minecraft::AlphaWorldStorage>(root, "ServerWorld", true);
 client.worldSession().ownedWorldMut() =
     std::make_unique<net::minecraft::World>(client.worldSession().ownedWorldStorage(), "ServerWorld", 12345, false);
 client.world = client.worldSession().ownedWorld();
 EXPECT_TRUE(client.serverProcessCoordinator().canStartServer());
 net::minecraft::ClientWorld remoteWorld(nullptr, 12345, 0);
 EXPECT_FALSE(net::minecraft::client::host::ServerProcessCoordinator::canHostWorld(&remoteWorld));
}
} // namespace net::minecraft::test
