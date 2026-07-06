#include "net/minecraft/server/MinecraftServer.hpp"
#include <cassert>
int main() {
  net::minecraft::server::MinecraftServer server;
  const int startTicks = server.ticks;
  server.tick();
  assert(server.ticks == startTicks + 1);
  server.queueCommands("list", server);
  server.runPendingCommands();
  server.addTickable(nullptr);
  server.stop();
  assert(!server.running);
  return 0;
}
