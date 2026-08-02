#include "net/minecraft/util/logging/Log.hpp"
#include <cstdlib>
#include <memory>
#include <mutex>
namespace net::minecraft::util::logging {
Logger& Log::LOGGER = Logger::getLogger("Minecraft");
void Log::init(const std::string& logFile) {
 static std::once_flag startFlag;
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
 std::call_once(startFlag, [] {
  LogDispatcher::instance().start();
  std::atexit([] { Log::shutdown(); });
 });
}
void Log::shutdown() {
 LogDispatcher::instance().shutdown();
 LOGGER.flush();
}
} // namespace net::minecraft::util::logging
