#include "net/minecraft/client/ClientLog.hpp"
namespace net::minecraft::client {
Logger& ClientLog::LOGGER = Log::LOGGER;
void ClientLog::init() {
 Log::init("client.log");
}
} // namespace net::minecraft::client
