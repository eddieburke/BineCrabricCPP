#include "net/minecraft/client/ClientLog.hpp"
namespace net::minecraft::client {
Logger& ClientLog::LOGGER = Log::LOGGER;
void ClientLog::init() {
 Log::init("client.log");
}
void ClientLog::shutdown() {
 Log::shutdown();
}
} // namespace net::minecraft::client
