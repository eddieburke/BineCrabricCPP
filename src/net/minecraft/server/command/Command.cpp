#include "net/minecraft/server/command/Command.hpp"

namespace net::minecraft::server::command {
Command::Command(std::string contents, CommandOutput& output) : commandAndArgs(std::move(contents)), output(output) {
}
}  // namespace net::minecraft::server::command
