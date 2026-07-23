#pragma once
#include <string>
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::command {
class Command;
class CommandOutput;
class ServerCommandHandler {
 public:
 explicit ServerCommandHandler(MinecraftServer* server);
 void executeCommand(Command& command);

 private:
 void executeWhitelist(const std::string& commandUser, const std::string& message, CommandOutput& output);
 void displayHelp(CommandOutput& output);
 void logCommand(const std::string& commandUser, const std::string& message);
 [[nodiscard]] int parseInt(const std::string& string, int fallback) const;
 MinecraftServer* server_ = nullptr;
};
} // namespace net::minecraft::server::command
