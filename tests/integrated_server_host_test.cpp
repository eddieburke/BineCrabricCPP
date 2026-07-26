#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include "tests/support/empty_server_mod_host.hpp"

namespace {
std::filesystem::path makeTempWorldRoot(const std::string& name) {
 static std::atomic<int> counter{0};
 const std::filesystem::path root =
     std::filesystem::temp_directory_path() / "minecraft_native_server_tests" / (name + "_" + std::to_string(++counter));
 std::error_code ec;
 std::filesystem::remove_all(root, ec);
 std::filesystem::create_directories(root, ec);
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
TEST(IntegratedServerHost, UsesAutomaticPortAndInMemoryProperties) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host");
 initializeEmptyServerModHost(root / "runtime");
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = 0;
 config.onlineMode = false;
 config.ownerName = "WorldHost";
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << (server.lastError().empty() ? "integrated server failed to start"
                                                                 : server.lastError());
 EXPECT_NE(server.boundPort(), 0);
 ASSERT_NE(server.properties, nullptr);
 EXPECT_FALSE(server.properties->persistsToFile());
 EXPECT_EQ(server.properties->getProperty("server-port", -1), 0);
 EXPECT_FALSE(server.playerManager.isOperator("worldhost"));
 EXPECT_TRUE(server.registerManagedHostLogin("WorldHost", "127.0.0.1:45000"));
 EXPECT_TRUE(server.playerManager.isOperator("worldhost"));
 server.stopAndJoin();
}

TEST(IntegratedServerHost, PreservesCustomPort) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host_custom_port");
 initializeEmptyServerModHost(root / "runtime");
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = reservePort();
 config.onlineMode = false;
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << (server.lastError().empty() ? "integrated server failed to start"
                                                                 : server.lastError());
 EXPECT_EQ(server.boundPort(), config.port);
 ASSERT_NE(server.properties, nullptr);
 EXPECT_EQ(server.properties->getProperty("server-port", -1), config.port);
 server.stopAndJoin();
}

TEST(IntegratedServerHost, WritesReadyFileOnlyAfterWorldInitialization) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host_ready");
 initializeEmptyServerModHost(root / "runtime");
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = 0;
 config.onlineMode = false;
 config.readyFile = root / "server.ready";
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << server.lastError();
 EXPECT_NE(server.getWorld(0), nullptr);
 EXPECT_TRUE(std::filesystem::is_regular_file(config.readyFile));
 server.stopAndJoin();
}

TEST(IntegratedServerHost, MissingRequiredModDoesNotWriteReadyFile) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host_missing_mod");
 initializeEmptyServerModHost(root / "runtime");
 const std::filesystem::path worldDirectory = root / "IntegratedWorld";
 std::filesystem::create_directories(worldDirectory);
 {
  std::ofstream requiredMods(worldDirectory / "required-mods.txt", std::ios::binary | std::ios::trunc);
  requiredMods << "missing_ready_mod\n";
 }
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = 0;
 config.onlineMode = false;
 config.readyFile = root / "server.ready";
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 EXPECT_FALSE(server.startAsync());
 EXPECT_EQ(server.lastError(), "Cannot load world \"IntegratedWorld\": missing required Lua mods: missing_ready_mod");
 EXPECT_FALSE(std::filesystem::exists(config.readyFile));
}

TEST(IntegratedServerHost, FirstManagedLoopbackLoginClaimsActualOwnerIdentity) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host_owner_claim");
 initializeEmptyServerModHost(root / "runtime");
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = 0;
 config.ownerName = "LaunchIdentity";
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << server.lastError();
 EXPECT_FALSE(server.registerManagedHostLogin("RemotePlayer", "192.168.1.5:45000"));
 EXPECT_FALSE(server.playerManager.isOperator("RemotePlayer"));
 EXPECT_TRUE(server.registerManagedHostLogin("JoinedIdentity", "127.0.0.1:45001"));
 EXPECT_TRUE(server.playerManager.isOperator("JoinedIdentity"));
 EXPECT_FALSE(server.registerManagedHostLogin("SecondLocalPlayer", "::1:45002"));
 EXPECT_FALSE(server.playerManager.isOperator("SecondLocalPlayer"));
 server.stopAndJoin();
}

TEST(IntegratedServerHost, AcceptsIpv4MappedLoopbackOwner) {
 const std::filesystem::path root = makeTempWorldRoot("integrated_host_mapped_owner");
 initializeEmptyServerModHost(root / "runtime");
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = root;
 config.worldName = "IntegratedWorld";
 config.bindAddress.clear();
 config.port = 0;
 config.ownerName = "LaunchIdentity";
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << server.lastError();
 EXPECT_TRUE(server.registerManagedHostLogin("JoinedIdentity", "::ffff:127.0.0.1:45001"));
 EXPECT_TRUE(server.playerManager.isOperator("JoinedIdentity"));
 server.stopAndJoin();
}

} // namespace net::minecraft::test
