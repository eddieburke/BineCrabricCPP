#pragma once
#include "net/minecraft/util/logging/Log.hpp"
namespace net::minecraft::server {
using net::minecraft::util::logging::ConsoleFormatter;
using net::minecraft::util::logging::ConsoleLogHandler;
using net::minecraft::util::logging::FileLogHandler;
using net::minecraft::util::logging::Log;
using net::minecraft::util::logging::LogFormatter;
using net::minecraft::util::logging::Logger;
using net::minecraft::util::logging::LogHandler;
using net::minecraft::util::logging::LogLevel;
using net::minecraft::util::logging::LogRecord;
class ServerLog {
 public:
 static Logger& LOGGER;
 static void init();
};
} // namespace net::minecraft::server
