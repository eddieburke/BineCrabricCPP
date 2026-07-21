#include "net/minecraft/server/ServerLog.hpp"
namespace net::minecraft::server {
Logger& ServerLog::LOGGER = Log::LOGGER;
void ServerLog::init() {
 Log::init("server.log");
}
} // namespace net::minecraft::server
