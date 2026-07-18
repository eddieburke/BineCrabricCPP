#pragma once
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client {
using net::minecraft::util::logging::LogLevel;
using net::minecraft::util::logging::Logger;
class ClientLog {
public:
  static Logger& LOGGER;
  static void init();
};
} // namespace net::minecraft::client
