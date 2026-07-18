#pragma once
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::server {
using net::minecraft::util::logging::ConsoleFormatter;
using net::minecraft::util::logging::ConsoleLogHandler;
using net::minecraft::util::logging::FileLogHandler;
using net::minecraft::util::logging::LogFormatter;
using net::minecraft::util::logging::LogHandler;
using net::minecraft::util::logging::LogLevel;
using net::minecraft::util::logging::LogRecord;
using net::minecraft::util::logging::Logger;
class ServerLog {
public:
  static Logger& LOGGER;
  static void init();
};
} // namespace net::minecraft::server
