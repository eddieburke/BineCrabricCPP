#pragma once
#include "net/minecraft/server/ServerLog.hpp"
#include <string>
namespace net::minecraft::server {
class ConsoleFormatter final : public LogFormatter {
public:
  [[nodiscard]] std::string format(const LogRecord& record) const override;

private:
  [[nodiscard]] static std::string formatTimestamp(const LogRecord& record);
  [[nodiscard]] static std::string formatLevel(LogLevel level);
  [[nodiscard]] static std::string formatThrown(const std::exception& thrown);
};
} // namespace net::minecraft::server
