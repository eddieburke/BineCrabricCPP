#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/host/LanHostCoordinator.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include <filesystem>
#include <gtest/gtest.h>
namespace {
class TestMinecraft final : public net::minecraft::client::Minecraft {
public:
  using net::minecraft::client::Minecraft::Minecraft;
  void handleCrash(const net::minecraft::util::crash::CrashReport&) override {}
};
std::filesystem::path makeTempWorldRoot(const std::string& name) {
  const std::filesystem::path root = std::filesystem::temp_directory_path() / "minecraft_native_client_tests" / name;
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  return root;
}
std::uint16_t reservePort() {
  net::minecraft::server::network::ServerSocket socket;
  socket.bindAndListen("127.0.0.1", 0);
  const std::uint16_t port = socket.boundPort();
  socket.close();
  return port;
}
} // namespace
namespace net::minecraft::test {
TEST(LanHostCoordinator, LocalWorldCanHostRemoteCannotAndDisconnectClearsState) {
  const std::filesystem::path root = makeTempWorldRoot("lan_host_coordinator");
  TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
  client.worldSession().ownedWorldStorageMut() = std::make_unique<net::minecraft::AlphaWorldStorage>(root, "LanWorld", true);
  client.worldSession().ownedWorldMut() = std::make_unique<net::minecraft::World>(
      client.worldSession().ownedWorldStorage(), "LanWorld", 12345, false);
  client.world = client.worldSession().ownedWorld();
  EXPECT_TRUE(client.lanHostCoordinator().canOpenLan());
  net::minecraft::ClientWorld remoteWorld(nullptr, 12345, 0);
  EXPECT_FALSE(net::minecraft::client::host::LanHostCoordinator::canHostWorld(&remoteWorld));
  std::string error;
  ASSERT_TRUE(client.lanHostCoordinator().beginHosting(reservePort(), error)) << error;
  while(!client.lanHostCoordinator().tickHosting(error)) {
  }
  ASSERT_TRUE(client.lanHostCoordinator().isAwaitingLoopback()) << error;
  EXPECT_EQ(client.world, nullptr);
  client.lanHostCoordinator().afterWorldChange(&remoteWorld);
  EXPECT_TRUE(client.lanHostCoordinator().isHosting());
  client.lanHostCoordinator().afterWorldChange(nullptr);
  EXPECT_FALSE(client.lanHostCoordinator().isHosting());
}
} // namespace net::minecraft::test
