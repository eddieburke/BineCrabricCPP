#include "net/minecraft/server/ServerLog.hpp"
#include <mutex>
namespace net::minecraft::server {
namespace {
std::once_flag initFlag;
}
Logger& ServerLog::LOGGER = util::logging::Logger::getLogger("Minecraft");
void ServerLog::init() {
 std::call_once(initFlag, [] { Log::init("server.log"); });
}
} // namespace net::minecraft::server
