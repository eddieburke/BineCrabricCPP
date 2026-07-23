#include <gtest/gtest.h>
#include "net/minecraft/server/MinecraftServer.hpp"
namespace net::minecraft::test {
TEST(MinecraftServer, TickAdvancesAndStopClearsRunning) {
 net::minecraft::server::MinecraftServer server;
 const int startTicks = server.ticks;
 server.tick();
 EXPECT_EQ(server.ticks, startTicks + 1);
 server.queueCommands("list", server);
 server.runPendingCommands();
 server.addTickable(nullptr);
 server.stop();
 EXPECT_FALSE(server.running);
}
} // namespace net::minecraft::test
