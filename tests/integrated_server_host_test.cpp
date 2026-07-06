#include "support/server_test_macros.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include <filesystem>
#include <string>
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
void testIntegratedServerUsesCustomPortAndInMemoryProperties() {
  net::minecraft::server::host::ServerLaunchConfig config;
  config.storageRoot = makeTempWorldRoot("integrated_host");
  config.worldName = "IntegratedWorld";
  config.bindAddress = "127.0.0.1";
  config.port = reservePort();
  config.onlineMode = false;
  config.useConsoleThread = false;
  config.useGui = false;
  net::minecraft::server::MinecraftServer server(config);
  if(!server.startAsync()) {
    throw std::runtime_error(server.lastError().empty() ? "integrated server failed to start" : server.lastError());
  }
  EXPECT_EQ(server.boundPort(), config.port);
  EXPECT_TRUE(server.properties != nullptr);
  EXPECT_FALSE(server.properties->persistsToFile());
  EXPECT_EQ(server.properties->getProperty("server-port", -1), static_cast<int>(config.port));
  server.stopAndJoin();
}
} // namespace
int main() {
  RUN_SERVER_TEST(testIntegratedServerUsesCustomPortAndInMemoryProperties);
  if(server_test::failureCount() != 0) {
    std::cout << server_test::failureCount() << " test(s) failed\n";
    return 1;
  }
  std::cout << "All integrated server host tests passed\n";
  return 0;
}
