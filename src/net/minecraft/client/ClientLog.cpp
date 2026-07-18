#include "net/minecraft/client/ClientLog.hpp"
namespace net::minecraft::client {
using net::minecraft::util::logging::ConsoleFormatter;
using net::minecraft::util::logging::ConsoleLogHandler;
using net::minecraft::util::logging::FileLogHandler;
Logger& ClientLog::LOGGER = Logger::getLogger("Minecraft");
void ClientLog::init() {
  const std::shared_ptr<ConsoleFormatter> consoleFormatter = std::make_shared<ConsoleFormatter>();
  LOGGER.setUseParentHandlers(false);
  auto consoleHandler = std::make_unique<ConsoleLogHandler>();
  consoleHandler->setFormatter(consoleFormatter);
  LOGGER.addHandler(std::move(consoleHandler));
  try {
    auto fileHandler = std::make_unique<FileLogHandler>("client.log", true);
    fileHandler->setFormatter(consoleFormatter);
    LOGGER.addHandler(std::move(fileHandler));
  } catch(const std::exception& exception) {
    LOGGER.log(LogLevel::Warning, "Failed to log to client.log", &exception);
  }
}
} // namespace net::minecraft::client
