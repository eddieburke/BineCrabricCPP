#include "net/minecraft/server/ServerLog.hpp"
namespace net::minecraft::server {
Logger& ServerLog::LOGGER = Logger::getLogger("Minecraft");
void ServerLog::init() {
  const std::shared_ptr<ConsoleFormatter> consoleFormatter = std::make_shared<ConsoleFormatter>();
  LOGGER.setUseParentHandlers(false);
  auto consoleHandler = std::make_unique<ConsoleLogHandler>();
  consoleHandler->setFormatter(consoleFormatter);
  LOGGER.addHandler(std::move(consoleHandler));
  try {
    auto fileHandler = std::make_unique<FileLogHandler>("server.log", true);
    fileHandler->setFormatter(consoleFormatter);
    LOGGER.addHandler(std::move(fileHandler));
  } catch(const std::exception& exception) {
    LOGGER.log(LogLevel::Warning, "Failed to log to server.log", &exception);
  }
}
} // namespace net::minecraft::server
