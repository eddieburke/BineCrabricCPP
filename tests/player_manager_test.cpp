#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include <cassert>
#include <filesystem>
int main() {
  const std::filesystem::path tempRoot =
      std::filesystem::temp_directory_path() / "minecraft_player_manager_test";
  std::filesystem::create_directories(tempRoot);
  net::minecraft::server::MinecraftServer server;
  server.properties = std::make_unique<net::minecraft::server::ServerProperties>(tempRoot / "server.properties");
  server.playerManager.configureFromProperties();
  assert(server.playerManager.isWhitelisted("anyone"));
  server.playerManager.addToOperators("Admin");
  assert(server.playerManager.isOperator("admin"));
  assert(server.playerManager.isWhitelisted("stranger"));
  server.properties->setProperty("white-list", true);
  server.playerManager.configureFromProperties();
  assert(!server.playerManager.isWhitelisted("stranger"));
  assert(server.playerManager.isWhitelisted("admin"));
  server.playerManager.addToWhitelist("guest");
  assert(server.playerManager.isWhitelisted("guest"));
  server.playerManager.banPlayer("banned");
  assert(!server.playerManager.isWhitelisted("banned"));
  server.playerManager.unbanPlayer("banned");
  assert(server.playerManager.getPlayerList().empty());
  return 0;
}
