#pragma once
#include <string>
#include "net/minecraft/server/command/CommandOutput.hpp"
namespace net::minecraft::server::command {
class Command {
public:
  const std::string commandAndArgs;
  CommandOutput& output;
  Command(std::string contents, CommandOutput& output);
};
} // namespace net::minecraft::server::command
