#pragma once
#include <string>
namespace net::minecraft::server::command {
class CommandOutput {
public:
  virtual ~CommandOutput();
  virtual void sendMessage(const std::string& message) = 0;
  [[nodiscard]] virtual std::string getName() = 0;
};
} // namespace net::minecraft::server::command
