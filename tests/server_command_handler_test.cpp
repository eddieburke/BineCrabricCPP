#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/command/CommandOutput.hpp"
#include "net/minecraft/server/command/ServerCommandHandler.hpp"
namespace {
class RecordingCommandOutput final : public net::minecraft::server::command::CommandOutput {
 public:
 explicit RecordingCommandOutput(std::string name) : name_(std::move(name)) {
 }
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
} // namespace
namespace net::minecraft::test {
TEST(ServerCommandHandler, HelpCommandListsKickSyntax) {
 net::minecraft::server::MinecraftServer server;
 net::minecraft::server::command::ServerCommandHandler handler(&server);
 RecordingCommandOutput output("Console");
 handler.executeCommand("help", output);
 bool foundKick = false;
 for(const std::string& message : output.messages()) {
  if(message.find("kick <player>") != std::string::npos) {
   foundKick = true;
   break;
  }
 }
 EXPECT_TRUE(foundKick);
}
TEST(ServerCommandHandler, QuestionMarkAliasShowsHelp) {
 net::minecraft::server::MinecraftServer server;
 net::minecraft::server::command::ServerCommandHandler handler(&server);
 RecordingCommandOutput output("Console");
 handler.executeCommand("?", output);
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
TEST(ServerCommandHandler, GiveWithInvalidArityIsSilent) {
 net::minecraft::server::MinecraftServer server;
 net::minecraft::server::command::ServerCommandHandler handler(&server);
 RecordingCommandOutput output("Console");
 handler.executeCommand("give Player", output);
 EXPECT_TRUE(output.messages().empty());
}
TEST(ServerCommandHandler, TpSyntaxErrorMessage) {
 net::minecraft::server::MinecraftServer server;
 net::minecraft::server::command::ServerCommandHandler handler(&server);
 RecordingCommandOutput output("Console");
 handler.executeCommand("tp onlyonearg", output);
 ASSERT_EQ(output.messages().size(), 1U);
 EXPECT_NE(output.messages().front().find("Syntax error"), std::string::npos);
}
} // namespace net::minecraft::test
