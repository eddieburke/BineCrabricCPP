#include "net/minecraft/server/command/ServerCommandHandler.hpp"
#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/ServerProperties.hpp"
#include "net/minecraft/server/command/Command.hpp"
#include "net/minecraft/server/command/CommandOutput.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
namespace net::minecraft::server::command {
using ::net::minecraft::entity::player::ServerPlayerEntity;
namespace {
Logger& commandLogger() {
  return ServerLog::LOGGER;
}
std::string toLowerCopy(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
bool startsWithIgnoreCase(const std::string& value, const std::string& prefix) {
  if(value.size() < prefix.size()) {
    return false;
  }
  return toLowerCopy(value.substr(0, prefix.size())) == toLowerCopy(prefix);
}
std::string argumentAfterFirstSpace(const std::string& value) {
  const std::size_t space = value.find(' ');
  if(space == std::string::npos) {
    return {};
  }
  std::string argument = value.substr(space + 1);
  const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  argument.erase(argument.begin(), std::find_if(argument.begin(), argument.end(), notSpace));
  argument.erase(std::find_if(argument.rbegin(), argument.rend(), notSpace).base(), argument.end());
  return argument;
}
std::vector<std::string> splitOnSingleSpace(const std::string& value) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while(start <= value.size()) {
    const std::size_t end = value.find(' ', start);
    if(end == std::string::npos) {
      parts.push_back(value.substr(start));
      break;
    }
    parts.push_back(value.substr(start, end - start));
    start = end + 1;
  }
  return parts;
}
constexpr const char* kColorGray =
    "\xC2\xA7"
    "7";
constexpr const char* kColorYellow =
    "\xC2\xA7"
    "e";
constexpr const char* kColorLightPurple =
    "\xC2\xA7"
    "d";
} // namespace
ServerCommandHandler::ServerCommandHandler(MinecraftServer* server) : server_(server) {
}
void ServerCommandHandler::executeCommand(Command& command) {
  std::string line = command.commandAndArgs;
  CommandOutput& commandOutput = command.output;
  const std::string commandUser = commandOutput.getName();
  PlayerManager& playerManager = server_->playerManager;
  if(startsWithIgnoreCase(line, "help") || startsWithIgnoreCase(line, "?")) {
    displayHelp(commandOutput);
    return;
  }
  if(startsWithIgnoreCase(line, "list")) {
    commandOutput.sendMessage("Connected players: " + playerManager.getPlayerList());
    return;
  }
  if(startsWithIgnoreCase(line, "stop")) {
    logCommand(commandUser, "Stopping the server..");
    server_->stop();
    return;
  }
  if(startsWithIgnoreCase(line, "save-all")) {
    logCommand(commandUser, "Forcing save..");
    playerManager.savePlayers();
    for(std::size_t i = 0; i < std::size(server_->worlds); ++i) {
      ServerWorld* serverWorld = server_->worlds[i];
      if(serverWorld != nullptr) {
        serverWorld->saveWithLoadingDisplay(true, nullptr);
      }
    }
    logCommand(commandUser, "Save complete.");
    return;
  }
  if(startsWithIgnoreCase(line, "save-off")) {
    logCommand(commandUser, "Disabling level saving..");
    for(std::size_t i = 0; i < std::size(server_->worlds); ++i) {
      ServerWorld* serverWorld = server_->worlds[i];
      if(serverWorld != nullptr) {
        serverWorld->savingDisabled = true;
      }
    }
    return;
  }
  if(startsWithIgnoreCase(line, "save-on")) {
    logCommand(commandUser, "Enabling level saving..");
    for(std::size_t i = 0; i < std::size(server_->worlds); ++i) {
      ServerWorld* serverWorld = server_->worlds[i];
      if(serverWorld != nullptr) {
        serverWorld->savingDisabled = false;
      }
    }
    return;
  }
  if(startsWithIgnoreCase(line, "op ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.addToOperators(target);
    logCommand(commandUser, "Opping " + target);
    playerManager.messagePlayer(target, std::string(kColorYellow) + "You are now op!");
    return;
  }
  if(startsWithIgnoreCase(line, "deop ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.removeFromOperators(target);
    playerManager.messagePlayer(target, std::string(kColorYellow) + "You are no longer op!");
    logCommand(commandUser, "De-opping " + target);
    return;
  }
  if(startsWithIgnoreCase(line, "ban-ip ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.banIp(target);
    logCommand(commandUser, "Banning ip " + target);
    return;
  }
  if(startsWithIgnoreCase(line, "pardon-ip ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.unbanIp(target);
    logCommand(commandUser, "Pardoning ip " + target);
    return;
  }
  if(startsWithIgnoreCase(line, "ban ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.banPlayer(target);
    logCommand(commandUser, "Banning " + target);
    if(ServerPlayerEntity* player = playerManager.getPlayer(target); player != nullptr) {
      if(player->networkHandler != nullptr) {
        player->networkHandler->disconnect("Banned by admin");
      }
    }
    return;
  }
  if(startsWithIgnoreCase(line, "pardon ")) {
    const std::string target = argumentAfterFirstSpace(line);
    playerManager.unbanPlayer(target);
    logCommand(commandUser, "Pardoning " + target);
    return;
  }
  if(startsWithIgnoreCase(line, "kick ")) {
    const std::string target = argumentAfterFirstSpace(line);
    ServerPlayerEntity* player = nullptr;
    for(ServerPlayerEntity* candidate : playerManager.players) {
      if(candidate != nullptr && toLowerCopy(candidate->name) == toLowerCopy(target)) {
        player = candidate;
        break;
      }
    }
    if(player != nullptr) {
      if(player->networkHandler != nullptr) {
        player->networkHandler->disconnect("Kicked by admin");
      }
      logCommand(commandUser, "Kicking " + player->name);
    } else {
      commandOutput.sendMessage("Can't find user " + target + ". No kick.");
    }
    return;
  }
  if(startsWithIgnoreCase(line, "tp ")) {
    const std::vector<std::string> parts = splitOnSingleSpace(line);
    if(parts.size() != 3) {
      commandOutput.sendMessage("Syntax error, please provice a source and a target.");
      return;
    }
    ServerPlayerEntity* sourcePlayer = playerManager.getPlayer(parts[1]);
    ServerPlayerEntity* targetPlayer = playerManager.getPlayer(parts[2]);
    if(sourcePlayer == nullptr) {
      commandOutput.sendMessage("Can't find user " + parts[1] + ". No tp.");
      return;
    }
    if(targetPlayer == nullptr) {
      commandOutput.sendMessage("Can't find user " + parts[2] + ". No tp.");
      return;
    }
    if(sourcePlayer->dimensionId != targetPlayer->dimensionId) {
      commandOutput.sendMessage("User " + parts[1] + " and " + parts[2] + " are in different dimensions. No tp.");
      return;
    }
    if(sourcePlayer->networkHandler != nullptr) {
      sourcePlayer->networkHandler->teleport(
          targetPlayer->x, targetPlayer->y, targetPlayer->z, targetPlayer->yaw, targetPlayer->pitch);
    }
    logCommand(commandUser, "Teleporting " + parts[1] + " to " + parts[2] + ".");
    return;
  }
  if(startsWithIgnoreCase(line, "give ")) {
    const std::vector<std::string> parts = splitOnSingleSpace(line);
    if(parts.size() != 3 && parts.size() != 4) {
      return;
    }
    const std::string& targetName = parts[1];
    ServerPlayerEntity* player = playerManager.getPlayer(targetName);
    if(player == nullptr) {
      commandOutput.sendMessage("Can't find user " + targetName);
      return;
    }
    try {
      const int itemId = std::stoi(parts[2]);
      if(Item::byId(itemId) != nullptr) {
        logCommand(commandUser, "Giving " + player->name + " some " + std::to_string(itemId));
        int count = 1;
        if(parts.size() > 3) {
          count = parseInt(parts[3], 1);
        }
        if(count < 1) {
          count = 1;
        }
        if(count > 64) {
          count = 64;
        }
        player->dropItem(ItemStack(itemId, count, 0));
        return;
      }
      commandOutput.sendMessage("There's no item with id " + std::to_string(itemId));
    } catch(const std::exception&) {
      commandOutput.sendMessage("There's no item with id " + parts[2]);
    }
    return;
  }
  if(startsWithIgnoreCase(line, "time ")) {
    const std::vector<std::string> parts = splitOnSingleSpace(line);
    if(parts.size() != 3) {
      return;
    }
    const std::string& method = parts[1];
    try {
      const int amount = std::stoi(parts[2]);
      if(toLowerCopy(method) == "add") {
        for(std::size_t i = 0; i < std::size(server_->worlds); ++i) {
          ServerWorld* serverWorld = server_->worlds[i];
          if(serverWorld != nullptr) {
            serverWorld->synchronizeTimeAndUpdates(serverWorld->getTime() +
                                                   static_cast<std::uint64_t>(amount));
          }
        }
        logCommand(commandUser, "Added " + std::to_string(amount) + " to time");
        return;
      }
      if(toLowerCopy(method) == "set") {
        for(std::size_t i = 0; i < std::size(server_->worlds); ++i) {
          ServerWorld* serverWorld = server_->worlds[i];
          if(serverWorld != nullptr) {
            serverWorld->synchronizeTimeAndUpdates(static_cast<std::uint64_t>(amount));
          }
        }
        logCommand(commandUser, "Set time to " + std::to_string(amount));
        return;
      }
      commandOutput.sendMessage("Unknown method, use either \"add\" or \"set\"");
    } catch(const std::exception&) {
      commandOutput.sendMessage("Unable to convert time value, " + parts[2]);
    }
    return;
  }
  if(startsWithIgnoreCase(line, "say ")) {
    const std::string message = argumentAfterFirstSpace(line);
    commandLogger().info("[" + commandUser + "] " + message);
    ChatMessagePacket packet;
    packet.chatMessage = std::string(kColorLightPurple) + "[Server] " + message;
    playerManager.sendToAll(packet);
    return;
  }
  if(startsWithIgnoreCase(line, "tell ")) {
    const std::vector<std::string> parts = splitOnSingleSpace(line);
    if(parts.size() >= 3) {
      std::string message = line;
      message = argumentAfterFirstSpace(message);
      message = argumentAfterFirstSpace(message);
      commandLogger().info("[" + commandUser + "->" + parts[1] + "] " + message);
      std::string whisper = std::string(kColorGray) + commandUser + " whispers " + message;
      commandLogger().info(whisper);
      ChatMessagePacket packet;
      packet.chatMessage = whisper;
      if(!playerManager.sendPacket(parts[1], packet)) {
        commandOutput.sendMessage("There's no player by that name online.");
      }
    }
    return;
  }
  if(startsWithIgnoreCase(line, "whitelist ")) {
    executeWhitelist(commandUser, line, commandOutput);
    return;
  }
  commandLogger().info("Unknown console command. Type \"help\" for help.");
}
void ServerCommandHandler::executeWhitelist(const std::string& commandUser,
                                            const std::string& message,
                                            CommandOutput& output) {
  const std::vector<std::string> parts = splitOnSingleSpace(message);
  if(parts.size() < 2) {
    return;
  }
  const std::string subcommand = toLowerCopy(parts[1]);
  if(subcommand == "on") {
    logCommand(commandUser, "Turned on white-listing");
    server_->properties->setProperty("white-list", true);
    return;
  }
  if(subcommand == "off") {
    logCommand(commandUser, "Turned off white-listing");
    server_->properties->setProperty("white-list", false);
    return;
  }
  if(subcommand == "list") {
    std::string listed;
    for(const std::string& name : server_->playerManager.getWhitelist()) {
      listed += name;
      listed += ' ';
    }
    output.sendMessage("White-listed players: " + listed);
    return;
  }
  if(subcommand == "add" && parts.size() == 3) {
    const std::string name = toLowerCopy(parts[2]);
    server_->playerManager.addToWhitelist(name);
    logCommand(commandUser, "Added " + name + " to white-list");
    return;
  }
  if(subcommand == "remove" && parts.size() == 3) {
    const std::string name = toLowerCopy(parts[2]);
    server_->playerManager.removeFromWhitelist(name);
    logCommand(commandUser, "Removed " + name + " from white-list");
    return;
  }
  if(subcommand == "reload") {
    server_->playerManager.reloadWhitelist();
    logCommand(commandUser, "Reloaded white-list from file");
  }
}
void ServerCommandHandler::displayHelp(CommandOutput& output) {
  output.sendMessage("To run the server without a gui, start it like this:");
  output.sendMessage("   java -Xmx1024M -Xms1024M -jar minecraft_server.jar nogui");
  output.sendMessage("Console commands:");
  output.sendMessage("   help  or  ?               shows this message");
  output.sendMessage("   kick <player>             removes a player from the server");
  output.sendMessage("   ban <player>              bans a player from the server");
  output.sendMessage("   pardon <player>           pardons a banned player so that they can connect again");
  output.sendMessage("   ban-ip <ip>               bans an IP address from the server");
  output.sendMessage("   pardon-ip <ip>            pardons a banned IP address so that they can connect again");
  output.sendMessage("   op <player>               turns a player into an op");
  output.sendMessage("   deop <player>             removes op status from a player");
  output.sendMessage("   tp <player1> <player2>    moves one player to the same location as another player");
  output.sendMessage("   give <player> <id> [num]  gives a player a resource");
  output.sendMessage("   tell <player> <message>   sends a private message to a player");
  output.sendMessage("   stop                      gracefully stops the server");
  output.sendMessage("   save-all                  forces a server-wide level save");
  output.sendMessage("   save-off                  disables terrain saving (useful for backup scripts)");
  output.sendMessage("   save-on                   re-enables terrain saving");
  output.sendMessage("   list                      lists all currently connected players");
  output.sendMessage("   say <message>             broadcasts a message to all players");
  output.sendMessage("   time <add|set> <amount>   adds to or sets the world time (0-24000)");
}
void ServerCommandHandler::logCommand(const std::string& commandUser, const std::string& message) {
  const std::string line = commandUser + ": " + message;
  server_->playerManager.broadcast(std::string(kColorGray) + "(" + line + ")");
  commandLogger().info(line);
}
int ServerCommandHandler::parseInt(const std::string& string, int fallback) const {
  try {
    return std::stoi(string);
  } catch(const std::exception&) {
    return fallback;
  }
}
} // namespace net::minecraft::server::command
