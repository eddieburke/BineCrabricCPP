#include "support/server_test_macros.hpp"
#include "net/minecraft/server/command/Command.hpp"
#include "net/minecraft/server/command/CommandOutput.hpp"
#include "net/minecraft/server/command/ServerCommandHandler.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include <iostream>
#include <string>
#include <vector>
namespace {
class RecordingCommandOutput final : public net::minecraft::server::command::CommandOutput {
public:
  explicit RecordingCommandOutput(std::string name) : name_(std::move(name)) {}
  void sendMessage(const std::string& message) override {
    messages_.push_back(message);
  }
  std::string getName() override {
    return name_;
  }
  [[nodiscard]] const std::vector<std::string>& messages() const {
    return messages_;
  }

private:
  std::string name_;
  std::vector<std::string> messages_;
};
void testHelpCommandListsKickSyntax() {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::server::command::ServerCommandHandler handler(&server);
  RecordingCommandOutput output("Console");
  net::minecraft::server::command::Command command("help", output);
  handler.executeCommand(command);
  bool foundKick = false;
  for(const std::string& message : output.messages()) {
    if(message.find("kick <player>") != std::string::npos) {
      foundKick = true;
      break;
    }
  }
  EXPECT_TRUE(foundKick);
}
void testQuestionMarkAliasShowsHelp() {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::server::command::ServerCommandHandler handler(&server);
  RecordingCommandOutput output("Console");
  net::minecraft::server::command::Command command("?", output);
  handler.executeCommand(command);
  EXPECT_FALSE(output.messages().empty());
  bool foundHelpHeader = false;
  for(const std::string& message : output.messages()) {
    if(message.find("Console commands:") != std::string::npos) {
      foundHelpHeader = true;
      break;
    }
  }
  EXPECT_TRUE(foundHelpHeader);
}
void testGiveWithInvalidArityIsSilent() {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::server::command::ServerCommandHandler handler(&server);
  RecordingCommandOutput output("Console");
  net::minecraft::server::command::Command command("give Player", output);
  handler.executeCommand(command);
  EXPECT_TRUE(output.messages().empty());
}
void testTpSyntaxErrorMessage() {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::server::command::ServerCommandHandler handler(&server);
  RecordingCommandOutput output("Console");
  net::minecraft::server::command::Command command("tp onlyonearg", output);
  handler.executeCommand(command);
  EXPECT_EQ(output.messages().size(), 1U);
  EXPECT_TRUE(output.messages().front().find("Syntax error") != std::string::npos);
}
void testCommandStoresCommandAndOutput() {
  RecordingCommandOutput output("Admin");
  net::minecraft::server::command::Command command("say hello", output);
  EXPECT_EQ(command.commandAndArgs, "say hello");
  EXPECT_EQ(&command.output, &output);
}
} // namespace
int main() {
  RUN_SERVER_TEST(testHelpCommandListsKickSyntax);
  RUN_SERVER_TEST(testQuestionMarkAliasShowsHelp);
  RUN_SERVER_TEST(testGiveWithInvalidArityIsSilent);
  RUN_SERVER_TEST(testTpSyntaxErrorMessage);
  RUN_SERVER_TEST(testCommandStoresCommandAndOutput);
  if(server_test::failureCount() != 0) {
    std::cout << server_test::failureCount() << " test(s) failed\n";
    return 1;
  }
  std::cout << "All server command handler tests passed\n";
  return 0;
}
