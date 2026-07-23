#include "net/minecraft/server/ServerLog.hpp"
namespace net::minecraft::server {
Logger& ServerLog::LOGGER = util::logging::Logger::getLogger("Minecraft");
void ServerLog::init() {
 Log::init("server.log");
}
} // namespace net::minecraft::server
