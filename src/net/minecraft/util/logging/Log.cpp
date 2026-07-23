#include "net/minecraft/util/logging/Log.hpp"
namespace net::minecraft::util::logging {
Logger& Log::LOGGER = Logger::getLogger("Minecraft");
void Log::init(const std::string& logFile) {
 auto consoleFormatter = std::make_shared<ConsoleFormatter>();
 LOGGER.setUseParentHandlers(false);
 auto consoleHandler = std::make_unique<ConsoleLogHandler>();
 consoleHandler->setFormatter(consoleFormatter);
 LOGGER.addHandler(std::move(consoleHandler));
 try {
  auto fileHandler = std::make_unique<FileLogHandler>(logFile, true);
  fileHandler->setFormatter(consoleFormatter);
  LOGGER.addHandler(std::move(fileHandler));
 } catch(const std::exception& ex) {
  LOGGER.log(LogLevel::Warning, "Failed to log to " + logFile, &ex);
 }
}
} // namespace net::minecraft::util::logging
