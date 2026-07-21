#pragma once
#include "net/minecraft/util/logging/Log.hpp"
namespace net::minecraft::client {
using net::minecraft::util::logging::Logger;
using net::minecraft::util::logging::LogLevel;
using net::minecraft::util::logging::Log;
class ClientLog {
 public:
 static Logger& LOGGER;
 static void init();
};
} // namespace net::minecraft::client
