#pragma once
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::util::logging {
struct Log {
 static Logger& LOGGER;
 static void init(const std::string& logFile);
};
} // namespace net::minecraft::util::logging
