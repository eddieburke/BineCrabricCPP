#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
namespace net::minecraft::test {
namespace {
class CurrentPathGuard {
public:
  explicit CurrentPathGuard(const std::filesystem::path& path) : previous_(std::filesystem::current_path()) {
    std::filesystem::current_path(path);
  }
  ~CurrentPathGuard() {
    std::filesystem::current_path(previous_);
  }

private:
  std::filesystem::path previous_;
};
} // namespace
TEST(PlayerManager, WhitelistAndOperatorState) {
  const std::filesystem::path tempRoot =
      std::filesystem::temp_directory_path() / "minecraft_player_manager_test";
  std::filesystem::remove_all(tempRoot);
  std::filesystem::create_directories(tempRoot);
  CurrentPathGuard currentPathGuard(tempRoot);
  net::minecraft::server::MinecraftServer server;
  server.properties = std::make_unique<net::minecraft::server::ServerProperties>(tempRoot / "server.properties");
  server.playerManager.configureFromProperties();
  EXPECT_TRUE(server.playerManager.isWhitelisted("anyone"));
  server.playerManager.addToOperators("Admin");
  EXPECT_TRUE(server.playerManager.isOperator("admin"));
  EXPECT_TRUE(server.playerManager.isWhitelisted("stranger"));
  server.properties->setProperty("white-list", true);
  server.playerManager.configureFromProperties();
  EXPECT_FALSE(server.playerManager.isWhitelisted("stranger"));
  EXPECT_TRUE(server.playerManager.isWhitelisted("admin"));
  server.playerManager.addToWhitelist("guest");
  EXPECT_TRUE(server.playerManager.isWhitelisted("guest"));
  server.playerManager.banPlayer("banned");
  EXPECT_FALSE(server.playerManager.isWhitelisted("banned"));
  server.playerManager.unbanPlayer("banned");
  EXPECT_TRUE(server.playerManager.getPlayerList().empty());
}
} // namespace net::minecraft::test
