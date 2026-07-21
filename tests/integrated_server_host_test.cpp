#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
namespace {
std::filesystem::path makeTempWorldRoot(const std::string& name) {
 const std::filesystem::path root = std::filesystem::temp_directory_path() / "minecraft_native_server_tests" / name;
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
TEST(IntegratedServerHost, UsesAutomaticPortAndInMemoryProperties) {
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = makeTempWorldRoot("integrated_host");
 config.worldName = "IntegratedWorld";
 config.bindAddress = "127.0.0.1";
 config.port = 0;
 config.onlineMode = false;
 config.useConsoleThread = false;
 config.useGui = false;
 net::minecraft::server::MinecraftServer server(config);
 ASSERT_TRUE(server.startAsync()) << (server.lastError().empty() ? "integrated server failed to start"
                                                                 : server.lastError());
 EXPECT_NE(server.boundPort(), 0);
 ASSERT_NE(server.properties, nullptr);
 EXPECT_FALSE(server.properties->persistsToFile());
 EXPECT_EQ(server.properties->getProperty("server-port", -1), 0);
 server.stopAndJoin();
}
TEST(IntegratedServerHost, PreservesCustomPort) {
 net::minecraft::server::host::ServerLaunchConfig config;
 config.storageRoot = makeTempWorldRoot("integrated_host_custom_port");
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
} // namespace net::minecraft::test
